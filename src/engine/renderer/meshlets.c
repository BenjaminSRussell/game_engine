// Meshlets System Implementation
// MESHLETS-001: Meshlet core architecture
// MESHLETS-002: Meshlet generation and clustering
// MESHLETS-003: Mesh shader pipeline
// MESHLETS-004: Culling and optimization
// MESHLETS-005: LOD management
// MESHLETS-006: Compression and streaming
// MESHLETS-007: Debugging and visualization
// MESHLETS-008: Performance optimization
// MESHLETS-009: Cross-platform support
// MESHLETS-010: Integration with existing systems

#include "../../include/renderer/meshlets.h"
#include "../../include/core/logger.h"
#include <stdlib.h>
#include <string.h>
#include <math.h>

#ifdef VULKAN_BUILD
#include <vulkan/vulkan.h>
#endif

// MESHLETS-001: Initialize meshlet system
bool meshlets_init(MeshletSystem* system, VulkanRenderer* renderer, const MeshletConfig* config) {
    if (!system || !renderer || !config) {
        LOG_ERROR("Invalid parameters for meshlet system initialization");
        return false;
    }
    
#ifdef VULKAN_BUILD
    // Check GPU support for mesh shaders
    if (!meshlets_check_gpu_support(renderer)) {
        LOG_ERROR("GPU does not support required mesh shader features");
        return false;
    }
    
    // Initialize system structure
    memset(system, 0, sizeof(MeshletSystem));
    system->config = *config;
    system->initialized = false;
    
    // Create vertex buffer
    u64 vertexBufferSize = config->maxMeshletsPerMesh * config->maxVerticesPerMeshlet * sizeof(MeshletVertex);
    
    if (!vulkan_create_buffer(renderer, vertexBufferSize,
                              VK_BUFFER_USAGE_VERTEX_BUFFER_BIT | VK_BUFFER_USAGE_STORAGE_BUFFER_BIT |
                              VK_BUFFER_USAGE_TRANSFER_DST_BIT,
                              VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT,
                              &system->vertexBuffer, &system->vertexMemory)) {
        LOG_ERROR("Failed to create meshlet vertex buffer");
        return false;
    }
    
    // Create index buffer
    u64 indexBufferSize = config->maxMeshletsPerMesh * config->maxPrimitivesPerMeshlet * 3 * sizeof(u32);
    
    if (!vulkan_create_buffer(renderer, indexBufferSize,
                              VK_BUFFER_USAGE_INDEX_BUFFER_BIT | VK_BUFFER_USAGE_STORAGE_BUFFER_BIT |
                              VK_BUFFER_USAGE_TRANSFER_DST_BIT,
                              VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT,
                              &system->indexBuffer, &system->indexMemory)) {
        LOG_ERROR("Failed to create meshlet index buffer");
        meshlets_shutdown(system, renderer);
        return false;
    }
    
    // Create meshlet data buffer
    u64 meshletBufferSize = config->maxMeshletsPerMesh * sizeof(Meshlet);
    
    if (!vulkan_create_buffer(renderer, meshletBufferSize,
                              VK_BUFFER_USAGE_STORAGE_BUFFER_BIT | VK_BUFFER_USAGE_TRANSFER_DST_BIT,
                              VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT,
                              &system->meshletBuffer, &system->meshletMemory)) {
        LOG_ERROR("Failed to create meshlet data buffer");
        meshlets_shutdown(system, renderer);
        return false;
    }
    
    // Create meshlet group buffer
    u64 groupBufferSize = MAX_MESHLET_GROUPS * sizeof(MeshletGroup);
    
    if (!vulkan_create_buffer(renderer, groupBufferSize,
                              VK_BUFFER_USAGE_STORAGE_BUFFER_BIT | VK_BUFFER_USAGE_TRANSFER_DST_BIT,
                              VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT,
                              &system->meshletGroupBuffer, &system->meshletGroupMemory)) {
        LOG_ERROR("Failed to create meshlet group buffer");
        meshlets_shutdown(system, renderer);
        return false;
    }
    
    // Create indirect draw buffer
    u64 indirectBufferSize = config->maxMeshletsPerMesh * sizeof(VkDrawMeshTasksIndirectCommandEXT);
    
    if (!vulkan_create_buffer(renderer, indirectBufferSize,
                              VK_BUFFER_USAGE_INDIRECT_BUFFER_BIT | VK_BUFFER_USAGE_STORAGE_BUFFER_BIT |
                              VK_BUFFER_USAGE_TRANSFER_DST_BIT,
                              VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT,
                              &system->indirectBuffer, &system->indirectMemory)) {
        LOG_ERROR("Failed to create meshlet indirect buffer");
        meshlets_shutdown(system, renderer);
        return false;
    }
    
    // Create culling indirect buffer
    if (!vulkan_create_buffer(renderer, indirectBufferSize,
                              VK_BUFFER_USAGE_INDIRECT_BUFFER_BIT | VK_BUFFER_USAGE_STORAGE_BUFFER_BIT |
                              VK_BUFFER_USAGE_TRANSFER_DST_BIT,
                              VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT,
                              &system->cullIndirectBuffer, &system->cullIndirectMemory)) {
        LOG_ERROR("Failed to create meshlet culling indirect buffer");
        meshlets_shutdown(system, renderer);
        return false;
    }
    
    // Create mesh shader descriptor set layout
    VkDescriptorSetLayoutBinding meshBindings[6] = {0};
    
    // Binding 0: Vertex buffer
    meshBindings[0].binding = 0;
    meshBindings[0].descriptorType = VK_DESCRIPTOR_TYPE_STORAGE_BUFFER;
    meshBindings[0].descriptorCount = 1;
    meshBindings[0].stageFlags = VK_SHADER_STAGE_MESH_BIT_EXT;
    
    // Binding 1: Index buffer
    meshBindings[1].binding = 1;
    meshBindings[1].descriptorType = VK_DESCRIPTOR_TYPE_STORAGE_BUFFER;
    meshBindings[1].descriptorCount = 1;
    meshBindings[1].stageFlags = VK_SHADER_STAGE_MESH_BIT_EXT;
    
    // Binding 2: Meshlet buffer
    meshBindings[2].binding = 2;
    meshBindings[2].descriptorType = VK_DESCRIPTOR_TYPE_STORAGE_BUFFER;
    meshBindings[2].descriptorCount = 1;
    meshBindings[2].stageFlags = VK_SHADER_STAGE_MESH_BIT_EXT;
    
    // Binding 3: Group buffer
    meshBindings[3].binding = 3;
    meshBindings[3].descriptorType = VK_DESCRIPTOR_TYPE_STORAGE_BUFFER;
    meshBindings[3].descriptorCount = 1;
    meshBindings[3].stageFlags = VK_SHADER_STAGE_MESH_BIT_EXT;
    
    // Binding 4: Material buffer
    meshBindings[4].binding = 4;
    meshBindings[4].descriptorType = VK_DESCRIPTOR_TYPE_STORAGE_BUFFER;
    meshBindings[4].descriptorCount = 1;
    meshBindings[4].stageFlags = VK_SHADER_STAGE_MESH_BIT_EXT | VK_SHADER_STAGE_FRAGMENT_BIT;
    
    // Binding 5: Camera uniform buffer
    meshBindings[5].binding = 5;
    meshBindings[5].descriptorType = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER;
    meshBindings[5].descriptorCount = 1;
    meshBindings[5].stageFlags = VK_SHADER_STAGE_MESH_BIT_EXT | VK_SHADER_STAGE_FRAGMENT_BIT;
    
    VkDescriptorSetLayoutCreateInfo meshLayoutInfo = {0};
    meshLayoutInfo.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_LAYOUT_CREATE_INFO;
    meshLayoutInfo.bindingCount = 6;
    meshLayoutInfo.pBindings = meshBindings;
    
    if (vkCreateDescriptorSetLayout(renderer->device, &meshLayoutInfo, NULL, &system->meshShaderDescriptorLayout) != VK_SUCCESS) {
        LOG_ERROR("Failed to create mesh shader descriptor set layout");
        meshlets_shutdown(system, renderer);
        return false;
    }
    
    // Create mesh shader pipeline layout
    VkPushConstantRange pushConstantRange = {0};
    pushConstantRange.stageFlags = VK_SHADER_STAGE_MESH_BIT_EXT | VK_SHADER_STAGE_FRAGMENT_BIT;
    pushConstantRange.offset = 0;
    pushConstantRange.size = sizeof(mat4) * 2; // View and projection matrices
    
    VkPipelineLayoutCreateInfo meshPipelineLayoutInfo = {0};
    meshPipelineLayoutInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_LAYOUT_CREATE_INFO;
    meshPipelineLayoutInfo.setLayoutCount = 1;
    meshPipelineLayoutInfo.pSetLayouts = &system->meshShaderDescriptorLayout;
    meshPipelineLayoutInfo.pushConstantRangeCount = 1;
    meshPipelineLayoutInfo.pPushConstantRanges = &pushConstantRange;
    
    if (vkCreatePipelineLayout(renderer->device, &meshPipelineLayoutInfo, NULL, &system->meshShaderLayout) != VK_SUCCESS) {
        LOG_ERROR("Failed to create mesh shader pipeline layout");
        meshlets_shutdown(system, renderer);
        return false;
    }
    
    // Create culling compute descriptor set layout
    VkDescriptorSetLayoutBinding cullBindings[4] = {0};
    
    // Binding 0: Meshlet buffer
    cullBindings[0].binding = 0;
    cullBindings[0].descriptorType = VK_DESCRIPTOR_TYPE_STORAGE_BUFFER;
    cullBindings[0].descriptorCount = 1;
    cullBindings[0].stageFlags = VK_SHADER_STAGE_COMPUTE_BIT;
    
    // Binding 1: Group buffer
    cullBindings[1].binding = 1;
    cullBindings[1].descriptorType = VK_DESCRIPTOR_TYPE_STORAGE_BUFFER;
    cullBindings[1].descriptorCount = 1;
    cullBindings[1].stageFlags = VK_SHADER_STAGE_COMPUTE_BIT;
    
    // Binding 2: Input indirect buffer
    cullBindings[2].binding = 2;
    cullBindings[2].descriptorType = VK_DESCRIPTOR_TYPE_STORAGE_BUFFER;
    cullBindings[2].descriptorCount = 1;
    cullBindings[2].stageFlags = VK_SHADER_STAGE_COMPUTE_BIT;
    
    // Binding 3: Output indirect buffer
    cullBindings[3].binding = 3;
    cullBindings[3].descriptorType = VK_DESCRIPTOR_TYPE_STORAGE_BUFFER;
    cullBindings[3].descriptorCount = 1;
    cullBindings[3].stageFlags = VK_SHADER_STAGE_COMPUTE_BIT;
    
    VkDescriptorSetLayoutCreateInfo cullLayoutInfo = {0};
    cullLayoutInfo.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_LAYOUT_CREATE_INFO;
    cullLayoutInfo.bindingCount = 4;
    cullLayoutInfo.pBindings = cullBindings;
    
    if (vkCreateDescriptorSetLayout(renderer->device, &cullLayoutInfo, NULL, &system->cullDescriptorLayout) != VK_SUCCESS) {
        LOG_ERROR("Failed to create culling descriptor set layout");
        meshlets_shutdown(system, renderer);
        return false;
    }
    
    // Create culling pipeline layout
    VkPipelineLayoutCreateInfo cullPipelineLayoutInfo = {0};
    cullPipelineLayoutInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_LAYOUT_CREATE_INFO;
    cullPipelineLayoutInfo.setLayoutCount = 1;
    cullPipelineLayoutInfo.pSetLayouts = &system->cullDescriptorLayout;
    
    if (vkCreatePipelineLayout(renderer->device, &cullPipelineLayoutInfo, NULL, &system->cullLayout) != VK_SUCCESS) {
        LOG_ERROR("Failed to create culling pipeline layout");
        meshlets_shutdown(system, renderer);
        return false;
    }
    
    system->stats.memoryUsage = meshlets_estimate_memory_usage(config, 0, 0);
    system->initialized = true;
    
    LOG_INFO("Meshlet system initialized successfully");
    LOG_INFO("  Max vertices per meshlet: %u", config->maxVerticesPerMeshlet);
    LOG_INFO("  Max primitives per meshlet: %u", config->maxPrimitivesPerMeshlet);
    LOG_INFO("  Max meshlets per mesh: %u", config->maxMeshletsPerMesh);
    LOG_INFO("  Culling mode: %s", meshlets_get_cull_mode_name(config->cullMode));
    LOG_INFO("  Memory usage: %.1f MB", system->stats.memoryUsage / (1024.0 * 1024.0));
    
    return true;
#else
    (void)system;
    (void)renderer;
    (void)config;
    LOG_ERROR("Meshlets not available (built without Vulkan)");
    return false;
#endif
}

