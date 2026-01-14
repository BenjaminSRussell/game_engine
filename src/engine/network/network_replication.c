// network_replication.c - Network Replication System Implementation
// Per-client delta snapshots, ownership, and distance relevancy.
// network_server_* / network_client_send add the only PacketHeader.
// Snapshot body byte 0 is flags. Entity destroy is a 4-byte id.
#include "include/network/network_replication.h"
#include "include/core/logger.h"
#include "include/network/network_manager.h"
#include "include/network/packet.h"
#include <string.h>
#include <time.h>

#define MAX_REPLICATED_ENTITIES 1024
#define MAX_REPLICATION_FIELDS 64
#define DEFAULT_RELEVANCY_DISTANCE 1000.0f
#define MAX_CLIENT_TRACKING 64
#define SNAPSHOT_BODY_PREFIX 7

typedef struct {
  uint32_t entity_id;
  uint32_t field_mask;
  uint16_t data_size;
  uint8_t field_data[MAX_REPLICATION_FIELDS];
  bool changed;
  bool always_replicate;
  uint32_t owner_id;
  uint32_t version;
  float position[3];
} ReplicatedEntity;

typedef struct {
  uint32_t entity_id;
  uint32_t last_sequence;
  uint32_t last_timestamp;
  ReplicatedEntity last_state;
  bool is_new;
} ClientEntityState;

// known_versions advances only when that version's snapshot is ACKed.
typedef struct {
  uint32_t client_id;
  bool active;
  uint16_t last_sent_sequence;
  uint16_t last_acked_sequence;
  uint32_t known_versions[MAX_REPLICATED_ENTITIES];
  uint32_t inflight_versions[MAX_REPLICATED_ENTITIES];
  uint16_t inflight_sequences[MAX_REPLICATED_ENTITIES];
} ReplicationClient;

typedef struct {
  bool is_server;
  uint32_t replication_rate;
  bool compression_enabled;
  float replication_timer;
  uint32_t last_sequence_number;
  ReplicatedEntity server_entities[MAX_REPLICATED_ENTITIES];
  uint32_t server_entity_count;
  ReplicationClient clients[MAX_CLIENT_TRACKING];
  ClientEntityState client_states[MAX_REPLICATED_ENTITIES];
  uint32_t client_state_count;
  float relevancy_distance;
} NetworkReplication;

static NetworkReplication g_replication = {0};

static ReplicationClient *get_replication_client(uint32_t client_id);
static void network_replication_notify_destroy(uint32_t entity_id);
static void apply_replicated_entity(const ReplicatedEntity *entity,
                                    uint16_t sequence, uint32_t timestamp);

static uint32_t read_u32_be(const uint8_t *p) {
  return ((uint32_t)p[0] << 24) | ((uint32_t)p[1] << 16) |
         ((uint32_t)p[2] << 8) | (uint32_t)p[3];
}

static uint16_t read_u16_be(const uint8_t *p) {
  return (uint16_t)(((uint16_t)p[0] << 8) | (uint16_t)p[1]);
}

static bool write_u8(uint8_t *buf, uint32_t cap, uint32_t *off, uint8_t v) {
  if (*off + 1 > cap)
    return false;
  buf[(*off)++] = v;
  return true;
}

static bool write_u16(uint8_t *buf, uint32_t cap, uint32_t *off, uint16_t v) {
  if (*off + 2 > cap)
    return false;
  buf[(*off)++] = (uint8_t)((v >> 8) & 0xFF);
  buf[(*off)++] = (uint8_t)(v & 0xFF);
  return true;
}

static bool write_u32(uint8_t *buf, uint32_t cap, uint32_t *off, uint32_t v) {
  if (*off + 4 > cap)
    return false;
  buf[(*off)++] = (uint8_t)((v >> 24) & 0xFF);
  buf[(*off)++] = (uint8_t)((v >> 16) & 0xFF);
  buf[(*off)++] = (uint8_t)((v >> 8) & 0xFF);
  buf[(*off)++] = (uint8_t)(v & 0xFF);
  return true;
}

