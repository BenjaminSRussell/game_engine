// VMA (Vulkan Memory Allocator) Integration Implementation
// Efficient device memory sub-allocation with custom host allocators

#include <core/vma_integration.h>
#include "engine/include/core/memory.h"
#include "engine/include/core/logger.h"

#ifdef VULKAN_BUILD

#define VMA_IMPLEMENTATION
#define VMA_STATIC_VULKAN_FUNCTIONS 0
#define VMA_DYNAMIC_VULKAN_FUNCTIONS 1
#include <vk_mem_alloc.h>

static VmaAllocator g_vma_allocator = NULL;
static VkPhysicalDeviceProperties g_device_props = {0};
static VkPhysicalDeviceMemoryProperties g_memory_props = {0};

bool vma_init(const VMAConfig* config) {
    if (!config) {
        LOG_ERROR("VMA init: invalid config");
        return false;
    }

    VkDevice device = (VkDevice)config->vulkan_device;
    VkPhysicalDevice physical_device = (VkPhysicalDevice)config->vulkan_physical_device;
    VkInstance instance = (VkInstance)config->vulkan_instance;

    // Query device properties for alignment requirements
    vkGetPhysicalDeviceProperties(physical_device, &g_device_props);
    vkGetPhysicalDeviceMemoryProperties(physical_device, &g_memory_props);

    LOG_INFO("VMA: Device limits - bufferImageGranularity: %llu", 
             g_device_props.limits.bufferImageGranularity);
    LOG_INFO("VMA: Device limits - minStorageBufferOffsetAlignment: %llu",
             g_device_props.limits.minStorageBufferOffsetAlignment);

    // Configure VMA
    VmaAllocatorCreateInfo create_info = {0};
    create_info.vulkanApiVersion = VK_API_VERSION_1_3;
    create_info.physicalDevice = physical_device;
    create_info.device = device;
    create_info.instance = instance;
    
    // Use custom allocation callbacks from memory system
    create_info.pAllocationCallbacks = (VkAllocationCallbacks*)config->vulkan_allocation_callbacks;
    
    // Enable buffer device address if supported
    if (config->enable_buffer_device_address) {
        create_info.flags |= VMA_ALLOCATOR_CREATE_BUFFER_DEVICE_ADDRESS_BIT;
        LOG_INFO("VMA: Buffer device address enabled");
    }

    // Create allocator
    VkResult result = vmaCreateAllocator(&create_info, &g_vma_allocator);
    if (result != VK_SUCCESS) {
        LOG_ERROR("VMA: Failed to create allocator (%d)", result);
        return false;
    }

    LOG_INFO("VMA initialized successfully");
    vma_print_stats();
    return true;
}

void vma_shutdown(void) {
    if (g_vma_allocator) {
        vmaDestroyAllocator(g_vma_allocator);
        g_vma_allocator = NULL;
        LOG_INFO("VMA shutdown complete");
    }
}

VmaPool vma_create_voxel_pool(u64 size_bytes) {
    if (!g_vma_allocator) {
        LOG_ERROR("VMA not initialized");
        return NULL;
    }

    // Find device-local memory type
    u32 memory_type_index = UINT32_MAX;
    for (u32 i = 0; i < g_memory_props.memoryTypeCount; i++) {
        if (g_memory_props.memoryTypes[i].propertyFlags & VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT) {
            memory_type_index = i;
            break;
        }
    }

    if (memory_type_index == UINT32_MAX) {
        LOG_ERROR("VMA: No device-local memory type found");
        return NULL;
    }

    // Create custom pool for voxel geometry
    VmaPoolCreateInfo pool_info = {0};
    pool_info.memoryTypeIndex = memory_type_index;
    pool_info.flags = VMA_POOL_CREATE_LINEAR_ALGORITHM_BIT; // Linear allocation for voxels
    pool_info.blockSize = size_bytes;
    pool_info.minBlockCount = 1;
    pool_info.maxBlockCount = 4;

    VmaPool pool = NULL;
    VkResult result = vmaCreatePool(g_vma_allocator, &pool_info, &pool);
    if (result != VK_SUCCESS) {
        LOG_ERROR("VMA: Failed to create voxel pool (%d)", result);
        return NULL;
    }

    LOG_INFO("VMA: Created voxel pool (%llu MB, linear algorithm)", size_bytes / (1024 * 1024));
    return pool;
}

void vma_destroy_pool(VmaPool pool) {
    if (pool) {
        vmaDestroyPool(g_vma_allocator, pool);
    }
}

