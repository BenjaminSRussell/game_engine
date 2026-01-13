// src/block/block_states.c
// Enhanced block state management system with transitions, validation,
// compression, serialization, notifications, caching, versioning,
// statistics, and persistence across chunk unloads.
#include <block/block.h>
#include <block/block_states.h>
#include <block/water_physics.h>
#include <chunk/chunk.h>
#include "engine/include/core/logger.h"
#include <containers/hashmap.h>
#include <containers/dynamic_array.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>
#include <math.h>
#include <stdio.h>

// Constants for block state system
#define BLOCK_STATE_CACHE_SIZE 1024
#define BLOCK_STATE_TRANSITION_CAPACITY 256
#define BLOCK_STATE_CALLBACK_CAPACITY 16
#define BLOCK_STATE_SERIALIZATION_VERSION 1
#define BLOCK_STATE_MAX_COMPRESSION_SIZE 64
#define BLOCK_STATE_PERSISTENCE_DIR "block_states/"

// Global time for tracking updates
static u32 g_current_time_ms = 0;

// Utility function to get current time in milliseconds
static u32 get_current_time_ms(void) {
  struct timespec ts;
  clock_gettime(CLOCK_MONOTONIC, &ts);
  return (u32)(ts.tv_sec * 1000 + ts.tv_nsec / 1000000);
}

// Utility function to hash position
u32 block_state_hash_position(i32 x, i32 y, i32 z) {
  // Simple hash function for 3D coordinates
  u32 hash = 5381;
  hash = ((hash << 5) + hash) + (u32)x;
  hash = ((hash << 5) + hash) + (u32)y;
  hash = ((hash << 5) + hash) + (u32)z;
  return hash;
}

// Utility function to check position equality
bool block_state_positions_equal(i32 x1, i32 y1, i32 z1,
                                 i32 x2, i32 y2, i32 z2) {
  return x1 == x2 && y1 == y2 && z1 == z2;
}

// String conversion functions
const char *block_state_transition_type_to_string(
    BlockStateTransitionType type) {
  switch (type) {
    case BLOCK_STATE_TRANSITION_NONE: return "NONE";
    case BLOCK_STATE_TRANSITION_WATER_FLOW: return "WATER_FLOW";
    case BLOCK_STATE_TRANSITION_LAVA_FLOW: return "LAVA_FLOW";
    case BLOCK_STATE_TRANSITION_FALLING_BLOCK: return "FALLING_BLOCK";
    case BLOCK_STATE_TRANSITION_FREEZING: return "FREEZING";
    case BLOCK_STATE_TRANSITION_MELTING: return "MELTING";
    case BLOCK_STATE_TRANSITION_EVAPORATION: return "EVAPORATION";
    default: return "UNKNOWN";
  }
}

const char *block_state_validation_result_to_string(
    BlockStateValidationResult result) {
  switch (result) {
    case BLOCK_STATE_VALID: return "VALID";
    case BLOCK_STATE_INVALID_POSITION: return "INVALID_POSITION";
    case BLOCK_STATE_INVALID_BLOCK_TYPE: return "INVALID_BLOCK_TYPE";
    case BLOCK_STATE_INVALID_STATE_DATA: return "INVALID_STATE_DATA";
    case BLOCK_STATE_OUT_OF_BOUNDS: return "OUT_OF_BOUNDS";
    default: return "UNKNOWN";
  }
}

// Initialize enhanced block state manager
void block_state_manager_init(BlockStateManager *manager, u32 capacity) {
  if (!manager)
    return;
    
  // Initialize basic state storage
  manager->states = (BlockState *)calloc(capacity, sizeof(BlockState));
  manager->count = 0;
  manager->capacity = capacity;
  
  // Initialize transition system
  manager->transitions = (BlockStateTransition *)calloc(
      BLOCK_STATE_TRANSITION_CAPACITY, sizeof(BlockStateTransition));
  manager->transition_count = 0;
  manager->transition_capacity = BLOCK_STATE_TRANSITION_CAPACITY;
  
  // Initialize caching system
  manager->cache = (BlockStateCacheEntry *)calloc(
      BLOCK_STATE_CACHE_SIZE, sizeof(BlockStateCacheEntry));
  manager->cache_size = 0;
  manager->cache_capacity = BLOCK_STATE_CACHE_SIZE;
  manager->state_lookup = hashmap_create(capacity * 2);
  
  // Initialize notification system
  manager->notification_callbacks = (BlockStateNotificationCallback *)calloc(
      BLOCK_STATE_CALLBACK_CAPACITY, sizeof(BlockStateNotificationCallback));
  manager->callback_user_data = (void **)calloc(
      BLOCK_STATE_CALLBACK_CAPACITY, sizeof(void *));
  manager->callback_count = 0;
  manager->callback_capacity = BLOCK_STATE_CALLBACK_CAPACITY;
  
  // Initialize versioning
  manager->version.major_version = 1;
  manager->version.minor_version = 0;
  manager->version.patch_version = 0;
  manager->version.serialization_format = BLOCK_STATE_SERIALIZATION_VERSION;
  manager->is_dirty = false;
  
  // Initialize statistics
  memset(&manager->stats, 0, sizeof(BlockStateStatistics));
  manager->last_stats_update = get_current_time_ms();
  
  g_current_time_ms = get_current_time_ms();
  
  LOG_INFO("Block state manager initialized with capacity %u", capacity);
}

// Free enhanced block state manager
void block_state_manager_free(BlockStateManager *manager) {
  if (!manager)
    return;
    
  // Free basic state storage
  if (manager->states) {
    free(manager->states);
    manager->states = NULL;
  }
  
  // Free transition system
  if (manager->transitions) {
    free(manager->transitions);
    manager->transitions = NULL;
  }
  
  // Free caching system
  if (manager->cache) {
    free(manager->cache);
    manager->cache = NULL;
  }
  if (manager->state_lookup) {
    hashmap_free(manager->state_lookup);
    manager->state_lookup = NULL;
  }
  
  // Free notification system
  if (manager->notification_callbacks) {
    free(manager->notification_callbacks);
    manager->notification_callbacks = NULL;
  }
  if (manager->callback_user_data) {
    free(manager->callback_user_data);
    manager->callback_user_data = NULL;
  }
  
  // Reset all counters and flags
  manager->count = 0;
  manager->capacity = 0;
  manager->transition_count = 0;
  manager->transition_capacity = 0;
  manager->cache_size = 0;
  manager->cache_capacity = 0;
  manager->callback_count = 0;
  manager->callback_capacity = 0;
  manager->is_dirty = false;
  
  LOG_INFO("Block state manager freed");
}

