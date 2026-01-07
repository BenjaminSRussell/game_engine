// src/engine/renderer/raytracing_acceleration_structures.c
// 
// Purpose: Dynamic acceleration structure management for raytracing
// Handles BLAS (Bottom-Level) and TLAS (Top-Level) acceleration structure updates
// for dynamic geometry in Minecraft v2

#include "rendering/ray_tracing.h"
#include "rendering/vulkan.h"
#include "../include/common.h"
#include <stdlib.h>
#include <string.h>

#ifdef VULKAN_BUILD

// Maximum number of dynamic BLAS updates per frame
#define MAX_BLAS_UPDATES_PER_FRAME 256
#define MAX_TLAS_INSTANCES 65536

// BLAS update flags
typedef enum {
    BLAS_UPDATE_NONE = 0,
    BLAS_UPDATE_REBUILD = 1,
    BLAS_UPDATE_REFIT = 2,
    BLAS_UPDATE_COMPACT = 4
} BLASUpdateFlags;

// Dynamic BLAS entry
typedef struct {
    VkAccelerationStructureKHR blas;
    VkBuffer vertex_buffer;
    VkBuffer index_buffer;
    u32 vertex_count;
    u32 index_count;
    u32 last_update_frame;
    BLASUpdateFlags update_flags;
    bool is_dirty;
    char name[64]; // For debugging
} DynamicBLAS;

// TLAS instance data
typedef struct {
    VkAccelerationStructureInstanceKHR instance;
    mat4 transform;
    u32 blas_index;
    u32 material_id;
    bool is_active;
    bool transform_dirty;
} TLASInstance;

// Acceleration structure manager
typedef struct {
    VulkanRenderer* renderer;
    
    // Dynamic BLAS management
    DynamicBLAS* dynamic_blas;
    u32 blas_count;
    u32 blas_capacity;
    
    // TLAS management
    TLASInstance* tlas_instances;
    u32 instance_count;
    u32 instance_capacity;
    
    // Update buffers
    VkBuffer blas_scratch_buffer;
    VkDeviceMemory blas_scratch_memory;
    VkDeviceSize blas_scratch_size;
    
    VkBuffer tlas_instance_buffer;
    VkDeviceMemory tlas_instance_memory;
    VkDeviceSize tlas_instance_size;
    
    // Command buffers for updates
    VkCommandPool update_command_pool;
    VkCommandBuffer update_command_buffer;
    
    // Synchronization
    VkSemaphore update_semaphore;
    VkFence update_fence;
    
    // Performance tracking
    u32 current_frame;
    u32 blas_updates_this_frame;
    u32 tlas_updates_this_frame;
    
    // Function pointers
    PFN_vkGetAccelerationStructureBuildSizesKHR vkGetAccelerationStructureBuildSizesKHR;
    PFN_vkCmdBuildAccelerationStructuresKHR vkCmdBuildAccelerationStructuresKHR;
    PFN_vkBuildAccelerationStructuresKHR vkBuildAccelerationStructuresKHR;
    PFN_vkCmdCopyAccelerationStructureKHR vkCmdCopyAccelerationStructureKHR;
    PFN_vkCmdWriteAccelerationStructuresPropertiesKHR vkCmdWriteAccelerationStructuresPropertiesKHR;
    PFN_vkGetAccelerationStructureDeviceAddressKHR vkGetAccelerationStructureDeviceAddressKHR;
    PFN_vkCreateAccelerationStructureKHR vkCreateAccelerationStructureKHR;
    PFN_vkDestroyAccelerationStructureKHR vkDestroyAccelerationStructureKHR;
    
} AccelerationStructureManager;

static AccelerationStructureManager g_as_manager = {0};

// Internal helper functions
static bool as_manager_create_buffers(void);
static bool as_manager_create_command_resources(void);
static bool as_manager_build_blas(DynamicBLAS* blas, BLASUpdateFlags flags);
static bool as_manager_update_tlas_instances(void);
static void as_manager_compact_blas(DynamicBLAS* blas);

