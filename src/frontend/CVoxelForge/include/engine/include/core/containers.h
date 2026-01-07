// include/core/containers.h
//
// Purpose: Defines generic dynamic data structures including `Vector`, `Queue`,
// and `Stack`, along with their associated APIs for common operations. This file
// provides type-agnostic implementations of these fundamental containers for
// use throughout the engine. For hash map functionality, refer to `include/containers/hashmap.h`.
//
// Public APIs:
// - `Vector`: A dynamic array structure and functions (`vector_create`, `vector_destroy`,
//   `vector_push`, `vector_pop`, `vector_get`, `vector_set`, `vector_clear`, `vector_size`)
//   for managing a growable array of `void*` elements.
// - `Queue`: A circular buffer queue structure and functions (`queue_create`, `queue_destroy`,
//   `queue_enqueue`, `queue_dequeue`, `queue_is_empty`, `queue_size`) for FIFO data management.
// - `Stack`: A stack structure and functions (`stack_create`, `stack_destroy`,
//   `stack_push`, `stack_pop`, `stack_is_empty`, `stack_size`) for LIFO data management.
//
// Roadmap: docs/CONTAINERS_ROADMAP.md.
//
// Ownership: Each container (Vector, Queue, Stack) manages its own internal memory.
// Users are responsible for creating and destroying these containers to prevent memory leaks.
// Elements stored are typically `void*` pointers; management of the memory pointed to
// by these elements is the responsibility of the user.
//
// Invariants:
// - All containers must be properly created and destroyed.
// - Capacity limits for `Queue` and `Stack` must be respected.
// - `Vector` automatically resizes, but careful usage can minimize reallocations.
//
#ifndef CONTAINERS_H
#define CONTAINERS_H

#include "../common.h"

typedef struct {
    void **elements;
    u32 count;
    u32 capacity;
} Vector;

typedef struct {
    void *data;
    u32 front;
    u32 rear;
    u32 count;
    u32 capacity;
    u32 element_size;
} Queue;

typedef struct {
    void *data;
    u32 top;
    u32 capacity;
    u32 element_size;
} Stack;

// HashMap functions are defined in core/hashmap.h


#endif
