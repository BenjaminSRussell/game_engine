/**
 * @file memory.c
 * @brief Core memory system implementation
 */

#include "Core/Public/Memory/memory_api.h"
#include <stdlib.h>
#include <string.h>

// ============================================================================
// Internal State
// ============================================================================

static struct {
  b8 initialized;
  VF_AllocationStats stats;
} g_memory_state = {0};

// ============================================================================
// Initialization
// ============================================================================

VF_Result vf_memory_init(void) {
  if (g_memory_state.initialized) {
    return VF_ERROR_ALREADY_EXISTS;
  }

  vf_mem_zero(&g_memory_state.stats, sizeof(g_memory_state.stats));
  g_memory_state.initialized = true;

  return VF_SUCCESS;
}

void vf_memory_shutdown(void) {
  if (!g_memory_state.initialized) {
    return;
  }

#ifdef VF_DEBUG
  if (g_memory_state.stats.current_usage > 0) {
    vf_memory_dump_leaks();
  }
#endif

  g_memory_state.initialized = false;
}

// ============================================================================
// Allocation Functions
// ============================================================================

void *vf_alloc(usize size, VF_MemoryTag tag) {
  return vf_alloc_aligned(size, 16, tag);
}

void *vf_alloc_aligned(usize size, usize alignment, VF_MemoryTag tag) {
  if (size == 0) {
    return NULL;
  }

  // Use platform aligned alloc
  void *ptr = NULL;

#if defined(VF_PLATFORM_WINDOWS)
  ptr = _aligned_malloc(size, alignment);
#else
  if (posix_memalign(&ptr, alignment, size) != 0) {
    ptr = NULL;
  }
#endif

  if (ptr) {
    g_memory_state.stats.total_allocated += size;
    g_memory_state.stats.current_usage += size;
    g_memory_state.stats.allocation_count++;
    g_memory_state.stats.tagged_usage[tag] += size;

    if (g_memory_state.stats.current_usage > g_memory_state.stats.peak_usage) {
      g_memory_state.stats.peak_usage = g_memory_state.stats.current_usage;
    }
  }

  return ptr;
}

void *vf_realloc(void *ptr, usize old_size, usize new_size, VF_MemoryTag tag) {
  if (ptr == NULL) {
    return vf_alloc(new_size, tag);
  }

  if (new_size == 0) {
    vf_free(ptr, old_size, tag);
    return NULL;
  }

  void *new_ptr = vf_alloc(new_size, tag);
  if (new_ptr) {
    vf_mem_copy(new_ptr, ptr, VF_MIN(old_size, new_size));
    vf_free(ptr, old_size, tag);
  }

  return new_ptr;
}

void vf_free(void *ptr, usize size, VF_MemoryTag tag) {
  if (ptr == NULL) {
    return;
  }

#if defined(VF_PLATFORM_WINDOWS)
  _aligned_free(ptr);
#else
  free(ptr);
#endif

  g_memory_state.stats.total_freed += size;
  g_memory_state.stats.current_usage -= size;
  g_memory_state.stats.free_count++;
  g_memory_state.stats.tagged_usage[tag] -= size;
}

// ============================================================================
// Memory Utilities
// ============================================================================

void vf_mem_copy(void *dst, const void *src, usize size) {
  memcpy(dst, src, size);
}

void vf_mem_move(void *dst, const void *src, usize size) {
  memmove(dst, src, size);
}

void vf_mem_set(void *dst, u8 value, usize size) { memset(dst, value, size); }

void vf_mem_zero(void *dst, usize size) { memset(dst, 0, size); }

i32 vf_mem_compare(const void *a, const void *b, usize size) {
  return memcmp(a, b, size);
}

// ============================================================================
// Statistics
// ============================================================================

void vf_memory_get_stats(VF_AllocationStats *out_stats) {
  if (out_stats) {
    *out_stats = g_memory_state.stats;
  }
}

u64 vf_memory_get_usage(void) { return g_memory_state.stats.current_usage; }

u64 vf_memory_get_peak_usage(void) { return g_memory_state.stats.peak_usage; }

u64 vf_memory_get_tag_usage(VF_MemoryTag tag) {
  if (tag < VF_MEMORY_TAG_COUNT) {
    return g_memory_state.stats.tagged_usage[tag];
  }
  return 0;
}

// ============================================================================
// Debug
// ============================================================================

#ifdef VF_DEBUG
void vf_memory_dump_leaks(void) {
  // TODO: Implement detailed leak tracking
  if (g_memory_state.stats.current_usage > 0) {
    // Log leak information
  }
}

void vf_memory_validate_heap(void) {
  // TODO: Implement heap validation
}
#endif
