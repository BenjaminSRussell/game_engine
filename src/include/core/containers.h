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

#include "engine/include/common.h"
#include "core/types.h"

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

// Vector API
Vector *vector_create(void);
void vector_destroy(Vector *vec);
void vector_push(Vector *vec, void *element);
void *vector_pop(Vector *vec);
void *vector_get(Vector *vec, u32 index);
void vector_set(Vector *vec, u32 index, void *element);
void vector_clear(Vector *vec);
u32 vector_size(Vector *vec);

// Queue API
Queue *queue_create(u32 capacity, u32 element_size);
void queue_destroy(Queue *queue);
bool queue_enqueue(Queue *queue, void *element);
bool queue_dequeue(Queue *queue, void *out);
bool queue_is_empty(Queue *queue);
u32 queue_size(Queue *queue);

// Stack API
Stack *stack_create(u32 capacity, u32 element_size);
void stack_destroy(Stack *stack);
bool stack_push(Stack *stack, void *element);
bool stack_pop(Stack *stack, void *out);
bool stack_is_empty(Stack *stack);
u32 stack_size(Stack *stack);


#endif
