// include/render/ray_tracing.h
//
// Purpose: Defines the public API for the ray tracing system in Minecraft v2.
// This header provides functions for initializing and managing RTX ray tracing,
// including acceleration structures, shader pipelines, and ray tracing commands.
//
// Public APIs:
// - `ray_tracing_init`: Initializes the ray tracing system, checking for RTX support
//   and loading required extensions and function pointers.
// - `ray_tracing_cleanup`: Destroys all ray tracing resources and frees memory.
// - `ray_tracing_trace`: Records ray tracing commands into a command buffer.
// - `ray_tracing_update_camera`: Updates camera parameters for ray tracing.
// - `ray_tracing_is_available`: Returns true if ray tracing is supported.
//
// Ownership: The ray tracing system manages its own resources and should be
// initialized once at startup and cleaned up at shutdown.
//
// Invariants:
// - Vulkan must be initialized before calling ray_tracing_init.
// - All ray tracing functions must only be called if ray_tracing_is_available() returns true.
// - The Vulkan device must support the required RTX extensions.
//
#ifndef RAY_TRACING_H
#define RAY_TRACING_H

#include <common.h>
#include "include/rendering/vulkan.h"
#include <math/vec3.h>

#ifdef __cplusplus
extern "C" {
#endif

// Initialize ray tracing system
// Returns true if ray tracing is successfully initialized and supported
bool ray_tracing_init(VulkanRenderer* renderer);

// Cleanup ray tracing system
void ray_tracing_cleanup(void);

// Update ray tracing camera parameters
void ray_tracing_update_camera(Vec3 position, Vec3 direction, Vec3 up, f32 fov, f32 aspect);

// Record ray tracing commands into command buffer
void ray_tracing_trace(VkCommandBuffer command_buffer);

// Check if ray tracing is available on current hardware
bool ray_tracing_is_available(void);

// Extended instance management API
uint32_t ray_tracing_add_instance(const vec3* position, const vec3* scale, const quat* rotation,
                                 uint32_t mesh_id, uint32_t material_id);
void ray_tracing_remove_instance(uint32_t instance_id);
void ray_tracing_set_instance_material(uint32_t instance_id, float roughness, float metallic, float emissive);

// Statistics and debugging
void ray_tracing_get_stats(uint64_t* total_rays, float* avg_time, uint32_t* rays_per_frame, float* frame_time);
void ray_tracing_debug_print(void);

#ifdef __cplusplus
}
#endif

#endif // RAY_TRACING_H
