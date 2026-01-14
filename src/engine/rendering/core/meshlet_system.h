// renderer/core/meshlet_system.h
//
// Purpose: Nanite-style meshlet system for GPU-driven rendering.
// Implements meshlet partitioning, culling, and rendering optimization.
//
// Key Features:
// - 128-triangle meshlet clusters
// - GPU-driven culling and rendering
// - Cone culling for back-face clusters
// - LOD proxy geometry generation
// - Compressed indexing and position quantization
//
// Architecture:
// - Meshlet partitioning: Split meshes into optimal clusters
// - GPU culling: Frustum and cone culling on GPU
// - Indirect rendering: Multi-draw indirect for efficiency
// - Memory optimization: Compressed data structures
//
#ifndef MESHLET_SYSTEM_H
#define MESHLET_SYSTEM_H

#include <common.h>
#include "include/math/vec3.h"
#include "include/math/vec4.h"
#include "rendering/vulkan.h"

#define MESHLET_MAX_TRIANGLES 128
#define MESHLET_MAX_VERTICES 256
#define MESHLET_MAX_MESHLETS 65536

// Meshlet bounds information (TASK_101)
typedef struct {
    Vec3 center;              // Center of meshlet
    f32 radius;              // Bounding sphere radius
    Vec3 cone_apex;           // Cone apex for backface culling
    Vec3 cone_axis;           // Cone axis direction
    f32 cone_cutoff;          // Cosine of cone angle
    u32 triangle_count;       // Number of triangles in meshlet
    u32 vertex_count;         // Number of vertices in meshlet
} MeshletBounds;

// Meshlet data structure
typedef struct {
    // Triangle indices (compressed)
    u8 triangle_indices[MESHLET_MAX_TRIANGLES * 3];  // 8-bit indices
    u32 triangle_count;
    
    // Vertex data (compressed)
    Vec3 vertex_positions[MESHLET_MAX_VERTICES];
    Vec3 vertex_normals[MESHLET_MAX_VERTICES];
    Vec2 vertex_uvs[MESHLET_MAX_VERTICES];
    u32 vertex_count;
    
    // Bounds and culling info
    MeshletBounds bounds;
    
    // LOD information
    u32 lod_level;            // Current LOD level
    u32 parent_meshlet;      // Parent meshlet for LOD hierarchy
} Meshlet;

// Mesh data for processing
typedef struct {
    Vec3* vertices;
    Vec3* normals;
    Vec2* uvs;
    u32* indices;
    u32 vertex_count;
    u32 triangle_count;
} MeshData;

// Meshlet generation context (TASK_100)
typedef struct {
    MeshData* input_mesh;
    Meshlet* meshlets;
    u32 max_meshlets;
    u32 meshlet_count;
    
    // Partitioning parameters
    u32 target_triangles_per_meshlet;
    f32 max_cone_angle;       // Maximum cone angle for culling
    bool optimize_for_cache;   // Optimize for vertex cache
    
    // Statistics
    u32 total_triangles_processed;
    u32 total_meshlets_generated;
    f32 average_triangles_per_meshlet;
} MeshletGenerationContext;

// GPU culling data (TASK_110, TASK_111)
typedef struct {
    Vec4 frustum_planes[6];     // Frustum planes for culling
    Vec3 camera_position;       // Camera position for distance culling
    f32 max_distance;          // Maximum render distance
    u32 culling_flags;          // Culling enable flags
} GPUCullingData;

// Indirect draw data (TASK_112, TASK_113)
typedef struct {
    u32 index_count;           // Number of indices to draw
    u32 instance_count;        // Number of instances
    u32 first_index;           // First index in index buffer
    u32 vertex_offset;         // Vertex offset in vertex buffer
    u32 first_instance;        // First instance ID
} IndirectDrawCommand;

typedef struct {
    IndirectDrawCommand commands[MESHLET_MAX_MESHLETS];
    u32 command_count;
} IndirectDrawBuffer;

// ==============================================================================
// MESHLET GENERATION (TASK_100, TASK_101, TASK_102, TASK_103)
// ==============================================================================

/**
 * Initialize meshlet generation context.
 *
 * @param context Pointer to MeshletGenerationContext
 * @param mesh Input mesh data
 * @param meshlets Output meshlet array
 * @param max_meshlets Maximum number of meshlets
 * @return true on success, false on failure
 */
bool meshlet_generation_init(MeshletGenerationContext* context, 
                              MeshData* mesh, Meshlet* meshlets, u32 max_meshlets);

/**
 * Generate meshlets from input mesh (TASK_100).
 *
 * @param context Meshlet generation context
 * @return true on success, false on failure
 */
bool meshlet_generate_meshlets(MeshletGenerationContext* context);

/**
 * Generate meshlet bounds (TASK_101).
 *
 * @param meshlet Meshlet to generate bounds for
 * @return true on success, false on failure
 */
bool meshlet_generate_bounds(Meshlet* meshlet);

/**
 * Generate cone culling data (TASK_102).
 *
 * @param meshlet Meshlet to generate cone data for
 * @return true on success, false on failure
 */
bool meshlet_generate_cone_culling(Meshlet* meshlet);

/**
 * Generate LOD proxy geometry (TASK_103).
 *
 * @param meshlet Original meshlet
 * @param lod_meshlet Output LOD meshlet
 * @param lod_level LOD level (0=original, 1=half, 2=quarter)
 * @return true on success, false on failure
 */
bool meshlet_generate_lod_proxy(const Meshlet* meshlet, Meshlet* lod_meshlet, u32 lod_level);