// Enhanced block state manager add function
void block_state_manager_add(BlockStateManager *manager, i32 x, i32 y, i32 z,
                             BlockID block_id) {
  if (!manager || !manager->states)
    return;

  // Validate the new state
  BlockState new_state = {
    .x = x, .y = y, .z = z,
    .block_id = block_id,
    .state_data = 0,
    .needs_update = true,
    .last_update_time = g_current_time_ms,
    .active_transition = NULL,
    .compression_flags = 0
  };
  
  BlockStateValidationResult validation = block_state_validate(&new_state, NULL);
  if (validation != BLOCK_STATE_VALID) {
    manager->stats.validation_failures++;
    LOG_WARN("Invalid block state at (%d,%d,%d): %s", 
             x, y, z, block_state_validation_result_to_string(validation));
    return;
  }

  // Check if state already exists
  for (u32 i = 0; i < manager->count; i++) {
    if (manager->states[i].x == x && manager->states[i].y == y &&
        manager->states[i].z == z) {
      
      BlockID old_block_id = manager->states[i].block_id;
      u32 old_state_data = manager->states[i].state_data;
      
      // Update existing state
      manager->states[i].block_id = block_id;
      manager->states[i].needs_update = true;
      manager->states[i].last_update_time = g_current_time_ms;
      manager->is_dirty = true;
      
      // Notify about the change
      block_state_manager_notify_change(manager, 
                                        BLOCK_STATE_NOTIFICATION_CHANGED,
                                        x, y, z, old_block_id, block_id,
                                        old_state_data, new_state.state_data);
      
      // Update cache
      block_state_manager_cache_add(manager, &manager->states[i]);
      
      return;
    }
  }

  // Check capacity
  if (manager->count >= manager->capacity) {
    LOG_WARN("Block state manager capacity reached, cannot add state at (%d,%d,%d)", 
             x, y, z);
    return;
  }

  // Add new state
  BlockState *state = &manager->states[manager->count++];
  memcpy(state, &new_state, sizeof(BlockState));
  manager->is_dirty = true;
  
  // Add to cache
  block_state_manager_cache_add(manager, state);
  
  // Notify about addition
  block_state_manager_notify_change(manager, BLOCK_STATE_NOTIFICATION_ADDED,
                                    x, y, z, 0, block_id, 0, new_state.state_data);
  
  LOG_DEBUG("Added block state %d at (%d,%d,%d)", block_id, x, y, z);
}

// Transition system implementation
void block_state_manager_add_transition(BlockStateManager *manager,
                                        i32 x, i32 y, i32 z,
                                        BlockStateTransitionType type,
                                        BlockID target_block_id,
                                        u32 target_state_data,
                                        u32 duration_ms) {
  if (!manager || !manager->transitions)
    return;
    
  // Check transition capacity
  if (manager->transition_count >= manager->transition_capacity) {
    LOG_WARN("Transition capacity reached, cannot add transition at (%d,%d,%d)", 
             x, y, z);
    return;
  }
  
  // Check if transition already exists for this position
  for (u32 i = 0; i < manager->transition_count; i++) {
    // Find the state this transition belongs to
    BlockState *state = NULL;
    for (u32 j = 0; j < manager->count; j++) {
      if (manager->states[j].x == x && manager->states[j].y == y &&
          manager->states[j].z == z) {
        state = &manager->states[j];
        break;
      }
    }
    
    if (state && state->active_transition == &manager->transitions[i]) {
      // Replace existing transition
      manager->transitions[i].type = type;
      manager->transitions[i].duration_ms = duration_ms;
      manager->transitions[i].elapsed_ms = 0;
      manager->transitions[i].target_block_id = target_block_id;
      manager->transitions[i].target_state_data = target_state_data;
      manager->transitions[i].is_reversible = (type != BLOCK_STATE_TRANSITION_FALLING_BLOCK);
      
      LOG_DEBUG("Updated transition at (%d,%d,%d) to type %s", 
                x, y, z, block_state_transition_type_to_string(type));
      return;
    }
  }
  
  // Find the state for this position
  BlockState *state = NULL;
  for (u32 i = 0; i < manager->count; i++) {
    if (manager->states[i].x == x && manager->states[i].y == y &&
        manager->states[i].z == z) {
      state = &manager->states[i];
      break;
    }
  }
  
  if (!state) {
    LOG_WARN("Cannot add transition: no block state found at (%d,%d,%d)", x, y, z);
    return;
  }
  
  // Add new transition
  BlockStateTransition *transition = &manager->transitions[manager->transition_count++];
  transition->type = type;
  transition->duration_ms = duration_ms;
  transition->elapsed_ms = 0;
  transition->target_block_id = target_block_id;
  transition->target_state_data = target_state_data;
  transition->is_reversible = (type != BLOCK_STATE_TRANSITION_FALLING_BLOCK);
  
  // Link transition to state
  state->active_transition = transition;
  
  LOG_DEBUG("Added transition %s at (%d,%d,%d) with duration %ums", 
            block_state_transition_type_to_string(type), x, y, z, duration_ms);
}

