/*
 * instance_data_gpu.h
 * GPU instance buffer
 *
 * Part of the Rendering subsystem
 * Advanced 3D Rendering Engine
 */

#ifndef RENDERING_INSTANCE_DATA_GPU_H
#define RENDERING_INSTANCE_DATA_GPU_H

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

/* ============================================================================
 * TYPES
 * ============================================================================ */

/* GPU-resident instance data (128 bytes, 16-byte aligned) */
typedef struct gpu_instance_data {
    float transform[16];           // 4x4 transform matrix (64 bytes)
    float bounds_center[3];        // Bounding sphere center (12 bytes)
    float bounds_radius;           // Bounding sphere radius (4 bytes)
    uint32_t mesh_id;              // Mesh identifier (4 bytes)
    uint32_t material_id;          // Material identifier (4 bytes)
    uint32_t lod_bias;             // LOD selection bias (4 bytes)
    uint32_t visibility_flags;     // Culling/rendering flags (4 bytes)
    uint32_t _padding[3];          // Padding to 128 bytes (12 bytes)
} gpu_instance_data_t;

/* CPU-side per-instance state */
typedef struct gpu_instance_state {
    gpu_instance_data_t data;      // GPU-resident data
    bool dirty;                    // Needs GPU update
    uint32_t frame_updated;        // Last frame updated
} gpu_instance_state_t;

typedef struct rendering_instance_data_gpu_handle {
    uint32_t id;
} rendering_instance_data_gpu_handle_t;

typedef struct rendering_instance_data_gpu_desc {
    metal_device_t* device;        // Metal device for buffer allocation
    uint32_t max_instances;        // Maximum instance capacity
    uint32_t flags;                // Descriptor flags
    void* user_data;               // User data pointer
} rendering_instance_data_gpu_desc_t;

typedef struct rendering_instance_data_gpu_info {
    uint32_t id;
    uint32_t flags;
    bool initialized;
    uint32_t instance_count;
    uint32_t max_capacity;
    size_t memory_used;
} rendering_instance_data_gpu_info_t;

/* ============================================================================
 * API
 * ============================================================================ */

/* Initialization */
int rendering_instance_data_gpu_init(void);
void rendering_instance_data_gpu_shutdown(void);

/* Lifecycle */
int rendering_instance_data_gpu_create(rendering_instance_data_gpu_handle_t* out_handle, const rendering_instance_data_gpu_desc_t* desc);
void rendering_instance_data_gpu_destroy(rendering_instance_data_gpu_handle_t handle);

/* Operations */
int rendering_instance_data_gpu_add_instance(rendering_instance_data_gpu_handle_t handle,
                                            const gpu_instance_data_t* instance_data,
                                            uint32_t* out_instance_id);
int rendering_instance_data_gpu_update_instance(rendering_instance_data_gpu_handle_t handle,
                                               uint32_t instance_id,
                                               const gpu_instance_data_t* instance_data);
int rendering_instance_data_gpu_update_instance_transform(rendering_instance_data_gpu_handle_t handle,
                                                         uint32_t instance_id,
                                                         const float* transform_matrix);
int rendering_instance_data_gpu_update_instance_bounds(rendering_instance_data_gpu_handle_t handle,
                                                      uint32_t instance_id,
                                                      const float* center,
                                                      float radius);
int rendering_instance_data_gpu_remove_instance(rendering_instance_data_gpu_handle_t handle,
                                               uint32_t instance_id);
int rendering_instance_data_gpu_update(rendering_instance_data_gpu_handle_t handle, const void* data, size_t size);
bool rendering_instance_data_gpu_is_valid(rendering_instance_data_gpu_handle_t handle);
int rendering_instance_data_gpu_get_info(rendering_instance_data_gpu_handle_t handle, rendering_instance_data_gpu_info_t* out_info);
void rendering_instance_data_gpu_mark_dirty(rendering_instance_data_gpu_handle_t handle);

/**
 * Upload pending instance data to GPU
 * @param device Metal device (required for upload operations)
 * @return Number of instances uploaded
 */
int rendering_instance_data_gpu_process_pending(metal_device_t* device);

/**
 * Get GPU buffer for compute shader binding
 * @return Metal buffer pointer or NULL if not allocated
 */
metal_buffer_t* rendering_instance_data_gpu_get_buffer(rendering_instance_data_gpu_handle_t handle);

/* Statistics */
uint32_t rendering_instance_data_gpu_get_count(void);
uint32_t rendering_instance_data_gpu_get_instance_count(rendering_instance_data_gpu_handle_t handle);
size_t rendering_instance_data_gpu_get_memory_usage(void);
void rendering_instance_data_gpu_debug_print(void);

#ifdef __cplusplus
}
#endif

#endif /* RENDERING_INSTANCE_DATA_GPU_H */
