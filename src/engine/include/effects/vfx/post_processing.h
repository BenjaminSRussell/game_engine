// include/vfx/post_processing.h
//
// Purpose: Post-processing effects system for screen-space visual enhancements
// including bloom, motion blur, color grading, and depth of field.
//
// Key Features:
// - Bloom effect for bright areas
// - Motion blur for dynamic movement
// - Color grading with LUT support
// - Depth of field with customizable focus
// - Vignette effect
// - Tone mapping and exposure control
// - Screen-space ambient occlusion (SSAO)
//
// Public APIs:
// - post_processing_init: Initialize post-processing system
// - post_processing_apply: Apply effects to rendered frame
// - post_processing_set_bloom: Configure bloom parameters
// - post_processing_set_color_grade: Apply color grading LUT
//
// Invariants:
// - Requires Vulkan render pass and framebuffer
// - Effects are applied as render passes
// - LUT textures must be 256x16 PNG format
//
#ifndef POST_PROCESSING_H
#define POST_PROCESSING_H

#include "engine/include/common.h"
#include <math/vec3.h>
#include <include/rendering/vulkan.h>

typedef enum {
    PP_EFFECT_NONE = 0,
    PP_EFFECT_BLOOM = 1 << 0,
    PP_EFFECT_MOTION_BLUR = 1 << 1,
    PP_EFFECT_COLOR_GRADE = 1 << 2,
    PP_EFFECT_DEPTH_OF_FIELD = 1 << 3,
    PP_EFFECT_VIGNETTE = 1 << 4,
    PP_EFFECT_FILM_GRAIN = 1 << 5,
    PP_EFFECT_CHROMATIC_ABERRATION = 1 << 6
} PostProcessEffectFlags;

typedef struct {
    f32 threshold;       // Brightness threshold for bloom (0.0-1.0)
    f32 intensity;       // Bloom intensity (0.0-2.0)
    f32 radius;          // Blur radius (1.0-10.0)
    f32 softness;        // Bloom softness (0.0-1.0)
} BloomParams;

typedef struct {
    f32 intensity;       // Motion blur amount (0.0-1.0)
    f32 samples;         // Number of samples (4-16)
} MotionBlurParams;

typedef struct {
    f32 focus_distance;  // Distance to focus plane
    f32 focus_range;     // Range of focus (depth)
    f32 blur_amount;     // Maximum blur amount (0.0-1.0)
} DepthOfFieldParams;

typedef struct {
    f32 strength;        // Vignette darkness (0.0-1.0)
    f32 radius;          // Vignette radius (0.0-2.0)
    f32 smoothness;      // Vignette smoothness (0.1-2.0)
} VignetteParams;

typedef struct {
    f32 intensity;       // Film grain intensity (0.0-1.0)
} FilmGrainParams;

typedef struct {
    f32 offset;          // Chromatic aberration offset (0.0-0.1)
} ChromaticAberrationParams;

typedef struct {
    f32 exposure;        // Exposure compensation (-2.0 to 2.0)
    f32 gamma;           // Gamma correction (0.5 to 3.0)
    f32 contrast;        // Contrast multiplier (0.5 to 2.0)
    f32 saturation;      // Color saturation (0.0 to 2.0)
} TonemapParams;

typedef struct {
    PostProcessEffectFlags active_effects;

    BloomParams bloom;
    MotionBlurParams motion_blur;
    DepthOfFieldParams depth_of_field;
    VignetteParams vignette;
    FilmGrainParams film_grain;
    ChromaticAberrationParams chroma_aberration;
    TonemapParams tonemap;

    // GPU resources
    VkImage bloom_image;
    VkImage blur_temp_image;
    VkFramebuffer bloom_framebuffer;

    VkPipeline bloom_pipeline;
    VkPipeline blur_pipeline;
    VkPipeline composite_pipeline;

    VkDescriptorSet descriptor_sets[3];
    VkSampler sampler;

    bool initialized;
} PostProcessingSystem;

// Lifecycle
void post_processing_init(PostProcessingSystem* system, VulkanRenderer* renderer);
void post_processing_shutdown(PostProcessingSystem* system, VulkanRenderer* renderer);

// Effect control
void post_processing_set_active_effects(PostProcessingSystem* system, PostProcessEffectFlags flags);
void post_processing_set_bloom_params(PostProcessingSystem* system, BloomParams params);
void post_processing_set_motion_blur_params(PostProcessingSystem* system, MotionBlurParams params);
void post_processing_set_dof_params(PostProcessingSystem* system, DepthOfFieldParams params);
void post_processing_set_vignette_params(PostProcessingSystem* system, VignetteParams params);
void post_processing_set_tonemap_params(PostProcessingSystem* system, TonemapParams params);

// Rendering
void post_processing_apply(PostProcessingSystem* system, VulkanRenderer* renderer,
                          VkImage source_image, VkImage target_image);

// Helper functions
void post_processing_enable_effect(PostProcessingSystem* system, PostProcessEffectFlags effect);
void post_processing_disable_effect(PostProcessingSystem* system, PostProcessEffectFlags effect);
bool post_processing_is_effect_enabled(PostProcessingSystem* system, PostProcessEffectFlags effect);

#endif // POST_PROCESSING_H