static bool write_bytes(uint8_t *buf, uint32_t cap, uint32_t *off,
                        const void *src, uint32_t n) {
  if (*off + n > cap)
    return false;
  if (n > 0 && src)
    memcpy(buf + *off, src, n);
  *off += n;
  return true;
}

static bool sequence_is_newer(uint16_t incoming, uint16_t prev) {
  return incoming != prev && (uint16_t)(incoming - prev) < 32768;
}

static float dist_sq(const float a[3], const float b[3]) {
  float dx = a[0] - b[0];
  float dy = a[1] - b[1];
  float dz = a[2] - b[2];
  return dx * dx + dy * dy + dz * dz;
}

static ReplicationClient *get_replication_client(uint32_t client_id) {
  for (uint32_t i = 0; i < MAX_CLIENT_TRACKING; i++) {
    if (g_replication.clients[i].active &&
        g_replication.clients[i].client_id == client_id) {
      return &g_replication.clients[i];
    }
  }

  for (uint32_t i = 0; i < MAX_CLIENT_TRACKING; i++) {
    if (!g_replication.clients[i].active) {
      memset(&g_replication.clients[i], 0, sizeof(g_replication.clients[i]));
      g_replication.clients[i].active = true;
      g_replication.clients[i].client_id = client_id;
      return &g_replication.clients[i];
    }
  }

  return NULL;
}

static void clear_client_slot(uint32_t index) {
  for (uint32_t c = 0; c < MAX_CLIENT_TRACKING; c++) {
    g_replication.clients[c].known_versions[index] = 0;
    g_replication.clients[c].inflight_versions[index] = 0;
    g_replication.clients[c].inflight_sequences[index] = 0;
  }
}

static void move_client_slot(uint32_t dst, uint32_t src) {
  for (uint32_t c = 0; c < MAX_CLIENT_TRACKING; c++) {
    ReplicationClient *client = &g_replication.clients[c];
    client->known_versions[dst] = client->known_versions[src];
    client->inflight_versions[dst] = client->inflight_versions[src];
    client->inflight_sequences[dst] = client->inflight_sequences[src];
  }
  clear_client_slot(src);
}

static void client_view_position(uint32_t client_id, float out[3]) {
  out[0] = 0.0f;
  out[1] = 0.0f;
  out[2] = 0.0f;
  if (client_id == 0)
    return;

  for (uint32_t i = 0; i < g_replication.server_entity_count; i++) {
    ReplicatedEntity *entity = &g_replication.server_entities[i];
    if (entity->owner_id == client_id) {
      out[0] = entity->position[0];
      out[1] = entity->position[1];
      out[2] = entity->position[2];
      return;
    }
  }
}

static bool entity_is_relevant(const ReplicatedEntity *entity,
                               const float client_pos[3]) {
  float limit;
  if (entity->always_replicate)
    return true;
  limit = g_replication.relevancy_distance;
  return dist_sq(entity->position, client_pos) <= limit * limit;
}

static bool write_entity_record(uint8_t *buf, uint32_t cap, uint32_t *off,
                                const ReplicatedEntity *entity) {
  uint32_t start = *off;
  uint16_t data_size = entity->data_size;
  if (data_size > MAX_REPLICATION_FIELDS)
    data_size = MAX_REPLICATION_FIELDS;

  if (!write_u32(buf, cap, off, entity->entity_id) ||
      !write_u32(buf, cap, off, entity->owner_id) ||
      !write_u32(buf, cap, off, entity->version) ||
      !write_bytes(buf, cap, off, entity->position, sizeof(entity->position)) ||
      !write_u32(buf, cap, off, entity->field_mask) ||
      !write_u16(buf, cap, off, data_size) ||
      (data_size > 0 &&
       !write_bytes(buf, cap, off, entity->field_data, data_size))) {
    *off = start;
    return false;
  }
  return true;
}

