/*
 * rt_soft_shadows.c
 * RT soft shadow penumbra
 *
 * Part of the Raytracing subsystem
 * Advanced 3D Rendering Engine
 */

#include "rt_soft_shadows.h"
#include <renderer/vulkan.h>
#include <core/logger.h>
#include <stdint.h>
#include <stdbool.h>
#include <stddef.h>
#include <stdlib.h>

/* ============================================================================
 * PUBLIC API
 * ============================================================================ */

void raytracing_rt_soft_shadows_apply(VulkanRenderer* renderer, VkCommandBuffer cmd) {
    if (!renderer) return;

    LOG_INFO("Applying ray-traced soft shadows...");
}

/* End of rt_soft_shadows.c */
