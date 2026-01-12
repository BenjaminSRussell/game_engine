// include/block/block_states.h
#ifndef BLOCK_STATES_H
#define BLOCK_STATES_H

#include "../chunk/chunk.h"
#include "../game_common.h"
#include "../containers/hashmap.h"
#include "../containers/dynamic_array.h"

// Forward declarations
typedef struct ChunkManager ChunkManager;
typedef struct PhysicsWorld PhysicsWorld;
typedef struct BlockRegistry BlockRegistry;
struct World;

// Block state transition types
typedef enum {
  BLOCK_STATE_TRANSITION_NONE = 0,
  BLOCK_STATE_TRANSITION_WATER_FLOW,
  BLOCK_STATE_TRANSITION_LAVA_FLOW,
  BLOCK_STATE_TRANSITION_FALLING_BLOCK,
  BLOCK_STATE_TRANSITION_FREEZING,
  BLOCK_STATE_TRANSITION_MELTING,
  BLOCK_STATE_TRANSITION_EVAPORATION,
  BLOCK_STATE_TRANSITION_COUNT
} BlockStateTransitionType;

// Block state validation result
typedef enum {
  BLOCK_STATE_VALID = 0,
  BLOCK_STATE_INVALID_POSITION,
  BLOCK_STATE_INVALID_BLOCK_TYPE,
  BLOCK_STATE_INVALID_STATE_DATA,
  BLOCK_STATE_OUT_OF_BOUNDS
} BlockStateValidationResult;

// Block state notification types
typedef enum {
  BLOCK_STATE_NOTIFICATION_ADDED = 0,
  BLOCK_STATE_NOTIFICATION_REMOVED,
  BLOCK_STATE_NOTIFICATION_CHANGED,
  BLOCK_STATE_NOTIFICATION_TRANSITION
} BlockStateNotificationType;

// Block state notification callback
typedef void (*BlockStateNotificationCallback)(
    BlockStateNotificationType type,
    i32 x, i32 y, i32 z,
    BlockID old_block_id,
    BlockID new_block_id,
    u32 old_state_data,
    u32 new_state_data,
    void *user_data
);

// Block state transition
typedef struct {
  BlockStateTransitionType type;
  u32 duration_ms;
  u32 elapsed_ms;
  BlockID target_block_id;
  u32 target_state_data;
  bool is_reversible;
} BlockStateTransition;

// Block state statistics
typedef struct {
  u32 total_states;
  u32 active_states;
  u32 cached_lookups;
  u32 cache_misses;
  u32 transitions_active;
  u32 validation_failures;
  f32 average_update_time_ms;
  u64 memory_usage_bytes;
} BlockStateStatistics;

// Block state cache entry
typedef struct {
  i32 x, y, z;
  BlockState state;
  u32 last_access_time;
  bool is_valid;
} BlockStateCacheEntry;

// Block state version info
typedef struct {
  u16 major_version;
  u16 minor_version;
  u16 patch_version;
  u32 serialization_format;
} BlockStateVersionInfo;

// Block State Structure (enhanced)
typedef struct {
  i32 x, y, z;
  BlockID block_id;
  u32 state_data; // Packed state information
  bool needs_update;
  u32 last_update_time;
  BlockStateTransition *active_transition;
  u32 compression_flags;
} BlockState;

// Block State Manager Structure (enhanced)
typedef struct {
  BlockState *states;
  u32 count;
  u32 capacity;
  
  // Transition system
  BlockStateTransition *transitions;
  u32 transition_count;
  u32 transition_capacity;
  
  // Caching system
  BlockStateCacheEntry *cache;
  u32 cache_size;
  u32 cache_capacity;
  HashMap *state_lookup;
  
  // Notification system
  BlockStateNotificationCallback *notification_callbacks;
  void **callback_user_data;
  u32 callback_count;
  u32 callback_capacity;
  
  // Versioning and serialization
  BlockStateVersionInfo version;
  bool is_dirty;
  
  // Statistics and profiling
  BlockStateStatistics stats;
  u32 last_stats_update;
} BlockStateManager;

// Block state functions
void block_update_water_flow(ChunkManager *chunk_manager, i32 x, i32 y, i32 z);
void block_update_lava_flow(ChunkManager *chunk_manager, i32 x, i32 y, i32 z);
void block_update_falling(ChunkManager *chunk_manager,
                          BlockRegistry *block_registry, struct World *world,
                          PhysicsWorld *physics_world, i32 x, i32 y, i32 z);

void falling_block_system_update(struct World *world,
                                 ChunkManager *chunk_manager,
                                 BlockRegistry *block_registry,
                                 PhysicsWorld *physics_world, f32 delta_time);

