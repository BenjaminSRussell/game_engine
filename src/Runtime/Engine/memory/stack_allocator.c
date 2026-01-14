#include "core/memory/stack_allocator.h"

#include <stdlib.h>
#include <string.h>

struct StackAllocator {
  uint8_t *buffer;
  size_t size;
  size_t back_offset;
  size_t front_offset;
  bool owns_buffer;
};

static _Thread_local StackAllocator *g_stack_tls = NULL;

static size_t align_up(size_t value, size_t alignment) {
  if (alignment == 0) {
    return value;
  }
  size_t mask = alignment - 1;
  return (value + mask) & ~mask;
}

static size_t align_down(size_t value, size_t alignment) {
  if (alignment == 0) {
    return value;
  }
  size_t mask = alignment - 1;
  return value & ~mask;
}

StackAllocator *stack_create(size_t size) {
  StackAllocator *stack = (StackAllocator *)calloc(1, sizeof(StackAllocator));
  if (!stack) {
    return NULL;
  }

  stack->buffer = (uint8_t *)malloc(size);
  if (!stack->buffer) {
    free(stack);
    return NULL;
  }

  stack->size = size;
  stack->back_offset = 0;
  stack->front_offset = 0;
  stack->owns_buffer = true;
  return stack;
}

void stack_destroy(StackAllocator *stack) {
  if (!stack) {
    return;
  }

  if (stack->owns_buffer) {
    free(stack->buffer);
  }
  free(stack);
}

void *stack_push(StackAllocator *stack, size_t size) {
  return stack_push_side(stack, size, 16u, STACK_SIDE_BACK);
}

void *stack_push_aligned(StackAllocator *stack, size_t size, size_t alignment) {
  return stack_push_side(stack, size, alignment, STACK_SIDE_BACK);
}

void *stack_push_side(StackAllocator *stack, size_t size, size_t alignment,
                      StackSide side) {
  if (!stack || size == 0) {
    return NULL;
  }

  if (alignment == 0) {
    alignment = 16u;
  }

  if (side == STACK_SIDE_BACK) {
    size_t aligned_offset = align_up(stack->back_offset, alignment);
    size_t new_offset = aligned_offset + size;
    if (new_offset + stack->front_offset > stack->size) {
      return NULL;
    }
    stack->back_offset = new_offset;
    return stack->buffer + aligned_offset;
  }

  size_t raw_end = stack->size - stack->front_offset;
  if (raw_end < size) {
    return NULL;
  }

  size_t aligned_start = align_down(raw_end - size, alignment);
  if (aligned_start < stack->back_offset) {
    return NULL;
  }

  stack->front_offset = stack->size - aligned_start;
  return stack->buffer + aligned_start;
}

bool stack_pop(StackAllocator *stack, size_t size) {
  return stack_pop_side(stack, size, STACK_SIDE_BACK);
}

bool stack_pop_side(StackAllocator *stack, size_t size, StackSide side) {
  if (!stack) {
    return false;
  }

  if (side == STACK_SIDE_BACK) {
    if (size > stack->back_offset) {
      return false;
    }
    stack->back_offset -= size;
    return true;
  }

  if (size > stack->front_offset) {
    return false;
  }
  stack->front_offset -= size;
  return true;
}

StackMarker stack_marker_get(const StackAllocator *stack, StackSide side) {
  StackMarker marker;
  marker.side = side;
  marker.offset = 0;

  if (!stack) {
    return marker;
  }

  marker.offset = (side == STACK_SIDE_BACK) ? stack->back_offset
                                            : stack->front_offset;
  return marker;
}

bool stack_marker_rewind(StackAllocator *stack, StackMarker marker) {
  if (!stack) {
    return false;
  }

  if (marker.side == STACK_SIDE_BACK) {
    if (marker.offset > stack->size - stack->front_offset) {
      return false;
    }
    stack->back_offset = marker.offset;
    return true;
  }

  if (marker.offset > stack->size - stack->back_offset) {
    return false;
  }
  stack->front_offset = marker.offset;
  return true;
}

StackAllocator *stack_thread_local_get(void) { return g_stack_tls; }

void stack_thread_local_set(StackAllocator *stack) { g_stack_tls = stack; }

size_t stack_bytes_used(const StackAllocator *stack) {
  if (!stack) {
    return 0;
  }
  return stack->back_offset + stack->front_offset;
}

size_t stack_bytes_remaining(const StackAllocator *stack) {
  if (!stack || stack->size < stack_bytes_used(stack)) {
    return 0;
  }
  return stack->size - stack_bytes_used(stack);
}

void stack_debug_dump(const StackAllocator *stack, FILE *out) {
  if (!stack || !out) {
    return;
  }

  fprintf(out,
          "StackAllocator size=%zu used=%zu remaining=%zu back=%zu front=%zu\n",
          stack->size, stack_bytes_used(stack),
          stack_bytes_remaining(stack), stack->back_offset, stack->front_offset);
}

bool stack_self_test(void) {
  StackAllocator *stack = stack_create(256);
  if (!stack) {
    return false;
  }

  void *a = stack_push_aligned(stack, 32, 16);
  void *b = stack_push_side(stack, 32, 16, STACK_SIDE_FRONT);
  if (!a || !b) {
    stack_destroy(stack);
    return false;
  }

  StackMarker marker = stack_marker_get(stack, STACK_SIDE_BACK);
  if (!stack_marker_rewind(stack, marker)) {
    stack_destroy(stack);
    return false;
  }

  bool pop_ok = stack_pop(stack, 16);
  stack_destroy(stack);
  return pop_ok;
}
