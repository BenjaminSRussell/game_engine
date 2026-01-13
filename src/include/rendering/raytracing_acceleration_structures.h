// src/engine/include/renderer/raytracing_acceleration_structures.h
//
// Purpose: Dynamic acceleration structure management for raytracing
// Provides API for BLAS/TLAS updates and dynamic geometry handling

#ifndef RAYTRACING_ACCELERATION_STRUCTURES_H
#define RAYTRACING_ACCELERATION_STRUCTURES_H

#include "engine/include/common.h"
#include "include/rendering/vulkan.h"
#include <math/mat4.h>

#ifdef __cplusplus
extern "C" {
#endif

// BLAS update flags
typedef enum {
    BLAS_UPDATE_NONE = 0,
    BLAS_UPDATE_REBUILD = 1,    // Full rebuild of the BLAS
    BLAS_UPDATE_REFIT = 2,      // Refit existing BLAS (faster for small changes)
    BLAS_UPDATE_COMPACT = 4     // Compact BLAS to reduce memory usage
} BLASUpdateFlags;

// Initialize acceleration structure manager
bool as_manager_init(VulkanRenderer* renderer);

// Register a dynamic BLAS for updates
u32 as_manager_register_dynamic_blas(VkAccelerationStructureKHR blas, VkBuffer vertex_buffer, 
                                   VkBuffer index_buffer, u32 vertex_count, u32 index_count, const char* name);

// Mark a BLAS as needing updates
void as_manager_mark_blas_dirty(u32 blas_index, BLASUpdateFlags flags);

// Update all dirty BLAS structures
void as_manager_update_dirty_blas(void);

// Register a TLAS instance
u32 as_manager_register_tlas_instance(u32 blas_index, const mat4* transform, u32 material_id);

// Update TLAS instance transform
void as_manager_update_instance_transform(u32 instance_index, const mat4* transform);

// Build TLAS with all registered instances
bool as_manager_build_tlas(VkAccelerationStructureKHR tlas);

// Frame management
void as_manager_begin_frame(u32 frame_index);
void as_manager_end_frame(void);

// Performance statistics
void as_manager_get_stats(u32* blas_updates, u32* tlas_updates, u32* active_blas, u32* active_instances);

// Cleanup acceleration structure manager
void as_manager_cleanup(void);

#ifdef __cplusplus
}
#endif

#endif // RAYTRACING_ACCELERATION_STRUCTURES_H