bool vma_allocate_buffer(void* vk_buffer, VMAAllocationInfo* out_info) {
    if (!g_vma_allocator || !vk_buffer || !out_info) {
        return false;
    }

    VkBuffer buffer = (VkBuffer)vk_buffer;
    VmaAllocationCreateInfo alloc_info = {0};
    alloc_info.usage = VMA_MEMORY_USAGE_AUTO;
    alloc_info.flags = VMA_ALLOCATION_CREATE_HOST_ACCESS_SEQUENTIAL_WRITE_BIT;

    VkResult result = vmaAllocateMemoryForBuffer(g_vma_allocator, buffer, &alloc_info, 
                                                  &out_info->allocation, NULL);
    if (result != VK_SUCCESS) {
        LOG_ERROR("VMA: Failed to allocate buffer memory (%d)", result);
        return false;
    }

    // Bind memory to buffer
    vmaBindBufferMemory(g_vma_allocator, out_info->allocation, buffer);
    
    return true;
}

bool vma_allocate_buffer_in_pool(void* vk_buffer, VmaPool pool, VMAAllocationInfo* out_info) {
    if (!g_vma_allocator || !vk_buffer || !pool || !out_info) {
        return false;
    }

    VkBuffer buffer = (VkBuffer)vk_buffer;
    VmaAllocationCreateInfo alloc_info = {0};
    alloc_info.pool = pool;

    VkResult result = vmaAllocateMemoryForBuffer(g_vma_allocator, buffer, &alloc_info,
                                                  &out_info->allocation, NULL);
    if (result != VK_SUCCESS) {
        LOG_ERROR("VMA: Failed to allocate buffer in pool (%d)", result);
        return false;
    }

    vmaBindBufferMemory(g_vma_allocator, out_info->allocation, buffer);
    return true;
}

void vma_free_buffer(VMAAllocationInfo* info) {
    if (info && info->allocation) {
        if (info->mapped_data) {
            vmaUnmapMemory(g_vma_allocator, info->allocation);
        }
        vmaFreeMemory(g_vma_allocator, info->allocation);
        info->allocation = NULL;
        info->mapped_data = NULL;
    }
}

bool vma_allocate_image(void* vk_image, VMAAllocationInfo* out_info) {
    if (!g_vma_allocator || !vk_image || !out_info) {
        return false;
    }

    VkImage image = (VkImage)vk_image;
    VmaAllocationCreateInfo alloc_info = {0};
    alloc_info.usage = VMA_MEMORY_USAGE_AUTO;

    VkResult result = vmaAllocateMemoryForImage(g_vma_allocator, image, &alloc_info,
                                                 &out_info->allocation, NULL);
    if (result != VK_SUCCESS) {
        LOG_ERROR("VMA: Failed to allocate image memory (%d)", result);
        return false;
    }

    vmaBindImageMemory(g_vma_allocator, out_info->allocation, image);
    return true;
}

void vma_free_image(VMAAllocationInfo* info) {
    if (info && info->allocation) {
        vmaFreeMemory(g_vma_allocator, info->allocation);
        info->allocation = NULL;
    }
}

void* vma_map_memory(VMAAllocationInfo* info) {
    if (!info || !info->allocation) {
        return NULL;
    }

    VkResult result = vmaMapMemory(g_vma_allocator, info->allocation, &info->mapped_data);
    if (result != VK_SUCCESS) {
        LOG_ERROR("VMA: Failed to map memory (%d)", result);
        return NULL;
    }

    return info->mapped_data;
}

void vma_unmap_memory(VMAAllocationInfo* info) {
    if (info && info->allocation && info->mapped_data) {
        vmaUnmapMemory(g_vma_allocator, info->allocation);
        info->mapped_data = NULL;
    }
}

u64 vma_get_buffer_device_address_alignment(void) {
    // Query bufferDeviceAddress alignment from physical device properties
    // This is critical for MoltenVK which may require 256-byte alignment
    return g_device_props.limits.minStorageBufferOffsetAlignment;
}

u64 vma_get_min_buffer_offset_alignment(void) {
    return g_device_props.limits.minStorageBufferOffsetAlignment;
}

void vma_print_stats(void) {
    if (!g_vma_allocator) {
        LOG_INFO("VMA not initialized");
        return;
    }

    VmaStats stats;
    vmaCalculateStats(g_vma_allocator, &stats);

    LOG_INFO("=== VMA Statistics ===");
    LOG_INFO("Total allocations: %u", stats.total.allocationCount);
    LOG_INFO("Total blocks: %u", stats.total.blockCount);
    LOG_INFO("Used bytes: %llu (%.2f MB)", stats.total.usedBytes, 
             stats.total.usedBytes / (1024.0f * 1024.0f));
    LOG_INFO("Unused bytes: %llu (%.2f MB)", stats.total.unusedBytes,
             stats.total.unusedBytes / (1024.0f * 1024.0f));
    
    LOG_INFO("Memory heaps:");
    for (u32 i = 0; i < g_memory_props.memoryHeapCount; i++) {
        if (stats.memoryHeap[i].blockCount > 0) {
            LOG_INFO("  Heap %u: %llu MB (%u blocks, %u allocs)",
                     i,
                     stats.memoryHeap[i].usedBytes / (1024 * 1024),
                     stats.memoryHeap[i].blockCount,
                     stats.memoryHeap[i].allocationCount);
        }
    }
    LOG_INFO("===================");
}

#endif // VULKAN_BUILD
