// GPU Particle System Implementation
// GPU-PARTICLES-001: Particle system core architecture
// GPU-PARTICLES-002: GPU-based particle simulation
// GPU-PARTICLES-003: Compute shader particle updates
// GPU-PARTICLES-004: Particle rendering pipeline
// GPU-PARTICLES-005: Emitter management
// GPU-PARTICLES-006: Force fields and physics
// GPU-PARTICLES-007: Collision detection
// GPU-PARTICLES-008: Sorting and culling
// GPU-PARTICLES-009: Performance optimization
// GPU-PARTICLES-010: Memory management

#include "../../include/renderer/gpu_particles.h"
#include "../../include/core/logger.h"
#include <stdlib.h>
#include <string.h>
#include <math.h>

#ifdef VULKAN_BUILD
#include <vulkan/vulkan.h>
#endif

// GPU-PARTICLES-001: Initialize GPU particle system
bool gpu_particles_init(ParticleSystem* system, VulkanRenderer* renderer, const ParticleSystemConfig* config) {
    if (!system || !renderer || !config) {
        LOG_ERROR("Invalid parameters for GPU particle system initialization");
        return false;
    }
    
#ifdef VULKAN_BUILD
    // Check GPU support
    if (!gpu_particles_check_gpu_support(renderer)) {
        LOG_ERROR("GPU does not support required particle system features");
        return false;
    }
    
    // Initialize system structure
    memset(system, 0, sizeof(ParticleSystem));
    system->config = *config;
    system->initialized = false;
    
    // Create particle buffers (double buffering)
    u64 particleBufferSize = config->maxParticles * sizeof(Particle);
    
    if (!vulkan_create_buffer(renderer, particleBufferSize,
                              VK_BUFFER_USAGE_STORAGE_BUFFER_BIT | VK_BUFFER_USAGE_TRANSFER_DST_BIT,
                              VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT,
                              &system->particleBuffer, &system->particleMemory)) {
        LOG_ERROR("Failed to create particle buffer");
        return false;
    }
    
    if (!vulkan_create_buffer(renderer, particleBufferSize,
                              VK_BUFFER_USAGE_STORAGE_BUFFER_BIT | VK_BUFFER_USAGE_TRANSFER_SRC_BIT,
                              VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT,
                              &system->particleBufferRead, &system->particleMemoryRead)) {
        LOG_ERROR("Failed to create particle read buffer");
        gpu_particles_shutdown(system, renderer);
        return false;
    }
    
    // Create emitter buffer
    u64 emitterBufferSize = config->maxEmitters * sizeof(ParticleEmitter);
    
    if (!vulkan_create_buffer(renderer, emitterBufferSize,
                              VK_BUFFER_USAGE_STORAGE_BUFFER_BIT | VK_BUFFER_USAGE_TRANSFER_DST_BIT,
                              VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT,
                              &system->emitterBuffer, &system->emitterMemory)) {
        LOG_ERROR("Failed to create emitter buffer");
        gpu_particles_shutdown(system, renderer);
        return false;
    }
    
    // Create force field buffer
    u64 forceFieldBufferSize = config->maxForceFields * sizeof(ForceField);
    
    if (!vulkan_create_buffer(renderer, forceFieldBufferSize,
                              VK_BUFFER_USAGE_STORAGE_BUFFER_BIT | VK_BUFFER_USAGE_TRANSFER_DST_BIT,
                              VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT,
                              &system->forceFieldBuffer, &system->forceFieldMemory)) {
        LOG_ERROR("Failed to create force field buffer");
        gpu_particles_shutdown(system, renderer);
        return false;
    }
    
    // Create indirect dispatch buffer
    u64 indirectBufferSize = sizeof(VkDispatchIndirectCommand);
    
    if (!vulkan_create_buffer(renderer, indirectBufferSize,
                              VK_BUFFER_USAGE_INDIRECT_BUFFER_BIT | VK_BUFFER_USAGE_STORAGE_BUFFER_BIT,
                              VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT,
                              &system->indirectBuffer, &system->indirectMemory)) {
        LOG_ERROR("Failed to create indirect buffer");
        gpu_particles_shutdown(system, renderer);
        return false;
    }
    
    // Create compute descriptor set layout
    VkDescriptorSetLayoutBinding computeBindings[5] = {0};
    
    // Binding 0: Particle buffer (read/write)
    computeBindings[0].binding = 0;
    computeBindings[0].descriptorType = VK_DESCRIPTOR_TYPE_STORAGE_BUFFER;
    computeBindings[0].descriptorCount = 1;
    computeBindings[0].stageFlags = VK_SHADER_STAGE_COMPUTE_BIT;
    
    // Binding 1: Particle buffer (read)
    computeBindings[1].binding = 1;
    computeBindings[1].descriptorType = VK_DESCRIPTOR_TYPE_STORAGE_BUFFER;
    computeBindings[1].descriptorCount = 1;
    computeBindings[1].stageFlags = VK_SHADER_STAGE_COMPUTE_BIT;
    
    // Binding 2: Emitter buffer
    computeBindings[2].binding = 2;
    computeBindings[2].descriptorType = VK_DESCRIPTOR_TYPE_STORAGE_BUFFER;
    computeBindings[2].descriptorCount = 1;
    computeBindings[2].stageFlags = VK_SHADER_STAGE_COMPUTE_BIT;
    
    // Binding 3: Force field buffer
    computeBindings[3].binding = 3;
    computeBindings[3].descriptorType = VK_DESCRIPTOR_TYPE_STORAGE_BUFFER;
    computeBindings[3].descriptorCount = 1;
    computeBindings[3].stageFlags = VK_SHADER_STAGE_COMPUTE_BIT;
    
    // Binding 4: Indirect buffer
    computeBindings[4].binding = 4;
    computeBindings[4].descriptorType = VK_DESCRIPTOR_TYPE_STORAGE_BUFFER;
    computeBindings[4].descriptorCount = 1;
    computeBindings[4].stageFlags = VK_SHADER_STAGE_COMPUTE_BIT;
    
    VkDescriptorSetLayoutCreateInfo computeLayoutInfo = {0};
    computeLayoutInfo.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_LAYOUT_CREATE_INFO;
    computeLayoutInfo.bindingCount = 5;
    computeLayoutInfo.pBindings = computeBindings;
    
    if (vkCreateDescriptorSetLayout(renderer->device, &computeLayoutInfo, NULL, &system->computeDescriptorLayout) != VK_SUCCESS) {
        LOG_ERROR("Failed to create compute descriptor set layout");
        gpu_particles_shutdown(system, renderer);
        return false;
    }
    
    // Create compute pipeline layout
    VkPushConstantRange pushConstantRange = {0};
    pushConstantRange.stageFlags = VK_SHADER_STAGE_COMPUTE_BIT;
    pushConstantRange.offset = 0;
    pushConstantRange.size = sizeof(float) * 4; // deltaTime, gravity, damping, padding
    
    VkPipelineLayoutCreateInfo computePipelineLayoutInfo = {0};
    computePipelineLayoutInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_LAYOUT_CREATE_INFO;
    computePipelineLayoutInfo.setLayoutCount = 1;
    computePipelineLayoutInfo.pSetLayouts = &system->computeDescriptorLayout;
    computePipelineLayoutInfo.pushConstantRangeCount = 1;
    computePipelineLayoutInfo.pPushConstantRanges = &pushConstantRange;
    
    if (vkCreatePipelineLayout(renderer->device, &computePipelineLayoutInfo, NULL, &system->computeLayout) != VK_SUCCESS) {
        LOG_ERROR("Failed to create compute pipeline layout");
        gpu_particles_shutdown(system, renderer);
        return false;
    }
    
    // Initialize CPU-side data
    memset(system->emitters, 0, sizeof(system->emitters));
    memset(system->forceFields, 0, sizeof(system->forceFields));
    system->emitterCount = 0;
    system->forceFieldCount = 0;
    
    system->stats.memoryUsage = gpu_particles_estimate_memory_usage(config);
    system->initialized = true;
    
    LOG_INFO("GPU particle system initialized successfully");
    LOG_INFO("  Max particles: %u", config->maxParticles);
    LOG_INFO("  Max emitters: %u", config->maxEmitters);
    LOG_INFO("  Memory usage: %.1f MB", system->stats.memoryUsage / (1024.0 * 1024.0));
    
    return true;
#else
    (void)system;
    (void)renderer;
    (void)config;
    LOG_ERROR("GPU particles not available (built without Vulkan)");
    return false;
#endif
}

