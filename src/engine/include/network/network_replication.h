#ifndef NETWORK_REPLICATION_H
#define NETWORK_REPLICATION_H

#include <stdbool.h>
#include <stdint.h>
#include "include/network/network_types.h"

// Initialize the replication system
bool network_replication_init(bool is_server, uint32_t replication_rate, bool enable_compression);

// Shutdown the replication system
void network_replication_shutdown(void);

// Update the replication system (call once per frame)
void network_replication_update(float delta_time);

// Entity management
uint32_t network_replication_add_entity(uint32_t entity_id, bool always_replicate);
void network_replication_remove_entity(uint32_t entity_id);
void network_replication_update_entity(uint32_t entity_id, uint32_t field_mask, const void *field_data, uint16_t data_size);

// Authority and Ownership
void network_replication_set_owner(uint32_t entity_id, uint32_t client_id);
uint32_t network_replication_get_owner(uint32_t entity_id);

// Relevancy
void network_replication_set_relevancy_distance(float distance);
void network_replication_set_entity_position(uint32_t entity_id, float x, float y, float z);

// Configuration
void network_replication_set_replication_rate(uint32_t rate);
void network_replication_set_compression_enabled(bool enabled);

// State queries
uint32_t network_replication_get_entity_count(void);
bool network_replication_is_server(void);
uint32_t network_replication_get_last_sequence_number(void);
void network_replication_clear_all_entities(void);
void network_replication_get_statistics(uint32_t *out_server_entities, uint32_t *out_client_states, uint32_t *out_last_sequence, bool *out_compression_enabled);

// Network packet processing (called by Network Manager)
bool network_replication_process_snapshot(const uint8_t *packet_data, uint32_t packet_size);
void network_replication_process_ack(uint32_t client_id, uint16_t sequence);

#endif // NETWORK_REPLICATION_H