void block_state_manager_update_transitions(BlockStateManager *manager,
                                            ChunkManager *chunk_manager,
                                            f32 delta_time) {
  if (!manager || !manager->transitions)
    return;
    
  u32 delta_ms = (u32)(delta_time * 1000.0f);
  g_current_time_ms += delta_ms;
  
  for (u32 i = 0; i < manager->transition_count; i++) {
    BlockStateTransition *transition = &manager->transitions[i];
    transition->elapsed_ms += delta_ms;
    
    // Check if transition is complete
    if (transition->elapsed_ms >= transition->duration_ms) {
      // Find the state this transition belongs to
      BlockState *state = NULL;
      for (u32 j = 0; j < manager->count; j++) {
        if (manager->states[j].active_transition == transition) {
          state = &manager->states[j];
          break;
        }
      }
      
      if (state) {
        BlockID old_block_id = state->block_id;
        u32 old_state_data = state->state_data;
        
        // Apply transition
        state->block_id = transition->target_block_id;
        state->state_data = transition->target_state_data;
        state->needs_update = true;
        state->last_update_time = g_current_time_ms;
        state->active_transition = NULL;
        manager->is_dirty = true;
        
        // Update world if chunk manager is available
        if (chunk_manager) {
          block_set(chunk_manager, state->x, state->y, state->z, state->block_id);
        }
        
        // Notify about transition completion
        block_state_manager_notify_change(manager, 
                                          BLOCK_STATE_NOTIFICATION_TRANSITION,
                                          state->x, state->y, state->z,
                                          old_block_id, state->block_id,
                                          old_state_data, state->state_data);
        
        LOG_DEBUG("Completed transition %s at (%d,%d,%d)", 
                  block_state_transition_type_to_string(transition->type),
                  state->x, state->y, state->z);
      }
      
      // Remove transition by shifting remaining transitions
      for (u32 j = i; j < manager->transition_count - 1; j++) {
        manager->transitions[j] = manager->transitions[j + 1];
      }
      manager->transition_count--;
      i--; // Adjust index since we removed an element
    }
  }
}

bool block_state_manager_has_transition(BlockStateManager *manager,
                                        i32 x, i32 y, i32 z) {
  if (!manager)
    return false;
    
  for (u32 i = 0; i < manager->count; i++) {
    if (manager->states[i].x == x && manager->states[i].y == y &&
        manager->states[i].z == z) {
      return manager->states[i].active_transition != NULL;
    }
  }
  return false;
}

// Validation system implementation
BlockStateValidationResult block_state_validate(BlockState *state,
                                                 ChunkManager *chunk_manager) {
  if (!state)
    return BLOCK_STATE_INVALID_POSITION;
    
  // Check position validity
  if (!block_state_is_valid_position(state->x, state->y, state->z, chunk_manager)) {
    return BLOCK_STATE_INVALID_POSITION;
  }
  
  // Check block type validity
  if (!block_state_is_valid_block_type(state->block_id)) {
    return BLOCK_STATE_INVALID_BLOCK_TYPE;
  }
  
  // Check state data validity
  if (!block_state_is_valid_state_data(state->block_id, state->state_data)) {
    return BLOCK_STATE_INVALID_STATE_DATA;
  }
  
  return BLOCK_STATE_VALID;
}

bool block_state_is_valid_position(i32 x, i32 y, i32 z,
                                   ChunkManager *chunk_manager) {
  // Check basic coordinate bounds (reasonable world limits)
  if (x < -1000000 || x > 1000000 ||
      y < -1000 || y > 1000 ||
      z < -1000000 || z > 1000000) {
    return false;
  }
  
  // If chunk manager is available, check if position is within loaded chunks
  if (chunk_manager) {
    ChunkPos chunk_pos = world_to_chunk_pos(x, y, z);
    Chunk *chunk = chunk_manager_get(chunk_manager, chunk_pos);
    if (!chunk) {
      // Position is in unloaded chunk, but still valid for state management
      return true;
    }
    
    // Check local chunk coordinates
    i32 local_x = x - chunk_pos.x * CHUNK_SIZE;
    i32 local_y = y - chunk_pos.y * CHUNK_SIZE;
    i32 local_z = z - chunk_pos.z * CHUNK_SIZE;
    
    if (local_x < 0 || local_x >= CHUNK_SIZE ||
        local_y < 0 || local_y >= CHUNK_SIZE ||
        local_z < 0 || local_z >= CHUNK_SIZE) {
      return false;
    }
  }
  
  return true;
}

bool block_state_is_valid_block_type(BlockID block_id) {
  // Check if block ID is within valid range
  // This should be updated based on actual block registry
  if (block_id == 0) return true; // Air
  if (block_id >= BLOCK_AIR && block_id <= BLOCK_MAX) return true;
  
  // Common valid block types (expand as needed)
  switch (block_id) {
    case BLOCK_AIR:
    case BLOCK_STONE:
    case BLOCK_DIRT:
    case BLOCK_GRASS:
    case BLOCK_WATER:
    case BLOCK_LAVA:
    case BLOCK_SAND:
    case BLOCK_GRAVEL:
    case BLOCK_WOOD:
    case BLOCK_LEAVES:
    case BLOCK_ICE:
    case BLOCK_SNOW:
      return true;
    default:
      return false;
  }
}

bool block_state_is_valid_state_data(BlockID block_id, u32 state_data) {
  // Different block types have different state data requirements
  switch (block_id) {
    case BLOCK_AIR:
    case BLOCK_STONE:
    case BLOCK_DIRT:
    case BLOCK_GRASS:
    case BLOCK_SAND:
    case BLOCK_GRAVEL:
    case BLOCK_WOOD:
      // Simple blocks: no state data expected
      return state_data == 0;
      
    case BLOCK_WATER:
    case BLOCK_LAVA:
      // Liquid blocks: level (0-15) and source flag
      {
        u8 level = (u8)(state_data & 0x0F);
        bool is_source = (state_data & 0x10) != 0;
        return (level <= 15) && (is_source ? level == 0 : true);
      }
      
    case BLOCK_LEAVES:
      // Leaves: decay flag and distance (0-7)
      {
        bool should_decay = (state_data & 0x01) != 0;
        u8 distance = (u8)((state_data >> 1) & 0x07);
        return distance <= 7;
      }
      
    case BLOCK_ICE:
    case BLOCK_SNOW:
      // Ice/snow: simple state data
      return state_data <= 3;
      
    default:
      // Unknown block type: be conservative
      return state_data < 256;
  }
}