// MESHLETS-002: Generate meshlets from mesh data
bool meshlets_generate_from_mesh(MeshletSystem* system, const void* vertices, u32 vertexCount,
                                 const u32* indices, u32 indexCount) {
    if (!system || !vertices || !indices || vertexCount == 0 || indexCount == 0) {
        LOG_ERROR("Invalid parameters for meshlet generation");
        return false;
    }
    
    // Free existing data
    if (system->vertices) free(system->vertices);
    if (system->primitives) free(system->primitives);
    if (system->meshlets) free(system->meshlets);
    
    // Convert input vertices to meshlet vertices
    system->vertices = malloc(vertexCount * sizeof(MeshletVertex));
    system->vertexCount = vertexCount;
    
    const float* inputVertices = (const float*)vertices;
    for (u32 i = 0; i < vertexCount; i++) {
        system->vertices[i].position.x = inputVertices[i * 8 + 0];
        system->vertices[i].position.y = inputVertices[i * 8 + 1];
        system->vertices[i].position.z = inputVertices[i * 8 + 2];
        system->vertices[i].normal.x = inputVertices[i * 8 + 3];
        system->vertices[i].normal.y = inputVertices[i * 8 + 4];
        system->vertices[i].normal.z = inputVertices[i * 8 + 5];
        system->vertices[i].texCoord0.x = inputVertices[i * 8 + 6];
        system->vertices[i].texCoord0.y = inputVertices[i * 8 + 7];
        system->vertices[i].color = (vec4){1.0f, 1.0f, 1.0f, 1.0f};
        system->vertices[i].meshletId = UINT32_MAX;
    }
    
    // Convert indices to primitives
    u32 primitiveCount = indexCount / 3;
    system->primitives = malloc(primitiveCount * sizeof(MeshletPrimitive));
    system->primitiveCount = primitiveCount;
    
    for (u32 i = 0; i < primitiveCount; i++) {
        system->primitives[i].indices[0] = indices[i * 3 + 0];
        system->primitives[i].indices[1] = indices[i * 3 + 1];
        system->primitives[i].indices[2] = indices[i * 3 + 2];
        system->primitives[i].meshletId = UINT32_MAX;
        
        // Calculate primitive normal and area
        vec3 v0 = system->vertices[indices[i * 3 + 0]].position;
        vec3 v1 = system->vertices[indices[i * 3 + 1]].position;
        vec3 v2 = system->vertices[indices[i * 3 + 2]].position;
        
        vec3 edge1 = vec3_sub(v1, v0);
        vec3 edge2 = vec3_sub(v2, v0);
        system->primitives[i].normal = vec3_normalize(vec3_cross(edge1, edge2));
        system->primitives[i].area = vec3_length(vec3_cross(edge1, edge2)) * 0.5f;
    }
    
    // Generate meshlets using simple clustering algorithm
    u32 maxVertices = system->config.maxVerticesPerMeshlet;
    u32 maxPrimitives = system->config.maxPrimitivesPerMeshlet;
    
    // Estimate meshlet count
    u32 estimatedMeshlets = (primitiveCount + maxPrimitives - 1) / maxPrimitives;
    system->meshlets = malloc(estimatedMeshlets * sizeof(Meshlet));
    system->meshletCount = 0;
    
    // Simple greedy clustering
    bool* usedPrimitives = calloc(primitiveCount, sizeof(bool));
    u32* vertexUsage = calloc(vertexCount, sizeof(u32));
    
    for (u32 i = 0; i < primitiveCount && system->meshletCount < estimatedMeshlets; i++) {
        if (usedPrimitives[i]) continue;
        
        // Start new meshlet
        Meshlet* meshlet = &system->meshlets[system->meshletCount];
        memset(meshlet, 0, sizeof(Meshlet));
        
        u32 vertexCount = 0;
        u32 primitiveCount = 0;
        
        // Add primitives to meshlet
        for (u32 j = i; j < primitiveCount && primitiveCount < maxPrimitives; j++) {
            if (usedPrimitives[j]) continue;
            
            MeshletPrimitive* prim = &system->primitives[j];
            
            // Check if adding this primitive would exceed vertex limit
            u32 newVertices = 0;
            for (u32 k = 0; k < 3; k++) {
                if (vertexUsage[prim->indices[k]] == 0) {
                    newVertices++;
                }
            }
            
            if (vertexCount + newVertices > maxVertices) continue;
            
            // Add primitive to meshlet
            usedPrimitives[j] = true;
            prim->meshletId = system->meshletCount;
            
            for (u32 k = 0; k < 3; k++) {
                u32 vertexIndex = prim->indices[k];
                if (vertexUsage[vertexIndex] == 0) {
                    vertexUsage[vertexIndex] = 1;
                    vertexCount++;
                }
                system->vertices[vertexIndex].meshletId = system->meshletCount;
            }
            
            primitiveCount++;
        }
        
        // Calculate meshlet bounds
        vec3 minBounds = {FLT_MAX, FLT_MAX, FLT_MAX};
        vec3 maxBounds = {-FLT_MAX, -FLT_MAX, -FLT_MAX};
        
        for (u32 j = 0; j < vertexCount; j++) {
            for (u32 k = 0; k < system->vertexCount; k++) {
                if (system->vertices[k].meshletId == system->meshletCount) {
                    vec3 pos = system->vertices[k].position;
                    minBounds.x = fminf(minBounds.x, pos.x);
                    minBounds.y = fminf(minBounds.y, pos.y);
                    minBounds.z = fminf(minBounds.z, pos.z);
                    maxBounds.x = fmaxf(maxBounds.x, pos.x);
                    maxBounds.y = fmaxf(maxBounds.y, pos.y);
                    maxBounds.z = fmaxf(maxBounds.z, pos.z);
                }
            }
        }
        
        meshlet->boundingBoxMin = minBounds;
        meshlet->boundingBoxMax = maxBounds;
        meshlet->boundingSphereCenter = vec3_scale(vec3_add(minBounds, maxBounds), 0.5f);
        meshlet->boundingSphereRadius = vec3_length(vec3_sub(maxBounds, minBounds)) * 0.5f;
        
        meshlet->vertexCount = vertexCount;
        meshlet->primitiveCount = primitiveCount;
        meshlet->visible = true;
        meshlet->culled = false;
        
        system->meshletCount++;
    }
    
    free(usedPrimitives);
    free(vertexUsage);
    
    // Create default meshlet group
    system->groups = malloc(sizeof(MeshletGroup));
    system->groupCount = 1;
    
    system->groups[0].meshletOffset = 0;
    system->groups[0].meshletCount = system->meshletCount;
    system->groups[0].materialId = 0;
    system->groups[0].visible = true;
    system->groups[0].castShadows = true;
    system->groups[0].receiveShadows = true;
    
    // Update statistics
    system->stats.totalVertices = vertexCount;
    system->stats.totalPrimitives = primitiveCount;
    system->stats.totalMeshlets = system->meshletCount;
    system->stats.totalGroups = system->groupCount;
    system->stats.averageVerticesPerMeshlet = (double)vertexCount / system->meshletCount;
    system->stats.averagePrimitivesPerMeshlet = (double)primitiveCount / system->meshletCount;
    
    LOG_INFO("Generated %u meshlets from %u vertices and %u primitives", 
             system->meshletCount, vertexCount, primitiveCount);
    LOG_INFO("Average %.1f vertices, %.1f primitives per meshlet",
             system->stats.averageVerticesPerMeshlet, system->stats.averagePrimitivesPerMeshlet);
    
    return true;
}

