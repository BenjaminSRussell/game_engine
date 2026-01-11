// src/vfx/status_effect_visuals.c
//
// Status effect visual rendering - requires Vulkan SDK when VULKAN_BUILD is defined

#ifndef VULKAN_BUILD

#include <stdbool.h>
#include <stddef.h>

typedef struct StatusEffectVisuals { bool initialized; } StatusEffectVisuals;
typedef struct VulkanRenderer { void* device; } VulkanRenderer;

void status_effect_visuals_init(StatusEffectVisuals* visuals, VulkanRenderer* renderer) {
    (void)visuals; (void)renderer;
}

void status_effect_visuals_shutdown(StatusEffectVisuals* visuals, VulkanRenderer* renderer) {
    (void)visuals; (void)renderer;
}

void status_effect_visuals_render(StatusEffectVisuals* visuals, VulkanRenderer* renderer) {
    (void)visuals; (void)renderer;
}

#endif // !VULKAN_BUILD
