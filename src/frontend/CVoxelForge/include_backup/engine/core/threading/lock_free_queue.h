#pragma once

#include <common.h>
#include <core/types.h>
#include <stddef.h>
#include <stdatomic.h>

/**
 * =================================================================================================
 *                              LOCK-FREE QUEUE
 * =================================================================================================
 *
 * Purpose: Thread-safe, lock-free queue implementation using atomic operations.
 * Designed for multi-producer, multi-consumer scenarios without mutex overhead.
 */

typedef struct {
    void **buffer;
    size_t capacity;
    _Atomic(size_t) head;
    _Atomic(size_t) tail;
} LockFreeQueue;

/* ===== LOCK-FREE QUEUE API ===== */

/**
 * Create a new lock-free queue with specified capacity
 */
LockFreeQueue* lock_free_queue_create(size_t capacity);

/**
 * Destroy the queue
 */
void lock_free_queue_destroy(LockFreeQueue *queue);

/**
 * Enqueue an element (thread-safe)
 * Returns true if successful, false if queue is full
 */
bool lock_free_queue_enqueue(LockFreeQueue *queue, void *item);

/**
 * Dequeue an element (thread-safe)
 * Returns true if item was dequeued, false if queue is empty
 */
bool lock_free_queue_dequeue(LockFreeQueue *queue, void **item);

/**
 * Check if queue is empty
 */
bool lock_free_queue_is_empty(LockFreeQueue *queue);

/**
 * Check if queue is full
 */
bool lock_free_queue_is_full(LockFreeQueue *queue);

/**
 * Get current queue size
 */
size_t lock_free_queue_size(LockFreeQueue *queue);

/**
 * Get queue capacity
 */
size_t lock_free_queue_capacity(LockFreeQueue *queue);

/**
 * Clear all elements from queue
 */
void lock_free_queue_clear(LockFreeQueue *queue);

#endif /* LOCK_FREE_QUEUE_H */
