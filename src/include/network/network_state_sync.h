#ifndef NETWORK_STATE_SYNC_H
#define NETWORK_STATE_SYNC_H

#include <stdint.h>
#include <stdbool.h>
#include <stddef.h>

// State synchronization system for network multiplayer
// Handles entity state tracking, delta compression, and snapshot management

// Initialize the state synchronization system
bool state_sync_init(void);

// Shutdown the state synchronization system
void state_sync_shutdown(void);

// Register an entity for state synchronization
// entity_id: Unique identifier for the entity
// state_size: Size of the entity's state data in bytes
// priority: 0=low, 1=medium, 2=high priority updates
bool state_sync_register_entity(uint32_t entity_id, size_t state_size, uint8_t priority);

// Unregister an entity from state synchronization
bool state_sync_unregister_entity(uint32_t entity_id);

// Update an entity's state data
// entity_id: Entity to update
// state_data: New state data
// state_size: Size of state data (must match registered size)
bool state_sync_update_entity(uint32_t entity_id, const void* state_data, size_t state_size);

// Update the synchronization system (call each frame)
// delta_time: Time since last frame in seconds
bool state_sync_update(double delta_time);

// Create a state snapshot for network transmission
// snapshot_data: Output pointer to snapshot data (caller must free)
// snapshot_size: Output size of snapshot data
bool state_sync_create_snapshot(uint8_t** snapshot_data, size_t* snapshot_size);

// Apply a received state snapshot
// snapshot_data: Received snapshot data
// snapshot_size: Size of snapshot data
bool state_sync_apply_snapshot(const uint8_t* snapshot_data, size_t snapshot_size);

// Get synchronization statistics
// synced_entities: Number of entities being synchronized
// bandwidth_used: Bandwidth used in current frame (bytes)
// snapshot_count: Number of snapshots in history
void state_sync_get_statistics(uint32_t* synced_entities, uint32_t* bandwidth_used,
                              uint32_t* snapshot_count);

#endif // NETWORK_STATE_SYNC_H
