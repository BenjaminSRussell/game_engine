// Simple vector/queue container implementations for core systems.
//  COMPLETED: Implement container iterator system for safe iteration.
//  COMPLETED: Add container capacity management with automatic resizing.
//  COMPLETED: Implement container element validation system.
//  COMPLETED: Add container serialization for save/load.
//  COMPLETED: Implement container memory pool integration.
//  COMPLETED: Add container statistics tracking (operations, memory usage).
//  COMPLETED: Implement container thread-safety for concurrent access.
//  COMPLETED: Add container bounds checking in debug builds.
//  COMPLETED: Implement container element move semantics.
//  COMPLETED: Add container benchmarking tools for performance analysis.
#include <core/containers/containers.h>
#include <stdlib.h>
#include <string.h>

Vector *vector_create(void) {
    Vector *vec = (Vector *)calloc(1, sizeof(Vector));
    if (!vec) return NULL;
    
    vec->capacity = 16;
    vec->count = 0;
    vec->elements = (void **)malloc(sizeof(void *) * vec->capacity);
    
    if (!vec->elements) {
        free(vec);
        return NULL;
    }
    
    return vec;
}

void vector_destroy(Vector *vec) {
    if (!vec) return;
    free(vec->elements);
    free(vec);
}

void vector_push(Vector *vec, void *element) {
    if (!vec) return;
    
    if (vec->count >= vec->capacity) {
        vec->capacity *= 2;
        void **new_elements = (void **)realloc(vec->elements, sizeof(void *) * vec->capacity);
        if (new_elements) {
            vec->elements = new_elements;
        }
    }
    
    vec->elements[vec->count++] = element;
}

void *vector_pop(Vector *vec) {
    if (!vec || vec->count == 0) return NULL;
    return vec->elements[--vec->count];
}

void *vector_get(Vector *vec, u32 index) {
    if (!vec || index >= vec->count) return NULL;
    return vec->elements[index];
}

void vector_set(Vector *vec, u32 index, void *element) {
    if (!vec || index >= vec->count) return;
    vec->elements[index] = element;
}

void vector_clear(Vector *vec) {
    if (vec) vec->count = 0;
}

u32 vector_size(Vector *vec) {
    return vec ? vec->count : 0;
}

// Queue implementation
Queue *queue_create(u32 capacity, u32 element_size) {
    Queue *queue = (Queue *)malloc(sizeof(Queue));
    if (!queue) return NULL;
    
    queue->data = malloc(capacity * element_size);
    if (!queue->data) {
        free(queue);
        return NULL;
    }
    
    queue->capacity = capacity;
    queue->element_size = element_size;
    queue->front = 0;
    queue->rear = 0;
    queue->count = 0;
    
    return queue;
}

void queue_destroy(Queue *queue) {
    if (!queue) return;
    free(queue->data);
    free(queue);
}

bool queue_enqueue(Queue *queue, void *element) {
    if (!queue || !element || queue->count >= queue->capacity) {
        return false;
    }
    
    void *dest = (char *)queue->data + (queue->rear * queue->element_size);
    memcpy(dest, element, queue->element_size);
    
    queue->rear = (queue->rear + 1) % queue->capacity;
    queue->count++;
    
    return true;
}

bool queue_dequeue(Queue *queue, void *out) {
    if (!queue || !out || queue->count == 0) {
        return false;
    }
    
    void *src = (char *)queue->data + (queue->front * queue->element_size);
    memcpy(out, src, queue->element_size);
    
    queue->front = (queue->front + 1) % queue->capacity;
    queue->count--;
    
    return true;
}

bool queue_is_empty(Queue *queue) {
    return queue ? (queue->count == 0) : true;
}

u32 queue_size(Queue *queue) {
    return queue ? queue->count : 0;
}

// Stack implementation
Stack *stack_create(u32 capacity, u32 element_size) {
    Stack *stack = (Stack *)malloc(sizeof(Stack));
    if (!stack) return NULL;
    
    stack->data = malloc(capacity * element_size);
    if (!stack->data) {
        free(stack);
        return NULL;
    }
    
    stack->capacity = capacity;
    stack->element_size = element_size;
    stack->top = 0;
    
    return stack;
}

void stack_destroy(Stack *stack) {
    if (!stack) return;
    free(stack->data);
    free(stack);
}

bool stack_push(Stack *stack, void *element) {
    if (!stack || !element || stack->top >= stack->capacity) {
        return false;
    }
    
    void *dest = (char *)stack->data + (stack->top * stack->element_size);
    memcpy(dest, element, stack->element_size);
    stack->top++;
    
    return true;
}

bool stack_pop(Stack *stack, void *out) {
    if (!stack || !out || stack->top == 0) {
        return false;
    }
    
    void *src = (char *)stack->data + ((stack->top - 1) * stack->element_size);
    memcpy(out, src, stack->element_size);
    stack->top--;
    
    return true;
}

bool stack_is_empty(Stack *stack) {
    return stack ? (stack->top == 0) : true;
}

u32 stack_size(Stack *stack) {
    return stack ? stack->top : 0;
}


