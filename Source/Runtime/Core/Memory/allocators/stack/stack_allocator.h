#ifndef STACK_ALLOCATOR_H
#define STACK_ALLOCATOR_H

#include <stdbool.h>
#include <stddef.h>

// Stack allocator for LIFO allocations
// Supports markers for bulk deallocation
typedef struct StackAllocator {
  void *buffer;
  size_t size;
  size_t offset;
  bool owns_memory;
} StackAllocator;

// Marker for stack position
typedef size_t StackMarker;

// Create stack with specified size
StackAllocator *stack_create(size_t size);

// Create stack from existing buffer
StackAllocator *stack_create_from_buffer(void *buffer, size_t size);

// Allocate from stack
void *stack_alloc(StackAllocator *stack, size_t size);

// Allocate with alignment
void *stack_alloc_aligned(StackAllocator *stack, size_t size, size_t alignment);

// Get current marker position
StackMarker stack_get_marker(StackAllocator *stack);

// Free to marker (deallocates everything after marker)
void stack_free_to_marker(StackAllocator *stack, StackMarker marker);

// Clear entire stack
void stack_clear(StackAllocator *stack);

// Get current offset
size_t stack_get_offset(StackAllocator *stack);

// Get remaining space
size_t stack_get_remaining(StackAllocator *stack);

// Destroy stack
void stack_destroy(StackAllocator *stack);

#endif // STACK_ALLOCATOR_H
