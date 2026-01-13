// include/chunk/chunk.h
//
// Purpose: Defines the core data structures and API for managing individual
// chunks within the game world. Chunks are fundamental units for storing block
// data, lighting information (skylight and blocklight), and mesh data for
// rendering. This header also includes the `ChunkManager` structure for
// handling multiple chunks and utility functions for coordinate conversions and
// block access.
//
// Public APIs:
// - `ChunkPos`: Structure representing the 3D coordinates of a chunk.
// - `ChunkState`: Enumeration defining the various states a chunk can be in
//   (e.g., unloaded, loaded, meshed).
// - `LightType`: Enumeration for different types of light (sky or block light).
// - `Chunk`: The main structure holding block IDs, light levels, mesh data,
//   and metadata for a single chunk.
// - `ChunkManager`: Structure for managing a collection of `Chunk` objects,
//   including thread-safe access (with a read-write lock) and LRU eviction.
// - `chunk_pos`, `chunk_pos_equal`, `world_to_chunk_pos`, `chunk_to_world_pos`:
//   Inline helper functions for coordinate conversions.
// - `chunk_block_index`: Inline helper for calculating the linear index of a
// block within a chunk.
// - `chunk_manager_init`, `chunk_manager_free`, `chunk_manager_get`,
// `chunk_manager_get_or_create`, etc.:
//   Functions for `ChunkManager` lifecycle, retrieval, unloading, and neighbor
//   management.
// - `chunk_get_block`, `chunk_set_block`, `chunk_get_light`, `chunk_set_light`:
//   Functions for accessing and modifying block and light data within a chunk.
// - `chunk_mark_mesh_dirty`, `chunk_needs_mesh_update`: Functions for managing
// chunk mesh regeneration.
//
// Ownership: The `ChunkManager` owns an array of `Chunk` objects, managing
// their allocation and deallocation. Individual `Chunk` objects manage their
// internal block, light, and mesh data buffers.
//
// Invariants:
// - A `ChunkManager` must be initialized with `chunk_manager_init` and freed
// with `chunk_manager_free`.
// - `Chunk` data (blocks, skylight, blocklight) are dynamically allocated
// arrays.
// - `ChunkPos` values represent integral chunk coordinates, not world
// coordinates.
// - `CHUNK_SIZE` and `CHUNK_SIZE_SQ` are assumed to be compile-time constants
// defining chunk dimensions.
//
#ifndef CHUNK_H
#define CHUNK_H

#include "../block/block.h"
#include "../game_common.h"
#include <math/mat4.h>
#include <math/vec3.h>
#include <pthread.h>

// Forward declarations for optional VFX/audio hooks and tile-entity types
struct ParticleSystem;
struct AudioSystem;
struct BrewingStand;

// Chunk coordinates
typedef struct {
  i32 x, y, z;
} ChunkPos;

// Chunk state
typedef enum {
  CHUNK_STATE_UNLOADED,
  CHUNK_STATE_LOADING,
  CHUNK_STATE_LOADED,
  CHUNK_STATE_GENERATING,
  CHUNK_STATE_GENERATED,
  CHUNK_STATE_MESHING,
  CHUNK_STATE_READY
} ChunkState;

typedef enum { LIGHT_TYPE_SKY, LIGHT_TYPE_BLOCK } ChunkLightType;

// Chunk structure - optimized for cache performance
typedef struct Chunk {
  ChunkPos pos;
  ChunkState state;

  // Block data (CHUNK_SIZE_CUBE blocks)
  // Stored as array for cache-friendly access: [z][y][x]
  BlockID *blocks;

  // Lighting data (0-15 per block)
  // Lighting data (0-15 per block)
  u8 *skylight;   // Sun/moon light
  u8 *blocklight; // Torch/lava light
  u8 *metadata;   // Extra data (liquid level, orientation, etc.)

  // Mesh data (generated on demand)
  struct {
    void *vertices; // Vertex buffer
    void *indices;  // Index buffer
    u32 vertex_count;
    u32 index_count;
    bool dirty; // Needs regeneration

    // GPU Resources
    void *vertex_buffer;
    void *vertex_buffer_memory;
    void *index_buffer;
    void *index_buffer_memory;
  } mesh;

  // Neighbor chunks (for face culling)
  struct Chunk *neighbors[6]; // [pos_x, neg_x, pos_y, neg_y, pos_z, neg_z]

  // Chunk management
  u64 last_accessed; // For LRU eviction
  bool modified;     // Needs saving
  // Profiling (filled by worker threads, read on main thread)
  f32 gen_time_ms;
  f32 mesh_time_ms;
  bool gen_time_pending;
  bool mesh_time_pending;
} Chunk;

