#include <vulkan/vulkan.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <assert.h>

// Vulkan memory allocator implementation
#define VK_MAX_MEMORY_TYPES 10
#define VK_MAX_MEMORY_HEAPS 32
#define VK_MAX_ALLOCATIONS 1024
#define VK_MIN_ALIGNMENT 256
#define VK_PAGE_SIZE 4096

typedef enum {
    VK_MEMORY_TYPE_UNIFORM = 0,
    VK_MEMORY_TYPE_VERTEX,
    VK_MEMORY_TYPE_INDEX,
    VK_MEMORY_TYPE_STAGING,
    VK_MEMORY_TYPE_STORAGE_IMAGE,
    VK_MEMORY_TYPE_STORAGE_BUFFER,
    VK_MEMORY_TYPE_UNIFORM_TEXEL,
    VK_MEMORY_TYPE_STORAGE_TEXEL,
    VK_MEMORY_TYPE_TRANSFER_DST,
    VK_MEMORY_TYPE_TRANSFER_SRC,
    VK_MEMORY_TYPE_COUNT
} vk_memory_type_t;

typedef struct vk_memory_block {
    VkDeviceMemory memory;
    VkDeviceSize size;
    VkDeviceSize offset;
    vk_memory_type_t type;
    void* mapped_data;
    bool is_mapped;
    bool is_free;
    u32 allocation_id;
} vk_memory_block_t;

typedef struct vk_memory_heap {
    VkDeviceSize size;
    VkDeviceSize used;
    VkDeviceSize peak_usage;
    vk_memory_block_t* blocks;
    u32 block_count;
    u32 block_capacity;
    u32 next_free_block;
    VkMemoryPropertyFlags properties;
    u32 memory_type_index;
    bool is_host_visible;
    bool is_device_local;
} vk_memory_heap_t;

typedef struct vk_memory_allocator {
    VkDevice device;
    VkPhysicalDevice physical_device;
    VkPhysicalDeviceMemoryProperties memory_properties;
    
    vk_memory_heap_t heaps[VK_MAX_MEMORY_HEAPS];
    u32 heap_count;
    
    vk_memory_block_t* allocations[VK_MAX_ALLOCATIONS];
    u32 allocation_count;
    u32 next_allocation_id;
    
    // Statistics
    u64 total_allocated;
    u64 total_mapped;
    u32 peak_allocations;
    u32 total_freed;
} vk_memory_allocator_t;

static vk_memory_allocator_t g_memory_allocator = {0};

// Get memory type index for properties
static u32 get_memory_type_index(VkMemoryPropertyFlags properties, u32 memory_type_bits) {
    for (u32 i = 0; i < g_memory_allocator.memory_properties.memoryTypeCount; i++) {
        if ((g_memory_allocator.memory_properties.memoryTypes[i].propertyFlags & properties) == properties &&
            (memory_type_bits & (1 << i))) {
            return i;
        }
    }
    return UINT32_MAX;
}

// Find suitable memory heap for memory type
static u32 find_memory_heap(u32 memory_type_index, VkMemoryPropertyFlags required_properties) {
    for (u32 i = 0; i < g_memory_allocator.heap_count; i++) {
        vk_memory_heap_t* heap = &g_memory_allocator.heaps[i];
        
        if (heap->memory_type_index == memory_type_index &&
            (heap->properties & required_properties) == required_properties) {
            return i;
        }
    }
    return UINT32_MAX;
}

// Create memory heap
static bool create_memory_heap(u32 memory_type_index, VkMemoryPropertyFlags properties, VkDeviceSize size) {
    if (g_memory_allocator.heap_count >= VK_MAX_MEMORY_HEAPS) {
        printf("Error: Maximum memory heaps reached\n");
        return false;
    }
    
    VkMemoryAllocateInfo alloc_info = {0};
    alloc_info.sType = VK_STRUCTURE_TYPE_MEMORY_ALLOCATE_INFO;
    alloc_info.allocationSize = size;
    alloc_info.memoryTypeIndex = memory_type_index;
    alloc_info.memoryPropertyFlags = properties;
    
    VkDeviceMemory memory;
    VkResult result = vkAllocateMemory(g_memory_allocator.device, &alloc_info, NULL, &memory);
    if (result != VK_SUCCESS) {
        printf("Error: Failed to allocate memory heap\n");
        return false;
    }
    
    vk_memory_heap_t* heap = &g_memory_allocator.heaps[g_memory_allocator.heap_count];
    heap->size = size;
    heap->used = 0;
    heap->peak_usage = 0;
    heap->blocks = (vk_memory_block_t*)calloc(32, sizeof(vk_memory_block_t));
    heap->block_count = 0;
    heap->block_capacity = 32;
    heap->next_free_block = 0;
    heap->properties = properties;
    heap->memory_type_index = memory_type_index;
    heap->is_host_visible = (properties & VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT) != 0;
    heap->is_device_local = (properties & VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT) != 0;
    
    g_memory_allocator.heap_count++;
    
    printf("Created memory heap %u (type: %u, size: %zu bytes, properties: %s)\n",
           g_memory_allocator.heap_count - 1, memory_type_index, size,
           heap->is_host_visible ? "host_visible" : "device_local");
    
    return true;
}

