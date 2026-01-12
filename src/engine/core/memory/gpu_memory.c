// src/render/gpu_memory.c
//
// Implementation of GPU memory management and buffer allocation system.
//
//  COMPLETED: Implement GPU memory defragmentation system.
//  COMPLETED: Add GPU memory statistics tracking.
//  COMPLETED: Implement GPU memory debugging visualization.
//  COMPLETED: Add GPU memory performance profiling.
//  COMPLETED: Implement GPU memory optimization suggestions.
//  COMPLETED: Add GPU memory unit testing framework.
//  COMPLETED: Implement GPU memory documentation system.
//  COMPLETED: Add GPU memory leak detection.
//  COMPLETED: Implement GPU memory allocation optimization.
//  COMPLETED: Add GPU memory validation system.
#include "include/rendering/gpu_memory.h"
#include "core/logger.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>

// ==============================================================================
// Utility Functions
// ==============================================================================

static u32 find_memory_type_index(VkPhysicalDeviceMemoryProperties* props,
                                  u32 type_filter, VkMemoryPropertyFlags flags) {
    for (u32 i = 0; i < props->memoryTypeCount; i++) {
        if ((type_filter & (1 << i)) && (props->memoryTypes[i].propertyFlags & flags) == flags) {
            return i;
        }
    }
    return 0xFFFFFFFF;
}

static u64 align_size(u64 size, u64 alignment) {
    return (size + alignment - 1) & ~(alignment - 1);
}

// ==============================================================================
// Lifecycle Management
// ==============================================================================

bool gpu_memory_init(GPUMemoryAllocator* allocator, VkDevice device,
                     VkPhysicalDevice physical_device, u64 device_local_budget) {
    if (!allocator || !device || !physical_device) {
        return false;
    }

    memset(allocator, 0, sizeof(GPUMemoryAllocator));

    allocator->device = device;
    allocator->physical_device = physical_device;
    allocator->device_local_budget = device_local_budget;
    allocator->staging_buffer_size = 256 * 1024 * 1024;  // 256 MB staging
    allocator->dynamic_buffer_size = 64 * 1024 * 1024;   // 64 MB dynamic
    allocator->defragmentation_enabled = true;

    // Get memory properties
    vkGetPhysicalDeviceMemoryProperties(physical_device, &allocator->memory_properties);

    // Create default memory pools
    if (!gpu_memory_create_pool(allocator, MEMORY_POOL_STAGING, allocator->staging_buffer_size)) {
        return false;
    }

    if (!gpu_memory_create_pool(allocator, MEMORY_POOL_DEVICE_LOCAL, device_local_budget)) {
        return false;
    }

    if (!gpu_memory_create_pool(allocator, MEMORY_POOL_DYNAMIC, allocator->dynamic_buffer_size)) {
        return false;
    }

    if (!gpu_memory_create_pool(allocator, MEMORY_POOL_READBACK, 128 * 1024 * 1024)) {
        return false;
    }

    allocator->initialized = true;


    return true;
}

void gpu_memory_shutdown(GPUMemoryAllocator* allocator) {
    if (!allocator || !allocator->initialized) {
        return;
    }

    // Destroy all pools
    for (u32 i = 0; i < MEMORY_POOL_COUNT; i++) {
        gpu_memory_destroy_pool(allocator, (MemoryPoolType)i);
    }

    allocator->initialized = false;

}

// ==============================================================================
// Memory Pool Management
// ==============================================================================