// Notification system implementation
void block_state_manager_add_notification_callback(
    BlockStateManager *manager,
    BlockStateNotificationCallback callback,
    void *user_data) {
  if (!manager || !callback)
    return;
    
  // Check callback capacity
  if (manager->callback_count >= manager->callback_capacity) {
    LOG_WARN("Notification callback capacity reached");
    return;
  }
  
  // Add callback
  u32 index = manager->callback_count++;
  manager->notification_callbacks[index] = callback;
  manager->callback_user_data[index] = user_data;
  
  LOG_DEBUG("Added notification callback %u", index);
}

void block_state_manager_remove_notification_callback(
    BlockStateManager *manager,
    BlockStateNotificationCallback callback) {
  if (!manager || !callback)
    return;
    
  // Find and remove callback
  for (u32 i = 0; i < manager->callback_count; i++) {
    if (manager->notification_callbacks[i] == callback) {
      // Shift remaining callbacks
      for (u32 j = i; j < manager->callback_count - 1; j++) {
        manager->notification_callbacks[j] = manager->notification_callbacks[j + 1];
        manager->callback_user_data[j] = manager->callback_user_data[j + 1];
      }
      manager->callback_count--;
      
      LOG_DEBUG("Removed notification callback at index %u", i);
      return;
    }
  }
  
  LOG_WARN("Callback not found for removal");
}

void block_state_manager_notify_change(BlockStateManager *manager,
                                       BlockStateNotificationType type,
                                       i32 x, i32 y, i32 z,
                                       BlockID old_block_id,
                                       BlockID new_block_id,
                                       u32 old_state_data,
                                       u32 new_state_data) {
  if (!manager)
    return;
    
  // Notify all registered callbacks
  for (u32 i = 0; i < manager->callback_count; i++) {
    BlockStateNotificationCallback callback = manager->notification_callbacks[i];
    void *user_data = manager->callback_user_data[i];
    
    if (callback) {
      callback(type, x, y, z, old_block_id, new_block_id,
               old_state_data, new_state_data, user_data);
    }
  }
}

// Caching system implementation
BlockState *block_state_manager_lookup_cached(BlockStateManager *manager,
                                               i32 x, i32 y, i32 z) {
  if (!manager || !manager->cache)
    return NULL;
    
  u32 hash = block_state_hash_position(x, y, z);
  u32 index = hash % manager->cache_capacity;
  
  // Check cache entry
  BlockStateCacheEntry *entry = &manager->cache[index];
  if (entry->is_valid && 
      entry->x == x && entry->y == y && entry->z == z) {
    
    // Update access time
    entry->last_access_time = g_current_time_ms;
    manager->stats.cached_lookups++;
    
    return &entry->state;
  }
  
  manager->stats.cache_misses++;
  return NULL;
}

void block_state_manager_cache_add(BlockStateManager *manager,
                                   const BlockState *state) {
  if (!manager || !manager->cache || !state)
    return;
    
  u32 hash = block_state_hash_position(state->x, state->y, state->z);
  u32 index = hash % manager->cache_capacity;
  
  // Add or update cache entry
  BlockStateCacheEntry *entry = &manager->cache[index];
  entry->x = state->x;
  entry->y = state->y;
  entry->z = state->z;
  entry->state = *state;
  entry->last_access_time = g_current_time_ms;
  entry->is_valid = true;
  
  // Update cache size if this is a new entry
  if (manager->cache_size < manager->cache_capacity) {
    manager->cache_size++;
  }
}

void block_state_manager_cache_remove(BlockStateManager *manager,
                                      i32 x, i32 y, i32 z) {
  if (!manager || !manager->cache)
    return;
    
  u32 hash = block_state_hash_position(x, y, z);
  u32 index = hash % manager->cache_capacity;
  
  // Invalidate cache entry
  BlockStateCacheEntry *entry = &manager->cache[index];
  if (entry->is_valid && 
      entry->x == x && entry->y == y && entry->z == z) {
    entry->is_valid = false;
    if (manager->cache_size > 0) {
      manager->cache_size--;
    }
  }
}

void block_state_manager_cache_clear(BlockStateManager *manager) {
  if (!manager || !manager->cache)
    return;
    
  // Invalidate all cache entries
  for (u32 i = 0; i < manager->cache_capacity; i++) {
    manager->cache[i].is_valid = false;
  }
  manager->cache_size = 0;
  
  LOG_DEBUG("Cleared block state cache");
}

void block_state_manager_cache_optimize(BlockStateManager *manager) {
  if (!manager || !manager->cache)
    return;
    
  // Remove old entries (simple LRU-like behavior)
  u32 current_time = g_current_time_ms;
  u32 timeout_ms = 60000; // 1 minute timeout
  
  for (u32 i = 0; i < manager->cache_capacity; i++) {
    BlockStateCacheEntry *entry = &manager->cache[i];
    if (entry->is_valid && 
        (current_time - entry->last_access_time) > timeout_ms) {
      entry->is_valid = false;
      if (manager->cache_size > 0) {
        manager->cache_size--;
      }
    }
  }
  
}

// Compression system implementation
u32 block_state_compress(BlockState *state, u8 *compressed_data, u32 max_size) {
  if (!state || !compressed_data || max_size == 0)
    return 0;
    
  u32 offset = 0;
  
  // Simple compression scheme:
  // - Use variable-length encoding for coordinates (relative to chunk origin)
  // - Pack block ID and state data together when possible
  // - Use bit flags for common boolean values
  
  // Pack position (relative encoding)
  if (offset + 12 > max_size) return 0; // 3 * 4 bytes for coordinates
  
  // Store coordinates as signed 32-bit integers
  memcpy(&compressed_data[offset], &state->x, 4);
  memcpy(&compressed_data[offset + 4], &state->y, 4);
  memcpy(&compressed_data[offset + 8], &state->z, 4);
  offset += 12;
  
  // Pack block ID and state data
  if (offset + 8 > max_size) return 0; // 4 bytes each
  
  memcpy(&compressed_data[offset], &state->block_id, 4);
  memcpy(&compressed_data[offset + 4], &state->state_data, 4);
  offset += 8;
  
  // Pack flags (needs_update, compression_flags)
  if (offset + 4 > max_size) return 0; // 4 bytes for flags
  
  u32 flags = 0;
  if (state->needs_update) flags |= 0x01;
  flags |= (state->compression_flags & 0xFE) << 1; // 7 bits for compression flags
  
  compressed_data[offset] = (u8)(flags & 0xFF);
  offset += 1;
  
  // Pack timestamp (delta compression)
  if (offset + 4 > max_size) return 0;
  
  // Store last update time relative to current time
  u32 time_delta = g_current_time_ms - state->last_update_time;
  memcpy(&compressed_data[offset], &time_delta, 4);
  offset += 4;
  
  return offset;
}

