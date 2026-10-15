/* Keep assert() active in every build type. A Release build defines NDEBUG,
 * which would turn these checks into no-ops and let the test pass vacuously. */
#undef NDEBUG

#include "include/core/logger.h"
#include "include/network/network_manager.h"
#include "include/network/network_replication.h"
#include "include/network/packet.h"

#include <assert.h>
#include <stdio.h>
#include <string.h>

typedef struct {
  int kind;
  uint32_t client_id;
  PacketType type;
  uint8_t data[MAX_PACKET_SIZE];
  size_t size;
} CapturedPacket;

enum { CAP_SEND = 1, CAP_BROADCAST = 2, CAP_CLIENT = 3 };

static CapturedPacket g_caps[32];
static int g_cap_count;
static uint32_t g_client_ids[8];
static int g_client_count;

void unified_logger_log(LogLevel level, LogCategory category, const char *file,
                        int line, const char *function, const char *format,
                        ...) {
  (void)level;
  (void)category;
  (void)file;
  (void)line;
  (void)function;
  (void)format;
}

int network_server_get_clients(uint32_t *out_client_ids, size_t max_count) {
  size_t n = (size_t)g_client_count;
  if (!out_client_ids || max_count == 0)
    return 0;
  if (n > max_count)
    n = max_count;
  for (size_t i = 0; i < n; i++)
    out_client_ids[i] = g_client_ids[i];
  return (int)n;
}

static void capture(int kind, uint32_t client_id, PacketType type,
                    const void *data, size_t data_size) {
  CapturedPacket *cap;
  if (g_cap_count >= (int)(sizeof(g_caps) / sizeof(g_caps[0])))
    return;
  cap = &g_caps[g_cap_count++];
  memset(cap, 0, sizeof(*cap));
  cap->kind = kind;
  cap->client_id = client_id;
  cap->type = type;
  if (data && data_size > 0) {
    if (data_size > sizeof(cap->data))
      data_size = sizeof(cap->data);
    memcpy(cap->data, data, data_size);
    cap->size = data_size;
  }
}

int network_server_send_to_client(uint32_t client_id, PacketType type,
                                  const void *data, size_t data_size) {
  capture(CAP_SEND, client_id, type, data, data_size);
  return 0;
}

int network_server_broadcast(PacketType type, const void *data,
                             size_t data_size) {
  capture(CAP_BROADCAST, 0, type, data, data_size);
  return 0;
}

int network_client_send(PacketType type, const void *data, size_t data_size) {
  capture(CAP_CLIENT, 0, type, data, data_size);
  return 0;
}

static void reset_net(void) {
  g_cap_count = 0;
  g_client_count = 0;
  memset(g_client_ids, 0, sizeof(g_client_ids));
  network_replication_shutdown();
}

static int count_kind(int kind, PacketType type) {
  int n = 0;
  for (int i = 0; i < g_cap_count; i++) {
    if (g_caps[i].kind == kind && g_caps[i].type == type)
      n++;
  }
  return n;
}

static CapturedPacket *find_send(uint32_t client_id, PacketType type) {
  for (int i = 0; i < g_cap_count; i++) {
    if (g_caps[i].kind == CAP_SEND && g_caps[i].client_id == client_id &&
        g_caps[i].type == type) {
      return &g_caps[i];
    }
  }
  return NULL;
}

typedef struct {
  uint32_t id;
  uint32_t owner;
  uint32_t version;
  uint32_t mask;
  uint16_t data_size;
  uint8_t data[64];
  float position[3];
} ParsedEntity;

