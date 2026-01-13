// src/engine/include/renderer/raytracing_denoiser.h
//
// Purpose: Denoising system for raytraced GI and reflections in Minecraft v2
// Provides API for spatial and temporal denoising to reduce noise

#ifndef RAYTRACING_DENOISER_H
#define RAYTRACING_DENOISER_H

#include "engine/include/common.h"
#include "include/rendering/vulkan.h"

#ifdef __cplusplus
extern "C" {
#endif

// Denoiser quality levels
typedef enum {
    DENOISE_QUALITY_OFF = 0,     // Denoising disabled
    DENOISE_QUALITY_LOW = 1,     // Spatial denoising only
    DENOISE_QUALITY_MEDIUM = 2,  // Spatial + temporal denoising
    DENOISE_QUALITY_HIGH = 3,    // Enhanced spatial + temporal
    DENOISE_QUALITY_ULTRA = 4    // Maximum quality denoising
} DenoiseQuality;

// Initialize denoiser system
bool denoiser_init(VulkanRenderer* renderer, DenoiseQuality quality);

// Process noisy raytraced input and return denoised result
void denoiser_process(VkCommandBuffer command_buffer, VkImageView input_view, VkImageView depth_view, 
                       VkImageView normal_view, VkImageView motion_vector_view);

// Get denoised result
VkImageView denoiser_get_result_view(void);

// Configuration
void denoiser_set_quality(DenoiseQuality quality);

// Statistics
void denoiser_get_stats(u32* frame_count, f32* last_frame_time, u32* total_pixels);

// Cleanup
void denoiser_cleanup(void);

#ifdef __cplusplus
}
#endif

#endif // RAYTRACING_DENOISER_H