// Initialize acceleration structure manager
bool as_manager_init(VulkanRenderer* renderer) {
    if (!renderer) return false;
    
    g_as_manager.renderer = renderer;
    
    // Load function pointers
    g_as_manager.vkGetAccelerationStructureBuildSizesKHR = 
        (PFN_vkGetAccelerationStructureBuildSizesKHR)vkGetDeviceProcAddr(renderer->device, "vkGetAccelerationStructureBuildSizesKHR");
    g_as_manager.vkCmdBuildAccelerationStructuresKHR = 
        (PFN_vkCmdBuildAccelerationStructuresKHR)vkGetDeviceProcAddr(renderer->device, "vkCmdBuildAccelerationStructuresKHR");
    g_as_manager.vkBuildAccelerationStructuresKHR = 
        (PFN_vkBuildAccelerationStructuresKHR)vkGetDeviceProcAddr(renderer->device, "vkBuildAccelerationStructuresKHR");
    g_as_manager.vkCmdCopyAccelerationStructureKHR = 
        (PFN_vkCmdCopyAccelerationStructureKHR)vkGetDeviceProcAddr(renderer->device, "vkCmdCopyAccelerationStructureKHR");
    g_as_manager.vkCmdWriteAccelerationStructuresPropertiesKHR = 
        (PFN_vkCmdWriteAccelerationStructuresPropertiesKHR)vkGetDeviceProcAddr(renderer->device, "vkCmdWriteAccelerationStructuresPropertiesKHR");
    g_as_manager.vkGetAccelerationStructureDeviceAddressKHR = 
        (PFN_vkGetAccelerationStructureDeviceAddressKHR)vkGetDeviceProcAddr(renderer->device, "vkGetAccelerationStructureDeviceAddressKHR");
    g_as_manager.vkCreateAccelerationStructureKHR = 
        (PFN_vkCreateAccelerationStructureKHR)vkGetDeviceProcAddr(renderer->device, "vkCreateAccelerationStructureKHR");
    g_as_manager.vkDestroyAccelerationStructureKHR = 
        (PFN_vkDestroyAccelerationStructureKHR)vkGetDeviceProcAddr(renderer->device, "vkDestroyAccelerationStructureKHR");
    
    // Check if all functions are loaded
    if (!g_as_manager.vkGetAccelerationStructureBuildSizesKHR || !g_as_manager.vkCmdBuildAccelerationStructuresKHR ||
        !g_as_manager.vkBuildAccelerationStructuresKHR || !g_as_manager.vkCmdCopyAccelerationStructureKHR ||
        !g_as_manager.vkCmdWriteAccelerationStructuresPropertiesKHR || !g_as_manager.vkGetAccelerationStructureDeviceAddressKHR ||
        !g_as_manager.vkCreateAccelerationStructureKHR || !g_as_manager.vkDestroyAccelerationStructureKHR) {
        LOG_ERROR("Failed to load ray tracing acceleration structure functions");
        return false;
    }
    
    // Initialize dynamic BLAS array
    g_as_manager.blas_capacity = 1024;
    g_as_manager.dynamic_blas = malloc(sizeof(DynamicBLAS) * g_as_manager.blas_capacity);
    if (!g_as_manager.dynamic_blas) {
        LOG_ERROR("Failed to allocate dynamic BLAS array");
        return false;
    }
    memset(g_as_manager.dynamic_blas, 0, sizeof(DynamicBLAS) * g_as_manager.blas_capacity);
    
    // Initialize TLAS instance array
    g_as_manager.instance_capacity = MAX_TLAS_INSTANCES;
    g_as_manager.tlas_instances = malloc(sizeof(TLASInstance) * g_as_manager.instance_capacity);
    if (!g_as_manager.tlas_instances) {
        LOG_ERROR("Failed to allocate TLAS instance array");
        return false;
    }
    memset(g_as_manager.tlas_instances, 0, sizeof(TLASInstance) * g_as_manager.instance_capacity);
    
    // Create buffers and command resources
    if (!as_manager_create_buffers()) {
        LOG_ERROR("Failed to create acceleration structure buffers");
        return false;
    }
    
    if (!as_manager_create_command_resources()) {
        LOG_ERROR("Failed to create acceleration structure command resources");
        return false;
    }
    
    LOG_INFO("Acceleration structure manager initialized successfully");
    return true;
}

