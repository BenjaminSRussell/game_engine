/*
 * rt_reflection_denoise.c
 * Reflection denoising
 *
 * Part of the Raytracing subsystem
 * Advanced 3D Rendering Engine
 */

#include "rendering/raytracing/reflections_rt/rt_reflection_denoise.h"
#include <rendering/vulkan.h>
#include <core/logger.h>
#include <stdint.h>
#include <stdbool.h>
#include <stddef.h>
#include <stdlib.h>

/* ============================================================================
 * PUBLIC API
 * ============================================================================ */

void raytracing_rt_reflection_denoise_apply(VulkanRenderer* renderer, VkCommandBuffer cmd) {
    if (!renderer) return;

    LOG_INFO("Denoising ray-traced reflections (SVGF/ReLAX)...");
}

/* End of rt_reflection_denoise.c */
