/*
 * rt_shadow_denoise.c
 * Shadow denoising
 *
 * Part of the Raytracing subsystem
 * Advanced 3D Rendering Engine
 */

#include "rt_shadow_denoise.h"
#include <renderer/vulkan.h>
#include <core/logger.h>
#include <stdint.h>
#include <stdbool.h>
#include <stddef.h>
#include <stdlib.h>

/* ============================================================================
 * PUBLIC API
 * ============================================================================ */

void raytracing_rt_shadow_denoise_apply(VulkanRenderer* renderer, VkCommandBuffer cmd) {
    if (!renderer) return;

    LOG_INFO("Denoising ray-traced shadows (SVGF/ReLAX)...");
}

/* End of rt_shadow_denoise.c */
