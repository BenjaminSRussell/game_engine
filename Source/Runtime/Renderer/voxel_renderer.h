// Voxel Renderer Implementation
// 3D voxel-based rendering system with block-based world representation

#ifndef VOXEL_RENDERER_H
#define VOXEL_RENDERER_H

#include "core/types.h"
#include "rendering/core/renderer.h"
#include "rendering/core/texture.h"
#include "rendering/core/buffer.h"
#include "engine/include/math/math_all.h"

#ifdef __cplusplus
extern "C" {
#endif

// Voxel block types
typedef enum {
    BLOCK_TYPE_AIR = 0,
    BLOCK_TYPE_STONE = 1,
    BLOCK_TYPE_GRASS = 2,
    BLOCK_TYPE_DIRT = 3,
    BLOCK_TYPE_WOOD = 4,
    BLOCK_TYPE_LEAVES = 5,
    BLOCK_TYPE_WATER = 6,
    BLOCK_TYPE_SAND = 7,
    BLOCK_TYPE_COAL = 8,
    BLOCK_TYPE_IRON = 9,
    BLOCK_TYPE_GOLD = 10,
    BLOCK_TYPE_DIAMOND = 11,
    BLOCK_TYPE_COUNT
} VoxelBlockType;

// Voxel vertex structure
typedef struct {
    Vec3 position;
    Vec3 normal;
    Vec2 uv;
    u8 ao;          // Ambient occlusion
    u8 light;       // Light level
    u16 block_id;   // Block type ID
} VoxelVertex;

// Voxel chunk structure
typedef struct {
    Vec3 position;   // World position
    u32 size_x, size_y, size_z;
    u16 *blocks;     // 3D block data (flattened)
    Buffer vertex_buffer;
    Buffer index_buffer;
    u32 vertex_count;
    u32 index_count;
    bool dirty;       // Needs mesh rebuild
    bool visible;     // In view frustum
} VoxelChunk;

// Voxel renderer context
typedef struct VoxelRenderer {
    // Rendering resources
    Texture block_texture;
    Buffer uniform_buffer;
    
    // Chunk management
    VoxelChunk *chunks;
    u32 chunk_count;
    u32 max_chunks;
    
    // Rendering state
    Vec3 camera_position;
    Vec3 camera_direction;
    Mat4 view_matrix;
    Mat4 projection_matrix;
    Mat4 view_projection_matrix;
    
    // Performance
    u32 chunks_rendered;
    u32 vertices_rendered;
    u32 triangles_rendered;
    
    bool initialized;
} VoxelRenderer;

// Create voxel renderer
VoxelRenderer *voxel_renderer_create(void);

// Destroy voxel renderer
void voxel_renderer_destroy(VoxelRenderer *renderer);

// Update renderer state
void voxel_renderer_update(VoxelRenderer *renderer, float delta_time);

// Render frame
void voxel_renderer_render(VoxelRenderer *renderer, const Mat4 *view, const Mat4 *projection);

// Chunk management
VoxelChunk *voxel_renderer_create_chunk(VoxelRenderer *renderer, const Vec3 *position, u32 size_x, u32 size_y, u32 size_z);
void voxel_renderer_destroy_chunk(VoxelRenderer *renderer, VoxelChunk *chunk);
void voxel_renderer_set_block(VoxelRenderer *renderer, VoxelChunk *chunk, u32 x, u32 y, u32 z, VoxelBlockType block_type);
VoxelBlockType voxel_renderer_get_block(VoxelRenderer *renderer, const VoxelChunk *chunk, u32 x, u32 y, u32 z);

// Mesh generation
void voxel_renderer_rebuild_chunk_mesh(VoxelRenderer *renderer, VoxelChunk *chunk);

// Frustum culling
bool voxel_renderer_is_chunk_visible(VoxelRenderer *renderer, const VoxelChunk *chunk);

// Get render statistics
void voxel_renderer_get_stats(VoxelRenderer *renderer, u32 *chunks_rendered, u32 *vertices_rendered, u32 *triangles_rendered);

#ifdef __cplusplus
}
#endif

#endif // VOXEL_RENDERER_H