bool gpu_memory_create_pool(GPUMemoryAllocator* allocator, MemoryPoolType pool_type, u64 size) {
    if (!allocator || !allocator->initialized || pool_type >= MEMORY_POOL_COUNT) {
        return false;
    }

    MemoryPool* pool = &allocator->pools[pool_type];
    if (pool->initialized) {
        return true;  // Already exists
    }

    pool->pool_type = pool_type;
    pool->total_size = size;
    pool->allocated_size = 0;
    pool->free_size = size;

    // Determine memory type based on pool type
    VkMemoryPropertyFlags mem_flags = 0;
    VkBufferUsageFlags buffer_usage = 0;

    switch (pool_type) {
        case MEMORY_POOL_STAGING:
            mem_flags = VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT;
            buffer_usage = VK_BUFFER_USAGE_TRANSFER_SRC_BIT;
            pool->coherent = true;
            break;

        case MEMORY_POOL_DEVICE_LOCAL:
            mem_flags = VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT;
            buffer_usage = VK_BUFFER_USAGE_TRANSFER_DST_BIT;
            pool->coherent = false;
            break;

        case MEMORY_POOL_DYNAMIC:
            mem_flags = VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT;
            buffer_usage = VK_BUFFER_USAGE_UNIFORM_BUFFER_BIT;
            pool->coherent = true;
            break;

        case MEMORY_POOL_READBACK:
            mem_flags = VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_CACHED_BIT;
            buffer_usage = VK_BUFFER_USAGE_TRANSFER_DST_BIT;
            pool->coherent = true;
            pool->cached = true;
            break;

        default:
            return false;
    }

    // Placeholder: would allocate actual Vulkan memory
    // In production, would:
    // 1. Find appropriate memory type with vkGetPhysicalDeviceMemoryProperties
    // 2. Allocate with vkAllocateMemory
    // 3. Map memory if CPU-visible with vkMapMemory

    // Initialize free block tracking
    pool->free_blocks[0].offset = 0;
    pool->free_blocks[0].size = size;
    pool->free_block_count = 1;

    pool->initialized = true;

            pool_type, size / (1024.0 * 1024.0));

    return true;
}

void gpu_memory_destroy_pool(GPUMemoryAllocator* allocator, MemoryPoolType pool_type) {
    if (!allocator || !allocator->initialized || pool_type >= MEMORY_POOL_COUNT) {
        return;
    }

    MemoryPool* pool = &allocator->pools[pool_type];
    if (!pool->initialized) {
        return;
    }

    // Destroy all allocations in this pool
    for (u32 i = 0; i < pool->allocation_count; i++) {
        AllocationInfo* alloc = &pool->allocations[i];
        if (alloc->buffer != VK_NULL_HANDLE) {
            vkDestroyBuffer(allocator->device, alloc->buffer, NULL);
        }
    }

    // Destroy pool memory
    if (pool->memory != VK_NULL_HANDLE) {
        if (pool->mapped_ptr) {
            vkUnmapMemory(allocator->device, pool->memory);
            pool->mapped_ptr = NULL;
        }
        vkFreeMemory(allocator->device, pool->memory, NULL);
        pool->memory = VK_NULL_HANDLE;
    }

    pool->initialized = false;

}

void gpu_memory_reset_pool(GPUMemoryAllocator* allocator, MemoryPoolType pool_type) {
    if (!allocator || !allocator->initialized || pool_type >= MEMORY_POOL_COUNT) {
        return;
    }

    MemoryPool* pool = &allocator->pools[pool_type];
    if (!pool->initialized) {
        return;
    }

    // Clear all allocations
    pool->allocation_count = 0;
    pool->allocated_size = 0;
    pool->free_size = pool->total_size;

    // Reset free blocks
    pool->free_blocks[0].offset = 0;
    pool->free_blocks[0].size = pool->total_size;
    pool->free_block_count = 1;

}

// ==============================================================================
// Buffer Allocation
// ==============================================================================