// Create buffers for acceleration structure operations
static bool as_manager_create_buffers(void) {
    VulkanRenderer* renderer = g_as_manager.renderer;
    
    // Create BLAS scratch buffer
    VkDeviceSize max_scratch_size = 64 * 1024 * 1024; // 64MB
    
    VkBufferCreateInfo scratch_info = {0};
    scratch_info.sType = VK_STRUCTURE_TYPE_BUFFER_CREATE_INFO;
    scratch_info.size = max_scratch_size;
    scratch_info.usage = VK_BUFFER_USAGE_STORAGE_BUFFER_BIT | VK_BUFFER_USAGE_SHADER_DEVICE_ADDRESS_BIT;
    scratch_info.sharingMode = VK_SHARING_MODE_EXCLUSIVE;
    
    if (vkCreateBuffer(renderer->device, &scratch_info, NULL, &g_as_manager.blas_scratch_buffer) != VK_SUCCESS) {
        LOG_ERROR("Failed to create BLAS scratch buffer");
        return false;
    }
    
    // Allocate memory for scratch buffer
    VkMemoryRequirements mem_reqs;
    vkGetBufferMemoryRequirements(renderer->device, g_as_manager.blas_scratch_buffer, &mem_reqs);
    
    VkMemoryAllocateFlagsInfo alloc_flags = {0};
    alloc_flags.sType = VK_STRUCTURE_TYPE_MEMORY_ALLOCATE_FLAGS_INFO;
    alloc_flags.flags = VK_MEMORY_ALLOCATE_DEVICE_ADDRESS_BIT;
    
    VkMemoryAllocateInfo alloc_info = {0};
    alloc_info.sType = VK_STRUCTURE_TYPE_MEMORY_ALLOCATE_INFO;
    alloc_info.pNext = &alloc_flags;
    alloc_info.allocationSize = mem_reqs.size;
    alloc_info.memoryTypeIndex = vulkan_find_memory_type(renderer->physical_device, mem_reqs.memoryTypeBits,
                                                         VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT);
    
    if (vkAllocateMemory(renderer->device, &alloc_info, NULL, &g_as_manager.blas_scratch_memory) != VK_SUCCESS) {
        LOG_ERROR("Failed to allocate BLAS scratch memory");
        return false;
    }
    
    vkBindBufferMemory(renderer->device, g_as_manager.blas_scratch_buffer, g_as_manager.blas_scratch_memory, 0);
    g_as_manager.blas_scratch_size = max_scratch_size;
    
    // Create TLAS instance buffer
    VkDeviceSize instance_buffer_size = sizeof(VkAccelerationStructureInstanceKHR) * MAX_TLAS_INSTANCES;
    
    VkBufferCreateInfo instance_info = {0};
    instance_info.sType = VK_STRUCTURE_TYPE_BUFFER_CREATE_INFO;
    instance_info.size = instance_buffer_size;
    instance_info.usage = VK_BUFFER_USAGE_ACCELERATION_STRUCTURE_BUILD_INPUT_READ_ONLY_BIT_KHR | 
                         VK_BUFFER_USAGE_SHADER_DEVICE_ADDRESS_BIT;
    instance_info.sharingMode = VK_SHARING_MODE_EXCLUSIVE;
    
    if (vkCreateBuffer(renderer->device, &instance_info, NULL, &g_as_manager.tlas_instance_buffer) != VK_SUCCESS) {
        LOG_ERROR("Failed to create TLAS instance buffer");
        return false;
    }
    
    // Allocate memory for instance buffer
    vkGetBufferMemoryRequirements(renderer->device, g_as_manager.tlas_instance_buffer, &mem_reqs);
    
    alloc_info.allocationSize = mem_reqs.size;
    alloc_info.memoryTypeIndex = vulkan_find_memory_type(renderer->physical_device, mem_reqs.memoryTypeBits,
                                                         VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT);
    
    if (vkAllocateMemory(renderer->device, &alloc_info, NULL, &g_as_manager.tlas_instance_memory) != VK_SUCCESS) {
        LOG_ERROR("Failed to allocate TLAS instance memory");
        return false;
    }
    
    vkBindBufferMemory(renderer->device, g_as_manager.tlas_instance_buffer, g_as_manager.tlas_instance_memory, 0);
    g_as_manager.tlas_instance_size = instance_buffer_size;
    
    return true;
}

