/*
 * gpu_culling.h
 * GPU-driven instance culling system
 *
 * Part of the Geometry subsystem
 * Advanced 3D Rendering Engine
 */

#ifndef GEOMETRY_GPU_CULLING_H
#define GEOMETRY_GPU_CULLING_H

#include <stdint.h>
#include <stdbool.h>
#include <stddef.h>

#ifdef __cplusplus
extern "C" {
#endif

/* ============================================================================
 * FORWARD DECLARATIONS
 * ============================================================================ */

typedef struct metal_device metal_device_t;
typedef struct metal_buffer metal_buffer_t;
typedef struct instance_buffer instance_buffer_t;

#ifdef __OBJC__
@class MTLComputePipelineState;
@class MTLCommandBuffer;
@class MTLComputeCommandEncoder;
#else
typedef void MTLComputePipelineState;
typedef void MTLCommandBuffer;
typedef void MTLComputeCommandEncoder;
#endif

/* ============================================================================
 * TYPES & STRUCTURES
 * ============================================================================ */

// Frustum plane (ax + by + cz + d = 0)
typedef struct frustum_plane {
    float a, b, c, d;
} frustum_plane_t;

// Camera frustum (6 planes: left, right, bottom, top, near, far)
typedef struct camera_frustum {
    frustum_plane_t planes[6];
} camera_frustum_t;

// Culling configuration
typedef struct gpu_culling_config {
    bool enable_frustum_culling;    // Frustum culling
    bool enable_occlusion_culling;  // Occlusion culling (requires hi-z)
    bool enable_distance_culling;   // Distance-based culling
    bool enable_backface_culling;   // Backface culling for large instances
    
    float near_distance;            // Near culling distance
    float far_distance;             // Far culling distance
    float lod0_distance;            // LOD 0 distance threshold
    float lod1_distance;            // LOD 1 distance threshold
    float lod2_distance;            // LOD 2 distance threshold
    
    uint32_t max_visible_instances; // Maximum visible instances
} gpu_culling_config_t;

// Culling statistics (computed on GPU, read back to CPU)
typedef struct gpu_culling_stats {
    uint32_t total_instances;       // Total instances tested
    uint32_t visible_instances;     // Instances that passed culling
    uint32_t frustum_culled;        // Culled by frustum
    uint32_t occlusion_culled;      // Culled by occlusion
    uint32_t distance_culled;       // Culled by distance
    uint32_t backface_culled;       // Culled by backface
    
    // Performance metrics
    float gpu_time_ms;              // GPU execution time
    float culling_efficiency;       // visible / total
} gpu_culling_stats_t;

// Culling context
typedef struct gpu_culling_context {
    metal_device_t* device;
    
    // Compute pipeline
    MTLComputePipelineState* culling_pipeline;
    
    // Buffers
    metal_buffer_t* frustum_buffer;         // Camera frustum
    metal_buffer_t* config_buffer;          // Culling config
    metal_buffer_t* visible_ids_buffer;     // Output: visible instance IDs
    metal_buffer_t* atomic_counter_buffer;  // Output: visible count
    metal_buffer_t* stats_buffer;           // Output: statistics
    
    // Configuration
    gpu_culling_config_t config;
    uint32_t max_instances;
    
    // Statistics
    gpu_culling_stats_t last_stats;
    uint32_t frame_count;
    
    // Debug
    char label[64];
    bool initialized;
} gpu_culling_context_t;

// Culling result
typedef struct gpu_culling_result {
    uint32_t* visible_instance_ids; // Array of visible IDs
    uint32_t visible_count;         // Number of visible instances
    gpu_culling_stats_t stats;      // Culling statistics
} gpu_culling_result_t;

/* ============================================================================
 * API - INITIALIZATION
 * ============================================================================ */

/**
 * Initialize GPU culling system
 */
int gpu_culling_init(void);

/**
 * Shutdown GPU culling system
 */
void gpu_culling_shutdown(void);

/**
 * Create a GPU culling context
 * @param device Metal device
 * @param max_instances Maximum instances to cull
 * @param config Culling configuration
 * @return Culling context or NULL on failure
 */
gpu_culling_context_t* gpu_culling_context_create(
    metal_device_t* device,
    uint32_t max_instances,
    const gpu_culling_config_t* config);

/**
 * Destroy a GPU culling context
 */
void gpu_culling_context_destroy(gpu_culling_context_t* context);

/* ============================================================================
 * API - CULLING OPERATIONS
 * ============================================================================ */

/**
 * Execute GPU culling
 * @param context Culling context
 * @param command_buffer Metal command buffer to encode into
 * @param instance_buffer Instance data buffer
 * @param camera_position Camera world position
 * @param frustum Camera frustum
 * @param hiz_texture Hi-Z texture for occlusion culling (optional)
 * @return 0 on success, -1 on failure
 */
int gpu_culling_execute(
    gpu_culling_context_t* context,
    MTLCommandBuffer* command_buffer,
    instance_buffer_t* instance_buffer,
    const float camera_position[3],
    const camera_frustum_t* frustum,
    void* hiz_texture);

/**
 * Get culling results (blocking - waits for GPU)
 * @param context Culling context
 * @param out_result Output result structure
 * @return 0 on success, -1 on failure
 */
int gpu_culling_get_result(
    gpu_culling_context_t* context,
    gpu_culling_result_t* out_result);

/**
 * Update culling configuration
 */
void gpu_culling_update_config(
    gpu_culling_context_t* context,
    const gpu_culling_config_t* config);

/**
 * Reset culling context (for next frame)
 */
void gpu_culling_reset(gpu_culling_context_t* context);

/* ============================================================================
 * API - UTILITIES
 * ============================================================================ */

/**
 * Extract frustum from view-projection matrix
 */
void gpu_culling_extract_frustum(
    const float view_proj_matrix[16],
    camera_frustum_t* out_frustum);

/**
 * Test if point is inside frustum (CPU reference)
 */
bool gpu_culling_test_point_frustum(
    const float point[3],
    const camera_frustum_t* frustum);

/**
 * Test if sphere is inside frustum (CPU reference)
 */
bool gpu_culling_test_sphere_frustum(
    const float center[3],
    float radius,
    const camera_frustum_t* frustum);

/**
 * Get culling statistics
 */
gpu_culling_stats_t gpu_culling_get_stats(const gpu_culling_context_t* context);

/**
 * Calculate optimal threadgroup size for culling
 */
uint32_t gpu_culling_calculate_threadgroup_size(uint32_t instance_count);

/**
 * Debug print culling context
 */
void gpu_culling_debug_print(const gpu_culling_context_t* context);

#ifdef __cplusplus
}
#endif

#endif /* GEOMETRY_GPU_CULLING_H */