static int parse_snapshot_body(const uint8_t *body, size_t size, uint8_t *flags,
                               uint16_t *sequence, ParsedEntity *out, int max_out) {
  size_t off;
  int count = 0;
  if (size < 7)
    return -1;
  *flags = body[0];
  *sequence = (uint16_t)((body[1] << 8) | body[2]);
  off = 7;
  while (off < size) {
    ParsedEntity ent;
    uint16_t data_size;
    if (count >= max_out || off + 30 > size)
      return -1;
    memset(&ent, 0, sizeof(ent));
    ent.id = ((uint32_t)body[off] << 24) | ((uint32_t)body[off + 1] << 16) |
             ((uint32_t)body[off + 2] << 8) | (uint32_t)body[off + 3];
    ent.owner = ((uint32_t)body[off + 4] << 24) | ((uint32_t)body[off + 5] << 16) |
                ((uint32_t)body[off + 6] << 8) | (uint32_t)body[off + 7];
    ent.version = ((uint32_t)body[off + 8] << 24) |
                  ((uint32_t)body[off + 9] << 16) |
                  ((uint32_t)body[off + 10] << 8) | (uint32_t)body[off + 11];
    memcpy(ent.position, body + off + 12, sizeof(ent.position));
    ent.mask = ((uint32_t)body[off + 24] << 24) | ((uint32_t)body[off + 25] << 16) |
               ((uint32_t)body[off + 26] << 8) | (uint32_t)body[off + 27];
    data_size = (uint16_t)((body[off + 28] << 8) | body[off + 29]);
    ent.data_size = data_size;
    off += 30;
    if (data_size > sizeof(ent.data) || off + data_size > size)
      return -1;
    if (data_size > 0)
      memcpy(ent.data, body + off, data_size);
    off += data_size;
    out[count++] = ent;
  }
  return count;
}

static uint32_t wrap_transport(PacketType type, const uint8_t *body, size_t body_size,
                               uint8_t *out, size_t out_cap) {
  PacketHeader header;
  if (sizeof(header) + body_size > out_cap)
    return 0;
  memset(&header, 0, sizeof(header));
  header.type = (uint8_t)type;
  header.data_size = (uint16_t)body_size;
  memcpy(out, &header, sizeof(header));
  if (body_size > 0)
    memcpy(out + sizeof(header), body, body_size);
  return (uint32_t)(sizeof(header) + body_size);
}

static void test_owner(void) {
  printf("Testing ownership...\n");
  reset_net();
  assert(network_replication_init(true, 20, true));
  assert(network_replication_add_entity(10, false) == 1);
  assert(network_replication_get_owner(10) == 0);
  network_replication_set_owner(10, 9);
  assert(network_replication_get_owner(10) == 9);
  network_replication_set_owner(10, 9);
  assert(network_replication_get_owner(10) == 9);
  printf("Ownership passed.\n");
}

static void test_delta_and_per_client_ack(void) {
  CapturedPacket *to_a;
  CapturedPacket *to_b;
  ParsedEntity ents[8];
  uint8_t flags = 0;
  uint16_t sequence = 0;
  int count;
  const char payload[] = {'a', 'b'};

  printf("Testing delta compression and snapshot ACKs...\n");
  reset_net();
  assert(network_replication_init(true, 20, true));
  network_replication_set_relevancy_distance(1000.0f);
  g_client_ids[0] = 3;
  g_client_ids[1] = 4;
  g_client_count = 2;

  assert(network_replication_add_entity(10, false) != 0);
  assert(network_replication_add_entity(11, false) != 0);
  network_replication_set_owner(10, 3);
  network_replication_update_entity(10, 0x5, payload, 2);
  network_replication_update(1.0f);

  assert(count_kind(CAP_SEND, PACKET_TYPE_SNAPSHOT) == 2);
  to_a = find_send(3, PACKET_TYPE_SNAPSHOT);
  to_b = find_send(4, PACKET_TYPE_SNAPSHOT);
  assert(to_a && to_b);
  assert(to_a->size >= 7);
  assert(to_a->data[0] == 0x01);
  assert(to_a->data[0] != 0 || to_a->size < 5 || to_a->data[4] != PACKET_TYPE_SNAPSHOT);
  count = parse_snapshot_body(to_a->data, to_a->size, &flags, &sequence, ents, 8);
  assert(count == 2);
  assert(flags == 0x01);
  assert(sequence == (uint16_t)network_replication_get_last_sequence_number());
  assert((ents[0].id == 10 && ents[1].id == 11) ||
         (ents[0].id == 11 && ents[1].id == 10));

  network_replication_process_ack(3, sequence);
  g_cap_count = 0;
  network_replication_update(1.0f);
  assert(find_send(3, PACKET_TYPE_SNAPSHOT) == NULL);
  to_b = find_send(4, PACKET_TYPE_SNAPSHOT);
  assert(to_b != NULL);
  count = parse_snapshot_body(to_b->data, to_b->size, &flags, &sequence, ents, 8);
  assert(count == 2);

  network_replication_process_ack(4, sequence);
  g_cap_count = 0;
  network_replication_update(1.0f);
  assert(count_kind(CAP_SEND, PACKET_TYPE_SNAPSHOT) == 0);

  network_replication_update_entity(11, 0x2, "z", 1);
  network_replication_update(1.0f);
  assert(count_kind(CAP_SEND, PACKET_TYPE_SNAPSHOT) == 2);
  to_a = find_send(3, PACKET_TYPE_SNAPSHOT);
  assert(to_a != NULL);
  count = parse_snapshot_body(to_a->data, to_a->size, &flags, &sequence, ents, 8);
  assert(count == 1);
  assert(ents[0].id == 11);
  assert(ents[0].mask == 0x2);
  assert(ents[0].data_size == 1);
  assert(ents[0].data[0] == 'z');
  printf("Delta compression passed.\n");
}

