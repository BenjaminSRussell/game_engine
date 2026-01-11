// include/world/world_data.h
//
// Purpose: Defines the abstract WorldData interface that decouples world
// representation from rendering. This allows the same game logic to work with:
// - VoxelWorldData: 3D grid of blocks (traditional BlockGame)
// - HeightmapWorldData: 2D heightmap with entity stacking (2.5D games)
//
// Public APIs:
// - `IWorldData`: Abstract world data interface
// - `WorldDataType`: Enumeration for different world representations
// - Concrete implementations for voxel and heightmap worlds
//
// Ownership: World data implementations manage their own storage. The interface
// provides a unified API for accessing world data regardless of representation.
//
// Invariants:
// - All function pointers must be non-NULL after initialization
// - World data must be initialized before use
//
#ifndef WORLD_DATA_H
#define WORLD_DATA_H

#include "../game_common.h"
#include <math/vec3.h>
#include "../block/block.h"
#include "../chunk/chunk.h"

// Forward declarations
struct ChunkManager;

// World data type enumeration
typedef enum {
    WORLD_DATA_TYPE_VOXEL,      // 3D grid of blocks
    WORLD_DATA_TYPE_HEIGHTMAP,  // 2D heightmap with entity stacking
    WORLD_DATA_TYPE_HYBRID,     // Hybrid (voxel + heightmap)
    WORLD_DATA_TYPE_COUNT
} WorldDataType;

// World data query result
typedef struct {
    BlockID block_id;
    u8 light_level;
    u8 metadata;
    bool valid;
} WorldDataQuery;

// Heightmap data (for 2.5D)
typedef struct {
    i16 height;              // Height at this position
    BlockID surface_block;   // Surface block type
    BlockID base_block;      // Base/filler block type
    u8 light_level;
} HeightmapCell;

// Abstract world data interface
typedef struct IWorldData {
    // World data type
    WorldDataType type;
    
    // Lifecycle
    bool (*init)(struct IWorldData *self, WorldDataType type, struct ChunkManager *chunk_manager);
    void (*cleanup)(struct IWorldData *self);
    
    // Block access (unified API for both voxel and heightmap)
    BlockID (*get_block)(struct IWorldData *self, i32 x, i32 y, i32 z);
    void (*set_block)(struct IWorldData *self, i32 x, i32 y, i32 z, BlockID block_id);
    bool (*is_solid)(struct IWorldData *self, i32 x, i32 y, i32 z);
    bool (*is_transparent)(struct IWorldData *self, i32 x, i32 y, i32 z);
    
    // Heightmap access (for 2.5D)
    i16 (*get_height)(struct IWorldData *self, i32 x, i32 z);
    void (*set_height)(struct IWorldData *self, i32 x, i32 z, i16 height);
    HeightmapCell (*get_heightmap_cell)(struct IWorldData *self, i32 x, i32 z);
    void (*set_heightmap_cell)(struct IWorldData *self, i32 x, i32 z, HeightmapCell cell);
    
    // Lighting
    u8 (*get_light)(struct IWorldData *self, i32 x, i32 y, i32 z);
    void (*set_light)(struct IWorldData *self, i32 x, i32 y, i32 z, u8 level);
    
    // Metadata
    u8 (*get_metadata)(struct IWorldData *self, i32 x, i32 y, i32 z);
    void (*set_metadata)(struct IWorldData *self, i32 x, i32 y, i32 z, u8 metadata);
    
    // Chunk management
    Chunk *(*get_chunk)(struct IWorldData *self, ChunkPos pos);
    Chunk *(*get_or_create_chunk)(struct IWorldData *self, ChunkPos pos);
    void (*mark_chunk_dirty)(struct IWorldData *self, ChunkPos pos);
    
    // Ray casting (for both voxel and heightmap)
    bool (*raycast)(struct IWorldData *self, Vec3 start, Vec3 direction, f32 max_distance,
                    Vec3 *hit_pos, Vec3 *hit_normal, BlockID *hit_block);
    
    // Height query (for 2.5D - get topmost block at x,z)
    i32 (*get_top_block_y)(struct IWorldData *self, i32 x, i32 z);
    
    // Conversion utilities
    Vec3 (*world_to_chunk_pos)(struct IWorldData *self, Vec3 world_pos);
    Vec3 (*chunk_to_world_pos)(struct IWorldData *self, ChunkPos chunk_pos);
    
    // Internal implementation data
    void *impl_data;
    struct ChunkManager *chunk_manager;
} IWorldData;

// Factory function to create world data
IWorldData *world_data_create(WorldDataType type, struct ChunkManager *chunk_manager);
void world_data_destroy(IWorldData *world_data);

// Convenience macros
#define WORLD_DATA_GET_BLOCK(world, x, y, z) ((world)->get_block((world), (x), (y), (z)))
#define WORLD_DATA_SET_BLOCK(world, x, y, z, block) ((world)->set_block((world), (x), (y), (z), (block)))
#define WORLD_DATA_GET_HEIGHT(world, x, z) ((world)->get_height((world), (x), (z)))
#define WORLD_DATA_IS_SOLID(world, x, y, z) ((world)->is_solid((world), (x), (y), (z)))

#endif // WORLD_DATA_H