// Create command resources for acceleration structure updates
static bool as_manager_create_command_resources(void) {
    VulkanRenderer* renderer = g_as_manager.renderer;
    
    // Create command pool for acceleration structure updates
    VkCommandPoolCreateInfo pool_info = {0};
    pool_info.sType = VK_STRUCTURE_TYPE_COMMAND_POOL_CREATE_INFO;
    pool_info.flags = VK_COMMAND_POOL_CREATE_RESET_COMMAND_BUFFER_BIT;
    pool_info.queueFamilyIndex = renderer->graphics_queue_family;
    
    if (vkCreateCommandPool(renderer->device, &pool_info, NULL, &g_as_manager.update_command_pool) != VK_SUCCESS) {
        LOG_ERROR("Failed to create acceleration structure update command pool");
        return false;
    }
    
    // Create command buffer
    VkCommandBufferAllocateInfo cmd_info = {0};
    cmd_info.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_ALLOCATE_INFO;
    cmd_info.commandPool = g_as_manager.update_command_pool;
    cmd_info.level = VK_COMMAND_BUFFER_LEVEL_PRIMARY;
    cmd_info.commandBufferCount = 1;
    
    if (vkAllocateCommandBuffers(renderer->device, &cmd_info, &g_as_manager.update_command_buffer) != VK_SUCCESS) {
        LOG_ERROR("Failed to allocate acceleration structure update command buffer");
        return false;
    }
    
    // Create semaphore for synchronization
    VkSemaphoreCreateInfo semaphore_info = {0};
    semaphore_info.sType = VK_STRUCTURE_TYPE_SEMAPHORE_CREATE_INFO;
    
    if (vkCreateSemaphore(renderer->device, &semaphore_info, NULL, &g_as_manager.update_semaphore) != VK_SUCCESS) {
        LOG_ERROR("Failed to create acceleration structure update semaphore");
        return false;
    }
    
    // Create fence for synchronization
    VkFenceCreateInfo fence_info = {0};
    fence_info.sType = VK_STRUCTURE_TYPE_FENCE_CREATE_INFO;
    fence_info.flags = VK_FENCE_CREATE_SIGNALED_BIT;
    
    if (vkCreateFence(renderer->device, &fence_info, NULL, &g_as_manager.update_fence) != VK_SUCCESS) {
        LOG_ERROR("Failed to create acceleration structure update fence");
        return false;
    }
    
    return true;
}

// Register a dynamic BLAS
u32 as_manager_register_dynamic_blas(VkAccelerationStructureKHR blas, VkBuffer vertex_buffer, 
                                   VkBuffer index_buffer, u32 vertex_count, u32 index_count, const char* name) {
    if (g_as_manager.blas_count >= g_as_manager.blas_capacity) {
        LOG_ERROR("Dynamic BLAS capacity exceeded");
        return UINT32_MAX;
    }
    
    u32 index = g_as_manager.blas_count++;
    DynamicBLAS* dynamic_blas = &g_as_manager.dynamic_blas[index];
    
    dynamic_blas->blas = blas;
    dynamic_blas->vertex_buffer = vertex_buffer;
    dynamic_blas->index_buffer = index_buffer;
    dynamic_blas->vertex_count = vertex_count;
    dynamic_blas->index_count = index_count;
    dynamic_blas->last_update_frame = 0;
    dynamic_blas->update_flags = BLAS_UPDATE_NONE;
    dynamic_blas->is_dirty = false;
    
    if (name) {
        strncpy(dynamic_blas->name, name, sizeof(dynamic_blas->name) - 1);
        dynamic_blas->name[sizeof(dynamic_blas->name) - 1] = '\0';
    } else {
        snprintf(dynamic_blas->name, sizeof(dynamic_blas->name), "BLAS_%u", index);
    }
    
    LOG_INFO("Registered dynamic BLAS: %s", dynamic_blas->name);
    return index;
}

// Mark a BLAS for update
void as_manager_mark_blas_dirty(u32 blas_index, BLASUpdateFlags flags) {
    if (blas_index >= g_as_manager.blas_count) {
        LOG_WARN("Invalid BLAS index: %u", blas_index);
        return;
    }
    
    DynamicBLAS* blas = &g_as_manager.dynamic_blas[blas_index];
    blas->is_dirty = true;
    blas->update_flags |= flags;
    
    LOG_TRACE("Marked BLAS %s as dirty with flags %u", blas->name, flags);
}

