// VMA (Vulkan Memory Allocator) Integration
// Provides efficient sub-allocation of device memory with custom allocators
//
// This implements the architectural audit recommendations for:
// 1. Device memory sub-allocation (avoiding 4096 allocation limit)
// 2. Integration with custom host allocators
// 3. Dedicated pools for voxel geometry (linear allocation)
// 4. Buffer device address alignment handling
// 5. MoltenVK-specific alignment requirements

#ifndef VMA_INTEGRATION_H
#define VMA_INTEGRATION_H

#include <common.h>

#ifdef VULKAN_BUILD

// Forward declarations
typedef struct VmaAllocator_T* VmaAllocator;
typedef struct VmaAllocation_T* VmaAllocation;
typedef struct VmaPool_T* VmaPool;

// VMA allocator configuration
typedef struct {
    void* vulkan_device;
    void* vulkan_physical_device;
    void* vulkan_instance;
    void* vulkan_allocation_callbacks; // VkAllocationCallbacks from memory system
    bool enable_buffer_device_address;
    u32 device_local_memory_mb;      // Size hint for device local memory
} VMAConfig;

// VMA allocation info
typedef struct {
    VmaAllocation allocation;
    void* mapped_data;
    u64 offset;
    u64 size;
} VMAAllocationInfo;

// Initialize VMA with custom allocators
bool vma_init(const VMAConfig* config);
void vma_shutdown(void);

// Create dedicated pool for voxel geometry (linear allocation)
VmaPool vma_create_voxel_pool(u64 size_bytes);
void vma_destroy_pool(VmaPool pool);

// Buffer allocation with device address support
bool vma_allocate_buffer(void* vk_buffer, VMAAllocationInfo* out_info);
bool vma_allocate_buffer_in_pool(void* vk_buffer, VmaPool pool, VMAAllocationInfo* out_info);
void vma_free_buffer(VMAAllocationInfo* info);

// Image allocation
bool vma_allocate_image(void* vk_image, VMAAllocationInfo* out_info);
void vma_free_image(VMAAllocationInfo* info);

// Memory mapping
void* vma_map_memory(VMAAllocationInfo* info);
void vma_unmap_memory(VMAAllocationInfo* info);

// Alignment queries for MoltenVK
u64 vma_get_buffer_device_address_alignment(void);
u64 vma_get_min_buffer_offset_alignment(void);

// Statistics
void vma_print_stats(void);

#endif // VULKAN_BUILD

#endif // VMA_INTEGRATION_H
