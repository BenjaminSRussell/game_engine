// network_replication.c - Network Replication System Implementation
// TODO: REPLICATION - Implement delta compression for entity state updates
// TODO: REPLICATION - Add entity prioritization based on relevance and distance
// TODO: REPLICATION - Implement adaptive replication rate based on bandwidth
// TODO: REPLICATION - Add replication for complex data types (arrays, structs)
// TODO: REPLICATION - Implement replication conflict resolution
#include <include/network/network_types.h>
#include <include/network/network_manager.h>
#include <include/core/logger.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>

#define MAX_REPLICATED_ENTITIES 1024
#define MAX_REPLICATION_FIELDS 64
#define REPLICATION_BUFFER_SIZE 8192
#define SNAPSHOT_COMPRESSION_THRESHOLD 1024

typedef struct {
  uint32_t entity_id;
  uint32_t field_mask;
  uint8_t field_data[MAX_REPLICATION_FIELDS];
  uint16_t data_size;
  bool changed;
  bool always_replicate;
} ReplicatedEntity;

typedef struct {
  uint32_t sequence_number;
  uint32_t timestamp;
  ReplicatedEntity entities[MAX_REPLICATED_ENTITIES];
  uint32_t entity_count;
  uint8_t compressed_data[REPLICATION_BUFFER_SIZE];
  uint32_t compressed_size;
  bool is_compressed;
} ReplicationSnapshot;

typedef struct {
  uint32_t entity_id;
  uint32_t last_sequence;
  uint32_t last_timestamp;
  ReplicatedEntity last_state;
  bool is_new;
} ClientEntityState;

typedef struct {
  ReplicatedEntity server_entities[MAX_REPLICATED_ENTITIES];
  uint32_t server_entity_count;
  
  ClientEntityState client_states[MAX_REPLICATED_ENTITIES];
  uint32_t client_state_count;
  
  ReplicationSnapshot last_snapshot;
  uint32_t last_sequence_number;
  
  uint32_t replication_rate;
  float replication_timer;
  
  bool is_server;
  bool compression_enabled;
  
} NetworkReplication;

static NetworkReplication g_replication = {0};

bool network_replication_init(bool is_server, uint32_t replication_rate, bool enable_compression) {
  memset(&g_replication, 0, sizeof(NetworkReplication));
  
  g_replication.is_server = is_server;
  g_replication.replication_rate = replication_rate;
  g_replication.compression_enabled = enable_compression;
  g_replication.replication_timer = 0.0f;
  
  LOG_INFO("Network replication initialized (server: %s, rate: %u, compression: %s)",
           is_server ? "yes" : "no", replication_rate, enable_compression ? "yes" : "no");
  
  return true;
}

void network_replication_shutdown(void) {
  memset(&g_replication, 0, sizeof(NetworkReplication));
  LOG_INFO("Network replication shutdown");
}

void network_replication_update(float delta_time) {
  g_replication.replication_timer += delta_time;
  
  if (g_replication.replication_timer >= 1.0f / g_replication.replication_rate) {
    if (g_replication.is_server) {
      network_replication_server_update();
    } else {
      network_replication_client_update();
    }
    g_replication.replication_timer = 0.0f;
  }
}

void network_replication_server_update(void) {
  // Create snapshot of current server state
  network_replication_create_snapshot();
  
  // Send snapshot to all clients
  network_replication_broadcast_snapshot();
}

void network_replication_client_update(void) {
  // Receive and process snapshots from server
  // This would be called when packets are received
}

void network_replication_create_snapshot(void) {
  if (!g_replication.is_server) return;
  
  ReplicationSnapshot *snapshot = &g_replication.last_snapshot;
  memset(snapshot, 0, sizeof(ReplicationSnapshot));
  
  snapshot->sequence_number = ++g_replication.last_sequence_number;
  snapshot->timestamp = (uint32_t)(time(NULL) * 1000);
  snapshot->entity_count = 0;
  
  // Collect changed entities
  for (uint32_t i = 0; i < g_replication.server_entity_count; i++) {
    ReplicatedEntity *entity = &g_replication.server_entities[i];
    
    if (entity->changed || entity->always_replicate) {
      snapshot->entities[snapshot->entity_count++] = *entity;
      entity->changed = false;
    }
  }
  
  // Compress snapshot if enabled and large enough
  if (g_replication.compression_enabled && snapshot->entity_count > 0) {
    network_replication_compress_snapshot(snapshot);
  }
  
  LOG_DEBUG("Created replication snapshot %u with %u entities", 
           snapshot->sequence_number, snapshot->entity_count);
}

