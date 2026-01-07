#ifndef TERRAIN_STREAMING_H
#define TERRAIN_STREAMING_H

#include "include/core/types.h"
#include "math/vec3.h"
#include "include/core/memory.h"

// Terrain tile configuration
#define TERRAIN_TILE_SIZE 64.0f  // 64x64 meter tiles
#define TERRAIN_TILE_VERTICES 129 // 128 segments + 1 for seamless tiling
#define TERRAIN_MAX_LOADED_TILES 1024
#define TERRAIN_STREAM_RADIUS 8 // Load tiles within 8 tiles of player
#define TERRAIN_UNLOAD_RADIUS 12 // Unload tiles beyond 12 tiles

// Terrain data representation
typedef enum {
    TERRAIN_DATA_HEIGHTMAP = 1,
    TERRAIN_DATA_SPLATMAP = 2,
    TERRAIN_DATA_HOLE_MAP = 4,
    TERRAIN_DATA_ALL = 0xFF
} TerrainDataFlags;

// Heightmap storage (16-bit precision)
typedef struct {
    u16 heights[TERRAIN_TILE_VERTICES][TERRAIN_TILE_VERTICES];
    f32 scale_y; // Height scaling factor
    f32 min_height, max_height;
} TerrainHeightmap;

// Splatmap for texture blending (Rock, Grass, Dirt, etc.)
typedef struct {
    u8 weights[TERRAIN_TILE_VERTICES][TERRAIN_TILE_VERTICES][4]; // RGBA channels
    const char* texture_names[4];
} TerrainSplatmap;

// Hole map for cutting caves
typedef struct {
    u8 holes[TERRAIN_TILE_VERTICES][TERRAIN_TILE_VERTICES]; // 0 = solid, 1 = hole
} TerrainHoleMap;

// Tile coordinate system
typedef struct {
    i32 x, z; // Tile coordinates in tile space (not world space)
} TerrainTileCoord;

typedef struct {
    i32 x, z; // World coordinates in tile units
} TerrainWorldCoord;

// Individual terrain tile
typedef struct {
    TerrainTileCoord coord;
    TerrainWorldCoord world_coord;
    
    // Terrain data
    TerrainHeightmap* heightmap;
    TerrainSplatmap* splatmap;
    TerrainHoleMap* holemap;
    
    // Loading state
    enum {
        TILE_STATE_UNLOADED,
        TILE_STATE_LOADING,
        TILE_STATE_LOADED_RAM,
        TILE_STATE_UPLOADING_GPU,
        TILE_STATE_LOADED_GPU,
        TILE_STATE_UNLOADING
    } state;
    
    // Priority for streaming (higher = more important)
    f32 priority;
    
    // Distance from player for LOD calculations
    f32 distance_from_player;
    
    // GPU resources
    u32 heightmap_texture_id;
    u32 splatmap_texture_id;
    u32 mesh_vertex_buffer_id;
    u32 mesh_index_buffer_id;
    
    // Timestamps for eviction
    u64 last_access_time;
    u64 load_time;
    
    // Reference counting
    u32 ref_count;
} TerrainTile;

// Eviction policy configuration
typedef enum {
    EVICTION_POLICY_LRU = 0,      // Least Recently Used
    EVICTION_POLICY_DISTANCE,     // Farthest from player
    EVICTION_POLICY_PRIORITY,    // Lowest priority
    EVICTION_POLICY_HYBRID        // Combination of factors
} TerrainEvictionPolicy;

// Eviction statistics
typedef struct {
    u32 tiles_evicted_total;
    u32 tiles_evicted_this_frame;
    u64 eviction_time_ms;
    f32 average_tile_age;
    f32 memory_usage_ratio;
} TerrainEvictionStats;

// Terrain streaming system
typedef struct {
    // Tile storage
    TerrainTile* tiles[TERRAIN_MAX_LOADED_TILES];
    u32 tile_count;
    
    // Player position for streaming center
    Vec3 player_position;
    TerrainTileCoord player_tile_coord;
    
    // Streaming configuration
    f32 stream_radius;
    f32 unload_radius;
    u32 max_loaded_tiles;
    
    // Eviction system
    TerrainEvictionPolicy eviction_policy;
    TerrainEvictionStats eviction_stats;
    f32 eviction_threshold;        // Memory usage threshold for eviction
    u32 max_evictions_per_frame;  // Limit evictions per frame
    
    // Memory pools
    MemoryPool* heightmap_pool;
    MemoryPool* splatmap_pool;
    MemoryPool* holemap_pool;
    
    // Threading
    volatile bool streaming_active;
    Thread* streaming_thread;
    
    // Statistics
    u32 tiles_loaded_total;
    u32 tiles_unloaded_total;
    u64 streaming_time_ms;
    
    // World origin shifting for large worlds
    Vec3 world_origin;
    Vec3 world_origin_offset;
} TerrainStreamingSystem;

// Core API functions
void terrain_streaming_init(TerrainStreamingSystem* system);
void terrain_streaming_shutdown(TerrainStreamingSystem* system);
void terrain_streaming_update(TerrainStreamingSystem* system, Vec3 player_position, f32 delta_time);

// Tile management
TerrainTile* terrain_streaming_get_tile(TerrainStreamingSystem* system, TerrainTileCoord coord);
TerrainTile* terrain_streaming_load_tile(TerrainStreamingSystem* system, TerrainTileCoord coord);
void terrain_streaming_unload_tile(TerrainStreamingSystem* system, TerrainTile* tile);
bool terrain_streaming_is_tile_loaded(TerrainStreamingSystem* system, TerrainTileCoord coord);

// Coordinate conversion
TerrainTileCoord terrain_world_to_tile_coord(Vec3 world_pos);
Vec3 terrain_tile_to_world_center(TerrainTileCoord coord);
TerrainWorldCoord terrain_tile_to_world_coord(TerrainTileCoord coord);

// Priority calculations
f32 terrain_calculate_tile_priority(TerrainStreamingSystem* system, TerrainTileCoord coord);
f32 terrain_calculate_tile_distance(TerrainStreamingSystem* system, TerrainTileCoord coord);

// LOD system
u32 terrain_get_tile_lod_level(TerrainTile* tile);
void terrain_update_tile_lod(TerrainTile* tile, f32 distance);

// World origin shifting
void terrain_shift_world_origin(TerrainStreamingSystem* system, Vec3 new_origin);
bool terrain_should_shift_origin(TerrainStreamingSystem* system);

// Eviction system
void terrain_streaming_set_eviction_policy(TerrainStreamingSystem* system, TerrainEvictionPolicy policy);
void terrain_streaming_process_eviction(TerrainStreamingSystem* system);
TerrainTile* terrain_streaming_select_eviction_candidate(TerrainStreamingSystem* system);
bool terrain_streaming_should_evict(TerrainStreamingSystem* system);
void terrain_streaming_get_eviction_stats(TerrainStreamingSystem* system, TerrainEvictionStats* stats);

// Utility functions
bool terrain_is_tile_in_range(TerrainTileCoord center, TerrainTileCoord tile, f32 radius);
TerrainTileCoord terrain_get_tile_neighbors(TerrainTileCoord coord, u32 direction); // 0=North, 1=East, 2=South, 3=West

#endif // TERRAIN_STREAMING_H