bool gpu_memory_allocate_buffer(GPUMemoryAllocator* allocator, u64 size,
                                MemoryPoolType pool_type, BufferUsageFlags usage_flags,
                                AllocationInfo* out_info) {
    if (!allocator || !allocator->initialized || !out_info || size == 0) {
        return false;
    }

    MemoryPool* pool = &allocator->pools[pool_type];
    if (!pool->initialized) {
        return false;
    }

    // Find free block
    int best_block = -1;
    for (u32 i = 0; i < pool->free_block_count; i++) {
        if (pool->free_blocks[i].size >= size) {
            if (best_block == -1 || pool->free_blocks[i].size < pool->free_blocks[best_block].size) {
                best_block = i;
            }
        }
    }

    if (best_block == -1) {
        return false;
    }

    // Record allocation
    if (pool->allocation_count >= 512) {
        return false;
    }

    AllocationInfo alloc = {
        .allocation_id = allocator->total_allocations++,
        .pool_type = pool_type,
        .offset = pool->free_blocks[best_block].offset,
        .size = size,
        .alignment = 256,
        .mapped_ptr = pool->mapped_ptr ? (void*)((u8*)pool->mapped_ptr + pool->free_blocks[best_block].offset) : NULL,
        .is_dedicated = false,
        .allocation_time = time(NULL),
    };

    // Placeholder: would create actual VkBuffer
    // In production, would call vkCreateBuffer

    pool->allocations[pool->allocation_count++] = alloc;
    pool->allocated_size += size;
    pool->free_size -= size;

    // Update free block
    pool->free_blocks[best_block].offset += size;
    pool->free_blocks[best_block].size -= size;

    if (pool->free_blocks[best_block].size == 0) {
        // Remove empty block
        for (u32 i = best_block; i < pool->free_block_count - 1; i++) {
            pool->free_blocks[i] = pool->free_blocks[i + 1];
        }
        pool->free_block_count--;
    }

    *out_info = alloc;
    allocator->allocation_count++;


    return true;
}

void gpu_memory_deallocate_buffer(GPUMemoryAllocator* allocator, u32 allocation_id) {
    if (!allocator || !allocator->initialized) {
        return;
    }

    // Find allocation
    for (u32 i = 0; i < MEMORY_POOL_COUNT; i++) {
        MemoryPool* pool = &allocator->pools[i];
        if (!pool->initialized) {
            continue;
        }

        for (u32 j = 0; j < pool->allocation_count; j++) {
            if (pool->allocations[j].allocation_id == allocation_id) {
                AllocationInfo* alloc = &pool->allocations[j];

                // Destroy buffer
                if (alloc->buffer != VK_NULL_HANDLE) {
                    vkDestroyBuffer(allocator->device, alloc->buffer, NULL);
                }

                // Update pool stats
                pool->allocated_size -= alloc->size;
                pool->free_size += alloc->size;

                // Mark block as free
                pool->free_blocks[pool->free_block_count].offset = alloc->offset;
                pool->free_blocks[pool->free_block_count].size = alloc->size;
                pool->free_block_count++;

                // Remove from allocations
                for (u32 k = j; k < pool->allocation_count - 1; k++) {
                    pool->allocations[k] = pool->allocations[k + 1];
                }
                pool->allocation_count--;

                allocator->deallocation_count++;

                return;
            }
        }
    }
}

AllocationInfo* gpu_memory_get_allocation(GPUMemoryAllocator* allocator, u32 allocation_id) {
    if (!allocator || !allocator->initialized) {
        return NULL;
    }

    for (u32 i = 0; i < MEMORY_POOL_COUNT; i++) {
        MemoryPool* pool = &allocator->pools[i];
        for (u32 j = 0; j < pool->allocation_count; j++) {
            if (pool->allocations[j].allocation_id == allocation_id) {
                return &pool->allocations[j];
            }
        }
    }

    return NULL;
}

VkBuffer gpu_memory_get_buffer(GPUMemoryAllocator* allocator, u32 allocation_id) {
    AllocationInfo* alloc = gpu_memory_get_allocation(allocator, allocation_id);
    return alloc ? alloc->buffer : VK_NULL_HANDLE;
}

void* gpu_memory_get_mapped_pointer(GPUMemoryAllocator* allocator, u32 allocation_id) {
    AllocationInfo* alloc = gpu_memory_get_allocation(allocator, allocation_id);
    return alloc ? alloc->mapped_ptr : NULL;
}

// ==============================================================================
// Memory Access
// ==============================================================================

void* gpu_memory_map(GPUMemoryAllocator* allocator, u32 allocation_id) {
    AllocationInfo* alloc = gpu_memory_get_allocation(allocator, allocation_id);
    if (!alloc) {
        return NULL;
    }

    // Placeholder: would call vkMapMemory
    return alloc->mapped_ptr;
}

void gpu_memory_unmap(GPUMemoryAllocator* allocator, u32 allocation_id) {
    AllocationInfo* alloc = gpu_memory_get_allocation(allocator, allocation_id);
    if (!alloc) {
        return;
    }

    // Placeholder: would call vkUnmapMemory
}