void network_replication_compress_snapshot(ReplicationSnapshot *snapshot) {
  if (!snapshot || snapshot->entity_count == 0) return;
  
  // Simple run-length encoding compression
  uint8_t *compressed_data = snapshot->compressed_data;
  uint32_t compressed_size = 0;
  
  for (uint32_t i = 0; i < snapshot->entity_count; i++) {
    ReplicatedEntity *entity = &snapshot->entities[i];
    
    // Write entity ID
    compressed_data[compressed_size++] = (entity->entity_id >> 24) & 0xFF;
    compressed_data[compressed_size++] = (entity->entity_id >> 16) & 0xFF;
    compressed_data[compressed_size++] = (entity->entity_id >> 8) & 0xFF;
    compressed_data[compressed_size++] = entity->entity_id & 0xFF;
    
    // Write field mask
    compressed_data[compressed_size++] = (entity->field_mask >> 24) & 0xFF;
    compressed_data[compressed_size++] = (entity->field_mask >> 16) & 0xFF;
    compressed_data[compressed_size++] = (entity->field_mask >> 8) & 0xFF;
    compressed_data[compressed_size++] = entity->field_mask & 0xFF;
    
    // Write data size
    compressed_data[compressed_size++] = (entity->data_size >> 8) & 0xFF;
    compressed_data[compressed_size++] = entity->data_size & 0xFF;
    
    // Write field data
    if (entity->data_size > 0 && entity->data_size <= MAX_REPLICATION_FIELDS) {
      memcpy(&compressed_data[compressed_size], entity->field_data, entity->data_size);
      compressed_size += entity->data_size;
    }
  }
  
  snapshot->compressed_size = compressed_size;
  snapshot->is_compressed = (compressed_size < REPLICATION_BUFFER_SIZE);
  
  LOG_DEBUG("Compressed snapshot from %u entities to %u bytes", 
           snapshot->entity_count, compressed_size);
}

void network_replication_broadcast_snapshot(void) {
  if (!g_replication.is_server) return;
  
  ReplicationSnapshot *snapshot = &g_replication.last_snapshot;
  
  // Create packet
  uint8_t packet_data[REPLICATION_BUFFER_SIZE];
  uint32_t packet_size = 0;
  
  // Write packet header
  PacketHeader header = {
    .type = PACKET_SNAPSHOT,
    .flags = snapshot->is_compressed ? 0x01 : 0x00,
    .sequence = (uint16_t)snapshot->sequence_number,
    .timestamp = snapshot->timestamp,
    .data_size = (uint16_t)snapshot->compressed_size
  };
  
  memcpy(&packet_data[packet_size], &header, sizeof(PacketHeader));
  packet_size += sizeof(PacketHeader);
  
  // Write snapshot data
  if (snapshot->is_compressed) {
    memcpy(&packet_data[packet_size], snapshot->compressed_data, snapshot->compressed_size);
    packet_size += snapshot->compressed_size;
  } else {
    // Write uncompressed entities
    for (uint32_t i = 0; i < snapshot->entity_count; i++) {
      ReplicatedEntity *entity = &snapshot->entities[i];
      
      memcpy(&packet_data[packet_size], entity, sizeof(ReplicatedEntity));
      packet_size += sizeof(ReplicatedEntity);
    }
  }
  
  // Send packet to all clients
  network_manager_broadcast(packet_data, packet_size);
  
  LOG_DEBUG("Broadcasted snapshot %u (%u bytes) to all clients", 
           snapshot->sequence_number, packet_size);
}

bool network_replication_process_snapshot(const uint8_t *packet_data, uint32_t packet_size) {
  if (!packet_data || packet_size < sizeof(PacketHeader)) {
    LOG_ERROR("Invalid snapshot packet data");
    return false;
  }
  
  // Read packet header
  PacketHeader header;
  memcpy(&header, packet_data, sizeof(PacketHeader));
  
  if (header.type != PACKET_SNAPSHOT) {
    LOG_ERROR("Packet is not a snapshot packet");
    return false;
  }
  
  ReplicationSnapshot snapshot;
  memset(&snapshot, 0, sizeof(ReplicationSnapshot));
  
  snapshot.sequence_number = header.sequence;
  snapshot.timestamp = header.timestamp;
  snapshot.is_compressed = (header.flags & 0x01) != 0;
  snapshot.compressed_size = header.data_size;
  
  const uint8_t *data_ptr = packet_data + sizeof(PacketHeader);
  
  if (snapshot.is_compressed) {
    // Decompress snapshot
    if (!network_replication_decompress_snapshot(&snapshot, data_ptr)) {
      LOG_ERROR("Failed to decompress snapshot %u", snapshot.sequence_number);
      return false;
    }
  } else {
    // Read uncompressed entities
    snapshot.entity_count = (packet_size - sizeof(PacketHeader)) / sizeof(ReplicatedEntity);
    
    for (uint32_t i = 0; i < snapshot.entity_count; i++) {
      memcpy(&snapshot.entities[i], data_ptr, sizeof(ReplicatedEntity));
      data_ptr += sizeof(ReplicatedEntity);
    }
  }
  
  // Apply snapshot to client state
  network_replication_apply_snapshot(&snapshot);
  
  LOG_DEBUG("Processed snapshot %u with %u entities", 
           snapshot.sequence_number, snapshot.entity_count);
  
  return true;
}

