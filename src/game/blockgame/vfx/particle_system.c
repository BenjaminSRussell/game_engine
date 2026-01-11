// src/vfx/particle_system.c
//
// Particle system implementation - requires Vulkan SDK when VULKAN_BUILD is defined
// Otherwise provides stub/no-op implementations

#ifndef VULKAN_BUILD

#include <stdbool.h>
#include <stddef.h>

// Stub types for non-Vulkan builds
typedef struct ParticleSystem { bool initialized; } ParticleSystem;
typedef struct VulkanRenderer { void* device; } VulkanRenderer;

void particle_system_init(ParticleSystem* system, VulkanRenderer* renderer) {
    (void)system; (void)renderer;
}

void particle_system_shutdown(ParticleSystem* system, VulkanRenderer* renderer) {
    (void)system; (void)renderer;
}

void particle_system_update(ParticleSystem* system, float delta_time) {
    (void)system; (void)delta_time;
}

void particle_system_render(ParticleSystem* system, VulkanRenderer* renderer) {
    (void)system; (void)renderer;
}

#endif // !VULKAN_BUILD
