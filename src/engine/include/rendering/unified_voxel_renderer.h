#ifndef UNIFIED_VOXEL_RENDERER_H
#define UNIFIED_VOXEL_RENDERER_H

#include <stdbool.h>
#include <stdint.h>
#include <stddef.h>

#ifdef __OBJC__
#include "backend/metal/mtl_buffer.h"
#include "backend/metal/mtl_device.h"
#include "backend/metal/mtl_encoder.h"
#else
typedef void* mtl_buffer_t;
typedef void* mtl_device_t;
typedef void* mtl_command_encoder_t;
typedef void* metal_buffer_t;
typedef void* metal_device_t;
typedef void* metal_render_command_encoder_t;
#endif

// ============================================================================
// Unified Voxel Renderer - Consolidates all voxel rendering functionality
// ============================================================================

// Forward declarations
typedef struct UnifiedVoxelRenderer UnifiedVoxelRenderer;
typedef struct VoxelMesh VoxelMesh;
typedef struct VoxelChunk VoxelChunk;

// Voxel vertex structure
typedef struct {
    float x, y, z;     // Position
    float nx, ny, nz;  // Normal
    float u, v;        // Texture coordinates
    uint8_t ao;        // Ambient occlusion
    uint8_t light;     // Light level
    uint16_t block_id; // Block type
    uint8_t texture_layer; // Texture array layer
} VoxelVertex;

// Voxel block data
typedef struct {
    uint16_t block_id;
    uint8_t light_level;
    uint8_t ao_value;
    bool visible;
    bool transparent;
} VoxelBlock;

// Voxel chunk data
struct VoxelChunk {
    uint32_t x, y, z;           // Chunk position
    uint32_t width, height, depth; // Chunk dimensions
    VoxelBlock* blocks;         // Block data
    VoxelMesh* mesh;            // Generated mesh
    bool dirty;                 // Needs remeshing
    bool visible;               // Visible to camera
    float bounding_box[6];     // Min/Max bounds
};

// Voxel mesh data
struct VoxelMesh {
    metal_buffer_t* vertex_buffer;
    metal_buffer_t* index_buffer;
    uint32_t vertex_count;
    uint32_t index_count;
    bool uploaded;
    bool gpu_optimized;
};

// Voxel renderer configuration
typedef struct {
    bool enable_instancing;
    bool enable_gpu_meshing;
    bool enable_ambient_occlusion;
    bool enable_lighting;
    bool enable_culling;
    uint32_t max_chunks_per_frame;
    uint32_t chunk_size;
    float lod_distance;
    bool enable_mipmapping;
} VoxelRendererConfig;

// Render statistics
typedef struct {
    uint32_t chunks_rendered;
    uint32_t triangles_drawn;
    uint32_t draw_calls;
    float meshing_time_ms;
    float render_time_ms;
    float culling_time_ms;
    uint32_t vertices_uploaded;
    uint32_t memory_usage_mb;
} VoxelRenderStats;

// ============================================================================
// Core Renderer Functions
// ============================================================================

// System lifecycle
UnifiedVoxelRenderer* unified_voxel_renderer_create(metal_device_t* device,
                                                   const VoxelRendererConfig* config);
void unified_voxel_renderer_destroy(UnifiedVoxelRenderer* renderer);

// Configuration
void unified_voxel_renderer_set_config(UnifiedVoxelRenderer* renderer,
                                      const VoxelRendererConfig* config);
void unified_voxel_renderer_get_config(const UnifiedVoxelRenderer* renderer,
                                      VoxelRendererConfig* out_config);

// ============================================================================
// Chunk Management
// ============================================================================

// Chunk lifecycle
VoxelChunk* unified_voxel_renderer_create_chunk(UnifiedVoxelRenderer* renderer,
                                               uint32_t x, uint32_t y, uint32_t z,
                                               uint32_t width, uint32_t height, uint32_t depth);
void unified_voxel_renderer_destroy_chunk(UnifiedVoxelRenderer* renderer,
                                         VoxelChunk* chunk);