static bool decode_entity_record(const uint8_t *data, uint32_t size,
                                 ReplicatedEntity *out, uint32_t *consumed) {
  uint16_t data_size;
  if (size < 30)
    return false;

  memset(out, 0, sizeof(*out));
  out->entity_id = read_u32_be(data);
  out->owner_id = read_u32_be(data + 4);
  out->version = read_u32_be(data + 8);
  memcpy(out->position, data + 12, sizeof(out->position));
  out->field_mask = read_u32_be(data + 24);
  data_size = read_u16_be(data + 28);
  if (data_size > MAX_REPLICATION_FIELDS)
    return false;
  if (size < 30u + data_size)
    return false;
  out->data_size = data_size;
  if (data_size > 0)
    memcpy(out->field_data, data + 30, data_size);
  *consumed = 30u + data_size;
  return true;
}

bool network_replication_init(bool is_server, uint32_t replication_rate,
                              bool enable_compression) {
  memset(&g_replication, 0, sizeof(NetworkReplication));
  g_replication.is_server = is_server;
  g_replication.replication_rate = replication_rate;
  g_replication.compression_enabled = enable_compression;
  g_replication.relevancy_distance = DEFAULT_RELEVANCY_DISTANCE;

  LOG_INFO_CAT(LOG_CAT_NETWORK,
               "Network replication initialized (server: %s, rate: %u, "
               "compression: %s)",
               is_server ? "yes" : "no", replication_rate,
               enable_compression ? "yes" : "no");
  return true;
}

void network_replication_shutdown(void) {
  memset(&g_replication, 0, sizeof(NetworkReplication));
  LOG_INFO_CAT(LOG_CAT_NETWORK, "Network replication shutdown");
}

void network_replication_server_update(void) {
  uint32_t client_ids[MAX_CLIENTS];
  int client_count;
  uint16_t sequence;
  uint32_t timestamp;
  uint32_t payload_cap;
  static uint8_t payload[MAX_PACKET_SIZE];
  bool used[MAX_CLIENT_TRACKING];

  if (!g_replication.is_server)
    return;

  client_count = network_server_get_clients(client_ids, MAX_CLIENTS);
  if (client_count <= 0) {
    for (uint32_t i = 0; i < MAX_CLIENT_TRACKING; i++)
      g_replication.clients[i].active = false;
    return;
  }

  g_replication.last_sequence_number++;
  sequence = (uint16_t)g_replication.last_sequence_number;
  timestamp = (uint32_t)time(NULL);
  payload_cap = MAX_PACKET_SIZE - (uint32_t)sizeof(PacketHeader);
  memset(used, 0, sizeof(used));

  for (int c = 0; c < client_count; c++) {
    uint32_t client_id = client_ids[c];
    ReplicationClient *rep_client = get_replication_client(client_id);
    float client_pos[3];
    uint32_t off = 0;
    bool any = false;
    uint32_t slot;

    if (!rep_client)
      continue;

    slot = (uint32_t)(rep_client - g_replication.clients);
    if (slot < MAX_CLIENT_TRACKING)
      used[slot] = true;

    client_view_position(client_id, client_pos);
    if (!write_u8(payload, payload_cap, &off,
                  g_replication.compression_enabled ? 0x01 : 0x00) ||
        !write_u16(payload, payload_cap, &off, sequence) ||
        !write_u32(payload, payload_cap, &off, timestamp)) {
      continue;
    }

    for (uint32_t i = 0; i < g_replication.server_entity_count; i++) {
      ReplicatedEntity *entity = &g_replication.server_entities[i];
      if (!entity_is_relevant(entity, client_pos))
        continue;
      if (entity->version <= rep_client->known_versions[i])
        continue;
      if (!write_entity_record(payload, payload_cap, &off, entity))
        break;
      rep_client->inflight_versions[i] = entity->version;
      rep_client->inflight_sequences[i] = sequence;
      any = true;
    }

    if (!any)
      continue;

    rep_client->last_sent_sequence = sequence;
    network_server_send_to_client(client_id, PACKET_TYPE_SNAPSHOT, payload, off);
    LOG_DEBUG_CAT(LOG_CAT_NETWORK,
                  "Sent snapshot %u to client %u (%u bytes)", sequence,
                  client_id, off);
  }

  for (uint32_t i = 0; i < MAX_CLIENT_TRACKING; i++) {
    if (!used[i])
      g_replication.clients[i].active = false;
  }
}