// Update a BLAS
static bool as_manager_build_blas(DynamicBLAS* blas, BLASUpdateFlags flags) {
    VulkanRenderer* renderer = g_as_manager.renderer;
    
    // Get build sizes
    VkAccelerationStructureGeometryKHR geometry = {0};
    geometry.sType = VK_STRUCTURE_TYPE_ACCELERATION_STRUCTURE_GEOMETRY_KHR;
    geometry.geometryType = VK_GEOMETRY_TYPE_TRIANGLES_KHR;
    geometry.flags = VK_GEOMETRY_OPAQUE_BIT_KHR;
    geometry.geometry.triangles.sType = VK_STRUCTURE_TYPE_ACCELERATION_STRUCTURE_GEOMETRY_TRIANGLES_DATA_KHR;
    geometry.geometry.triangles.vertexFormat = VK_FORMAT_R32G32B32_SFLOAT;
    geometry.geometry.triangles.vertexStride = sizeof(Vertex);
    geometry.geometry.triangles.maxVertex = blas->vertex_count - 1;
    geometry.geometry.triangles.indexType = VK_INDEX_TYPE_UINT32;
    geometry.geometry.triangles.vertexData.deviceAddress = vulkan_get_buffer_device_address(renderer->device, blas->vertex_buffer);
    geometry.geometry.triangles.indexData.deviceAddress = vulkan_get_buffer_device_address(renderer->device, blas->index_buffer);
    
    VkAccelerationStructureBuildGeometryInfoKHR build_info = {0};
    build_info.sType = VK_STRUCTURE_TYPE_ACCELERATION_STRUCTURE_BUILD_GEOMETRY_INFO_KHR;
    build_info.type = VK_ACCELERATION_STRUCTURE_TYPE_BOTTOM_LEVEL_KHR;
    build_info.flags = VK_BUILD_ACCELERATION_STRUCTURE_PREFER_FAST_TRACE_BIT_KHR;
    build_info.mode = (flags & BLAS_UPDATE_REFIT) ? VK_BUILD_ACCELERATION_STRUCTURE_MODE_UPDATE_KHR : 
                                                    VK_BUILD_ACCELERATION_STRUCTURE_MODE_BUILD_KHR;
    build_info.geometryCount = 1;
    build_info.pGeometries = &geometry;
    build_info.scratchData.deviceAddress = vulkan_get_buffer_device_address(renderer->device, g_as_manager.blas_scratch_buffer);
    
    u32 primitive_count = blas->index_count / 3;
    
    VkAccelerationStructureBuildSizesInfoKHR size_info = {0};
    size_info.sType = VK_STRUCTURE_TYPE_ACCELERATION_STRUCTURE_BUILD_SIZES_INFO_KHR;
    
    g_as_manager.vkGetAccelerationStructureBuildSizesKHR(
        renderer->device,
        VK_ACCELERATION_STRUCTURE_BUILD_TYPE_DEVICE_KHR,
        &build_info,
        &primitive_count,
        &size_info
    );
    
    // Check if scratch buffer is large enough
    if (size_info.buildScratchSize > g_as_manager.blas_scratch_size) {
        LOG_ERROR("BLAS scratch buffer too small for %s", blas->name);
        return false;
    }
    
    // Record build command
    VkCommandBufferBeginInfo begin_info = {0};
    begin_info.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO;
    begin_info.flags = VK_COMMAND_BUFFER_USAGE_ONE_TIME_SUBMIT_BIT;
    
    if (vkBeginCommandBuffer(g_as_manager.update_command_buffer, &begin_info) != VK_SUCCESS) {
        LOG_ERROR("Failed to begin BLAS build command buffer");
        return false;
    }
    
    VkAccelerationStructureBuildRangeInfoKHR range_info = {0};
    range_info.primitiveCount = primitive_count;
    range_info.primitiveOffset = 0;
    range_info.firstVertex = 0;
    range_info.transformOffset = 0;
    
    const VkAccelerationStructureBuildRangeInfoKHR* range_infos = &range_info;
    
    g_as_manager.vkCmdBuildAccelerationStructuresKHR(
        g_as_manager.update_command_buffer,
        1,
        &build_info,
        &range_infos
    );
    
    if (vkEndCommandBuffer(g_as_manager.update_command_buffer) != VK_SUCCESS) {
        LOG_ERROR("Failed to end BLAS build command buffer");
        return false;
    }
    
    // Submit command
    VkSubmitInfo submit_info = {0};
    submit_info.sType = VK_STRUCTURE_TYPE_SUBMIT_INFO;
    submit_info.commandBufferCount = 1;
    submit_info.pCommandBuffers = &g_as_manager.update_command_buffer;
    
    if (vkQueueSubmit(renderer->graphics_queue, 1, &submit_info, VK_NULL_HANDLE) != VK_SUCCESS) {
        LOG_ERROR("Failed to submit BLAS build command");
        return false;
    }
    
    vkQueueWaitIdle(renderer->graphics_queue);
    
    blas->is_dirty = false;
    blas->update_flags = BLAS_UPDATE_NONE;
    blas->last_update_frame = g_as_manager.current_frame;
    
    LOG_TRACE("Updated BLAS %s", blas->name);
    return true;
}