// ==============================================================================
// GPU CULLING (TASK_110, TASK_111)
// ==============================================================================

/**
 * Create GPU culling buffers.
 *
 * @param device Vulkan device
 * @param physical_device Vulkan physical device
 * @param culling_data Output culling data buffer
 * @param indirect_buffer Output indirect draw buffer
 * @return true on success, false on failure
 */
bool meshlet_create_culling_buffers(VkDevice device, VkPhysicalDevice physical_device,
                                     GPUCullingData** culling_data, 
                                     IndirectDrawBuffer** indirect_buffer);

/**
 * Update GPU culling data.
 *
 * @param culling_data Culling data to update
 * @param camera_pos Camera position
 * @param view_matrix View matrix
 * @param projection_matrix Projection matrix
 * @return true on success, false on failure
 */
bool meshlet_update_culling_data(GPUCullingData* culling_data, 
                                 Vec3 camera_pos, const f32* view_matrix, 
                                 const f32* projection_matrix);

/**
 * Execute instance culling compute shader (TASK_110).
 *
 * @param command_buffer Vulkan command buffer
 * @param culling_data GPU culling data
 * @param meshlets Meshlet data
 * @param meshlet_count Number of meshlets
 * @param indirect_buffer Output indirect draw buffer
 * @return true on success, false on failure
 */
bool meshlet_execute_instance_culling(VkCommandBuffer command_buffer,
                                      GPUCullingData* culling_data,
                                      Meshlet* meshlets, u32 meshlet_count,
                                      IndirectDrawBuffer* indirect_buffer);

/**
 * Execute meshlet culling compute shader (TASK_111).
 *
 * @param command_buffer Vulkan command buffer
 * @param culling_data GPU culling data
 * @param meshlets Meshlet data
 * @param meshlet_count Number of meshlets
 * @param indirect_buffer Output indirect draw buffer
 * @return true on success, false on failure
 */
bool meshlet_execute_meshlet_culling(VkCommandBuffer command_buffer,
                                      GPUCullingData* culling_data,
                                      Meshlet* meshlets, u32 meshlet_count,
                                      IndirectDrawBuffer* indirect_buffer);

// ==============================================================================
// INDIRECT DRAWING (TASK_112, TASK_113)
// ==============================================================================

/**
 * Generate indirect draw buffer (TASK_112).
 *
 * @param meshlets Meshlet data
 * @param meshlet_count Number of meshlets
 * @param indirect_buffer Output indirect draw buffer
 * @return true on success, false on failure
 */
bool meshlet_generate_indirect_draw_buffer(Meshlet* meshlets, u32 meshlet_count,
                                           IndirectDrawBuffer* indirect_buffer);

/**
 * Execute multi-draw indirect (TASK_113).
 *
 * @param command_buffer Vulkan command buffer
 * @param indirect_buffer Indirect draw buffer
 * @param vertex_buffer Vertex buffer
 * @param index_buffer Index buffer
 * @return true on success, false on failure
 */
bool meshlet_execute_multi_draw_indirect(VkCommandBuffer command_buffer,
                                          IndirectDrawBuffer* indirect_buffer,
                                          VkBuffer vertex_buffer, VkBuffer index_buffer);

// ==============================================================================
// DATA OPTIMIZATION (TASK_120, TASK_121, TASK_122, TASK_123)
// ==============================================================================

/**
 * Implement compressed indexing (TASK_120).
 *
 * @param meshlet Meshlet to compress
 * @return true on success, false on failure
 */
bool meshlet_compress_indices(Meshlet* meshlet);

/**
 * Add position quantization (TASK_121).
 *
 * @param meshlet Meshlet to quantize
 * @param precision_bits Number of bits for quantization (16, 8, etc.)
 * @return true on success, false on failure
 */
bool meshlet_quantize_positions(Meshlet* meshlet, u32 precision_bits);

/**
 * Use Structure of Arrays (SoA) for vertex buffers (TASK_122).
 *
 * @param meshlets Meshlet array
 * @param meshlet_count Number of meshlets
 * @param position_buffer Output position buffer
 * @param normal_buffer Output normal buffer
 * @param uv_buffer Output UV buffer
 * @return true on success, false on failure
 */
bool meshlet_create_soa_buffers(Meshlet* meshlets, u32 meshlet_count,
                                VkBuffer* position_buffer, VkBuffer* normal_buffer,
                                VkBuffer* uv_buffer);

/**
 * Add vertex-cache optimization (TASK_123).
 *
 * @param meshlet Meshlet to optimize
 * @return true on success, false on failure
 */
bool meshlet_optimize_vertex_cache(Meshlet* meshlet);

// ==============================================================================
// UTILITY FUNCTIONS
// ==============================================================================

/**
 * Get meshlet generation statistics.
 *
 * @param context Meshlet generation context
 * @param total_meshlets Output total meshlets generated
 * @param avg_triangles Output average triangles per meshlet
 * @return true on success, false on failure
 */
bool meshlet_get_generation_stats(const MeshletGenerationContext* context,
                                 u32* total_meshlets, f32* avg_triangles);

/**
 * Validate meshlet data.
 *
 * @param meshlet Meshlet to validate
 * @return true if valid, false otherwise
 */
bool meshlet_validate(const Meshlet* meshlet);

/**
 * Calculate meshlet memory usage.
 *
 * @param meshlet Meshlet to analyze
 * @param memory_usage Output memory usage in bytes
 * @return true on success, false on failure
 */
bool meshlet_calculate_memory_usage(const Meshlet* meshlet, u32* memory_usage);

#endif // MESHLET_SYSTEM_H