void network_replication_client_update(void) {}

void network_replication_update(float delta_time) {
  float interval;

  if (g_replication.replication_rate == 0)
    return;

  g_replication.replication_timer += delta_time;
  interval = 1.0f / (float)g_replication.replication_rate;
  if (g_replication.replication_timer >= interval) {
    if (g_replication.is_server)
      network_replication_server_update();
    else
      network_replication_client_update();
    g_replication.replication_timer = 0.0f;
  }
}

void network_replication_set_relevancy_distance(float distance) {
  g_replication.relevancy_distance = distance;
}

void network_replication_set_entity_position(uint32_t entity_id, float x,
                                             float y, float z) {
  if (!g_replication.is_server)
    return;

  for (uint32_t i = 0; i < g_replication.server_entity_count; i++) {
    ReplicatedEntity *entity = &g_replication.server_entities[i];
    if (entity->entity_id != entity_id)
      continue;
    if (entity->position[0] == x && entity->position[1] == y &&
        entity->position[2] == z) {
      return;
    }
    entity->position[0] = x;
    entity->position[1] = y;
    entity->position[2] = z;
    entity->changed = true;
    entity->version++;
    return;
  }
}

void network_replication_set_owner(uint32_t entity_id, uint32_t client_id) {
  if (!g_replication.is_server)
    return;

  for (uint32_t i = 0; i < g_replication.server_entity_count; i++) {
    ReplicatedEntity *entity = &g_replication.server_entities[i];
    if (entity->entity_id != entity_id)
      continue;
    if (entity->owner_id == client_id)
      return;
    entity->owner_id = client_id;
    entity->changed = true;
    entity->version++;
    return;
  }
}

uint32_t network_replication_get_owner(uint32_t entity_id) {
  if (g_replication.is_server) {
    for (uint32_t i = 0; i < g_replication.server_entity_count; i++) {
      if (g_replication.server_entities[i].entity_id == entity_id)
        return g_replication.server_entities[i].owner_id;
    }
  } else {
    ClientEntityState *state = NULL;
    for (uint32_t i = 0; i < g_replication.client_state_count; i++) {
      if (g_replication.client_states[i].entity_id == entity_id) {
        state = &g_replication.client_states[i];
        break;
      }
    }
    if (state)
      return state->last_state.owner_id;
  }
  return 0;
}

void network_replication_process_ack(uint32_t client_id, uint16_t sequence) {
  ReplicationClient *rep_client;

  if (!g_replication.is_server)
    return;

  rep_client = get_replication_client(client_id);
  if (!rep_client)
    return;

  rep_client->last_acked_sequence = sequence;
  for (uint32_t i = 0; i < MAX_REPLICATED_ENTITIES; i++) {
    if (rep_client->inflight_sequences[i] != sequence)
      continue;
    if (rep_client->inflight_versions[i] > rep_client->known_versions[i])
      rep_client->known_versions[i] = rep_client->inflight_versions[i];
  }
}

static void apply_replicated_entity(const ReplicatedEntity *entity,
                                    uint16_t sequence, uint32_t timestamp) {
  ClientEntityState *client_state = NULL;
  bool newer;

  for (uint32_t i = 0; i < g_replication.client_state_count; i++) {
    if (g_replication.client_states[i].entity_id == entity->entity_id) {
      client_state = &g_replication.client_states[i];
      break;
    }
  }

  if (!client_state) {
    if (g_replication.client_state_count >= MAX_REPLICATED_ENTITIES) {
      LOG_WARN_CAT(LOG_CAT_NETWORK,
                   "Client state array full, cannot create state for entity %u",
                   entity->entity_id);
      return;
    }
    client_state =
        &g_replication.client_states[g_replication.client_state_count++];
    memset(client_state, 0, sizeof(*client_state));
    client_state->entity_id = entity->entity_id;
    client_state->is_new = true;
  }

  newer = client_state->is_new ||
          sequence_is_newer(sequence, (uint16_t)client_state->last_sequence);
  if (!newer)
    return;

  client_state->is_new = false;
  client_state->last_sequence = sequence;
  client_state->last_timestamp = timestamp;
  client_state->last_state = *entity;
  LOG_DEBUG_CAT(LOG_CAT_NETWORK, "Entity %u updated (ver: %u, owner: %u)",
                entity->entity_id, entity->version, entity->owner_id);
}

