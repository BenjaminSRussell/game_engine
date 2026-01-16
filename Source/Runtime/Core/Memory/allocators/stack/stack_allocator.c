// stack_allocator.c - Stack/LIFO allocator implementation
#include "stack_allocator.h"
#include <assert.h>
#include <stdlib.h>
#include <string.h>

// Align pointer to specified alignment
static inline void *align_pointer(void *ptr, size_t alignment) {
  uintptr_t addr = (uintptr_t)ptr;
  uintptr_t aligned = (addr + alignment - 1) & ~(alignment - 1);
  return (void *)aligned;
}

StackAllocator *stack_create(size_t size) {
  StackAllocator *stack = (StackAllocator *)malloc(sizeof(StackAllocator));
  if (!stack)
    return NULL;

  stack->buffer = malloc(size);
  if (!stack->buffer) {
    free(stack);
    return NULL;
  }

  stack->size = size;
  stack->offset = 0;
  stack->owns_memory = true;

  return stack;
}

StackAllocator *stack_create_from_buffer(void *buffer, size_t size) {
  if (!buffer || size == 0)
    return NULL;

  StackAllocator *stack = (StackAllocator *)malloc(sizeof(StackAllocator));
  if (!stack)
    return NULL;

  stack->buffer = buffer;
  stack->size = size;
  stack->offset = 0;
  stack->owns_memory = false;

  return stack;
}

void *stack_alloc(StackAllocator *stack, size_t size) {
  if (!stack || size == 0)
    return NULL;

  // Check if we have enough space
  if (stack->offset + size > stack->size) {
    return NULL; // Stack full
  }

  void *ptr = (char *)stack->buffer + stack->offset;
  stack->offset += size;

  // Zero the memory
  memset(ptr, 0, size);

  return ptr;
}

void *stack_alloc_aligned(StackAllocator *stack, size_t size,
                          size_t alignment) {
  if (!stack || size == 0)
    return NULL;

  // Calculate aligned offset
  void *current_ptr = (char *)stack->buffer + stack->offset;
  void *aligned_ptr = align_pointer(current_ptr, alignment);
  size_t padding = (char *)aligned_ptr - (char *)current_ptr;

  // Check if we have enough space including padding
  if (stack->offset + padding + size > stack->size) {
    return NULL; // Stack full
  }

  stack->offset += padding + size;

  // Zero the memory
  memset(aligned_ptr, 0, size);

  return aligned_ptr;
}

StackMarker stack_get_marker(StackAllocator *stack) {
  return stack ? stack->offset : 0;
}

void stack_free_to_marker(StackAllocator *stack, StackMarker marker) {
  if (!stack)
    return;

  // Ensure marker is valid
  if (marker > stack->offset) {
    return;
  }

  stack->offset = marker;
}

void stack_clear(StackAllocator *stack) {
  if (!stack)
    return;
  stack->offset = 0;
}

size_t stack_get_offset(StackAllocator *stack) {
  return stack ? stack->offset : 0;
}

size_t stack_get_remaining(StackAllocator *stack) {
  if (!stack)
    return 0;
  return stack->size - stack->offset;
}

void stack_destroy(StackAllocator *stack) {
  if (!stack)
    return;

  if (stack->owns_memory && stack->buffer) {
    free(stack->buffer);
  }

  free(stack);
}