bool network_replication_decompress_snapshot(ReplicationSnapshot *snapshot, const uint8_t *compressed_data) {
  if (!snapshot || !compressed_data) return false;
  
  uint8_t *decompressed_data = (uint8_t*)malloc(MAX_REPLICATED_ENTITIES * sizeof(ReplicatedEntity));
  if (!decompressed_data) {
    LOG_ERROR("Failed to allocate memory for decompression");
    return false;
  }
  
  uint32_t decompressed_size = 0;
  const uint8_t *read_ptr = compressed_data;
  
  while (read_ptr < compressed_data + snapshot->compressed_size) {
    if (decompressed_size >= MAX_REPLICATED_ENTITIES * sizeof(ReplicatedEntity)) {
      LOG_WARN("Decompression buffer overflow");
      break;
    }
    
    ReplicatedEntity *entity = (ReplicatedEntity*)&decompressed_data[decompressed_size];
    
    // Read entity ID
    entity->entity_id = (read_ptr[0] << 24) | (read_ptr[1] << 16) | (read_ptr[2] << 8) | read_ptr[3];
    read_ptr += 4;
    
    // Read field mask
    entity->field_mask = (read_ptr[0] << 24) | (read_ptr[1] << 16) | (read_ptr[2] << 8) | read_ptr[3];
    read_ptr += 4;
    
    // Read data size
    entity->data_size = (read_ptr[0] << 8) | read_ptr[1];
    read_ptr += 2;
    
    // Read field data
    if (entity->data_size > 0 && entity->data_size <= MAX_REPLICATION_FIELDS) {
      memcpy(entity->field_data, read_ptr, entity->data_size);
      read_ptr += entity->data_size;
    }
    
    decompressed_size += sizeof(ReplicatedEntity);
    snapshot->entity_count++;
  }
  
  // Copy decompressed entities to snapshot
  for (uint32_t i = 0; i < snapshot->entity_count; i++) {
    snapshot->entities[i] = ((ReplicatedEntity*)decompressed_data)[i];
  }
  
  free(decompressed_data);
  
  LOG_DEBUG("Decompressed snapshot to %u entities", snapshot->entity_count);
  return true;
}

void network_replication_apply_snapshot(const ReplicationSnapshot *snapshot) {
  if (!snapshot) return;
  
  for (uint32_t i = 0; i < snapshot->entity_count; i++) {
    const ReplicatedEntity *replicated_entity = &snapshot->entities[i];
    
    // Find or create client state for this entity
    ClientEntityState *client_state = network_replication_get_client_state(replicated_entity->entity_id);
    
    if (!client_state) {
      // New entity, create client state
      client_state = network_replication_create_client_state(replicated_entity->entity_id);
    }
    
    if (client_state) {
      // Check if this is newer than what we have
      if (snapshot->sequence_number > client_state->last_sequence) {
        // Apply the update
        network_replication_apply_entity_update(client_state, replicated_entity);
        
        client_state->last_sequence = snapshot->sequence_number;
        client_state->last_timestamp = snapshot->timestamp;
        client_state->last_state = *replicated_entity;
      }
    }
  }
}

ClientEntityState* network_replication_get_client_state(uint32_t entity_id) {
  for (uint32_t i = 0; i < g_replication.client_state_count; i++) {
    if (g_replication.client_states[i].entity_id == entity_id) {
      return &g_replication.client_states[i];
    }
  }
  return NULL;
}

ClientEntityState* network_replication_create_client_state(uint32_t entity_id) {
  if (g_replication.client_state_count >= MAX_REPLICATED_ENTITIES) {
    LOG_WARN("Client state array full, cannot create state for entity %u", entity_id);
    return NULL;
  }
  
  ClientEntityState *state = &g_replication.client_states[g_replication.client_state_count++];
  memset(state, 0, sizeof(ClientEntityState));
  
  state->entity_id = entity_id;
  state->last_sequence = 0;
  state->last_timestamp = 0;
  state->is_new = true;
  
  LOG_DEBUG("Created client state for entity %u", entity_id);
  return state;
}