// GPU-PARTICLES-002: Update particle simulation
bool gpu_particles_update(ParticleSystem* system, VulkanRenderer* renderer, VkCommandBuffer commandBuffer, float deltaTime) {
    if (!system || !renderer || !commandBuffer || !system->initialized) {
        LOG_ERROR("Invalid parameters for particle update");
        return false;
    }
    
#ifdef VULKAN_BUILD
    u64 startTime = get_current_time_ms();
    
    // Update emitter data on GPU
    if (system->emitterCount > 0) {
        void* emitterPtr;
        vkMapMemory(renderer->device, system->emitterMemory, 0, 
                    system->emitterCount * sizeof(ParticleEmitter), 0, &emitterPtr);
        memcpy(emitterPtr, system->emitters, system->emitterCount * sizeof(ParticleEmitter));
        vkUnmapMemory(renderer->device, system->emitterMemory);
    }
    
    // Update force field data on GPU
    if (system->forceFieldCount > 0) {
        void* fieldPtr;
        vkMapMemory(renderer->device, system->forceFieldMemory, 0,
                    system->forceFieldCount * sizeof(ForceField), 0, &fieldPtr);
        memcpy(fieldPtr, system->forceFields, system->forceFieldCount * sizeof(ForceField));
        vkUnmapMemory(renderer->device, system->forceFieldMemory);
    }
    
    // Update indirect dispatch count
    VkDispatchIndirectCommand indirectCmd = {0};
    indirectCmd.x = gpu_particles_calculate_work_group_count(system->config.maxParticles);
    indirectCmd.y = 1;
    indirectCmd.z = 1;
    
    void* indirectPtr;
    vkMapMemory(renderer->device, system->indirectMemory, 0, sizeof(VkDispatchIndirectCommand), 0, &indirectPtr);
    memcpy(indirectPtr, &indirectCmd, sizeof(VkDispatchIndirectCommand));
    vkUnmapMemory(renderer->device, system->indirectMemory);
    
    // Bind compute pipeline
    vkCmdBindPipeline(commandBuffer, VK_PIPELINE_BIND_POINT_COMPUTE, system->computePipeline);
    
    // Bind descriptor sets
    vkCmdBindDescriptorSets(commandBuffer, VK_PIPELINE_BIND_POINT_COMPUTE,
                           system->computeLayout, 0, 1, &system->computeDescriptorSet, 0, NULL);
    
    // Set push constants
    float pushConstants[4] = {
        deltaTime,
        system->config.gravity,
        system->config.damping,
        0.0f // padding
    };
    vkCmdPushConstants(commandBuffer, system->computeLayout, VK_SHADER_STAGE_COMPUTE_BIT,
                      0, sizeof(pushConstants), pushConstants);
    
    // Dispatch compute shader
    vkCmdDispatchIndirect(commandBuffer, system->indirectBuffer, 0);
    
    // Add memory barrier to ensure compute completion before rendering
    VkMemoryBarrier memoryBarrier = {0};
    memoryBarrier.sType = VK_STRUCTURE_TYPE_MEMORY_BARRIER;
    memoryBarrier.srcAccessMask = VK_ACCESS_SHADER_WRITE_BIT;
    memoryBarrier.dstAccessMask = VK_ACCESS_SHADER_READ_BIT;
    
    vkCmdPipelineBarrier(commandBuffer,
                        VK_PIPELINE_STAGE_COMPUTE_SHADER_BIT,
                        VK_PIPELINE_STAGE_VERTEX_SHADER_BIT | VK_PIPELINE_STAGE_FRAGMENT_SHADER_BIT,
                        0, 0, &memoryBarrier, 0, NULL, 0, NULL);
    
    // Update statistics
    u64 endTime = get_current_time_ms();
    system->stats.simulationTime = (endTime - startTime) / 1000.0;
    system->stats.totalUpdates++;
    
    return true;
#else
    (void)system;
    (void)renderer;
    (void)commandBuffer;
    (void)deltaTime;
    return false;
#endif
}