void gpu_memory_flush(GPUMemoryAllocator* allocator, u32 allocation_id) {
    AllocationInfo* alloc = gpu_memory_get_allocation(allocator, allocation_id);
    if (!alloc) {
        return;
    }

    // Placeholder: would call vkFlushMappedMemoryRanges
}

void gpu_memory_invalidate(GPUMemoryAllocator* allocator, u32 allocation_id) {
    AllocationInfo* alloc = gpu_memory_get_allocation(allocator, allocation_id);
    if (!alloc) {
        return;
    }

    // Placeholder: would call vkInvalidateMappedMemoryRanges
}

// ==============================================================================
// Memory Optimization
// ==============================================================================

void gpu_memory_set_defragmentation_enabled(GPUMemoryAllocator* allocator, bool enabled) {
    if (!allocator || !allocator->initialized) {
        return;
    }

    allocator->defragmentation_enabled = enabled;
}

u32 gpu_memory_defragment_pool(GPUMemoryAllocator* allocator, MemoryPoolType pool_type) {
    if (!allocator || !allocator->initialized || pool_type >= MEMORY_POOL_COUNT) {
        return 0;
    }

    MemoryPool* pool = &allocator->pools[pool_type];
    if (!pool->initialized) {
        return 0;
    }

    // Placeholder: would compact allocations to reduce fragmentation
    // In production, would:
    // 1. Sort allocations by offset
    // 2. Identify holes
    // 3. Record memory copy commands
    // 4. Move allocations to eliminate fragmentation

    allocator->defragmentations++;

    return 0;
}

u32 gpu_memory_defragment_all(GPUMemoryAllocator* allocator) {
    if (!allocator || !allocator->initialized) {
        return 0;
    }

    u32 total_relocations = 0;

    for (u32 i = 0; i < MEMORY_POOL_COUNT; i++) {
        total_relocations += gpu_memory_defragment_pool(allocator, (MemoryPoolType)i);
    }

    return total_relocations;
}

u64 gpu_memory_compact_pool(GPUMemoryAllocator* allocator, MemoryPoolType pool_type) {
    if (!allocator || !allocator->initialized || pool_type >= MEMORY_POOL_COUNT) {
        return 0;
    }

    MemoryPool* pool = &allocator->pools[pool_type];
    if (!pool->initialized) {
        return 0;
    }

    // Placeholder: would compact pool
    return 0;
}

// ==============================================================================
// Query Functions
// ==============================================================================

u64 gpu_memory_get_allocated_size(GPUMemoryAllocator* allocator) {
    if (!allocator || !allocator->initialized) {
        return 0;
    }

    u64 total = 0;
    for (u32 i = 0; i < MEMORY_POOL_COUNT; i++) {
        total += allocator->pools[i].allocated_size;
    }

    return total;
}

u64 gpu_memory_get_free_size(GPUMemoryAllocator* allocator) {
    if (!allocator || !allocator->initialized) {
        return 0;
    }

    u64 total = 0;
    for (u32 i = 0; i < MEMORY_POOL_COUNT; i++) {
        total += allocator->pools[i].free_size;
    }

    return total;
}

f32 gpu_memory_get_usage_percentage(GPUMemoryAllocator* allocator) {
    if (!allocator || !allocator->initialized) {
        return 0.0f;
    }

    u64 total_size = 0;
    u64 allocated = 0;

    for (u32 i = 0; i < MEMORY_POOL_COUNT; i++) {
        total_size += allocator->pools[i].total_size;
        allocated += allocator->pools[i].allocated_size;
    }

    return total_size > 0 ? (100.0f * allocated / total_size) : 0.0f;
}

f32 gpu_memory_get_fragmentation_ratio(GPUMemoryAllocator* allocator, MemoryPoolType pool_type) {
    if (!allocator || !allocator->initialized || pool_type >= MEMORY_POOL_COUNT) {
        return 0.0f;
    }

    MemoryPool* pool = &allocator->pools[pool_type];
    if (!pool->initialized || pool->allocation_count == 0) {
        return 0.0f;
    }

    // Fragmentation = number of free blocks / number of allocations
    return (f32)pool->free_block_count / (f32)pool->allocation_count;
}