static void test_relevancy(void) {
  ParsedEntity ents[8];
  uint8_t flags = 0;
  uint16_t sequence = 0;
  CapturedPacket *cap;
  int count;
  bool saw1 = false;
  bool saw2 = false;
  bool saw3 = false;

  printf("Testing relevancy...\n");
  reset_net();
  assert(network_replication_init(true, 20, true));
  network_replication_set_relevancy_distance(10.0f);
  g_client_ids[0] = 8;
  g_client_count = 1;

  assert(network_replication_add_entity(1, false) != 0);
  assert(network_replication_add_entity(2, false) != 0);
  assert(network_replication_add_entity(3, true) != 0);
  network_replication_set_entity_position(1, 0.0f, 0.0f, 0.0f);
  network_replication_set_entity_position(2, 100.0f, 0.0f, 0.0f);
  network_replication_set_entity_position(3, 100.0f, 0.0f, 0.0f);
  network_replication_update(1.0f);

  cap = find_send(8, PACKET_TYPE_SNAPSHOT);
  assert(cap != NULL);
  count = parse_snapshot_body(cap->data, cap->size, &flags, &sequence, ents, 8);
  assert(count == 2);
  for (int i = 0; i < count; i++) {
    if (ents[i].id == 1)
      saw1 = true;
    if (ents[i].id == 2)
      saw2 = true;
    if (ents[i].id == 3)
      saw3 = true;
  }
  assert(saw1 && saw3 && !saw2);

  network_replication_shutdown();
  assert(network_replication_init(true, 20, true));
  network_replication_set_relevancy_distance(5.0f);
  g_cap_count = 0;
  g_client_ids[0] = 7;
  g_client_count = 1;
  assert(network_replication_add_entity(10, false) != 0);
  assert(network_replication_add_entity(11, false) != 0);
  network_replication_set_owner(10, 7);
  network_replication_set_entity_position(10, 50.0f, 0.0f, 0.0f);
  network_replication_set_entity_position(11, 0.0f, 0.0f, 0.0f);
  network_replication_update(1.0f);
  cap = find_send(7, PACKET_TYPE_SNAPSHOT);
  assert(cap != NULL);
  count = parse_snapshot_body(cap->data, cap->size, &flags, &sequence, ents, 8);
  assert(count == 1);
  assert(ents[0].id == 10);
  assert(ents[0].owner == 7);
  assert(ents[0].position[0] == 50.0f);
  printf("Relevancy passed.\n");
}

