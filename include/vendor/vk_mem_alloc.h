/**
 * Mock implementation of Vulkan Memory Allocator (VMA)
 * Created to allow compilation of Phase 1-5 validation tests without external dependencies.
 * 
 * IMPORTANT: This is NOT the real VMA library. It provides stubs only.
 * In a real build, the actual vk_mem_alloc.h should be used.
 */

#ifndef AMD_VULKAN_MEMORY_ALLOCATOR_H
#define AMD_VULKAN_MEMORY_ALLOCATOR_H

#ifdef __cplusplus
extern "C" {
#endif

#include <vulkan/vulkan.h>

// Handle types
VK_DEFINE_HANDLE(VmaAllocator)
VK_DEFINE_HANDLE(VmaPool)
VK_DEFINE_HANDLE(VmaAllocation)

// Flags and Enums
typedef enum VmaAllocatorCreateFlagBits {
    VMA_ALLOCATOR_CREATE_EXTERNALLY_SYNCHRONIZED_BIT = 0x00000001,
    VMA_ALLOCATOR_CREATE_KHR_DEDICATED_ALLOCATION_BIT = 0x00000002,
    VMA_ALLOCATOR_CREATE_KHR_BIND_MEMORY2_BIT = 0x00000004,
    VMA_ALLOCATOR_CREATE_EXT_MEMORY_BUDGET_BIT = 0x00000008,
    VMA_ALLOCATOR_CREATE_AMD_DEVICE_COHERENT_MEMORY_BIT = 0x00000010,
    VMA_ALLOCATOR_CREATE_BUFFER_DEVICE_ADDRESS_BIT = 0x00000020,
    VMA_ALLOCATOR_CREATE_EXT_MEMORY_PRIORITY_BIT = 0x00000040,
    VMA_ALLOCATOR_CREATE_FLAG_BITS_MAX_ENUM = 0x7FFFFFFF
} VmaAllocatorCreateFlagBits;
typedef VkFlags VmaAllocatorCreateFlags;

typedef enum VmaPoolCreateFlagBits {
    VMA_POOL_CREATE_IGNORE_BUFFER_IMAGE_GRANULARITY_BIT = 0x00000002,
    VMA_POOL_CREATE_LINEAR_ALGORITHM_BIT = 0x00000004,
    VMA_POOL_CREATE_ALGORITHM_MASK = 0x00000004,
    VMA_POOL_CREATE_FLAG_BITS_MAX_ENUM = 0x7FFFFFFF
} VmaPoolCreateFlagBits;
typedef VkFlags VmaPoolCreateFlags;

typedef enum VmaMemoryUsage {
    VMA_MEMORY_USAGE_UNKNOWN = 0,
    VMA_MEMORY_USAGE_GPU_ONLY = 1,
    VMA_MEMORY_USAGE_CPU_ONLY = 2,
    VMA_MEMORY_USAGE_CPU_TO_GPU = 3,
    VMA_MEMORY_USAGE_GPU_TO_CPU = 4,
    VMA_MEMORY_USAGE_CPU_COPY = 5,
    VMA_MEMORY_USAGE_GPU_LAZILY_ALLOCATED = 6,
    VMA_MEMORY_USAGE_AUTO = 7,
    VMA_MEMORY_USAGE_AUTO_PREFER_DEVICE = 8,
    VMA_MEMORY_USAGE_AUTO_PREFER_HOST = 9,
    VMA_MEMORY_USAGE_MAX_ENUM = 0x7FFFFFFF
} VmaMemoryUsage;

typedef enum VmaAllocationCreateFlagBits {
    VMA_ALLOCATION_CREATE_DEDICATED_MEMORY_BIT = 0x00000001,
    VMA_ALLOCATION_CREATE_NEVER_ALLOCATE_BIT = 0x00000002,
    VMA_ALLOCATION_CREATE_MAPPED_BIT = 0x00000004,
    VMA_ALLOCATION_CREATE_USER_DATA_COPY_STRING_BIT = 0x00000020,
    VMA_ALLOCATION_CREATE_UPPER_ADDRESS_BIT = 0x00000040,
    VMA_ALLOCATION_CREATE_DONT_BIND_BIT = 0x00000080,
    VMA_ALLOCATION_CREATE_WITHIN_BUDGET_BIT = 0x00000100,
    VMA_ALLOCATION_CREATE_CAN_ALIAS_BIT = 0x00000200,
    VMA_ALLOCATION_CREATE_HOST_ACCESS_SEQUENTIAL_WRITE_BIT = 0x00000400,
    VMA_ALLOCATION_CREATE_HOST_ACCESS_RANDOM_BIT = 0x00000800,
    VMA_ALLOCATION_CREATE_HOST_ACCESS_ALLOW_TRANSFER_INSTEAD_BIT = 0x00001000,
    VMA_ALLOCATION_CREATE_STRATEGY_MIN_MEMORY_BIT = 0x00010000,
    VMA_ALLOCATION_CREATE_STRATEGY_MIN_TIME_BIT = 0x00020000,
    VMA_ALLOCATION_CREATE_STRATEGY_MIN_OFFSET_BIT = 0x00040000,
    VMA_ALLOCATION_CREATE_FLAG_BITS_MAX_ENUM = 0x7FFFFFFF
} VmaAllocationCreateFlagBits;
typedef VkFlags VmaAllocationCreateFlags;

// Structures
typedef struct VmaDeviceMemoryCallbacks {
    PFN_vkAllocateMemory pfnAllocate;
    PFN_vkFreeMemory pfnFree;
    void* pUserData;
} VmaDeviceMemoryCallbacks;

typedef struct VmaVulkanFunctions {
    PFN_vkGetPhysicalDeviceProperties vkGetPhysicalDeviceProperties;
    PFN_vkGetPhysicalDeviceMemoryProperties vkGetPhysicalDeviceMemoryProperties;
    PFN_vkAllocateMemory vkAllocateMemory;
    PFN_vkFreeMemory vkFreeMemory;
    PFN_vkMapMemory vkMapMemory;
    PFN_vkUnmapMemory vkUnmapMemory;
    PFN_vkFlushMappedMemoryRanges vkFlushMappedMemoryRanges;
    PFN_vkInvalidateMappedMemoryRanges vkInvalidateMappedMemoryRanges;
    PFN_vkBindBufferMemory vkBindBufferMemory;
    PFN_vkBindImageMemory vkBindImageMemory;
    PFN_vkGetBufferMemoryRequirements vkGetBufferMemoryRequirements;
    PFN_vkGetImageMemoryRequirements vkGetImageMemoryRequirements;
    PFN_vkCreateBuffer vkCreateBuffer;
    PFN_vkDestroyBuffer vkDestroyBuffer;
    PFN_vkCreateImage vkCreateImage;
    PFN_vkDestroyImage vkDestroyImage;
    PFN_vkCmdCopyBuffer vkCmdCopyBuffer;
} VmaVulkanFunctions;

typedef struct VmaAllocatorCreateInfo {
    VmaAllocatorCreateFlags flags;
    VkPhysicalDevice physicalDevice;
    VkDevice device;
    VkDeviceSize preferredLargeHeapBlockSize;
    const VkAllocationCallbacks* pAllocationCallbacks;
    const VmaDeviceMemoryCallbacks* pDeviceMemoryCallbacks;
    const VmaVulkanFunctions* pVulkanFunctions;
    VkInstance instance;
    uint32_t vulkanApiVersion;
    const VkExternalMemoryHandleTypeFlagsKHR* pTypeExternalMemoryHandleTypes;
} VmaAllocatorCreateInfo;

typedef struct VmaPoolCreateInfo {
    uint32_t memoryTypeIndex;
    VmaPoolCreateFlags flags;
    VkDeviceSize blockSize;
    size_t minBlockCount;
    size_t maxBlockCount;
    uint32_t frameInUseCount;
    float priority;
    VkDeviceSize minAllocationAlignment;
    void* pMemoryAllocateNext;
} VmaPoolCreateInfo;

typedef struct VmaAllocationCreateInfo {
    VmaAllocationCreateFlags flags;
    VmaMemoryUsage usage;
    VkMemoryPropertyFlags requiredFlags;
    VkMemoryPropertyFlags preferredFlags;
    uint32_t memoryTypeBits;
    VmaPool pool;
    void* pUserData;
    float priority;
} VmaAllocationCreateInfo;

typedef struct VmaStatInfo {
    uint32_t blockCount;
    uint32_t allocationCount;
    uint32_t unusedRangeCount;
    VkDeviceSize usedBytes;
    VkDeviceSize unusedBytes;
    VkDeviceSize allocationSizeMin;
    VkDeviceSize allocationSizeAvg;
    VkDeviceSize allocationSizeMax;
    VkDeviceSize unusedRangeSizeMin;
    VkDeviceSize unusedRangeSizeAvg;
    VkDeviceSize unusedRangeSizeMax;
} VmaStatInfo;

typedef struct VmaStats {
    VmaStatInfo memoryType[VK_MAX_MEMORY_TYPES];
    VmaStatInfo memoryHeap[VK_MAX_MEMORY_HEAPS];
    VmaStatInfo total;
} VmaStats;

typedef struct VmaAllocationInfo {
    uint32_t memoryType;
    VkDeviceMemory deviceMemory;
    VkDeviceSize offset;
    VkDeviceSize size;
    void* pMappedData;
    void* pUserData;
    const char* pName;
} VmaAllocationInfo;

// Functions
#ifndef VMA_IMPLEMENTATION
#define VMA_CALL_PRE
#define VMA_CALL_POST
#else
#define VMA_CALL_PRE
#define VMA_CALL_POST
#endif

VMA_CALL_PRE VkResult VMA_CALL_POST vmaCreateAllocator(
    const VmaAllocatorCreateInfo* pCreateInfo,
    VmaAllocator* pAllocator);

VMA_CALL_PRE void VMA_CALL_POST vmaDestroyAllocator(
    VmaAllocator allocator);

VMA_CALL_PRE void VMA_CALL_POST vmaGetPhysicalDeviceProperties(
    VmaAllocator allocator,
    const VkPhysicalDeviceProperties** ppPhysicalDeviceProperties);

VMA_CALL_PRE void VMA_CALL_POST vmaGetMemoryProperties(
    VmaAllocator allocator,
    const VkPhysicalDeviceMemoryProperties** ppPhysicalDeviceMemoryProperties);

VMA_CALL_PRE void VMA_CALL_POST vmaGetMemoryTypeProperties(
    VmaAllocator allocator,
    uint32_t memoryTypeIndex,
    VkMemoryPropertyFlags* pFlags);

VMA_CALL_PRE void VMA_CALL_POST vmaCalculateStats(
    VmaAllocator allocator,
    VmaStats* pStats);

VMA_CALL_PRE VkResult VMA_CALL_POST vmaCreatePool(
    VmaAllocator allocator,
    const VmaPoolCreateInfo* pCreateInfo,
    VmaPool* pPool);

VMA_CALL_PRE void VMA_CALL_POST vmaDestroyPool(
    VmaAllocator allocator,
    VmaPool pool);

VMA_CALL_PRE void VMA_CALL_POST vmaGetPoolStats(
    VmaAllocator allocator,
    VmaPool pool,
    VmaStatInfo* pPoolStats);

VMA_CALL_PRE VkResult VMA_CALL_POST vmaAllocateMemory(
    VmaAllocator allocator,
    const VkMemoryRequirements* pVkMemoryRequirements,
    const VmaAllocationCreateInfo* pCreateInfo,
    VmaAllocation* pAllocation,
    VmaAllocationInfo* pAllocationInfo);

VMA_CALL_PRE VkResult VMA_CALL_POST vmaAllocateMemoryForBuffer(
    VmaAllocator allocator,
    VkBuffer buffer,
    const VmaAllocationCreateInfo* pCreateInfo,
    VmaAllocation* pAllocation,
    VmaAllocationInfo* pAllocationInfo);

VMA_CALL_PRE VkResult VMA_CALL_POST vmaAllocateMemoryForImage(
    VmaAllocator allocator,
    VkImage image,
    const VmaAllocationCreateInfo* pCreateInfo,
    VmaAllocation* pAllocation,
    VmaAllocationInfo* pAllocationInfo);

VMA_CALL_PRE void VMA_CALL_POST vmaFreeMemory(
    VmaAllocator allocator,
    VmaAllocation allocation);

VMA_CALL_PRE VkResult VMA_CALL_POST vmaMapMemory(
    VmaAllocator allocator,
    VmaAllocation allocation,
    void** ppData);

VMA_CALL_PRE void VMA_CALL_POST vmaUnmapMemory(
    VmaAllocator allocator,
    VmaAllocation allocation);

VMA_CALL_PRE VkResult VMA_CALL_POST vmaBindBufferMemory(
    VmaAllocator allocator,
    VmaAllocation allocation,
    VkBuffer buffer);

VMA_CALL_PRE VkResult VMA_CALL_POST vmaBindImageMemory(
    VmaAllocator allocator,
    VmaAllocation allocation,
    VkImage image);

// Implementation Stubs (if VMA_IMPLEMENTATION is defined)
#ifdef VMA_IMPLEMENTATION
#include <stdio.h>

VkResult vmaCreateAllocator(const VmaAllocatorCreateInfo* pCreateInfo, VmaAllocator* pAllocator) {
    if (pAllocator) { *pAllocator = (VmaAllocator)1; }
    return VK_SUCCESS;
}
void vmaDestroyAllocator(VmaAllocator allocator) { (void)allocator; }
void vmaCalculateStats(VmaAllocator allocator, VmaStats* pStats) {
    (void)allocator;
    if (pStats) {
        // Initialize to zero
        for(int i=0; i<VK_MAX_MEMORY_HEAPS; i++) {
            pStats->memoryHeap[i].blockCount = 0;
            pStats->memoryHeap[i].usedBytes = 0;
        }
        pStats->total.usedBytes = 0;
        pStats->total.unusedBytes = 0;
    }
}
VkResult vmaCreatePool(VmaAllocator allocator, const VmaPoolCreateInfo* pCreateInfo, VmaPool* pPool) {
    (void)allocator; (void)pCreateInfo;
    if (pPool) { *pPool = (VmaPool)1; }
    return VK_SUCCESS;
}
void vmaDestroyPool(VmaAllocator allocator, VmaPool pool) { (void)allocator; (void)pool; }
VkResult vmaAllocateMemoryForBuffer(VmaAllocator allocator, VkBuffer buffer, const VmaAllocationCreateInfo* pCreateInfo, VmaAllocation* pAllocation, VmaAllocationInfo* pAllocationInfo) {
    (void)allocator; (void)buffer; (void)pCreateInfo;
    if (pAllocation) { *pAllocation = (VmaAllocation)1; }
    if (pAllocationInfo) { pAllocationInfo->deviceMemory = VK_NULL_HANDLE; pAllocationInfo->offset = 0; }
    return VK_SUCCESS;
}
VkResult vmaAllocateMemoryForImage(VmaAllocator allocator, VkImage image, const VmaAllocationCreateInfo* pCreateInfo, VmaAllocation* pAllocation, VmaAllocationInfo* pAllocationInfo) {
    (void)allocator; (void)image; (void)pCreateInfo;
    if (pAllocation) { *pAllocation = (VmaAllocation)1; }
    return VK_SUCCESS;
}
void vmaFreeMemory(VmaAllocator allocator, VmaAllocation allocation) { (void)allocator; (void)allocation; }
VkResult vmaMapMemory(VmaAllocator allocator, VmaAllocation allocation, void** ppData) {
    (void)allocator; (void)allocation;
    // Static buffer for mapping
    static char mock_buffer[1024];
    if (ppData) { *ppData = mock_buffer; }
    return VK_SUCCESS;
}
void vmaUnmapMemory(VmaAllocator allocator, VmaAllocation allocation) { (void)allocator; (void)allocation; }
VkResult vmaBindBufferMemory(VmaAllocator allocator, VmaAllocation allocation, VkBuffer buffer) {
    (void)allocator; (void)allocation; (void)buffer;
    return VK_SUCCESS;
}
VkResult vmaBindImageMemory(VmaAllocator allocator, VmaAllocation allocation, VkImage image) {
    (void)allocator; (void)allocation; (void)image;
    return VK_SUCCESS;
}

#endif // VMA_IMPLEMENTATION

#ifdef __cplusplus
}
#endif

#endif // AMD_VULKAN_MEMORY_ALLOCATOR_H