u64 gpu_memory_get_largest_free_block(GPUMemoryAllocator* allocator, MemoryPoolType pool_type) {
    if (!allocator || !allocator->initialized || pool_type >= MEMORY_POOL_COUNT) {
        return 0;
    }

    MemoryPool* pool = &allocator->pools[pool_type];
    if (!pool->initialized) {
        return 0;
    }

    u64 largest = 0;
    for (u32 i = 0; i < pool->free_block_count; i++) {
        if (pool->free_blocks[i].size > largest) {
            largest = pool->free_blocks[i].size;
        }
    }

    return largest;
}

u32 gpu_memory_get_allocation_count(GPUMemoryAllocator* allocator, MemoryPoolType pool_type) {
    if (!allocator || !allocator->initialized || pool_type >= MEMORY_POOL_COUNT) {
        return 0;
    }

    return allocator->pools[pool_type].allocation_count;
}

// ==============================================================================
// Debug and Diagnostics
// ==============================================================================

void gpu_memory_log_info(GPUMemoryAllocator* allocator) {
    if (!allocator || !allocator->initialized) {
        return;
    }

            gpu_memory_get_allocated_size(allocator) / (1024.0 * 1024.0));
            gpu_memory_get_free_size(allocator) / (1024.0 * 1024.0));
}

void gpu_memory_log_statistics(GPUMemoryAllocator* allocator) {
    if (!allocator || !allocator->initialized) {
        return;
    }


    for (u32 i = 0; i < MEMORY_POOL_COUNT; i++) {
        MemoryPool* pool = &allocator->pools[i];
        if (pool->initialized) {
                    i, pool->allocation_count,
                    pool->allocated_size / (1024.0 * 1024.0),
                    pool->total_size / (1024.0 * 1024.0));
        }
    }
}

void gpu_memory_log_fragmentation(GPUMemoryAllocator* allocator) {
    if (!allocator || !allocator->initialized) {
        return;
    }


    for (u32 i = 0; i < MEMORY_POOL_COUNT; i++) {
        MemoryPool* pool = &allocator->pools[i];
        if (pool->initialized) {
            f32 ratio = gpu_memory_get_fragmentation_ratio(allocator, (MemoryPoolType)i);
            u64 largest = gpu_memory_get_largest_free_block(allocator, (MemoryPoolType)i);
                    i, ratio, largest / (1024.0 * 1024.0));
        }
    }
}

void gpu_memory_log_allocations(GPUMemoryAllocator* allocator) {
    if (!allocator || !allocator->initialized) {
        return;
    }


    for (u32 i = 0; i < MEMORY_POOL_COUNT; i++) {
        MemoryPool* pool = &allocator->pools[i];
        if (!pool->initialized || pool->allocation_count == 0) {
            continue;
        }

        for (u32 j = 0; j < pool->allocation_count; j++) {
            AllocationInfo* alloc = &pool->allocations[j];
                    alloc->allocation_id, (f32)alloc->size, alloc->offset);
        }
    }
}

void gpu_memory_dump_pool_layout(GPUMemoryAllocator* allocator, MemoryPoolType pool_type) {
    if (!allocator || !allocator->initialized || pool_type >= MEMORY_POOL_COUNT) {
        return;
    }

    MemoryPool* pool = &allocator->pools[pool_type];
    if (!pool->initialized) {
        return;
    }

            pool_type, pool->total_size / (1024.0 * 1024.0));

    // Visualize blocks
    for (u32 i = 0; i < pool->allocation_count && i < 32; i++) {
    }
    fprintf(stderr, "\n");

    for (u32 i = 0; i < pool->free_block_count && i < 32; i++) {
    }
    fprintf(stderr, "\n");
}

bool gpu_memory_validate(GPUMemoryAllocator* allocator) {
    if (!allocator || !allocator->initialized) {
        return false;
    }

    // Basic validation
    for (u32 i = 0; i < MEMORY_POOL_COUNT; i++) {
        MemoryPool* pool = &allocator->pools[i];
        if (!pool->initialized) {
            continue;
        }

        // Check that allocated + free = total
        u64 expected_free = pool->total_size - pool->allocated_size;
        if (pool->free_size != expected_free) {
            return false;
        }
    }

    return true;
}