// GPU-PARTICLES-005: Add emitter
u32 gpu_particles_add_emitter(ParticleSystem* system, const ParticleEmitter* emitter) {
    if (!system || !emitter || system->emitterCount >= MAX_EMITTERS) {
        LOG_ERROR("Cannot add emitter: invalid parameters or emitter limit reached");
        return UINT32_MAX;
    }
    
    u32 emitterId = system->emitterCount++;
    system->emitters[emitterId] = *emitter;
    system->emitters[emitterId].enabled = true;
    
    system->stats.activeEmitters++;
    system->stats.totalParticlesEmitted += emitter->maxParticles;
    
    LOG_INFO("Added particle emitter %u", emitterId);
    return emitterId;
}

// GPU-PARTICLES-006: Add force field
u32 gpu_particles_add_force_field(ParticleSystem* system, const ForceField* forceField) {
    if (!system || !forceField || system->forceFieldCount >= MAX_FORCE_FIELDS) {
        LOG_ERROR("Cannot add force field: invalid parameters or field limit reached");
        return UINT32_MAX;
    }
    
    u32 fieldId = system->forceFieldCount++;
    system->forceFields[fieldId] = *forceField;
    system->forceFields[fieldId].enabled = true;
    
    system->stats.activeForceFields++;
    
    LOG_INFO("Added force field %u", fieldId);
    return fieldId;
}

