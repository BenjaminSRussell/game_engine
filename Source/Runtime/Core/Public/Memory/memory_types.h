/**
 * @file memory_types.h
 * @brief Memory subsystem type definitions
 */

#ifndef VOXELFORGE_MEMORY_TYPES_H
#define VOXELFORGE_MEMORY_TYPES_H

#include "Core/Public/core_types.h"

#ifdef __cplusplus
extern "C" {
#endif

// ============================================================================
// Memory Tags (for tracking allocations)
// ============================================================================

typedef enum VF_MemoryTag {
  VF_MEMORY_TAG_UNKNOWN = 0,
  VF_MEMORY_TAG_CORE,
  VF_MEMORY_TAG_RENDERER,
  VF_MEMORY_TAG_PHYSICS,
  VF_MEMORY_TAG_AUDIO,
  VF_MEMORY_TAG_ANIMATION,
  VF_MEMORY_TAG_AI,
  VF_MEMORY_TAG_NETWORKING,
  VF_MEMORY_TAG_UI,
  VF_MEMORY_TAG_SCENE,
  VF_MEMORY_TAG_ASSET,
  VF_MEMORY_TAG_STRING,
  VF_MEMORY_TAG_CONTAINER,
  VF_MEMORY_TAG_TEMPORARY,
  VF_MEMORY_TAG_DEBUG,
  VF_MEMORY_TAG_COUNT
} VF_MemoryTag;

// ============================================================================
// Allocator Interface
// ============================================================================

typedef struct VF_Allocator VF_Allocator;

typedef void *(*VF_AllocFn)(VF_Allocator *allocator, usize size,
                            usize alignment);
typedef void *(*VF_ReallocFn)(VF_Allocator *allocator, void *ptr,
                              usize old_size, usize new_size, usize alignment);
typedef void (*VF_FreeFn)(VF_Allocator *allocator, void *ptr, usize size);
typedef void (*VF_ResetFn)(VF_Allocator *allocator);

struct VF_Allocator {
  VF_AllocFn alloc;
  VF_ReallocFn realloc;
  VF_FreeFn free;
  VF_ResetFn reset; // Optional: reset allocator state
  void *user_data;
};

// ============================================================================
// Allocation Stats
// ============================================================================

typedef struct VF_AllocationStats {
  u64 total_allocated;
  u64 total_freed;
  u64 peak_usage;
  u64 current_usage;
  u64 allocation_count;
  u64 free_count;
  u64 tagged_usage[VF_MEMORY_TAG_COUNT];
} VF_AllocationStats;

// ============================================================================
// Memory Block
// ============================================================================

typedef struct VF_MemoryBlock {
  void *data;
  usize size;
  usize capacity;
} VF_MemoryBlock;

#ifdef __cplusplus
}
#endif

#endif // VOXELFORGE_MEMORY_TYPES_H
