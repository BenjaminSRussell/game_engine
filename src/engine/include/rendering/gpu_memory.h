// include/render/gpu_memory.h
//
// Purpose: GPU memory management and buffer allocation system.
// Provides efficient allocation, suballocation, defragmentation, and resource
// tracking.
//
#ifndef GPU_MEMORY_H
#define GPU_MEMORY_H

#include "engine/include/common.h"
#include <time.h>

#include <include/rendering/vulkan.h>

// Forward declarations
typedef struct GPUMemoryAllocator GPUMemoryAllocator;
typedef struct GPUMemoryBlock GPUMemoryBlock;

// Memory pool types
typedef enum {
  MEMORY_POOL_STAGING, // CPU-visible, host-coherent (for CPU->GPU transfers)
  MEMORY_POOL_DEVICE_LOCAL, // GPU-only, fastest (for GPU-accessed data)
  MEMORY_POOL_DYNAMIC,      // CPU-visible, for per-frame updates
  MEMORY_POOL_READBACK,     // CPU-visible, for GPU->CPU transfers
  MEMORY_POOL_COUNT,
} MemoryPoolType;

// Buffer usage flags
typedef enum {
  BUFFER_USAGE_VERTEX = 1 << 0,
  BUFFER_USAGE_INDEX = 1 << 1,
  BUFFER_USAGE_UNIFORM = 1 << 2,
  BUFFER_USAGE_STORAGE = 1 << 3,
  BUFFER_USAGE_TRANSFER_SRC = 1 << 4,
  BUFFER_USAGE_TRANSFER_DST = 1 << 5,
  BUFFER_USAGE_INDIRECT = 1 << 6,
} BufferUsageFlags;

// Allocation info
typedef struct {
  u32 allocation_id;
  MemoryPoolType pool_type;
  VkBuffer buffer;
  VkDeviceMemory memory;
  u64 offset;
  u64 size;
  u32 alignment;
  void *mapped_ptr;  // For CPU-visible memory
  bool is_dedicated; // Dedicated vs suballocated
  time_t allocation_time;
} AllocationInfo;

// Memory pool for one memory type
typedef struct {
  MemoryPoolType pool_type;
  VkDeviceMemory memory;
  u64 total_size;
  u64 allocated_size;
  u64 free_size;

  // Free blocks for suballocation (fragmentation tracking)
  struct {
    u64 offset;
    u64 size;
  } free_blocks[256];
  u32 free_block_count;

  // Allocations
  AllocationInfo allocations[512];
  u32 allocation_count;

  void *mapped_ptr; // For CPU-visible pools
  bool coherent;    // Host coherent (no flush needed)
  bool cached;      // Host cached (faster reads)

  bool initialized;
} MemoryPool;

// GPU memory allocator
struct GPUMemoryAllocator {
  // Vulkan objects
  VkDevice device;
  VkPhysicalDevice physical_device;
  VkPhysicalDeviceMemoryProperties memory_properties;

  // Memory pools
  MemoryPool pools[MEMORY_POOL_COUNT];

  // Global allocation tracking
  AllocationInfo all_allocations[2048];
  u32 total_allocations;

  // Statistics
  u64 total_allocated;
  u64 total_freed;
  u32 allocation_count;
  u32 deallocation_count;
  u32 defragmentations;

  // Configuration
  u64 device_local_budget; // Maximum GPU-only memory to use
  u64 staging_buffer_size; // Size of staging pool
  u64 dynamic_buffer_size; // Size of dynamic pool
  bool defragmentation_enabled;
  bool verbose;

  bool initialized;
};

// ==============================================================================
// Lifecycle Management
// ==============================================================================

/**
 * Initialize GPU memory allocator.
 *
 * @param allocator Pointer to GPUMemoryAllocator
 * @param device Vulkan logical device
 * @param physical_device Vulkan physical device
 * @param device_local_budget Total GPU memory budget (e.g., 4GB)
 * @return true on success, false on failure
 */
bool gpu_memory_init(GPUMemoryAllocator *allocator, VkDevice device,
                     VkPhysicalDevice physical_device, u64 device_local_budget);

/**
 * Shutdown GPU memory allocator and free all resources.
 *
 * @param allocator Pointer to GPUMemoryAllocator
 */
void gpu_memory_shutdown(GPUMemoryAllocator *allocator);

// ==============================================================================
// Memory Pool Management
// ==============================================================================