// Initialize memory allocator
bool vk_memory_allocator_init(VkDevice device, VkPhysicalDevice physical_device) {
    if (!device || !physical_device) {
        printf("Error: Invalid device or physical device for memory allocator\n");
        return false;
    }
    
    g_memory_allocator.device = device;
    g_memory_allocator.physical_device = physical_device;
    
    vkGetPhysicalDeviceMemoryProperties(physical_device, &g_memory_allocator.memory_properties);
    
    // Create heaps for each memory type
    for (u32 i = 0; i < g_memory_allocator.memory_properties.memoryTypeCount; i++) {
        VkMemoryType type = g_memory_allocator.memory_properties.memoryTypes[i];
        
        // Create heap for host visible memory
        if (type.propertyFlags & VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT) {
            VkDeviceSize heap_size = type.heapSize;
            if (heap_size > 0) {
                create_memory_heap(i, VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT, heap_size);
            }
        }
        
        // Create heap for device local memory
        if (type.propertyFlags & VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT) {
            VkDeviceSize heap_size = type.heapSize;
            if (heap_size > 0) {
                create_memory_heap(i, VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT, heap_size);
            }
        }
    }
    
    printf("Vulkan memory allocator initialized with %u heaps\n", g_memory_allocator.heap_count);
    return true;
}

// Cleanup memory allocator
void vk_memory_allocator_cleanup(void) {
    if (!g_memory_allocator.device) {
        return;
    }
    
    // Free all allocations
    for (u32 i = 0; i < g_memory_allocator.allocation_count; i++) {
        if (g_memory_allocator.allocations[i].memory != VK_NULL_HANDLE) {
            vkFreeMemory(g_memory_allocator.device, g_memory_allocator.allocations[i].memory, NULL);
        }
    }
    
    // Free all heaps
    for (u32 i = 0; i < g_memory_allocator.heap_count; i++) {
        if (g_memory_allocator.heaps[i].handle) {
            vkFreeMemory(g_memory_allocator.device, g_memory_allocator.heaps[i].handle, NULL);
        }
        free(g_memory_allocator.heaps[i].blocks);
    }
    
    memset(&g_memory_allocator, 0, sizeof(g_memory_allocator));
    
    printf("Vulkan memory allocator cleaned up\n");
}

// Allocate memory block
static u32 allocate_block(vk_memory_heap_t* heap, VkDeviceSize size, VkDeviceSize alignment) {
    if (!heap || size == 0) {
        return UINT32_MAX;
    }
    
    // Align size to minimum alignment
    size = (size + VK_MIN_ALIGNMENT - 1) & ~(VK_MIN_ALIGNMENT - 1);
    
    // Find free block
    for (u32 i = 0; i < heap->block_count; i++) {
        vk_memory_block_t* block = &heap->blocks[i];
        
        if (block->is_free && block->size >= size) {
            // Check alignment
            VkDeviceSize aligned_offset = (block->offset + alignment - 1) & ~(alignment - 1);
            
            if (aligned_offset + size <= block->offset + block->size) {
                // Split block if necessary
                VkDeviceSize remaining_size = block->size - (aligned_offset - block->offset + size);
                
                if (remaining_size > VK_MIN_ALIGNMENT) {
                    // Create new block for remaining space
                    u32 new_block_index = heap->block_count;
                    if (new_block_index < heap->block_capacity) {
                        vk_memory_block_t* new_block = &heap->blocks[new_block_index];
                        new_block->memory = block->memory;
                        new_block->size = remaining_size;
                        new_block->offset = aligned_offset + size;
                        new_block->type = block->type;
                        new_block->mapped_data = block->mapped_data;
                        new_block->is_mapped = block->is_mapped;
                        new_block->is_free = true;
                        new_block->allocation_id = 0;
                        
                        heap->block_count++;
                    }
                }
                
                // Update original block
                block->size = aligned_offset - block->offset + size;
                block->is_free = false;
                block->allocation_id = g_memory_allocator.next_allocation_id++;
                
                heap->used += block->size;
                if (heap->used > heap->peak_usage) {
                    heap->peak_usage = heap->used;
                }
                
                return g_memory_allocator.next_allocation_id++;
            }
        }
    }
    
    return UINT32_MAX;
}

