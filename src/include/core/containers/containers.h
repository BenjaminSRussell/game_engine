#ifndef CORE_CONTAINERS_CONTAINERS_H
#define CORE_CONTAINERS_CONTAINERS_H

#include "engine/include/common.h"
#include <stdbool.h>

// Vector
typedef struct {
    void **elements;
    u32 capacity;
    u32 count;
} Vector;

Vector *vector_create(void);
void vector_destroy(Vector *vec);
void vector_push(Vector *vec, void *element);
void *vector_pop(Vector *vec);
void *vector_get(Vector *vec, u32 index);
void vector_set(Vector *vec, u32 index, void *element);
void vector_clear(Vector *vec);
u32 vector_size(Vector *vec);

// Queue
typedef struct {
    void *data;
    u32 capacity;
    u32 element_size;
    u32 front;
    u32 rear;
    u32 count;
} Queue;

Queue *queue_create(u32 capacity, u32 element_size);
void queue_destroy(Queue *queue);
bool queue_enqueue(Queue *queue, void *element);
bool queue_dequeue(Queue *queue, void *out);
bool queue_is_empty(Queue *queue);
u32 queue_size(Queue *queue);

// Stack
typedef struct {
    void *data;
    u32 capacity;
    u32 element_size;
    u32 top;
} Stack;

Stack *stack_create(u32 capacity, u32 element_size);
void stack_destroy(Stack *stack);
bool stack_push(Stack *stack, void *element);
bool stack_pop(Stack *stack, void *out);
bool stack_is_empty(Stack *stack);
u32 stack_size(Stack *stack);

#endif // CORE_CONTAINERS_CONTAINERS_H