/**
 * Create memory pool of specified type and size.
 *
 * @param allocator Pointer to GPUMemoryAllocator
 * @param pool_type Type of memory pool
 * @param size Size of pool in bytes
 * @return true on success, false on failure
 */
bool gpu_memory_create_pool(GPUMemoryAllocator *allocator,
                            MemoryPoolType pool_type, u64 size);

/**
 * Destroy memory pool and free all allocations within it.
 *
 * @param allocator Pointer to GPUMemoryAllocator
 * @param pool_type Type of memory pool
 */
void gpu_memory_destroy_pool(GPUMemoryAllocator *allocator,
                             MemoryPoolType pool_type);

/**
 * Reset memory pool (clear all allocations, mark memory as free).
 *
 * @param allocator Pointer to GPUMemoryAllocator
 * @param pool_type Type of memory pool
 */
void gpu_memory_reset_pool(GPUMemoryAllocator *allocator,
                           MemoryPoolType pool_type);

// ==============================================================================
// Buffer Allocation
// ==============================================================================

/**
 * Allocate buffer from memory pool.
 *
 * @param allocator Pointer to GPUMemoryAllocator
 * @param size Size in bytes to allocate
 * @param pool_type Memory pool type
 * @param usage_flags VkBufferUsageFlags (BUFFER_USAGE_*)
 * @param out_info Pointer to receive allocation info
 * @return true on success, false on failure
 */
bool gpu_memory_allocate_buffer(GPUMemoryAllocator *allocator, u64 size,
                                MemoryPoolType pool_type,
                                BufferUsageFlags usage_flags,
                                AllocationInfo *out_info);

/**
 * Deallocate previously allocated buffer.
 *
 * @param allocator Pointer to GPUMemoryAllocator
 * @param allocation_id Allocation ID returned from allocate_buffer
 */
void gpu_memory_deallocate_buffer(GPUMemoryAllocator *allocator,
                                  u32 allocation_id);

/**
 * Get allocation info by ID.
 *
 * @param allocator Pointer to GPUMemoryAllocator
 * @param allocation_id Allocation ID
 * @return Pointer to AllocationInfo or NULL
 */
AllocationInfo *gpu_memory_get_allocation(GPUMemoryAllocator *allocator,
                                          u32 allocation_id);

/**
 * Get VkBuffer handle from allocation ID.
 *
 * @param allocator Pointer to GPUMemoryAllocator
 * @param allocation_id Allocation ID
 * @return VkBuffer handle or VK_NULL_HANDLE
 */
VkBuffer gpu_memory_get_buffer(GPUMemoryAllocator *allocator,
                               u32 allocation_id);

/**
 * Get CPU-visible pointer for CPU->GPU updates.
 *
 * @param allocator Pointer to GPUMemoryAllocator
 * @param allocation_id Allocation ID
 * @return Pointer to mapped memory or NULL
 */
void *gpu_memory_get_mapped_pointer(GPUMemoryAllocator *allocator,
                                    u32 allocation_id);

// ==============================================================================
// Memory Access
// ==============================================================================

/**
 * Map allocation memory to CPU address space.
 *
 * @param allocator Pointer to GPUMemoryAllocator
 * @param allocation_id Allocation ID
 * @return Pointer to mapped memory or NULL
 */
void *gpu_memory_map(GPUMemoryAllocator *allocator, u32 allocation_id);

/**
 * Unmap allocation memory.
 *
 * @param allocator Pointer to GPUMemoryAllocator
 * @param allocation_id Allocation ID
 */
void gpu_memory_unmap(GPUMemoryAllocator *allocator, u32 allocation_id);

/**
 * Flush memory writes to GPU (for non-coherent memory).
 *
 * @param allocator Pointer to GPUMemoryAllocator
 * @param allocation_id Allocation ID
 */
void gpu_memory_flush(GPUMemoryAllocator *allocator, u32 allocation_id);

/**
 * Invalidate memory cache (for non-coherent reads).
 *
 * @param allocator Pointer to GPUMemoryAllocator
 * @param allocation_id Allocation ID
 */
void gpu_memory_invalidate(GPUMemoryAllocator *allocator, u32 allocation_id);

// ==============================================================================
// Memory Optimization
// ==============================================================================

/**
 * Enable/disable automatic defragmentation.
 *
 * @param allocator Pointer to GPUMemoryAllocator
 * @param enabled Enable flag
 */