// Chunk loading priority levels
typedef enum {
  CHUNK_PRIORITY_CRITICAL = 0, // Player's current chunk and immediate neighbors
  CHUNK_PRIORITY_HIGH = 1,     // Chunks within render distance
  CHUNK_PRIORITY_MEDIUM = 2,   // Chunks in preload zone
  CHUNK_PRIORITY_LOW = 3,      // Background chunks
  CHUNK_PRIORITY_COUNT = 4
} ChunkPriority;

// Chunk loading request
typedef struct {
  ChunkPos pos;
  ChunkPriority priority;
  f32 distance_to_player;
  u64 request_time;
  bool is_preload;
} ChunkLoadRequest;

// Priority queue for chunk loading
#define MAX_CHUNK_QUEUE_SIZE 512
typedef struct {
  ChunkLoadRequest requests[MAX_CHUNK_QUEUE_SIZE];
  u32 count;
  u32 capacity;
} ChunkPriorityQueue;

// Streaming system state
typedef struct {
  Vec3 last_player_position;
  Vec3 player_velocity;
  f32 streaming_radius;
  f32 preload_radius;
  u32 max_chunks_per_frame;
  bool enabled;
} ChunkStreamingState;

// Chunk manager
typedef struct ChunkManager {
  Chunk *chunks;
  u32 count;
  u32 capacity;
  u64 access_counter;

  // Registered tile-entities (minimal list for brewing stands / future types)
  struct BrewingStand **brewing_stands;
  u32 brewing_stand_count;
  u32 brewing_stand_capacity;

  // Streaming and priority queue
  ChunkPriorityQueue load_queue;
  ChunkStreamingState streaming;
  ChunkPos *preload_positions;
  u32 preload_count;
  u32 preload_capacity;

#ifndef PLATFORM_WEB
  pthread_rwlock_t lock; // Read-write lock for thread safety
#endif
} ChunkManager;

// Register/unregister brewing stands (tile-entity plumbing)
void chunk_manager_register_brewing_stand(ChunkManager *manager,
                                          struct BrewingStand *stand);
void chunk_manager_unregister_brewing_stand(ChunkManager *manager,
                                            struct BrewingStand *stand);
// Update and render
void chunk_manager_update(ChunkManager *manager, f32 delta_time);

// Update and render

//  // Moved to
// end of file

// Chunk position helpers
INLINE ChunkPos chunk_pos(i32 x, i32 y, i32 z) {
  ChunkPos p = {x, y, z};
  return p;
}

INLINE bool chunk_pos_equal(ChunkPos a, ChunkPos b) {
  return a.x == b.x && a.y == b.y && a.z == b.z;
}

// Convert world position to chunk position
INLINE ChunkPos world_to_chunk_pos(i32 wx, i32 wy, i32 wz) {
  return chunk_pos(
      wx < 0 ? (wx - CHUNK_SIZE + 1) / CHUNK_SIZE : wx / CHUNK_SIZE,
      wy < 0 ? (wy - CHUNK_SIZE + 1) / CHUNK_SIZE : wy / CHUNK_SIZE,
      wz < 0 ? (wz - CHUNK_SIZE + 1) / CHUNK_SIZE : wz / CHUNK_SIZE);
}

// Convert chunk position to world position (min corner)
INLINE void chunk_to_world_pos(ChunkPos cp, i32 *wx, i32 *wy, i32 *wz) {
  *wx = cp.x * CHUNK_SIZE;
  *wy = cp.y * CHUNK_SIZE;
  *wz = cp.z * CHUNK_SIZE;
}