// Allocate memory
u32 vk_memory_allocate(vk_memory_type_t type, VkDeviceSize size, VkDeviceSize alignment) {
    if (!g_memory_allocator.device || type >= VK_MEMORY_TYPE_COUNT || size == 0) {
        return 0;
    }
    
    u32 memory_type_bits = 1 << type;
    VkMemoryPropertyFlags properties = 0;
    
    // Determine required properties based on type
    switch (type) {
        case VK_MEMORY_TYPE_UNIFORM:
            properties = VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT;
            break;
        case VK_MEMORY_TYPE_VERTEX:
        case VK_MEMORY_TYPE_INDEX:
            properties = VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT;
            break;
        case VK_MEMORY_TYPE_STAGING:
            properties = VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT;
            break;
        case VK_MEMORY_TYPE_STORAGE_IMAGE:
        case VK_MEMORY_TYPE_STORAGE_BUFFER:
            properties = VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT;
            break;
        case VK_MEMORY_TYPE_UNIFORM_TEXEL:
        case VK_MEMORY_TYPE_STORAGE_TEXEL:
            properties = VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT;
            break;
        case VK_MEMORY_TYPE_TRANSFER_DST:
        case VK_MEMORY_TYPE_TRANSFER_SRC:
            properties = VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT;
            break;
        default:
            properties = VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT;
            break;
    }
    
    u32 memory_type_index = get_memory_type_index(properties, memory_type_bits);
    if (memory_type_index == UINT32_MAX) {
        printf("Error: No suitable memory type found for type %u\n", type);
        return 0;
    }
    
    u32 heap_index = find_memory_heap(memory_type_index, properties);
    if (heap_index == UINT32_MAX) {
        printf("Error: No suitable heap found for memory type %u\n", type);
        return 0;
    }
    
    vk_memory_heap_t* heap = &g_memory_allocator.heaps[heap_index];
    u32 block_id = allocate_block(heap, size, alignment);
    
    if (block_id == UINT32_MAX) {
        printf("Error: Failed to allocate memory block of size %zu\n", size);
        return 0;
    }
    
    // Store allocation
    if (g_memory_allocator.allocation_count >= VK_MAX_ALLOCATIONS) {
        printf("Error: Maximum allocations reached\n");
        return 0;
    }
    
    u32 allocation_index = g_memory_allocator.next_allocation_id - 1;
    vk_memory_block_t* allocation = &g_memory_allocator.allocations[allocation_index];
    *allocation = heap->blocks[block_id];
    
    g_memory_allocator.total_allocated += allocation->size;
    g_memory_allocator.allocation_count++;
    
    if (g_memory_allocator.allocation_count > g_memory_allocator.peak_allocations) {
        g_memory_allocator.peak_allocations = g_memory_allocator.allocation_count;
    }
    
    printf("Allocated memory %u (size: %zu bytes, type: %u)\n", 
           allocation_index, allocation->size, type);
    
    return allocation_index;
}

// Free memory
bool vk_memory_free(u32 allocation_id) {
    if (!g_memory_allocator.device || allocation_id == 0) {
        return false;
    }
    
    if (allocation_id >= VK_MAX_ALLOCATIONS) {
        printf("Error: Invalid allocation ID %u\n", allocation_id);
        return false;
    }
    
    vk_memory_block_t* allocation = &g_memory_allocator.allocations[allocation_id];
    
    if (allocation->memory == VK_NULL_HANDLE) {
        return false;
    }
    
    vk_memory_heap_t* heap = NULL;
    for (u32 i = 0; i < g_memory_allocator.heap_count; i++) {
        if (g_memory_allocator.heaps[i].blocks && 
            g_memory_allocator.heaps[i].blocks[allocation->allocation_id].memory == allocation->memory) {
            heap = &g_memory_allocator.heaps[i];
            break;
        }
    }
    
    if (!heap) {
        printf("Error: Could not find heap for allocation %u\n", allocation_id);
        return false;
    }
    
    // Unmap if mapped
    if (allocation->is_mapped) {
        vkUnmapMemory(g_memory_allocator.device, allocation->memory, 0);
        allocation->is_mapped = false;
        allocation->mapped_data = NULL;
        g_memory_allocator.total_mapped -= allocation->size;
    }
    
    // Mark block as free
    allocation->is_free = true;
    allocation->allocation_id = 0;
    
    heap->used -= allocation->size;
    
    // Try to merge with adjacent free blocks
    for (u32 i = 0; i < heap->block_count; i++) {
        vk_memory_block_t* block = &heap->blocks[i];
        
        if (block->is_free && block != allocation) {
            // Check if blocks are adjacent
            if (block->offset + block->size == allocation->offset) {
                // Merge block before allocation
                block->size += allocation->size;
                allocation->size = 0;
                allocation->memory = VK_NULL_HANDLE;
                allocation->is_free = true;
                allocation->allocation_id = 0;
                break;
            } else if (allocation->offset + allocation->size == block->offset) {
                // Merge block after allocation
                allocation->size += block->size;
                block->size = 0;
                block->memory = VK_NULL_HANDLE;
                block->is_free = true;
                block->allocation_id = 0;
                break;
            }
        }
    }
    
    g_memory_allocator.total_allocated -= allocation->size;
    g_memory_allocator.total_freed++;
    
    printf("Freed memory %u (size: %zu bytes)\n", allocation_id, allocation->size);
    return true;
}