void gpu_memory_set_defragmentation_enabled(GPUMemoryAllocator *allocator,
                                            bool enabled);

/**
 * Perform defragmentation on memory pool.
 * Compacts free blocks and relocates allocations to reduce fragmentation.
 *
 * @param allocator Pointer to GPUMemoryAllocator
 * @param pool_type Memory pool type
 * @return Number of relocations performed
 */
u32 gpu_memory_defragment_pool(GPUMemoryAllocator *allocator,
                               MemoryPoolType pool_type);

/**
 * Defragment all memory pools.
 *
 * @param allocator Pointer to GPUMemoryAllocator
 * @return Total number of relocations performed
 */
u32 gpu_memory_defragment_all(GPUMemoryAllocator *allocator);

/**
 * Compact pool to reduce wasted space.
 *
 * @param allocator Pointer to GPUMemoryAllocator
 * @param pool_type Memory pool type
 * @return Bytes freed
 */
u64 gpu_memory_compact_pool(GPUMemoryAllocator *allocator,
                            MemoryPoolType pool_type);

// ==============================================================================
// Query Functions
// ==============================================================================

/**
 * Get total allocated memory in bytes.
 *
 * @param allocator Pointer to GPUMemoryAllocator
 * @return Total allocated bytes
 */
u64 gpu_memory_get_allocated_size(GPUMemoryAllocator *allocator);

/**
 * Get free memory available for allocation.
 *
 * @param allocator Pointer to GPUMemoryAllocator
 * @return Free bytes available
 */
u64 gpu_memory_get_free_size(GPUMemoryAllocator *allocator);

/**
 * Get memory usage percentage.
 *
 * @param allocator Pointer to GPUMemoryAllocator
 * @return Percentage used (0.0 - 100.0)
 */
f32 gpu_memory_get_usage_percentage(GPUMemoryAllocator *allocator);

/**
 * Get fragmentation ratio for pool.
 *
 * @param allocator Pointer to GPUMemoryAllocator
 * @param pool_type Memory pool type
 * @return Fragmentation ratio (0.0 = no fragmentation, 1.0 = maximum)
 */
f32 gpu_memory_get_fragmentation_ratio(GPUMemoryAllocator *allocator,
                                       MemoryPoolType pool_type);

/**
 * Get largest contiguous free block size in pool.
 *
 * @param allocator Pointer to GPUMemoryAllocator
 * @param pool_type Memory pool type
 * @return Size in bytes
 */
u64 gpu_memory_get_largest_free_block(GPUMemoryAllocator *allocator,
                                      MemoryPoolType pool_type);

/**
 * Get number of allocations in pool.
 *
 * @param allocator Pointer to GPUMemoryAllocator
 * @param pool_type Memory pool type
 * @return Allocation count
 */
u32 gpu_memory_get_allocation_count(GPUMemoryAllocator *allocator,
                                    MemoryPoolType pool_type);

// ==============================================================================
// Debug and Diagnostics
// ==============================================================================

/**
 * Log GPU memory allocator information.
 *
 * @param allocator Pointer to GPUMemoryAllocator
 */
void gpu_memory_log_info(GPUMemoryAllocator *allocator);

/**
 * Log detailed memory usage statistics.
 *
 * @param allocator Pointer to GPUMemoryAllocator
 */
void gpu_memory_log_statistics(GPUMemoryAllocator *allocator);

/**
 * Log fragmentation analysis per pool.
 *
 * @param allocator Pointer to GPUMemoryAllocator
 */
void gpu_memory_log_fragmentation(GPUMemoryAllocator *allocator);

/**
 * Log all active allocations.
 *
 * @param allocator Pointer to GPUMemoryAllocator
 */
void gpu_memory_log_allocations(GPUMemoryAllocator *allocator);

/**
 * Dump memory layout visualization for pool.
 *
 * @param allocator Pointer to GPUMemoryAllocator
 * @param pool_type Memory pool type
 */
void gpu_memory_dump_pool_layout(GPUMemoryAllocator *allocator,
                                 MemoryPoolType pool_type);

/**
 * Validate allocator integrity.
 *
 * @param allocator Pointer to GPUMemoryAllocator
 * @return true if valid, false if corruption detected
 */
bool gpu_memory_validate(GPUMemoryAllocator *allocator);

#endif // GPU_MEMORY_H