// Update all dirty BLAS
void as_manager_update_dirty_blas(void) {
    g_as_manager.blas_updates_this_frame = 0;
    
    for (u32 i = 0; i < g_as_manager.blas_count; ++i) {
        DynamicBLAS* blas = &g_as_manager.dynamic_blas[i];
        
        if (blas->is_dirty && g_as_manager.blas_updates_this_frame < MAX_BLAS_UPDATES_PER_FRAME) {
            if (as_manager_build_blas(blas, blas->update_flags)) {
                g_as_manager.blas_updates_this_frame++;
            } else {
                LOG_ERROR("Failed to update BLAS %s", blas->name);
            }
        }
    }
    
    if (g_as_manager.blas_updates_this_frame > 0) {
        LOG_TRACE("Updated %u BLAS this frame", g_as_manager.blas_updates_this_frame);
    }
}

// Register a TLAS instance
u32 as_manager_register_tlas_instance(u32 blas_index, const mat4* transform, u32 material_id) {
    if (blas_index >= g_as_manager.blas_count) {
        LOG_ERROR("Invalid BLAS index for TLAS instance: %u", blas_index);
        return UINT32_MAX;
    }
    
    if (g_as_manager.instance_count >= g_as_manager.instance_capacity) {
        LOG_ERROR("TLAS instance capacity exceeded");
        return UINT32_MAX;
    }
    
    u32 index = g_as_manager.instance_count++;
    TLASInstance* instance = &g_as_manager.tlas_instances[index];
    
    // Initialize instance data
    VkAccelerationStructureInstanceKHR* vk_instance = &instance->instance;
    memset(vk_instance, 0, sizeof(VkAccelerationStructureInstanceKHR));
    
    // Set transform (column-major)
    if (transform) {
        memcpy(vk_instance->transform, transform->data, sizeof(mat4));
    } else {
        // Identity matrix
        vk_instance->transform[0][0] = 1.0f;
        vk_instance->transform[1][1] = 1.0f;
        vk_instance->transform[2][2] = 1.0f;
        vk_instance->transform[3][3] = 1.0f;
    }
    
    // Set instance properties
    vk_instance->instanceCustomIndex : 24 = material_id;
    vk_instance->mask : 8 = 0xFF;
    vk_instance->instanceShaderBindingTableRecordOffset : 24 = 0;
    vk_instance->flags : 8 = VK_GEOMETRY_INSTANCE_TRIANGLE_FACING_CULL_DISABLE_BIT_KHR;
    vk_instance->accelerationStructureReference = g_as_manager.vkGetAccelerationStructureDeviceAddressKHR(
        g_as_manager.renderer->device, 
        (VkAccelerationStructureKHR){.handle = g_as_manager.dynamic_blas[blas_index].blas}
    );
    
    instance->blas_index = blas_index;
    instance->material_id = material_id;
    instance->is_active = true;
    instance->transform_dirty = false;
    
    if (transform) {
        instance->transform = *transform;
    } else {
        mat4_identity(&instance->transform);
    }
    
    return index;
}

// Update TLAS instance transform
void as_manager_update_instance_transform(u32 instance_index, const mat4* transform) {
    if (instance_index >= g_as_manager.instance_count) {
        LOG_WARN("Invalid TLAS instance index: %u", instance_index);
        return;
    }
    
    TLASInstance* instance = &g_as_manager.tlas_instances[instance_index];
    
    if (transform) {
        instance->transform = *transform;
        memcpy(instance->instance.transform, transform->data, sizeof(mat4));
    } else {
        mat4_identity(&instance->transform);
        // Reset to identity
        memset(instance->instance.transform, 0, sizeof(VkTransformMatrixKHR));
        instance->instance.transform[0][0] = 1.0f;
        instance->instance.transform[1][1] = 1.0f;
        instance->instance.transform[2][2] = 1.0f;
        instance->instance.transform[3][3] = 1.0f;
    }
    
    instance->transform_dirty = true;
}

