#ifndef CORE_MEMORY_STACK_ALLOCATOR_H
#define CORE_MEMORY_STACK_ALLOCATOR_H

#include <stddef.h>
#include <stdbool.h>
#include <stdint.h>
#include <stdio.h>

#ifdef __cplusplus
extern "C" {
#endif

typedef enum StackSide {
  STACK_SIDE_BACK = 0,
  STACK_SIDE_FRONT = 1
} StackSide;

typedef struct StackAllocator StackAllocator;

typedef struct StackMarker {
  size_t offset;
  StackSide side;
} StackMarker;

StackAllocator *stack_create(size_t size);
void stack_destroy(StackAllocator *stack);

void *stack_push(StackAllocator *stack, size_t size);
void *stack_push_aligned(StackAllocator *stack, size_t size, size_t alignment);
void *stack_push_side(StackAllocator *stack, size_t size, size_t alignment,
                      StackSide side);

bool stack_pop(StackAllocator *stack, size_t size);
bool stack_pop_side(StackAllocator *stack, size_t size, StackSide side);

StackMarker stack_marker_get(const StackAllocator *stack, StackSide side);
bool stack_marker_rewind(StackAllocator *stack, StackMarker marker);

StackAllocator *stack_thread_local_get(void);
void stack_thread_local_set(StackAllocator *stack);

size_t stack_bytes_used(const StackAllocator *stack);
size_t stack_bytes_remaining(const StackAllocator *stack);
void stack_debug_dump(const StackAllocator *stack, FILE *out);

bool stack_self_test(void);

#ifdef __cplusplus
}
#endif

#endif
