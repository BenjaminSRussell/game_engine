/**
 * @file memory_api.h
 * @brief Public memory API for VoxelForge Engine
 */

#ifndef VOXELFORGE_MEMORY_API_H
#define VOXELFORGE_MEMORY_API_H

#include "Core/Public/Memory/memory_types.h"
#include "Core/Public/core_types.h"

#ifdef __cplusplus
extern "C" {
#endif

// ============================================================================
// Memory System Initialization
// ============================================================================

VF_API VF_Result vf_memory_init(void);
VF_API void vf_memory_shutdown(void);

// ============================================================================
// Global Allocator Functions
// ============================================================================

VF_API void *vf_alloc(usize size, VF_MemoryTag tag);
VF_API void *vf_alloc_aligned(usize size, usize alignment, VF_MemoryTag tag);
VF_API void *vf_realloc(void *ptr, usize old_size, usize new_size,
                        VF_MemoryTag tag);
VF_API void vf_free(void *ptr, usize size, VF_MemoryTag tag);

// ============================================================================
// Convenience Macros
// ============================================================================

#define VF_NEW(type, tag) ((type *)vf_alloc(sizeof(type), (tag)))

#define VF_NEW_ARRAY(type, count, tag)                                         \
  ((type *)vf_alloc(sizeof(type) * (count), (tag)))

#define VF_DELETE(ptr, type, tag)                                              \
  do {                                                                         \
    vf_free((ptr), sizeof(type), (tag));                                       \
    (ptr) = NULL;                                                              \
  } while (0)

#define VF_DELETE_ARRAY(ptr, type, count, tag)                                 \
  do {                                                                         \
    vf_free((ptr), sizeof(type) * (count), (tag));                             \
    (ptr) = NULL;                                                              \
  } while (0)

// ============================================================================
// Memory Utilities
// ============================================================================

VF_API void vf_mem_copy(void *dst, const void *src, usize size);
VF_API void vf_mem_move(void *dst, const void *src, usize size);
VF_API void vf_mem_set(void *dst, u8 value, usize size);
VF_API void vf_mem_zero(void *dst, usize size);
VF_API i32 vf_mem_compare(const void *a, const void *b, usize size);

// ============================================================================
// Statistics
// ============================================================================

VF_API void vf_memory_get_stats(VF_AllocationStats *out_stats);
VF_API u64 vf_memory_get_usage(void);
VF_API u64 vf_memory_get_peak_usage(void);
VF_API u64 vf_memory_get_tag_usage(VF_MemoryTag tag);

// ============================================================================
// Debug
// ============================================================================

#ifdef VF_DEBUG
VF_API void vf_memory_dump_leaks(void);
VF_API void vf_memory_validate_heap(void);
#endif

#ifdef __cplusplus
}
#endif

#endif // VOXELFORGE_MEMORY_API_H