// Update TLAS instances buffer
static bool as_manager_update_tlas_instances(void) {
    if (g_as_manager.instance_count == 0) {
        return true;
    }
    
    // Map instance buffer
    void* mapped_data;
    if (vkMapMemory(g_as_manager.renderer->device, g_as_manager.tlas_instance_memory, 0, 
                    g_as_manager.instance_count * sizeof(VkAccelerationStructureInstanceKHR), 0, &mapped_data) != VK_SUCCESS) {
        LOG_ERROR("Failed to map TLAS instance buffer");
        return false;
    }
    
    // Copy instance data
    VkAccelerationStructureInstanceKHR* instances = (VkAccelerationStructureInstanceKHR*)mapped_data;
    for (u32 i = 0; i < g_as_manager.instance_count; ++i) {
        if (g_as_manager.tlas_instances[i].is_active) {
            instances[i] = g_as_manager.tlas_instances[i].instance;
        }
    }
    
    vkUnmapMemory(g_as_manager.renderer->device, g_as_manager.tlas_instance_memory);
    
    g_as_manager.tlas_updates_this_frame++;
    return true;
}

// Build TLAS
bool as_manager_build_tlas(VkAccelerationStructureKHR tlas) {
    if (!as_manager_update_tlas_instances()) {
        LOG_ERROR("Failed to update TLAS instances");
        return false;
    }
    
    VulkanRenderer* renderer = g_as_manager.renderer;
    
    // Get TLAS build sizes
    VkAccelerationStructureGeometryKHR geometry = {0};
    geometry.sType = VK_STRUCTURE_TYPE_ACCELERATION_STRUCTURE_GEOMETRY_KHR;
    geometry.geometryType = VK_GEOMETRY_TYPE_INSTANCES_KHR;
    geometry.geometry.instances.sType = VK_STRUCTURE_TYPE_ACCELERATION_STRUCTURE_GEOMETRY_INSTANCES_DATA_KHR;
    geometry.geometry.instances.arrayOfPointers = VK_FALSE;
    geometry.geometry.instances.data.deviceAddress = vulkan_get_buffer_device_address(renderer->device, g_as_manager.tlas_instance_buffer);
    
    VkAccelerationStructureBuildGeometryInfoKHR build_info = {0};
    build_info.sType = VK_STRUCTURE_TYPE_ACCELERATION_STRUCTURE_BUILD_GEOMETRY_INFO_KHR;
    build_info.type = VK_ACCELERATION_STRUCTURE_TYPE_TOP_LEVEL_KHR;
    build_info.flags = VK_BUILD_ACCELERATION_STRUCTURE_PREFER_FAST_TRACE_BIT_KHR;
    build_info.mode = VK_BUILD_ACCELERATION_STRUCTURE_MODE_BUILD_KHR;
    build_info.geometryCount = 1;
    build_info.pGeometries = &geometry;
    build_info.scratchData.deviceAddress = vulkan_get_buffer_device_address(renderer->device, g_as_manager.blas_scratch_buffer);
    
    VkAccelerationStructureBuildSizesInfoKHR size_info = {0};
    size_info.sType = VK_STRUCTURE_TYPE_ACCELERATION_STRUCTURE_BUILD_SIZES_INFO_KHR;
    
    g_as_manager.vkGetAccelerationStructureBuildSizesKHR(
        renderer->device,
        VK_ACCELERATION_STRUCTURE_BUILD_TYPE_DEVICE_KHR,
        &build_info,
        &g_as_manager.instance_count,
        &size_info
    );
    
    // Record build command
    VkCommandBufferBeginInfo begin_info = {0};
    begin_info.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO;
    begin_info.flags = VK_COMMAND_BUFFER_USAGE_ONE_TIME_SUBMIT_BIT;
    
    if (vkBeginCommandBuffer(g_as_manager.update_command_buffer, &begin_info) != VK_SUCCESS) {
        LOG_ERROR("Failed to begin TLAS build command buffer");
        return false;
    }
    
    VkAccelerationStructureBuildRangeInfoKHR range_info = {0};
    range_info.primitiveCount = g_as_manager.instance_count;
    range_info.primitiveOffset = 0;
    range_info.firstVertex = 0;
    range_info.transformOffset = 0;
    
    const VkAccelerationStructureBuildRangeInfoKHR* range_infos = &range_info;
    
    g_as_manager.vkCmdBuildAccelerationStructuresKHR(
        g_as_manager.update_command_buffer,
        1,
        &build_info,
        &range_infos
    );
    
    if (vkEndCommandBuffer(g_as_manager.update_command_buffer) != VK_SUCCESS) {
        LOG_ERROR("Failed to end TLAS build command buffer");
        return false;
    }
    
    // Submit command
    VkSubmitInfo submit_info = {0};
    submit_info.sType = VK_STRUCTURE_TYPE_SUBMIT_INFO;
    submit_info.commandBufferCount = 1;
    submit_info.pCommandBuffers = &g_as_manager.update_command_buffer;
    
    if (vkQueueSubmit(renderer->graphics_queue, 1, &submit_info, VK_NULL_HANDLE) != VK_SUCCESS) {
        LOG_ERROR("Failed to submit TLAS build command");
        return false;
    }
    
    vkQueueWaitIdle(renderer->graphics_queue);
    
    LOG_TRACE("Built TLAS with %u instances", g_as_manager.instance_count);
    return true;
}

