// src/engine/include/renderer/raytracing_reflections.h
//
// Purpose: Ray-traced reflections system for Minecraft v2
// Provides API for perfect mirror reflections, rough surface reflections, and water refractions

#ifndef RAYTRACING_REFLECTIONS_H
#define RAYTRACING_REFLECTIONS_H

#include <common.h>
#include "vulkan.h"
#include <math/mat4.h>

#ifdef __cplusplus
extern "C" {
#endif

// Reflection quality levels
typedef enum {
    REFLECTION_QUALITY_OFF = 0,     // Reflections disabled
    REFLECTION_QUALITY_LOW = 1,     // 1 bounce, 1 sample, perfect reflections only
    REFLECTION_QUALITY_MEDIUM = 2,  // 2 bounces, 2 samples, rough reflections enabled
    REFLECTION_QUALITY_HIGH = 3,    // 3 bounces, 4 samples, rough + water refraction
    REFLECTION_QUALITY_ULTRA = 4    // 4 bounces, 8 samples, full feature set
} ReflectionQuality;

// Initialize reflection system
bool reflection_init(VulkanRenderer* renderer, ReflectionQuality quality);

// Render reflections for current frame
void reflection_render(VkCommandBuffer command_buffer, VkAccelerationStructureKHR tlas, 
                      const mat4* view_matrix, const mat4* proj_matrix, const mat4* prev_view_matrix,
                      VkImageView gbuffer_normal_view, VkImageView gbuffer_roughness_view, 
                      VkImageView gbuffer_metalness_view, VkImageView motion_vector_view);

// Get reflection result for compositing
VkImage reflection_get_result_image(void);
VkImageView reflection_get_result_view(void);

// Configuration
void reflection_set_quality(ReflectionQuality quality);

// Statistics
void reflection_get_stats(u32* frame_count, u32* samples_per_pixel, f32* last_frame_time, u32* total_rays);

// Cleanup
void reflection_cleanup(void);

#ifdef __cplusplus
}
#endif

#endif // RAYTRACING_REFLECTIONS_H