// MESHLETS-003: Render meshlets
bool meshlets_render(MeshletSystem* system, VulkanRenderer* renderer, VkCommandBuffer commandBuffer,
                    const mat4* viewMatrix, const mat4* projMatrix) {
    if (!system || !renderer || !commandBuffer || !viewMatrix || !projMatrix) {
        LOG_ERROR("Invalid parameters for meshlet rendering");
        return false;
    }
    
#ifdef VULKAN_BUILD
    u64 startTime = get_current_time_ms();
    
    // Update meshlet data on GPU
    if (system->meshletCount > 0) {
        // Upload vertex data
        u64 vertexSize = system->vertexCount * sizeof(MeshletVertex);
        vulkan_buffer_update(renderer, system->vertexBuffer, 0, system->vertices, vertexSize);
        
        // Upload primitive data
        u64 primitiveSize = system->primitiveCount * sizeof(MeshletPrimitive);
        vulkan_buffer_update(renderer, system->indexBuffer, 0, system->primitives, primitiveSize);
        
        // Upload meshlet data
        u64 meshletSize = system->meshletCount * sizeof(Meshlet);
        vulkan_buffer_update(renderer, system->meshletBuffer, 0, system->meshlets, meshletSize);
        
        // Upload group data
        u64 groupSize = system->groupCount * sizeof(MeshletGroup);
        void* groupPtr;
        vkMapMemory(renderer->device, system->meshletGroupMemory, 0, groupSize, 0, &groupPtr);
        memcpy(groupPtr, system->groups, groupSize);
        vkUnmapMemory(renderer->device, system->meshletGroupMemory);
    }
    
    // Perform culling if enabled
    if (system->config.cullMode != MESHLET_CULL_NONE) {
        if (!meshlets_cull_frustum(system, renderer, commandBuffer)) {
            LOG_ERROR("Meshlet culling failed");
            return false;
        }
    }
    
    // Bind mesh shader pipeline
    vkCmdBindPipeline(commandBuffer, VK_PIPELINE_BIND_POINT_GRAPHICS, system->meshShaderPipeline);
    
    // Bind descriptor sets
    vkCmdBindDescriptorSets(commandBuffer, VK_PIPELINE_BIND_POINT_GRAPHICS,
                           system->meshShaderLayout, 0, 1, &system->meshShaderDescriptorSet, 0, NULL);
    
    // Set push constants (view and projection matrices)
    mat4 matrices[2] = {*viewMatrix, *projMatrix};
    vkCmdPushConstants(commandBuffer, system->meshShaderLayout,
                      VK_SHADER_STAGE_MESH_BIT_EXT | VK_SHADER_STAGE_FRAGMENT_BIT,
                      0, sizeof(matrices), matrices);
    
    // Issue mesh shader draw calls
    for (u32 i = 0; i < system->groupCount; i++) {
        if (!system->groups[i].visible) continue;
        
        VkDrawMeshTasksIndirectCommandEXT drawCmd = {0};
        drawCmd.taskCount = system->groups[i].meshletCount;
        drawCmd.firstTask = system->groups[i].meshletOffset;
        
        vkCmdDrawMeshTasksIndirectEXT(commandBuffer, system->indirectBuffer,
                                      i * sizeof(VkDrawMeshTasksIndirectCommandEXT), 1);
    }
    
    // Update statistics
    u64 endTime = get_current_time_ms();
    system->stats.drawingTime = (endTime - startTime) / 1000.0;
    system->stats.drawnMeshlets = system->stats.totalMeshlets - system->stats.culledMeshlets;
    
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

// MESHLETS-004: Frustum culling
bool meshlets_cull_frustum(MeshletSystem* system, VulkanRenderer* renderer, VkCommandBuffer commandBuffer) {
    if (!system || !renderer || !commandBuffer) {
        return false;
    }
    
#ifdef VULKAN_BUILD
    u64 startTime = get_current_time_ms();
    
    // Bind culling compute pipeline
    vkCmdBindPipeline(commandBuffer, VK_PIPELINE_BIND_POINT_COMPUTE, system->cullPipeline);
    
    // Bind descriptor sets
    vkCmdBindDescriptorSets(commandBuffer, VK_PIPELINE_BIND_POINT_COMPUTE,
                           system->cullLayout, 0, 1, &system->cullDescriptorSet, 0, NULL);
    
    // Dispatch compute shader
    u32 workGroupCount = (system->meshletCount + 63) / 64; // Assuming 64 work group size
    vkCmdDispatch(commandBuffer, workGroupCount, 1, 1);
    
    // Update statistics
    u64 endTime = get_current_time_ms();
    system->stats.cullingTime = (endTime - startTime) / 1000.0;
    
    return true;
#else
    (void)system;
    (void)renderer;
    (void)commandBuffer;
    return false;
#endif
}

// Utility functions
u64 meshlets_estimate_memory_usage(const MeshletConfig* config, u32 vertexCount, u32 primitiveCount) {
    if (!config) return 0;
    
    u64 memoryUsage = 0;
    
    // Vertex buffer
    memoryUsage += config->maxMeshletsPerMesh * config->maxVerticesPerMeshlet * sizeof(MeshletVertex);
    
    // Index buffer
    memoryUsage += config->maxMeshletsPerMesh * config->maxPrimitivesPerMeshlet * 3 * sizeof(u32);
    
    // Meshlet data buffer
    memoryUsage += config->maxMeshletsPerMesh * sizeof(Meshlet);
    
    // Group buffer
    memoryUsage += MAX_MESHLET_GROUPS * sizeof(MeshletGroup);
    
    // Indirect buffers
    memoryUsage += config->maxMeshletsPerMesh * sizeof(VkDrawMeshTasksIndirectCommandEXT) * 2;
    
    // LOD buffer
    memoryUsage += 8 * sizeof(MeshletLOD);
    
    // Debug buffers
    memoryUsage += 1024 * 1024; // 1MB debug buffer
    
    return memoryUsage;
}

bool meshlets_check_gpu_support(VulkanRenderer* renderer) {
    if (!renderer) return false;
    
    // Check for mesh shader support
    u32 extensionCount = 0;
    vkEnumerateDeviceExtensionProperties(renderer->physicalDevice, NULL, &extensionCount, NULL);
    
    if (extensionCount > 0) {
        VkExtensionProperties* extensions = malloc(sizeof(VkExtensionProperties) * extensionCount);
        vkEnumerateDeviceExtensionProperties(renderer->physicalDevice, NULL, &extensionCount, extensions);
        
        bool hasMeshShaders = false;
        bool hasTaskShaders = false;
        
        for (u32 i = 0; i < extensionCount; i++) {
            if (strcmp(extensions[i].extensionName, "VK_EXT_mesh_shader") == 0) {
                hasMeshShaders = true;
            } else if (strcmp(extensions[i].extensionName, "VK_NV_mesh_shader") == 0) {
                hasTaskShaders = true;
            }
        }
        
        free(extensions);
        
        if (!hasMeshShaders && !hasTaskShaders) {
            LOG_ERROR("GPU does not support mesh shaders");
            return false;
        }
    }
    
    return true;
}

const char* meshlets_get_cull_mode_name(MeshletCullMode mode) {
    switch (mode) {
        case MESHLET_CULL_NONE: return "None";
        case MESHLET_CULL_FRUSTUM: return "Frustum";
        case MESHLET_CULL_OCCLUSION: return "Occlusion";
        case MESHLET_CULL_DISTANCE: return "Distance";
        case MESHLET_CULL_ALL: return "All";
        default: return "Unknown";
    }
}

void meshlets_get_stats(const MeshletSystem* system, MeshletStats* outStats) {
    if (!system || !outStats) return;
    
    *outStats = system->stats;
}

void meshlets_reset_stats(MeshletSystem* system) {
    if (!system) return;
    
    memset(&system->stats, 0, sizeof(MeshletStats));
}

void meshlets_debug_print_stats(const MeshletSystem* system) {
    if (!system) return;
    
    const MeshletStats* stats = &system->stats;
    
    LOG_INFO("=== Meshlet System Statistics ===");
    LOG_INFO("Total Vertices: %llu", stats->totalVertices);
    LOG_INFO("Total Primitives: %llu", stats->totalPrimitives);
    LOG_INFO("Total Meshlets: %llu", stats->totalMeshlets);
    LOG_INFO("Total Groups: %llu", stats->totalGroups);
    LOG_INFO("Culled Meshlets: %llu", stats->culledMeshlets);
    LOG_INFO("Drawn Meshlets: %llu", stats->drawnMeshlets);
    LOG_INFO("Average Vertices per Meshlet: %.1f", stats->averageVerticesPerMeshlet);
    LOG_INFO("Average Primitives per Meshlet: %.1f", stats->averagePrimitivesPerMeshlet);
    LOG_INFO("Culling Time: %.2f ms", stats->cullingTime * 1000.0);
    LOG_INFO("Drawing Time: %.2f ms", stats->drawingTime * 1000.0);
    LOG_INFO("Memory Usage: %.1f MB", stats->memoryUsage / (1024.0 * 1024.0));
}

void meshlets_shutdown(MeshletSystem* system, VulkanRenderer* renderer) {
    if (!system || !renderer) return;
    
#ifdef VULKAN_BUILD
    // Cleanup CPU-side data
    if (system->vertices) free(system->vertices);
    if (system->primitives) free(system->primitives);
    if (system->meshlets) free(system->meshlets);
    if (system->groups) free(system->groups);
    if (system->lods) free(system->lods);
    
    // Cleanup GPU resources
    if (system->vertexBuffer) vulkan_destroy_buffer(renderer, system->vertexBuffer, system->vertexMemory);
    if (system->indexBuffer) vulkan_destroy_buffer(renderer, system->indexBuffer, system->indexMemory);
    if (system->meshletBuffer) vulkan_destroy_buffer(renderer, system->meshletBuffer, system->meshletMemory);
    if (system->meshletGroupBuffer) vulkan_destroy_buffer(renderer, system->meshletGroupBuffer, system->meshletGroupMemory);
    if (system->indirectBuffer) vulkan_destroy_buffer(renderer, system->indirectBuffer, system->indirectMemory);
    if (system->cullIndirectBuffer) vulkan_destroy_buffer(renderer, system->cullIndirectBuffer, system->cullIndirectMemory);
    
    // Cleanup pipelines and layouts
    if (system->meshShaderPipeline) vkDestroyPipeline(renderer->device, system->meshShaderPipeline, NULL);
    if (system->meshShaderLayout) vkDestroyPipelineLayout(renderer->device, system->meshShaderLayout, NULL);
    if (system->meshShaderDescriptorLayout) vkDestroyDescriptorSetLayout(renderer->device, system->meshShaderDescriptorLayout, NULL);
    
    if (system->cullPipeline) vkDestroyPipeline(renderer->device, system->cullPipeline, NULL);
    if (system->cullLayout) vkDestroyPipelineLayout(renderer->device, system->cullLayout, NULL);
    if (system->cullDescriptorLayout) vkDestroyDescriptorSetLayout(renderer->device, system->cullDescriptorLayout, NULL);
    
    memset(system, 0, sizeof(MeshletSystem));
    
    LOG_INFO("Meshlet system shutdown complete");
#endif
}

bool meshlets_is_initialized(const MeshletSystem* system) {
    return system && system->initialized;
}