// Begin frame
void as_manager_begin_frame(u32 frame_index) {
    g_as_manager.current_frame = frame_index;
    g_as_manager.blas_updates_this_frame = 0;
    g_as_manager.tlas_updates_this_frame = 0;
}

// End frame
void as_manager_end_frame(void) {
    // Reset update counters
    g_as_manager.blas_updates_this_frame = 0;
    g_as_manager.tlas_updates_this_frame = 0;
}

// Get performance statistics
void as_manager_get_stats(u32* blas_updates, u32* tlas_updates, u32* active_blas, u32* active_instances) {
    if (blas_updates) *blas_updates = g_as_manager.blas_updates_this_frame;
    if (tlas_updates) *tlas_updates = g_as_manager.tlas_updates_this_frame;
    if (active_blas) *active_blas = g_as_manager.blas_count;
    if (active_instances) *active_instances = g_as_manager.instance_count;
}

// Cleanup acceleration structure manager
void as_manager_cleanup(void) {
    VulkanRenderer* renderer = g_as_manager.renderer;
    if (!renderer) return;
    
    vkDeviceWaitIdle(renderer->device);
    
    // Cleanup command resources
    if (g_as_manager.update_fence) {
        vkDestroyFence(renderer->device, g_as_manager.update_fence, NULL);
    }
    if (g_as_manager.update_semaphore) {
        vkDestroySemaphore(renderer->device, g_as_manager.update_semaphore, NULL);
    }
    if (g_as_manager.update_command_buffer) {
        vkFreeCommandBuffers(renderer->device, g_as_manager.update_command_pool, 1, &g_as_manager.update_command_buffer);
    }
    if (g_as_manager.update_command_pool) {
        vkDestroyCommandPool(renderer->device, g_as_manager.update_command_pool, NULL);
    }
    
    // Cleanup buffers
    if (g_as_manager.tlas_instance_memory) {
        vkFreeMemory(renderer->device, g_as_manager.tlas_instance_memory, NULL);
    }
    if (g_as_manager.tlas_instance_buffer) {
        vkDestroyBuffer(renderer->device, g_as_manager.tlas_instance_buffer, NULL);
    }
    if (g_as_manager.blas_scratch_memory) {
        vkFreeMemory(renderer->device, g_as_manager.blas_scratch_memory, NULL);
    }
    if (g_as_manager.blas_scratch_buffer) {
        vkDestroyBuffer(renderer->device, g_as_manager.blas_scratch_buffer, NULL);
    }
    
    // Cleanup arrays
    free(g_as_manager.dynamic_blas);
    free(g_as_manager.tlas_instances);
    
    memset(&g_as_manager, 0, sizeof(AccelerationStructureManager));
    LOG_INFO("Acceleration structure manager cleaned up");
}

#endif // VULKAN_BUILD