bool network_replication_process_snapshot(const uint8_t *packet_data,
                                          uint32_t packet_size) {
  PacketHeader header;
  const uint8_t *body;
  uint32_t body_size;
  uint8_t flags;
  uint16_t sequence;
  uint32_t timestamp;
  uint32_t off;
  ReplicatedEntity decoded[64];
  uint32_t count = 0;

  if (!packet_data || packet_size < sizeof(PacketHeader) + SNAPSHOT_BODY_PREFIX) {
    LOG_ERROR_CAT(LOG_CAT_NETWORK, "Invalid snapshot packet data");
    return false;
  }

  memcpy(&header, packet_data, sizeof(PacketHeader));
  if (header.type != PACKET_TYPE_SNAPSHOT) {
    LOG_ERROR_CAT(LOG_CAT_NETWORK, "Packet is not a snapshot packet");
    return false;
  }

  body = packet_data + sizeof(PacketHeader);
  body_size = packet_size - (uint32_t)sizeof(PacketHeader);
  flags = body[0];
  sequence = read_u16_be(body + 1);
  timestamp = read_u32_be(body + 3);
  off = SNAPSHOT_BODY_PREFIX;

  while (off < body_size) {
    uint32_t consumed = 0;
    if (count >= 64 ||
        !decode_entity_record(body + off, body_size - off, &decoded[count],
                              &consumed)) {
      LOG_ERROR_CAT(LOG_CAT_NETWORK, "Failed to decode snapshot %u", sequence);
      return false;
    }
    off += consumed;
    count++;
  }

  for (uint32_t i = 0; i < count; i++)
    apply_replicated_entity(&decoded[i], sequence, timestamp);

  if (!g_replication.is_server) {
    uint8_t ack[2];
    ack[0] = (uint8_t)((sequence >> 8) & 0xFF);
    ack[1] = (uint8_t)(sequence & 0xFF);
    network_client_send(PACKET_TYPE_SNAPSHOT_ACK, ack, 2);
  }

  LOG_DEBUG_CAT(LOG_CAT_NETWORK,
                "Processed snapshot %u with %u entities (flags 0x%02X)",
                sequence, count, flags);
  return true;
}

bool network_replication_process_destroy(const uint8_t *packet_data,
                                         uint32_t packet_size) {
  const uint8_t *data;
  uint32_t entity_id;

  if (!packet_data || packet_size < sizeof(PacketHeader) + 4)
    return false;

  data = packet_data + sizeof(PacketHeader);
  entity_id = read_u32_be(data);

  for (uint32_t i = 0; i < g_replication.client_state_count; i++) {
    if (g_replication.client_states[i].entity_id != entity_id)
      continue;
    if (i < g_replication.client_state_count - 1) {
      g_replication.client_states[i] =
          g_replication.client_states[g_replication.client_state_count - 1];
    }
    g_replication.client_state_count--;
    LOG_DEBUG_CAT(LOG_CAT_NETWORK, "Processed destroy for entity %u", entity_id);
    return true;
  }

  return false;
}

uint32_t network_replication_add_entity(uint32_t entity_id,
                                        bool always_replicate) {
  ReplicatedEntity *entity;

  if (!g_replication.is_server ||
      g_replication.server_entity_count >= MAX_REPLICATED_ENTITIES) {
    return 0;
  }

  entity = &g_replication.server_entities[g_replication.server_entity_count++];
  memset(entity, 0, sizeof(*entity));
  entity->entity_id = entity_id;
  entity->always_replicate = always_replicate;
  entity->changed = true;
  entity->version = 1;

  LOG_DEBUG_CAT(LOG_CAT_NETWORK,
                "Added entity %u to replication (always_replicate: %s)",
                entity_id, always_replicate ? "yes" : "no");
  return g_replication.server_entity_count;
}

