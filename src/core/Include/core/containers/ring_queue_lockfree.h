#ifndef CORE_CONTAINERS_RING_QUEUE_LOCKFREE_H
#define CORE_CONTAINERS_RING_QUEUE_LOCKFREE_H

#include <stdbool.h>
#include <stddef.h>

typedef struct LockFreeRingQueue LockFreeRingQueue;

typedef struct {
    size_t enqueue_count;
    size_t dequeue_count;
    size_t overflow_count;
    size_t underflow_count;
} RingQueueStats;

LockFreeRingQueue* ring_queue_create(size_t capacity, bool allow_overwrite);
void ring_queue_destroy(LockFreeRingQueue* queue);

bool ring_queue_enqueue(LockFreeRingQueue* queue, void* data);
void* ring_queue_dequeue(LockFreeRingQueue* queue);
bool ring_queue_try_dequeue(LockFreeRingQueue* queue, void** data);
void* ring_queue_peek(const LockFreeRingQueue* queue);

bool ring_queue_is_empty(const LockFreeRingQueue* queue);
bool ring_queue_is_full(const LockFreeRingQueue* queue);
size_t ring_queue_size(const LockFreeRingQueue* queue);
size_t ring_queue_capacity(const LockFreeRingQueue* queue);
void ring_queue_clear(LockFreeRingQueue* queue);

void ring_queue_get_stats(const LockFreeRingQueue* queue, RingQueueStats* stats);

size_t ring_queue_enqueue_batch(LockFreeRingQueue* queue, void** data, size_t count);
size_t ring_queue_dequeue_batch(LockFreeRingQueue* queue, void** data, size_t count);

// MPSC
typedef struct MPSCRingQueue MPSCRingQueue;

MPSCRingQueue* mpsc_ring_queue_create(size_t capacity);
void mpsc_ring_queue_destroy(MPSCRingQueue* queue);
bool mpsc_ring_queue_enqueue(MPSCRingQueue* queue, void* data);
void* mpsc_ring_queue_dequeue(MPSCRingQueue* queue);

#endif // CORE_CONTAINERS_RING_QUEUE_LOCKFREE_H
