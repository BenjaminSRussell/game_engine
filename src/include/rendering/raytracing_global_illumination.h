// src/engine/include/renderer/raytracing_global_illumination.h
//
// Purpose: Real-time Global Illumination system for Minecraft v2
// Provides API for path tracing, diffuse interreflection, and indirect lighting

#ifndef RAYTRACING_GLOBAL_ILLUMINATION_H
#define RAYTRACING_GLOBAL_ILLUMINATION_H

#include "engine/include/common.h"
#include "include/rendering/vulkan.h"
#include <math/mat4.h>

#ifdef __cplusplus
extern "C" {
#endif

// GI quality levels
typedef enum {
    GI_QUALITY_OFF = 0,     // GI disabled
    GI_QUALITY_LOW = 1,     // 2 bounces, 1 sample, no denoising
    GI_QUALITY_MEDIUM = 2,  // 3 bounces, 2 samples, spatial denoising
    GI_QUALITY_HIGH = 3,    // 4 bounces, 4 samples, spatial + temporal denoising
    GI_QUALITY_ULTRA = 4    // 6 bounces, 8 samples, full denoising pipeline
} GIQuality;

// Initialize GI system
bool gi_init(VulkanRenderer* renderer, GIQuality quality);

// Render GI for current frame
void gi_render(VkCommandBuffer command_buffer, VkAccelerationStructureKHR tlas, 
               const mat4* view_matrix, const mat4* proj_matrix, const mat4* prev_view_matrix);

// Get GI result for compositing
VkImage gi_get_result_image(void);
VkImageView gi_get_result_view(void);

// Configuration
void gi_set_quality(GIQuality quality);

// Statistics
void gi_get_stats(u32* frame_count, u32* samples_per_pixel, f32* last_frame_time);

// Cleanup
void gi_cleanup(void);

#ifdef __cplusplus
}
#endif

#endif // RAYTRACING_GLOBAL_ILLUMINATION_H
