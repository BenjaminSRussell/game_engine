// src/vfx/post_processing.c
//
// Post-processing effects - requires Vulkan SDK when VULKAN_BUILD is defined

#ifndef VULKAN_BUILD

#include <stdbool.h>
#include <stddef.h>

typedef struct PostProcessingPipeline { bool initialized; } PostProcessingPipeline;
typedef struct VulkanRenderer { void* device; } VulkanRenderer;

void post_processing_init(PostProcessingPipeline* pipeline, VulkanRenderer* renderer) {
    (void)pipeline; (void)renderer;
}

void post_processing_shutdown(PostProcessingPipeline* pipeline, VulkanRenderer* renderer) {
    (void)pipeline; (void)renderer;
}

void post_processing_apply(PostProcessingPipeline* pipeline, VulkanRenderer* renderer) {
    (void)pipeline; (void)renderer;
}

#endif // !VULKAN_BUILD
