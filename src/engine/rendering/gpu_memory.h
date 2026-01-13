// src/engine/rendering/gpu_memory.h
// GPU Memory Management and Defragmentation System - Header file

#ifndef GPU_MEMORY_H
#define GPU_MEMORY_H

#include <stdbool.h>
#include <stdint.h>

#if defined(__APPLE__) && defined(__OBJC__)
#include <Metal/Metal.h>
#endif

#ifdef __cplusplus
extern "C" {
#endif

// ============================================================================
// Memory Pool Types
// ============================================================================

typedef enum {
  MEMORY_POOL_TEXTURE,
  MEMORY_POOL_BUFFER,
  MEMORY_POOL_UNIFORM,
  MEMORY_POOL_VERTEX,
  MEMORY_POOL_INDEX,
  MEMORY_POOL_COUNT
} MemoryPoolType;

// ============================================================================
// GPU Memory Allocation Handle
// ============================================================================

typedef struct {
  MemoryPoolType pool_type;
  uint64_t offset;
  uint64_t size;
  uint32_t allocation_id;

#ifdef __APPLE__
#ifdef __OBJC__
  id<MTLBuffer> metal_buffer;
#else
  void *metal_buffer;
#endif
  void *cpu_pointer;
#endif
} GPUMemoryAllocation;

// ============================================================================
// GPU Memory Statistics
// ============================================================================

typedef struct {
  uint64_t total_memory;
  uint64_t used_memory;
  uint64_t free_memory;
  uint32_t total_allocations;
  uint32_t total_frees;
  uint64_t peak_usage;
  float fragmentation_ratio;
} GPUMemoryStats;

// ============================================================================
// GPU Memory Manager Lifecycle
// ============================================================================

// Initialize the GPU memory manager
bool gpu_memory_init(void);

// Shutdown the GPU memory manager
void gpu_memory_shutdown(void);

// Check if GPU memory manager is initialized
bool gpu_memory_is_initialized(void);

// ============================================================================
// Memory Allocation
// ============================================================================

// Allocate GPU memory from a specific pool
GPUMemoryAllocation gpu_memory_allocate(MemoryPoolType type, uint64_t size,
                                        const char *name);

// Free GPU memory allocation
void gpu_memory_free(GPUMemoryAllocation *allocation);

// Get CPU pointer for shared memory allocations
void *gpu_memory_get_cpu_pointer(GPUMemoryAllocation *allocation);

// ============================================================================
// Memory Defragmentation
// ============================================================================

// Perform automatic defragmentation on pools that need it
void gpu_memory_defragment(void);

// Force defragmentation on all memory pools
void gpu_memory_force_defragmentation(void);

// ============================================================================
// Statistics and Debugging
// ============================================================================

// Get current memory statistics
void gpu_memory_get_stats(GPUMemoryStats *out);

// Log memory statistics to console
void gpu_memory_log_stats(void);

#ifdef __cplusplus
}
#endif

#endif // GPU_MEMORY_H