// Manager functions
void block_state_manager_init(BlockStateManager *manager, u32 capacity);
void block_state_manager_free(BlockStateManager *manager);
void block_state_manager_update(BlockStateManager *manager,
                                ChunkManager *chunk_manager, f32 delta_time);
void block_state_manager_add(BlockStateManager *manager, i32 x, i32 y, i32 z,
                             BlockID block_id);

// Transition system
void block_state_manager_add_transition(BlockStateManager *manager,
                                        i32 x, i32 y, i32 z,
                                        BlockStateTransitionType type,
                                        BlockID target_block_id,
                                        u32 target_state_data,
                                        u32 duration_ms);
void block_state_manager_update_transitions(BlockStateManager *manager,
                                            ChunkManager *chunk_manager,
                                            f32 delta_time);
bool block_state_manager_has_transition(BlockStateManager *manager,
                                        i32 x, i32 y, i32 z);

// Validation system
BlockStateValidationResult block_state_validate(BlockState *state,
                                                 ChunkManager *chunk_manager);
bool block_state_is_valid_position(i32 x, i32 y, i32 z,
                                   ChunkManager *chunk_manager);
bool block_state_is_valid_block_type(BlockID block_id);
bool block_state_is_valid_state_data(BlockID block_id, u32 state_data);

// Compression system
u32 block_state_compress(BlockState *state, u8 *compressed_data, u32 max_size);
bool block_state_decompress(const u8 *compressed_data, u32 compressed_size,
                            BlockState *state);
void block_state_manager_compress_all(BlockStateManager *manager);
void block_state_manager_optimize_memory(BlockStateManager *manager);

// Serialization system
bool block_state_serialize(const BlockState *state, u8 *buffer, u32 buffer_size,
                          u32 *bytes_written);
bool block_state_deserialize(const u8 *buffer, u32 buffer_size,
                            BlockState *state, u32 *bytes_read);
bool block_state_manager_save(const BlockStateManager *manager,
                             const char *filename);
bool block_state_manager_load(BlockStateManager *manager,
                             const char *filename);

// Notification system
void block_state_manager_add_notification_callback(
    BlockStateManager *manager,
    BlockStateNotificationCallback callback,
    void *user_data);
void block_state_manager_remove_notification_callback(
    BlockStateManager *manager,
    BlockStateNotificationCallback callback);
void block_state_manager_notify_change(BlockStateManager *manager,
                                       BlockStateNotificationType type,
                                       i32 x, i32 y, i32 z,
                                       BlockID old_block_id,
                                       BlockID new_block_id,
                                       u32 old_state_data,
                                       u32 new_state_data);

// Caching system
BlockState *block_state_manager_lookup_cached(BlockStateManager *manager,
                                               i32 x, i32 y, i32 z);
void block_state_manager_cache_add(BlockStateManager *manager,
                                   const BlockState *state);
void block_state_manager_cache_remove(BlockStateManager *manager,
                                      i32 x, i32 y, i32 z);
void block_state_manager_cache_clear(BlockStateManager *manager);
void block_state_manager_cache_optimize(BlockStateManager *manager);

// Versioning and migration
void block_state_manager_set_version(BlockStateManager *manager,
                                     u16 major, u16 minor, u16 patch);
bool block_state_manager_needs_migration(const BlockStateManager *manager,
                                         const BlockStateVersionInfo *target_version);
bool block_state_manager_migrate(BlockStateManager *manager,
                                 const BlockStateVersionInfo *target_version);

// Statistics and profiling
void block_state_manager_update_statistics(BlockStateManager *manager);
BlockStateStatistics block_state_manager_get_statistics(
    const BlockStateManager *manager);
void block_state_manager_reset_statistics(BlockStateManager *manager);
void block_state_manager_print_statistics(const BlockStateManager *manager);

// Persistence across chunk unloads
void block_state_manager_persist_chunk(BlockStateManager *manager,
                                       const ChunkPos *chunk_pos);
void block_state_manager_unload_chunk(BlockStateManager *manager,
                                      const ChunkPos *chunk_pos);
void block_state_manager_load_chunk(BlockStateManager *manager,
                                    const ChunkPos *chunk_pos);
bool block_state_manager_is_chunk_persisted(const BlockStateManager *manager,
                                            const ChunkPos *chunk_pos);

// Utility functions
u32 block_state_hash_position(i32 x, i32 y, i32 z);
bool block_state_positions_equal(i32 x1, i32 y1, i32 z1,
                                 i32 x2, i32 y2, i32 z2);
const char *block_state_transition_type_to_string(
    BlockStateTransitionType type);
const char *block_state_validation_result_to_string(
    BlockStateValidationResult result);

#endif // BLOCK_STATES_H
