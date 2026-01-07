#ifndef CORE_CONTAINERS_ADVANCED_PRIORITY_QUEUE_H
#define CORE_CONTAINERS_ADVANCED_PRIORITY_QUEUE_H

#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>

#ifdef __cplusplus
extern "C" {
#endif

typedef struct PriorityQueue PriorityQueue;

PriorityQueue *pqueue_create(size_t capacity, uint32_t arity);
PriorityQueue *pqueue_create_fibonacci(size_t capacity);
void pqueue_destroy(PriorityQueue *queue);

bool pqueue_push(PriorityQueue *queue, void *item, double priority);
void *pqueue_pop(PriorityQueue *queue, double *out_priority);
bool pqueue_update_priority(PriorityQueue *queue, void *item, double priority);

size_t pqueue_size(const PriorityQueue *queue);

bool pqueue_self_test(void);

#ifdef __cplusplus
}
#endif

#endif