bool block_state_decompress(const u8 *compressed_data, u32 compressed_size,
                            BlockState *state) {
  if (!compressed_data || !state || compressed_size < 25) // Minimum size check
    return false;
    
  u32 offset = 0;
  
  // Unpack position
  if (offset + 12 > compressed_size) return false;
  
  memcpy(&state->x, &compressed_data[offset], 4);
  memcpy(&state->y, &compressed_data[offset + 4], 4);
  memcpy(&state->z, &compressed_data[offset + 8], 4);
  offset += 12;
  
  // Unpack block ID and state data
  if (offset + 8 > compressed_size) return false;
  
  memcpy(&state->block_id, &compressed_data[offset], 4);
  memcpy(&state->state_data, &compressed_data[offset + 4], 4);
  offset += 8;
  
  // Unpack flags
  if (offset + 1 > compressed_size) return false;
  
  u32 flags = compressed_data[offset];
  state->needs_update = (flags & 0x01) != 0;
  state->compression_flags = (flags >> 1) & 0x7F;
  offset += 1;
  
  // Unpack timestamp
  if (offset + 4 > compressed_size) return false;
  
  u32 time_delta;
  memcpy(&time_delta, &compressed_data[offset], 4);
  state->last_update_time = g_current_time_ms - time_delta;
  offset += 4;
  
  // Initialize transition pointer to NULL (transitions are handled separately)
  state->active_transition = NULL;
  
  return true;
}

void block_state_manager_compress_all(BlockStateManager *manager) {
  if (!manager || !manager->states)
    return;
    
  u8 compressed_data[BLOCK_STATE_MAX_COMPRESSION_SIZE];
  u32 total_compressed_size = 0;
  u32 compressed_count = 0;
  
  for (u32 i = 0; i < manager->count; i++) {
    BlockState *state = &manager->states[i];
    
    u32 compressed_size = block_state_compress(state, compressed_data, 
                                              BLOCK_STATE_MAX_COMPRESSION_SIZE);
    if (compressed_size > 0) {
      total_compressed_size += compressed_size;
      compressed_count++;
      
      // Mark as compressed
      state->compression_flags |= 0x01;
    }
  }
  
  f32 compression_ratio = manager->count > 0 ? 
      (f32)total_compressed_size / (manager->count * sizeof(BlockState)) : 0.0f;
  
  LOG_INFO("Compressed %u/%u block states, ratio: %.2f", 
           compressed_count, manager->count, compression_ratio);
}