// Get block index in chunk
INLINE u32 chunk_block_index(i32 x, i32 y, i32 z) {
  // Ensure coordinates are in range [0, CHUNK_SIZE)
  x = x & (CHUNK_SIZE - 1);
  y = y & (CHUNK_SIZE - 1);
  z = z & (CHUNK_SIZE - 1);
  return z * CHUNK_SIZE_SQ + y * CHUNK_SIZE + x;
}

// Chunk manager functions
void chunk_manager_init(ChunkManager *manager, u32 capacity);
void chunk_manager_free(ChunkManager *manager);
Chunk *chunk_manager_get(ChunkManager *manager, ChunkPos pos);
Chunk *chunk_manager_get_or_create(ChunkManager *manager, ChunkPos pos);
void chunk_manager_unload(ChunkManager *manager, ChunkPos pos);
void chunk_manager_update_neighbors(ChunkManager *manager, ChunkPos pos);
void chunk_manager_unload_distant(ChunkManager *manager, Vec3 position,
                                  f32 max_distance);
u32 chunk_manager_get_chunks_in_radius(ChunkManager *manager, Vec3 position,
                                       f32 radius, Chunk **out_chunks,
                                       u32 max_count);
void chunk_manager_evict_lru(ChunkManager *manager);
BlockID chunk_manager_get_block(ChunkManager *manager, i32 x, i32 y, i32 z);

// Update and render
void chunk_manager_render(ChunkManager *manager, void *renderer, Mat4 view,
                          Mat4 proj);

// Chunk block/light operations
BlockID chunk_get_block(Chunk *chunk, i32 x, i32 y, i32 z);
void chunk_set_block(Chunk *chunk, i32 x, i32 y, i32 z, BlockID block);
u8 chunk_get_light(Chunk *chunk, i32 x, i32 y, i32 z, ChunkLightType type);
void chunk_set_light(Chunk *chunk, i32 x, i32 y, i32 z, u8 level,
                     ChunkLightType type);

// Metadata access
u8 chunk_get_metadata(Chunk *chunk, i32 x, i32 y, i32 z);
void chunk_set_metadata(Chunk *chunk, i32 x, i32 y, i32 z, u8 data);

// Chunk mesh operations
void chunk_mark_mesh_dirty(Chunk *chunk);
bool chunk_needs_mesh_update(Chunk *chunk);

// Chunk validation and statistics
typedef struct {
  u64 total_chunks_created;
  u64 total_chunks_loaded;
  u64 total_chunks_saved;
  u64 total_chunks_unloaded;
  u64 total_blocks_modified;
  u64 total_serializations;
  u64 total_deserializations;
  f32 avg_serialization_time_ms;
  f32 avg_deserialization_time_ms;
  f32 max_serialization_time_ms;
  f32 max_deserialization_time_ms;
  u64 validation_errors;
  u64 validation_repairs;
} ChunkStatistics;

typedef struct {
  bool is_valid;
  bool has_corrupted_blocks;
  bool has_invalid_lighting;
  bool has_missing_neighbors;
  bool has_invalid_metadata;
  u32 error_count;
  char error_messages[512];
} ChunkValidationResult;

// Validation
ChunkValidationResult chunk_validate(Chunk *chunk);
bool chunk_repair(Chunk *chunk, ChunkValidationResult *validation);

// Statistics
void chunk_manager_get_statistics(ChunkManager *manager,
                                  ChunkStatistics *out_stats);
void chunk_manager_reset_statistics(ChunkManager *manager);

// Streaming and priority queue
void chunk_manager_init_streaming(ChunkManager *manager, f32 streaming_radius,
                                  f32 preload_radius);
void chunk_manager_update_streaming(ChunkManager *manager, Vec3 player_pos,
                                    Vec3 player_velocity, f32 delta_time);
void chunk_manager_queue_chunk_load(ChunkManager *manager, ChunkPos pos,
                                    ChunkPriority priority, f32 distance);
void chunk_manager_process_load_queue(ChunkManager *manager,
                                      u32 max_chunks_per_frame);
ChunkPriority chunk_manager_calculate_priority(ChunkManager *manager,
                                               ChunkPos pos, Vec3 player_pos);
void chunk_manager_preload_chunks(ChunkManager *manager, Vec3 player_pos,
                                  Vec3 player_velocity);

#endif // CHUNK_H
