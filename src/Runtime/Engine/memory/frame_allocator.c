#include "core/memory/memory_macros.h"
#include "core/memory/unified_memory_allocator.h"
#include <stdio.h>

// ============================================================================
// FRAME ALLOCATOR - For per-frame temporary allocations
// ============================================================================

static StackAllocator *g_frame_allocator = NULL;

bool frame_allocator_init(void) {
  // 16MB stack for per-frame allocations
  g_frame_allocator = unified_memory_stack_create(16 * 1024 * 1024);

  if (!g_frame_allocator) {
    fprintf(stderr, "Failed to create frame allocator\n");
    return false;
  }

  printf("Frame allocator initialized: 16MB stack\n");
  return true;
}

void frame_allocator_shutdown(void) {
  if (g_frame_allocator) {
    unified_memory_stack_destroy(g_frame_allocator);
    g_frame_allocator = NULL;
  }
}

void *frame_alloc(size_t size) {
  if (!g_frame_allocator) {
    fprintf(stderr, "Frame allocator not initialized\n");
    return NULL;
  }

  return unified_memory_stack_alloc(g_frame_allocator, size, 0);
}

void *frame_alloc_zero(size_t size) {
  if (!g_frame_allocator) {
    fprintf(stderr, "Frame allocator not initialized\n");
    return NULL;
  }

  return unified_memory_stack_alloc(g_frame_allocator, size, MEMORY_FLAG_ZERO);
}

void frame_allocator_reset(void) {
  if (g_frame_allocator) {
    unified_memory_stack_reset(g_frame_allocator);
  }
}

size_t frame_allocator_get_usage(void) {
  if (!g_frame_allocator) {
    return 0;
  }
  return g_frame_allocator->current_offset;
}

size_t frame_allocator_get_peak(void) {
  if (!g_frame_allocator) {
    return 0;
  }
  return g_frame_allocator->peak_offset;
}

void frame_allocator_print_stats(void) {
  if (!g_frame_allocator) {
    printf("Frame allocator not initialized\n");
    return;
  }

  printf("\n=== Frame Allocator Statistics ===\n");
  printf("Current usage: %.2f MB / %.2f MB\n",
         g_frame_allocator->current_offset / (1024.0 * 1024.0),
         g_frame_allocator->buffer_size / (1024.0 * 1024.0));
  printf("Peak usage: %.2f MB\n",
         g_frame_allocator->peak_offset / (1024.0 * 1024.0));
  printf("==================================\n\n");
}