// Map memory
void* vk_memory_map(u32 allocation_id) {
    if (!g_memory_allocator.device || allocation_id == 0) {
        return NULL;
    }
    
    if (allocation_id >= VK_MAX_ALLOCATIONS) {
        printf("Error: Invalid allocation ID %u for mapping\n", allocation_id);
        return NULL;
    }
    
    vk_memory_block_t* allocation = &g_memory_allocator.allocations[allocation_id];
    
    if (allocation->memory == VK_NULL_HANDLE || allocation->is_mapped) {
        return allocation->mapped_data;
    }
    
    vk_memory_heap_t* heap = NULL;
    for (u32 i = 0; i < g_memory_allocator.heap_count; i++) {
        if (g_memory_allocator.heaps[i].blocks && 
            g_memory_allocator.heaps[i].blocks[allocation->allocation_id].memory == allocation->memory) {
            heap = &g_memory_allocator.heaps[i];
            break;
        }
    }
    
    if (!heap || !heap->is_host_visible) {
        printf("Error: Cannot map memory that is not host visible\n");
        return NULL;
    }
    
    VkResult result = vkMapMemory(g_memory_allocator.device, allocation->memory, 
                                   allocation->offset, allocation->size, 0, 
                                   &allocation->mapped_data);
    
    if (result != VK_SUCCESS) {
        printf("Error: Failed to map memory\n");
        return NULL;
    }
    
    allocation->is_mapped = true;
    g_memory_allocator.total_mapped += allocation->size;
    
    return allocation->mapped_data;
}

// Unmap memory
void vk_memory_unmap(u32 allocation_id) {
    if (!g_memory_allocator.device || allocation_id == 0) {
        return;
    }
    
    if (allocation_id >= VK_MAX_ALLOCATIONS) {
        printf("Error: Invalid allocation ID %u for unmapping\n", allocation_id);
        return;
    }
    
    vk_memory_block_t* allocation = &g_memory_allocator.allocations[allocation_id];
    
    if (!allocation->is_mapped || allocation->memory == VK_NULL_HANDLE) {
        return;
    }
    
    vkUnmapMemory(g_memory_allocator.device, allocation->memory, allocation->size);
    
    allocation->is_mapped = false;
    allocation->mapped_data = NULL;
    g_memory_allocator.total_mapped -= allocation->size;
}

// Get allocation info
bool vk_memory_get_info(u32 allocation_id, VkDeviceSize* size, vk_memory_type_t* type, void** mapped_data) {
    if (!g_memory_allocator.device || allocation_id == 0) {
        return false;
    }
    
    if (allocation_id >= VK_MAX_ALLOCATIONS) {
        return false;
    }
    
    vk_memory_block_t* allocation = &g_memory_allocator.allocations[allocation_id];
    
    if (allocation->memory == VK_NULL_HANDLE) {
        return false;
    }
    
    if (size) *size = allocation->size;
    if (type) *type = allocation->type;
    if (mapped_data) *mapped_data = allocation->mapped_data;
    
    return true;
}

// Get statistics
void vk_memory_get_stats(u64* total_allocated, u64* total_mapped, u32* allocation_count, u32* peak_usage) {
    if (total_allocated) *total_allocated = g_memory_allocator.total_allocated;
    if (total_mapped) *total_mapped = g_memory_allocator.total_mapped;
    if (allocation_count) *allocation_count = g_memory_allocator.allocation_count;
    if (peak_usage) *peak_usage = g_memory_allocator.peak_allocations;
}

// Flush allocations
void vk_memory_flush(void) {
    if (!g_memory_allocator.device) {
        return;
    }
    
    // Free all allocations
    for (u32 i = 0; i < g_memory_allocator.allocation_count; i++) {
        if (g_memory_allocator.allocations[i].memory != VK_NULL_HANDLE) {
            vk_memory_free(i);
        }
    }
    
    printf("Flushed all memory allocations\n");
}
