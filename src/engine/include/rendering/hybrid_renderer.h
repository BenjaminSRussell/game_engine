// src/engine/include/renderer/hybrid_renderer.h
//
// Purpose: Hybrid rendering pipeline combining raytracing and rasterization
// Provides API for optimal performance by using raytracing for specific effects

#ifndef HYBRID_RENDERER_H
#define HYBRID_RENDERER_H

#include <common.h>
#include "include/rendering/vulkan.h"
#include <math/mat4.h>

#ifdef __cplusplus
extern "C" {
#endif

// Hybrid rendering modes
typedef enum {
    HYBRID_MODE_RASTER_ONLY = 0,        // Traditional rasterization only
    HYBRID_MODE_RASTER_GI = 1,          // Rasterization + raytraced GI
    HYBRID_MODE_RASTER_REFLECTIONS = 2, // Rasterization + raytraced reflections
    HYBRID_MODE_RASTER_GI_REFLECTIONS = 3, // Rasterization + GI + reflections
    HYBRID_MODE_RAYTRACED = 4           // Full raytracing
} HybridMode;

// Hybrid renderer configuration
typedef struct {
    HybridMode mode;
    bool enable_gi;
    bool enable_reflections;
    bool enable_shadows;
    bool enable_ambient_occlusion;
    float gi_quality;
    float reflection_quality;
    u32 max_raytraced_pixels;
    bool adaptive_quality;
} HybridConfig;

// Initialize hybrid renderer
bool hybrid_init(VulkanRenderer* renderer, HybridConfig config);

// Render a frame using hybrid pipeline
void hybrid_render_frame(VkCommandBuffer command_buffer, VkAccelerationStructureKHR tlas, 
                         const mat4* view_matrix, const mat4* proj_matrix, const mat4* prev_view_matrix);

// Get G-buffer views for raytracing systems
void hybrid_get_gbuffer_views(VkImageView* albedo, VkImageView* normal, VkImageView* roughness, 
                             VkImageView* metalness, VkImageView* depth, VkImageView* motion_vector);

// Configuration
void hybrid_set_mode(HybridMode mode);

// Performance statistics
void hybrid_get_stats(f32* last_frame_time, u32* geometry_time_us, u32* raytracing_time_us, 
                     u32* compositing_time_us, u32* total_pixels);

// Cleanup
void hybrid_cleanup(void);

#ifdef __cplusplus
}
#endif

#endif // HYBRID_RENDERER_H