void block_state_manager_optimize_memory(BlockStateManager *manager) {
  if (!manager)
    return;
    
  // Remove invalid states
  u32 valid_count = 0;
  for (u32 i = 0; i < manager->count; i++) {
    BlockState *state = &manager->states[i];
    
    // Validate state
    if (block_state_validate(state, NULL) == BLOCK_STATE_VALID) {
      // Move valid state to front if needed
      if (i != valid_count) {
        manager->states[valid_count] = *state;
      }
      valid_count++;
    }
  }
  
  u32 removed_count = manager->count - valid_count;
  manager->count = valid_count;
  
  // Optimize cache
  block_state_manager_cache_optimize(manager);
  
  // Update memory usage statistics
  manager->stats.memory_usage_bytes = 
      manager->count * sizeof(BlockState) +
      manager->transition_count * sizeof(BlockStateTransition) +
      manager->cache_size * sizeof(BlockStateCacheEntry);
  
  LOG_INFO("Memory optimization completed: removed %u invalid states, "
           "current memory usage: %llu bytes", 
}

// Serialization system implementation
bool block_state_serialize(const BlockState *state, u8 *buffer, u32 buffer_size,
                          u32 *bytes_written) {
  if (!state || !buffer || !bytes_written || buffer_size < 64)
    return false;
    
  u32 offset = 0;
  
  // Serialize header (magic number and version)
  u32 magic = 0x54415442; // "BTAT" (Block State)
  u16 version = BLOCK_STATE_SERIALIZATION_VERSION;
  
  if (offset + 6 > buffer_size) return false;
  memcpy(&buffer[offset], &magic, 4);
  memcpy(&buffer[offset + 4], &version, 2);
  offset += 6;
  
  // Serialize position
  if (offset + 12 > buffer_size) return false;
  memcpy(&buffer[offset], &state->x, 4);
  memcpy(&buffer[offset + 4], &state->y, 4);
  memcpy(&buffer[offset + 8], &state->z, 4);
  offset += 12;
  
  // Serialize block data
  if (offset + 8 > buffer_size) return false;
  memcpy(&buffer[offset], &state->block_id, 4);
  memcpy(&buffer[offset + 4], &state->state_data, 4);
  offset += 8;
  
  // Serialize flags and timestamp
  if (offset + 8 > buffer_size) return false;
  u8 flags = (state->needs_update ? 0x01 : 0x00) | 
                (state->compression_flags & 0xFE);
  buffer[offset] = flags;
  offset += 1;
  
  // Padding for alignment
  buffer[offset] = 0;
  offset += 3;
  
  memcpy(&buffer[offset], &state->last_update_time, 4);
  offset += 4;
  
  // Serialize transition info (if present)
  if (state->active_transition) {
    if (offset + 20 > buffer_size) return false;
    
    BlockStateTransition *transition = state->active_transition;
    u16 transition_type = (u16)transition->type;
    
    memcpy(&buffer[offset], &transition_type, 2);
    memcpy(&buffer[offset + 2], &transition->duration_ms, 4);
    memcpy(&buffer[offset + 6], &transition->elapsed_ms, 4);
    memcpy(&buffer[offset + 10], &transition->target_block_id, 4);
    memcpy(&buffer[offset + 14], &transition->target_state_data, 4);
    buffer[offset + 18] = transition->is_reversible ? 1 : 0;
    
    offset += 20;
  } else {
    // No transition - write zero marker
    if (offset + 2 > buffer_size) return false;
    u16 no_transition = 0xFFFF;
    memcpy(&buffer[offset], &no_transition, 2);
    offset += 2;
  }
  
  *bytes_written = offset;
  return true;
}

bool block_state_deserialize(const u8 *buffer, u32 buffer_size,
                            BlockState *state, u32 *bytes_read) {
  if (!buffer || !state || !bytes_read || buffer_size < 20)
    return false;
    
  u32 offset = 0;
  
  // Verify header
  u32 magic;
  u16 version;
  
  if (offset + 6 > buffer_size) return false;
  memcpy(&magic, &buffer[offset], 4);
  memcpy(&version, &buffer[offset + 4], 2);
  offset += 6;
  
  if (magic != 0x54415442) { // "BTAT"
    LOG_WARN("Invalid block state serialization magic number");
    return false;
  }
  
  if (version != BLOCK_STATE_SERIALIZATION_VERSION) {
    LOG_WARN("Unsupported block state serialization version: %u", version);
    return false;
  }
  
  // Deserialize position
  if (offset + 12 > buffer_size) return false;
  memcpy(&state->x, &buffer[offset], 4);
  memcpy(&state->y, &buffer[offset + 4], 4);
  memcpy(&state->z, &buffer[offset + 8], 4);
  offset += 12;
  
  // Deserialize block data
  if (offset + 8 > buffer_size) return false;
  memcpy(&state->block_id, &buffer[offset], 4);
  memcpy(&state->state_data, &buffer[offset + 4], 4);
  offset += 8;
  
  // Deserialize flags and timestamp
  if (offset + 8 > buffer_size) return false;
  u8 flags = buffer[offset];
  state->needs_update = (flags & 0x01) != 0;
  state->compression_flags = flags & 0xFE;
  offset += 4; // Skip padding
  
  memcpy(&state->last_update_time, &buffer[offset], 4);
  offset += 4;
  
  // Deserialize transition info
  if (offset + 2 > buffer_size) return false;
  
  u16 transition_marker;
  memcpy(&transition_marker, &buffer[offset], 2);
  offset += 2;
  
  if (transition_marker != 0xFFFF && offset + 18 <= buffer_size) {
    // Transition present - but we'll skip detailed deserialization
    // since transitions are managed separately
    state->active_transition = NULL;
    offset += 18;
  } else {
    state->active_transition = NULL;
  }
  
  *bytes_read = offset;
  return true;
}

bool block_state_manager_save(const BlockStateManager *manager,
                             const char *filename) {
  if (!manager || !filename)
    return false;
    
  FILE *file = fopen(filename, "wb");
  if (!file) {
    LOG_ERROR("Failed to open file for writing: %s", filename);
    return false;
  }
  
  bool success = false;
  
  // Write file header
  u32 file_magic = 0x53415442; // "BTS" (Block State)
  u16 file_version = BLOCK_STATE_SERIALIZATION_VERSION;
  u32 state_count = manager->count;
  u64 timestamp = (u64)time(NULL);
  
  if (fwrite(&file_magic, 4, 1, file) != 1) goto cleanup;
  if (fwrite(&file_version, 2, 1, file) != 1) goto cleanup;
  if (fwrite(&state_count, 4, 1, file) != 1) goto cleanup;
  if (fwrite(&timestamp, 8, 1, file) != 1) goto cleanup;
  
  // Write states
  u8 buffer[256];
  for (u32 i = 0; i < manager->count; i++) {
    u32 bytes_written;
    if (block_state_serialize(&manager->states[i], buffer, sizeof(buffer), 
                             &bytes_written)) {
      if (fwrite(buffer, 1, bytes_written, file) != bytes_written) {
        goto cleanup;
      }
    }
  }
  
  success = true;
  LOG_INFO("Saved %u block states to %s", manager->count, filename);
  
cleanup:
  fclose(file);
  return success;
}

bool block_state_manager_load(BlockStateManager *manager,
                             const char *filename) {
  if (!manager || !filename)
    return false;
    
  FILE *file = fopen(filename, "rb");
  if (!file) {
    LOG_ERROR("Failed to open file for reading: %s", filename);
    return false;
  }
  
  bool success = false;
  
  // Read file header
  u32 file_magic;
  u16 file_version;
  u32 state_count;
  u64 timestamp;
  
  if (fread(&file_magic, 4, 1, file) != 1) goto cleanup;
  if (fread(&file_version, 2, 1, file) != 1) goto cleanup;
  if (fread(&state_count, 4, 1, file) != 1) goto cleanup;
  if (fread(&timestamp, 8, 1, file) != 1) goto cleanup;
  
  if (file_magic != 0x53415442) {
    LOG_ERROR("Invalid block state file magic number");
    goto cleanup;
  }
  
  if (file_version != BLOCK_STATE_SERIALIZATION_VERSION) {
    LOG_ERROR("Unsupported block state file version: %u", file_version);
    goto cleanup;
  }
  
  // Clear existing states
  manager->count = 0;
  
  // Read states
  u8 buffer[256];
  for (u32 i = 0; i < state_count; i++) {
    u32 bytes_read;
    if (fread(buffer, 1, 20, file) != 20) goto cleanup; // Read minimum header
    
    BlockState state;
    if (block_state_deserialize(buffer, sizeof(buffer), &state, &bytes_read)) {
      // Seek back and read full state
      fseek(file, -(long)bytes_read, SEEK_CUR);
      if (fread(buffer, 1, bytes_read, file) != bytes_read) goto cleanup;
      
      if (block_state_deserialize(buffer, bytes_read, &state, &bytes_read)) {
        block_state_manager_add(manager, state.x, state.y, state.z, state.block_id);
      }
    }
  }
  
  success = true;
  LOG_INFO("Loaded %u block states from %s", manager->count, filename);
  
cleanup:
  fclose(file);
  return success;
}

// Enhanced block state manager update function
void block_state_manager_update(BlockStateManager *manager,
                                ChunkManager *chunk_manager, f32 delta_time) {
  if (!manager || !chunk_manager)
    return;

  // Update transitions first
  block_state_manager_update_transitions(manager, chunk_manager, delta_time);
  
  // Update statistics periodically
  u32 current_time = get_current_time_ms();
  if (current_time - manager->last_stats_update > 1000) { // Update every second
    block_state_manager_update_statistics(manager);
    manager->last_stats_update = current_time;
  }
  
  // Optimize cache periodically
  static u32 last_cache_optimization = 0;
  if (current_time - last_cache_optimization > 30000) { // Optimize every 30 seconds
    block_state_manager_cache_optimize(manager);
    last_cache_optimization = current_time;
  }

  // Update block states
  for (u32 i = 0; i < manager->count; i++) {
    BlockState *state = &manager->states[i];
    if (!state->needs_update)
      continue;

    // Dispatch based on block type
    bool changed = false;
    switch (state->block_id) {
      case BLOCK_WATER:
        changed = water_update_tick(chunk_manager, state->x, state->y, state->z);
        break;
        
      case BLOCK_LAVA:
        // Lava flow would be implemented here
        changed = false; // Placeholder
        break;
        
      default:
        // No update needed for other block types
        changed = false;
        break;
    }
    
    if (!changed) {
      // If stabilized, stop updating
      state->needs_update = false;
    } else {
      // State changed, update cache
      block_state_manager_cache_add(manager, state);
      manager->is_dirty = true;
    }
  }
}

// Legacy placeholders implementation
void block_update_water_flow(ChunkManager *chunk_manager, i32 x, i32 y, i32 z) {
  water_update_tick(chunk_manager, x, y, z);
}

void block_update_lava_flow(ChunkManager *chunk_manager, i32 x, i32 y, i32 z) {
  (void)chunk_manager;
  (void)x;
  (void)y;
  (void)z;
}

// Statistics and profiling system
void block_state_manager_update_statistics(BlockStateManager *manager) {
  if (!manager)
    return;
    
  // Update basic statistics
  manager->stats.total_states = manager->count;
  manager->stats.active_states = 0;
  manager->stats.transitions_active = manager->transition_count;
  
  // Count active states (those needing updates)
  for (u32 i = 0; i < manager->count; i++) {
    if (manager->states[i].needs_update) {
      manager->stats.active_states++;
    }
  }
  
  // Calculate memory usage
  manager->stats.memory_usage_bytes = 
      manager->count * sizeof(BlockState) +
      manager->transition_count * sizeof(BlockStateTransition) +
      manager->cache_size * sizeof(BlockStateCacheEntry) +
      sizeof(BlockStateManager);
  
  // Calculate cache hit ratio
  u32 total_lookups = manager->stats.cached_lookups + manager->stats.cache_misses;
  if (total_lookups > 0) {
    f32 hit_ratio = (f32)manager->stats.cached_lookups / total_lookups;
    // Could store this in stats if needed
  }
}

BlockStateStatistics block_state_manager_get_statistics(
    const BlockStateManager *manager) {
  if (!manager) {
    BlockStateStatistics empty_stats = {0};
    return empty_stats;
  }
  
  return manager->stats;
}

void block_state_manager_reset_statistics(BlockStateManager *manager) {
  if (!manager)
    return;
    
  memset(&manager->stats, 0, sizeof(BlockStateStatistics));
  manager->last_stats_update = get_current_time_ms();
  
  LOG_INFO("Block state statistics reset");
}

void block_state_manager_print_statistics(const BlockStateManager *manager) {
  if (!manager)
    return;
    
  const BlockStateStatistics *stats = &manager->stats;
  
  LOG_INFO("=== Block State Statistics ===");
  LOG_INFO("Total states: %u", stats->total_states);
  LOG_INFO("Active states: %u", stats->active_states);
  LOG_INFO("Cached lookups: %u", stats->cached_lookups);
  LOG_INFO("Cache misses: %u", stats->cache_misses);
  LOG_INFO("Active transitions: %u", stats->transitions_active);
  LOG_INFO("Validation failures: %u", stats->validation_failures);
  LOG_INFO("Average update time: %.2f ms", stats->average_update_time_ms);
  LOG_INFO("Memory usage: %llu bytes", (unsigned long long)stats->memory_usage_bytes);
  
  // Calculate cache hit ratio
  u32 total_lookups = stats->cached_lookups + stats->cache_misses;
  if (total_lookups > 0) {
    f32 hit_ratio = (f32)stats->cached_lookups / total_lookups * 100.0f;
    LOG_INFO("Cache hit ratio: %.1f%%", hit_ratio);
  }
  
  LOG_INFO("=============================");
}

// Versioning and migration system
void block_state_manager_set_version(BlockStateManager *manager,
                                     u16 major, u16 minor, u16 patch) {
  if (!manager)
    return;
    
  manager->version.major_version = major;
  manager->version.minor_version = minor;
  manager->version.patch_version = patch;
  
  LOG_INFO("Block state manager version set to %u.%u.%u", major, minor, patch);
}

bool block_state_manager_needs_migration(const BlockStateManager *manager,
                                         const BlockStateVersionInfo *target_version) {
  if (!manager || !target_version)
    return false;
    
  const BlockStateVersionInfo *current = &manager->version;
  
  // Compare major version first
  if (current->major_version != target_version->major_version) {
    return current->major_version < target_version->major_version;
  }
  
  // Compare minor version
  if (current->minor_version != target_version->minor_version) {
    return current->minor_version < target_version->minor_version;
  }
  
  // Compare patch version
  return current->patch_version < target_version->patch_version;
}

bool block_state_manager_migrate(BlockStateManager *manager,
                                 const BlockStateVersionInfo *target_version) {
  if (!manager || !target_version)
    return false;
    
  if (!block_state_manager_needs_migration(manager, target_version)) {
    LOG_INFO("No migration needed");
    return true;
  }
  
  const BlockStateVersionInfo *current = &manager->version;
  LOG_INFO("Migrating block states from %u.%u.%u to %u.%u.%u",
           current->major_version, current->minor_version, current->patch_version,
           target_version->major_version, target_version->minor_version, target_version->patch_version);
  
  // Simple migration: validate all states and update version
  u32 migrated_count = 0;
  u32 failed_count = 0;
  
  for (u32 i = 0; i < manager->count; i++) {
    BlockState *state = &manager->states[i];
    
    // Validate state against new version requirements
    if (block_state_validate(state, NULL) == BLOCK_STATE_VALID) {
      migrated_count++;
    } else {
      // Remove invalid state
      if (i < manager->count - 1) {
        manager->states[i] = manager->states[manager->count - 1];
        i--; // Re-check this position
      }
      manager->count--;
      failed_count++;
    }
  }
  
  // Update version
  manager->version = *target_version;
  
  LOG_INFO("Migration completed: %u states migrated, %u states removed",
           migrated_count, failed_count);
  
  return failed_count == 0; // Return true if all states migrated successfully
}

// Persistence across chunk unloads
void block_state_manager_persist_chunk(BlockStateManager *manager,
                                       const ChunkPos *chunk_pos) {
  if (!manager || !chunk_pos)
    return;
    
  // Create filename for chunk
  char filename[256];
  snprintf(filename, sizeof(filename), "%schunk_%d_%d_%d.bts",
           BLOCK_STATE_PERSISTENCE_DIR,
           chunk_pos->x, chunk_pos->y, chunk_pos->z);
    
  // Filter states belonging to this chunk
  u32 chunk_states = 0;
  for (u32 i = 0; i < manager->count; i++) {
    BlockState *state = &manager->states[i];
    ChunkPos state_chunk = world_to_chunk_pos(state->x, state->y, state->z);
    
    if (state_chunk.x == chunk_pos->x &&
        state_chunk.y == chunk_pos->y &&
        state_chunk.z == chunk_pos->z) {
      chunk_states++;
    }
  }
  
  if (chunk_states == 0) {
    LOG_DEBUG("No block states to persist for chunk (%d,%d,%d)",
              chunk_pos->x, chunk_pos->y, chunk_pos->z);
    return;
  }
  
  // Create temporary manager for this chunk
  BlockStateManager chunk_manager;
  block_state_manager_init(&chunk_manager, chunk_states);
  
  // Copy states for this chunk
  for (u32 i = 0; i < manager->count; i++) {
    BlockState *state = &manager->states[i];
    ChunkPos state_chunk = world_to_chunk_pos(state->x, state->y, state->z);
    
    if (state_chunk.x == chunk_pos->x &&
        state_chunk.y == chunk_pos->y &&
        state_chunk.z == chunk_pos->z) {
      block_state_manager_add(&chunk_manager, state->x, state->y, state->z, state->block_id);
    }
  }
  
  // Save chunk states
  bool success = block_state_manager_save(&chunk_manager, filename);
  
  // Cleanup
  block_state_manager_free(&chunk_manager);
  
  if (success) {
    LOG_INFO("Persisted %u block states for chunk (%d,%d,%d)",
             chunk_states, chunk_pos->x, chunk_pos->y, chunk_pos->z);
  } else {
    LOG_ERROR("Failed to persist block states for chunk (%d,%d,%d)",
              chunk_pos->x, chunk_pos->y, chunk_pos->z);
  }
}

void block_state_manager_unload_chunk(BlockStateManager *manager,
                                      const ChunkPos *chunk_pos) {
  if (!manager || !chunk_pos)
    return;
    
  // Remove states belonging to this chunk from memory
  u32 removed_count = 0;
  for (u32 i = 0; i < manager->count; i++) {
    BlockState *state = &manager->states[i];
    ChunkPos state_chunk = world_to_chunk_pos(state->x, state->y, state->z);
    
    if (state_chunk.x == chunk_pos->x &&
        state_chunk.y == chunk_pos->y &&
        state_chunk.z == chunk_pos->z) {
      
      // Remove from cache
      block_state_manager_cache_remove(manager, state->x, state->y, state->z);
      
      // Remove from array
      if (i < manager->count - 1) {
        manager->states[i] = manager->states[manager->count - 1];
        i--; // Re-check this position
      }
      manager->count--;
      removed_count++;
    }
  }
  
  LOG_INFO("Unloaded %u block states for chunk (%d,%d,%d)",
           removed_count, chunk_pos->x, chunk_pos->y, chunk_pos->z);
}

void block_state_manager_load_chunk(BlockStateManager *manager,
                                    const ChunkPos *chunk_pos) {
  if (!manager || !chunk_pos)
    return;
    
  // Create filename for chunk
  char filename[256];
  snprintf(filename, sizeof(filename), "%schunk_%d_%d_%d.bts",
           BLOCK_STATE_PERSISTENCE_DIR,
           chunk_pos->x, chunk_pos->y, chunk_pos->z);
    
  // Load chunk states
  BlockStateManager chunk_manager;
  if (block_state_manager_load(&chunk_manager, filename)) {
    // Copy states to main manager
    for (u32 i = 0; i < chunk_manager.count; i++) {
      BlockState *state = &chunk_manager.states[i];
      block_state_manager_add(manager, state->x, state->y, state->z, state->block_id);
    }
    
    LOG_INFO("Loaded %u block states for chunk (%d,%d,%d)",
             chunk_manager.count, chunk_pos->x, chunk_pos->y, chunk_pos->z);
  } else {
    LOG_DEBUG("No persisted block states found for chunk (%d,%d,%d)",
              chunk_pos->x, chunk_pos->y, chunk_pos->z);
  }
  
  // Cleanup
  block_state_manager_free(&chunk_manager);
}

bool block_state_manager_is_chunk_persisted(const BlockStateManager *manager,
                                            const ChunkPos *chunk_pos) {
  if (!manager || !chunk_pos)
    return false;
    
  // Create filename for chunk
  char filename[256];
  snprintf(filename, sizeof(filename), "%schunk_%d_%d_%d.bts",
           BLOCK_STATE_PERSISTENCE_DIR,
           chunk_pos->x, chunk_pos->y, chunk_pos->z);
    
  // Check if file exists
  FILE *file = fopen(filename, "rb");
  if (file) {
    fclose(file);
    return true;
  }
  
  return false;
}