static void test_destroy_and_roundtrip(void) {
  CapturedPacket snap_copy;
  CapturedPacket destroy_copy;
  CapturedPacket *snap;
  CapturedPacket *destroy_pkt;
  CapturedPacket *ack;
  uint8_t packet[MAX_PACKET_SIZE];
  uint32_t packet_size;
  ParsedEntity ents[4];
  uint8_t flags = 0;
  uint16_t sequence = 0;

  printf("Testing destroy payload and snapshot roundtrip...\n");
  reset_net();
  assert(network_replication_init(true, 20, true));
  g_client_ids[0] = 5;
  g_client_count = 1;
  assert(network_replication_add_entity(42, true) != 0);
  network_replication_set_owner(42, 5);
  network_replication_update_entity(42, 0x11, "xyz", 3);
  network_replication_update(1.0f);

  snap = find_send(5, PACKET_TYPE_SNAPSHOT);
  assert(snap != NULL);
  assert(snap->data[0] == 0x01);
  assert(parse_snapshot_body(snap->data, snap->size, &flags, &sequence, ents, 4) == 1);
  assert(ents[0].id == 42);
  assert(ents[0].owner == 5);
  assert(ents[0].data_size == 3);
  assert(memcmp(ents[0].data, "xyz", 3) == 0);
  snap_copy = *snap;

  network_replication_remove_entity(42);
  assert(count_kind(CAP_BROADCAST, PACKET_TYPE_ENTITY_DESTROY) == 1);
  destroy_pkt = NULL;
  for (int i = 0; i < g_cap_count; i++) {
    if (g_caps[i].kind == CAP_BROADCAST &&
        g_caps[i].type == PACKET_TYPE_ENTITY_DESTROY) {
      destroy_pkt = &g_caps[i];
    }
  }
  assert(destroy_pkt != NULL);
  assert(destroy_pkt->size == 4);
  assert(destroy_pkt->size != sizeof(PacketHeader) + 4);
  assert(destroy_pkt->data[0] == 0 && destroy_pkt->data[1] == 0 &&
         destroy_pkt->data[2] == 0 && destroy_pkt->data[3] == 42);
  destroy_copy = *destroy_pkt;

  network_replication_shutdown();
  g_cap_count = 0;
  assert(network_replication_init(false, 20, true));
  packet_size = wrap_transport(PACKET_TYPE_SNAPSHOT, snap_copy.data, snap_copy.size,
                               packet, sizeof(packet));
  assert(packet_size > sizeof(PacketHeader));
  assert(network_replication_process_snapshot(packet, packet_size));
  assert(network_replication_get_entity_count() == 1);
  assert(network_replication_get_owner(42) == 5);
  ack = NULL;
  for (int i = 0; i < g_cap_count; i++) {
    if (g_caps[i].kind == CAP_CLIENT && g_caps[i].type == PACKET_TYPE_SNAPSHOT_ACK)
      ack = &g_caps[i];
  }
  assert(ack != NULL);
  assert(ack->size == 2);
  assert(ack->data[0] == (uint8_t)((sequence >> 8) & 0xFF));
  assert(ack->data[1] == (uint8_t)(sequence & 0xFF));

  assert(network_replication_process_destroy(destroy_copy.data, (uint32_t)destroy_copy.size) ==
         false);
  packet_size = wrap_transport(PACKET_TYPE_ENTITY_DESTROY, destroy_copy.data,
                               destroy_copy.size, packet, sizeof(packet));
  assert(network_replication_process_destroy(packet, packet_size));
  assert(network_replication_get_entity_count() == 0);
  printf("Destroy and roundtrip passed.\n");
}

static void test_idle_cases(void) {
  printf("Testing idle replication cases...\n");
  reset_net();
  assert(network_replication_init(true, 0, false));
  g_client_ids[0] = 1;
  g_client_count = 1;
  assert(network_replication_add_entity(1, true) != 0);
  network_replication_update(5.0f);
  assert(count_kind(CAP_SEND, PACKET_TYPE_SNAPSHOT) == 0);

  network_replication_shutdown();
  assert(network_replication_init(true, 20, true));
  g_client_count = 0;
  assert(network_replication_add_entity(1, true) != 0);
  network_replication_update(1.0f);
  assert(count_kind(CAP_SEND, PACKET_TYPE_SNAPSHOT) == 0);
  printf("Idle cases passed.\n");
}

int main(void) {
  test_owner();
  test_delta_and_per_client_ack();
  test_relevancy();
  test_destroy_and_roundtrip();
  test_idle_cases();
  printf("All network replication tests passed.\n");
  return 0;
}