void network_replication_remove_entity(uint32_t entity_id) {
  if (!g_replication.is_server)
    return;

  network_replication_notify_destroy(entity_id);

  for (uint32_t i = 0; i < g_replication.server_entity_count; i++) {
    uint32_t last;
    if (g_replication.server_entities[i].entity_id != entity_id)
      continue;

    last = g_replication.server_entity_count - 1;
    if (i < last) {
      g_replication.server_entities[i] = g_replication.server_entities[last];
      move_client_slot(i, last);
    } else {
      clear_client_slot(i);
    }
    g_replication.server_entity_count--;
    LOG_DEBUG_CAT(LOG_CAT_NETWORK, "Removed entity %u from replication",
                  entity_id);
    return;
  }
}

static void network_replication_notify_destroy(uint32_t entity_id) {
  uint8_t data[4];

  if (!g_replication.is_server)
    return;

  data[0] = (uint8_t)((entity_id >> 24) & 0xFF);
  data[1] = (uint8_t)((entity_id >> 16) & 0xFF);
  data[2] = (uint8_t)((entity_id >> 8) & 0xFF);
  data[3] = (uint8_t)(entity_id & 0xFF);
  network_server_broadcast(PACKET_TYPE_ENTITY_DESTROY, data, 4);
  LOG_DEBUG_CAT(LOG_CAT_NETWORK, "Broadcasted destroy for entity %u", entity_id);
}

void network_replication_update_entity(uint32_t entity_id, uint32_t field_mask,
                                       const void *field_data,
                                       uint16_t data_size) {
  if (!g_replication.is_server)
    return;

  for (uint32_t i = 0; i < g_replication.server_entity_count; i++) {
    ReplicatedEntity *entity = &g_replication.server_entities[i];
    if (entity->entity_id != entity_id)
      continue;

    entity->field_mask |= field_mask;
    if (field_data && data_size > 0 && data_size <= MAX_REPLICATION_FIELDS) {
      memcpy(entity->field_data, field_data, data_size);
      entity->data_size = data_size;
    }
    entity->changed = true;
    entity->version++;
    LOG_DEBUG_CAT(LOG_CAT_NETWORK,
                  "Updated entity %u (mask: 0x%08X, size: %u)", entity_id,
                  field_mask, data_size);
    return;
  }

  LOG_WARN_CAT(LOG_CAT_NETWORK, "Entity %u not found for replication update",
               entity_id);
}

void network_replication_set_replication_rate(uint32_t rate) {
  g_replication.replication_rate = rate;
  LOG_INFO_CAT(LOG_CAT_NETWORK, "Set replication rate to %u Hz", rate);
}

void network_replication_set_compression_enabled(bool enabled) {
  g_replication.compression_enabled = enabled;
  LOG_INFO_CAT(LOG_CAT_NETWORK, "Set replication compression %s",
               enabled ? "enabled" : "disabled");
}

uint32_t network_replication_get_entity_count(void) {
  return g_replication.is_server ? g_replication.server_entity_count
                                 : g_replication.client_state_count;
}

bool network_replication_is_server(void) { return g_replication.is_server; }

uint32_t network_replication_get_last_sequence_number(void) {
  return g_replication.last_sequence_number;
}

void network_replication_clear_all_entities(void) {
  if (g_replication.is_server) {
    g_replication.server_entity_count = 0;
    for (uint32_t i = 0; i < MAX_REPLICATED_ENTITIES; i++)
      clear_client_slot(i);
  } else {
    g_replication.client_state_count = 0;
  }
  LOG_INFO_CAT(LOG_CAT_NETWORK, "Cleared all replicated entities");
}

void network_replication_get_statistics(uint32_t *out_server_entities,
                                        uint32_t *out_client_states,
                                        uint32_t *out_last_sequence,
                                        bool *out_compression_enabled) {
  if (out_server_entities)
    *out_server_entities = g_replication.server_entity_count;
  if (out_client_states)
    *out_client_states = g_replication.client_state_count;
  if (out_last_sequence)
    *out_last_sequence = g_replication.last_sequence_number;
  if (out_compression_enabled)
    *out_compression_enabled = g_replication.compression_enabled;
}