// GPU-PARTICLES-004: Render particles
bool gpu_particles_render(ParticleSystem* system, VulkanRenderer* renderer, VkCommandBuffer commandBuffer,
                          const mat4* viewMatrix, const mat4* projMatrix) {
    if (!system || !renderer || !commandBuffer || !viewMatrix || !projMatrix) {
        LOG_ERROR("Invalid parameters for particle rendering");
        return false;
    }
    
#ifdef VULKAN_BUILD
    u64 startTime = get_current_time_ms();
    
    // Bind render pipeline
    vkCmdBindPipeline(commandBuffer, VK_PIPELINE_BIND_POINT_GRAPHICS, system->renderPipeline);
    
    // Bind descriptor sets
    vkCmdBindDescriptorSets(commandBuffer, VK_PIPELINE_BIND_POINT_GRAPHICS,
                           system->renderLayout, 0, 1, &system->renderDescriptorSet, 0, NULL);
    
    // Set push constants (matrices)
    mat4 viewProjMatrix;
    mat4_mul(&viewProjMatrix, projMatrix, viewMatrix);
    
    vkCmdPushConstants(commandBuffer, system->renderLayout,
                      VK_SHADER_STAGE_VERTEX_BIT | VK_SHADER_STAGE_FRAGMENT_BIT,
                      0, sizeof(mat4), &viewProjMatrix);
    
    // Draw particles (instanced rendering)
    vkCmdDrawIndirect(commandBuffer, system->indirectBuffer, 0, 1, sizeof(VkDrawIndirectCommand));
    
    // Update statistics
    u64 endTime = get_current_time_ms();
    system->stats.renderTime = (endTime - startTime) / 1000.0;
    
    return true;
#else
    (void)system;
    (void)renderer;
    (void)commandBuffer;
    (void)viewMatrix;
    (void)projMatrix;
    return false;
#endif
}

// Utility functions
u64 gpu_particles_estimate_memory_usage(const ParticleSystemConfig* config) {
    if (!config) return 0;
    
    u64 memoryUsage = 0;
    
    // Particle buffers (double buffering)
    memoryUsage += config->maxParticles * sizeof(Particle) * 2;
    
    // Emitter buffer
    memoryUsage += config->maxEmitters * sizeof(ParticleEmitter);
    
    // Force field buffer
    memoryUsage += config->maxForceFields * sizeof(ForceField);
    
    // Indirect buffer
    memoryUsage += sizeof(VkDispatchIndirectCommand);
    
    // Texture atlas (estimate)
    memoryUsage += 1024ULL * 1024 * 4; // 1MB texture atlas
    
    return memoryUsage;
}