// Chunk operations
void unified_voxel_renderer_set_block(UnifiedVoxelRenderer* renderer,
                                     VoxelChunk* chunk,
                                     uint32_t x, uint32_t y, uint32_t z,
                                     uint16_t block_id);
uint16_t unified_voxel_renderer_get_block(const UnifiedVoxelRenderer* renderer,
                                         const VoxelChunk* chunk,
                                         uint32_t x, uint32_t y, uint32_t z);

// Mesh generation
void unified_voxel_renderer_update_chunk_mesh(UnifiedVoxelRenderer* renderer,
                                            VoxelChunk* chunk);
void unified_voxel_renderer_update_all_dirty_chunks(UnifiedVoxelRenderer* renderer);

// ============================================================================
// Rendering
// ============================================================================

// Main rendering interface
void unified_voxel_renderer_render(UnifiedVoxelRenderer* renderer,
                                   metal_render_command_encoder_t encoder,
                                   const float view_projection_matrix[16],
                                   const float camera_position[3]);

// Culling and LOD
void unified_voxel_renderer_cull_chunks(UnifiedVoxelRenderer* renderer,
                                       const float view_projection_matrix[16],
                                       const float camera_position[3]);
void unified_voxel_renderer_apply_lod(UnifiedVoxelRenderer* renderer,
                                     const float camera_position[3]);

// ============================================================================
// Lighting and Effects
// ============================================================================

// Lighting
void unified_voxel_renderer_set_ambient_light(UnifiedVoxelRenderer* renderer,
                                             float r, float g, float b);
void unified_voxel_renderer_set_sun_direction(UnifiedVoxelRenderer* renderer,
                                             float x, float y, float z);
void unified_voxel_renderer_set_sun_color(UnifiedVoxelRenderer* renderer,
                                         float r, float g, float b);

// Ambient occlusion
void unified_voxel_renderer_enable_ao(UnifiedVoxelRenderer* renderer, bool enable);
void unified_voxel_renderer_set_ao_strength(UnifiedVoxelRenderer* renderer, float strength);

// ============================================================================
// Texture Management
// ============================================================================

// Texture arrays
void unified_voxel_renderer_load_texture_array(UnifiedVoxelRenderer* renderer,
                                              const char** texture_files,
                                              uint32_t texture_count);
void unified_voxel_renderer_set_texture_filter(UnifiedVoxelRenderer* renderer,
                                             bool linear);

// ============================================================================
// Statistics and Debugging
// ============================================================================

// Performance monitoring
void unified_voxel_renderer_get_stats(const UnifiedVoxelRenderer* renderer,
                                     VoxelRenderStats* out_stats);
void unified_voxel_renderer_reset_stats(UnifiedVoxelRenderer* renderer);

// Debug utilities
void unified_voxel_renderer_debug_render_chunk_bounds(UnifiedVoxelRenderer* renderer);
void unified_voxel_renderer_debug_render_wireframe(UnifiedVoxelRenderer* renderer, bool enable);

// ============================================================================
// Platform-specific Interface
// ============================================================================

#ifdef __APPLE__
// Metal-specific functions
void unified_voxel_renderer_metal_update_buffers(UnifiedVoxelRenderer* renderer,
                                                 id<MTLCommandBuffer> command_buffer);
void unified_voxel_renderer_metal_render(UnifiedVoxelRenderer* renderer,
                                        id<MTLRenderCommandEncoder> encoder);
#endif

// ============================================================================
// Utility Functions
// ============================================================================

// Coordinate conversion
void unified_voxel_world_to_chunk(const float world_pos[3], uint32_t chunk_pos[3]);
void unified_voxel_chunk_to_world(const uint32_t chunk_pos[3], float world_pos[3]);

// Block utilities
bool unified_voxel_is_block_transparent(uint16_t block_id);
bool unified_voxel_is_block_solid(uint16_t block_id);
uint8_t unified_voxel_get_block_light_level(uint16_t block_id);

#endif // UNIFIED_VOXEL_RENDERER_H