void network_replication_apply_entity_update(ClientEntityState *client_state, const ReplicatedEntity *replicated_entity) {
  if (!client_state || !replicated_entity) return;
  
  // Apply field updates based on field mask
  uint32_t field_mask = replicated_entity->field_mask;
  
  // This would integrate with the actual entity system
  // For now, we'll just log the update
  LOG_DEBUG("Applying update to entity %u (mask: 0x%08X, size: %u)", 
           replicated_entity->entity_id, field_mask, replicated_entity->data_size);
  
  // Mark entity as no longer new
  if (client_state->is_new) {
    client_state->is_new = false;
    LOG_DEBUG("Entity %u is no longer new", replicated_entity->entity_id);
  }
}

uint32_t network_replication_add_entity(uint32_t entity_id, bool always_replicate) {
  if (!g_replication.is_server || g_replication.server_entity_count >= MAX_REPLICATED_ENTITIES) {
    return 0;
  }
  
  ReplicatedEntity *entity = &g_replication.server_entities[g_replication.server_entity_count++];
  memset(entity, 0, sizeof(ReplicatedEntity));
  
  entity->entity_id = entity_id;
  entity->field_mask = 0;
  entity->data_size = 0;
  entity->changed = true;
  entity->always_replicate = always_replicate;
  
  LOG_DEBUG("Added entity %u to replication (always_replicate: %s)", 
           entity_id, always_replicate ? "yes" : "no");
  
  return g_replication.server_entity_count;
}

void network_replication_remove_entity(uint32_t entity_id) {
  if (!g_replication.is_server) return;
  
  // Find and remove entity
  for (uint32_t i = 0; i < g_replication.server_entity_count; i++) {
    if (g_replication.server_entities[i].entity_id == entity_id) {
      // Move last entity to this position
      if (i < g_replication.server_entity_count - 1) {
        g_replication.server_entities[i] = g_replication.server_entities[g_replication.server_entity_count - 1];
      }
      g_replication.server_entity_count--;
      
      LOG_DEBUG("Removed entity %u from replication", entity_id);
      return;
    }
  }
}

void network_replication_update_entity(uint32_t entity_id, uint32_t field_mask, 
                                       const void *field_data, uint16_t data_size) {
  if (!g_replication.is_server) return;
  
  // Find entity
  for (uint32_t i = 0; i < g_replication.server_entity_count; i++) {
    ReplicatedEntity *entity = &g_replication.server_entities[i];
    
    if (entity->entity_id == entity_id) {
      entity->field_mask |= field_mask;
      
      if (field_data && data_size > 0 && data_size <= MAX_REPLICATION_FIELDS) {
        memcpy(entity->field_data, field_data, data_size);
        entity->data_size = data_size;
      }
      
      entity->changed = true;
      
      LOG_DEBUG("Updated entity %u (mask: 0x%08X, size: %u)", entity_id, field_mask, data_size);
      return;
    }
  }
  
  LOG_WARN("Entity %u not found for replication update", entity_id);
}

void network_replication_set_replication_rate(uint32_t rate) {
  g_replication.replication_rate = rate;
  LOG_INFO("Set replication rate to %u Hz", rate);
}

void network_replication_set_compression_enabled(bool enabled) {
  g_replication.compression_enabled = enabled;
  LOG_INFO("Set replication compression %s", enabled ? "enabled" : "disabled");
}

uint32_t network_replication_get_entity_count(void) {
  return g_replication.is_server ? g_replication.server_entity_count : g_replication.client_state_count;
}

bool network_replication_is_server(void) {
  return g_replication.is_server;
}

uint32_t network_replication_get_last_sequence_number(void) {
  return g_replication.last_sequence_number;
}

void network_replication_clear_all_entities(void) {
  if (g_replication.is_server) {
    g_replication.server_entity_count = 0;
    memset(g_replication.server_entities, 0, sizeof(g_replication.server_entities));
  } else {
    g_replication.client_state_count = 0;
    memset(g_replication.client_states, 0, sizeof(g_replication.client_states));
  }
  
  LOG_INFO("Cleared all replicated entities");
}

void network_replication_get_statistics(uint32_t *out_server_entities, uint32_t *out_client_states,
                                         uint32_t *out_last_sequence, bool *out_compression_enabled) {
  if (out_server_entities) *out_server_entities = g_replication.server_entity_count;
  if (out_client_states) *out_client_states = g_replication.client_state_count;
  if (out_last_sequence) *out_last_sequence = g_replication.last_sequence_number;
  if (out_compression_enabled) *out_compression_enabled = g_replication.compression_enabled;
}

// TODO: REPLICATION - Add replication bandwidth monitoring and throttling
// TODO: REPLICATION - Implement replication debugging and visualization tools
// TODO: REPLICATION - Add replication for entity creation/destruction events
// TODO: REPLICATION - Implement replication rollback and state synchronization