bool gpu_particles_check_gpu_support(VulkanRenderer* renderer) {
    if (!renderer) return false;
    
    // Check for compute shader support
    u32 queueFamilyIndex = UINT32_MAX;
    for (u32 i = 0; i < renderer->queueFamilyCount; i++) {
        if (renderer->queueFamilies[i].queueFlags & VK_QUEUE_COMPUTE_BIT) {
            queueFamilyIndex = i;
            break;
        }
    }
    
    if (queueFamilyIndex == UINT32_MAX) {
        LOG_ERROR("GPU does not support compute shaders");
        return false;
    }
    
    // Check for storage buffer support
    VkPhysicalDeviceFeatures features;
    vkGetPhysicalDeviceFeatures(renderer->physicalDevice, &features);
    
    if (!features.vertexPipelineStoresAndAtomics || !features.fragmentStoresAndAtomics) {
        LOG_ERROR("GPU does not support required atomic operations");
        return false;
    }
    
    return true;
}

u32 gpu_particles_calculate_work_group_count(u32 particleCount) {
    // Assuming work group size of 256
    const u32 workGroupSize = 256;
    return (particleCount + workGroupSize - 1) / workGroupSize;
}

void gpu_particles_get_stats(const ParticleSystem* system, ParticleSystemStats* outStats) {
    if (!system || !outStats) return;
    
    *outStats = system->stats;
    outStats->activeParticles = system->config.maxParticles; // Estimate
}

void gpu_particles_reset_stats(ParticleSystem* system) {
    if (!system) return;
    
    memset(&system->stats, 0, sizeof(ParticleSystemStats));
}

void gpu_particles_debug_print_stats(const ParticleSystem* system) {
    if (!system) return;
    
    const ParticleSystemStats* stats = &system->stats;
    
    LOG_INFO("=== GPU Particle System Statistics ===");
    LOG_INFO("Active Particles: %u", stats->activeParticles);
    LOG_INFO("Active Emitters: %u", stats->activeEmitters);
    LOG_INFO("Active Force Fields: %u", stats->activeForceFields);
    LOG_INFO("Total Particles Emitted: %llu", stats->totalParticlesEmitted);
    LOG_INFO("Total Updates: %llu", stats->totalUpdates);
    LOG_INFO("Simulation Time: %.2f ms", stats->simulationTime * 1000.0);
    LOG_INFO("Render Time: %.2f ms", stats->renderTime * 1000.0);
    LOG_INFO("Memory Usage: %.1f MB", stats->memoryUsage / (1024.0 * 1024.0));
}

void gpu_particles_shutdown(ParticleSystem* system, VulkanRenderer* renderer) {
    if (!system || !renderer) return;
    
#ifdef VULKAN_BUILD
    if (system->particleBuffer) {
        vulkan_destroy_buffer(renderer, system->particleBuffer, system->particleMemory);
    }
    
    if (system->particleBufferRead) {
        vulkan_destroy_buffer(renderer, system->particleBufferRead, system->particleMemoryRead);
    }
    
    if (system->emitterBuffer) {
        vulkan_destroy_buffer(renderer, system->emitterBuffer, system->emitterMemory);
    }
    
    if (system->forceFieldBuffer) {
        vulkan_destroy_buffer(renderer, system->forceFieldBuffer, system->forceFieldMemory);
    }
    
    if (system->indirectBuffer) {
        vulkan_destroy_buffer(renderer, system->indirectBuffer, system->indirectMemory);
    }
    
    if (system->computeDescriptorLayout) {
        vkDestroyDescriptorSetLayout(renderer->device, system->computeDescriptorLayout, NULL);
    }
    
    if (system->computeLayout) {
        vkDestroyPipelineLayout(renderer->device, system->computeLayout, NULL);
    }
    
    if (system->renderDescriptorLayout) {
        vkDestroyDescriptorSetLayout(renderer->device, system->renderDescriptorLayout, NULL);
    }
    
    if (system->renderLayout) {
        vkDestroyPipelineLayout(renderer->device, system->renderLayout, NULL);
    }
    
    memset(system, 0, sizeof(ParticleSystem));
    
    LOG_INFO("GPU particle system shutdown complete");
#endif
}

bool gpu_particles_is_initialized(const ParticleSystem* system) {
    return system && system->initialized;
}
