#ifndef LOCK_FREE_QUEUE_H
#define LOCK_FREE_QUEUE_H

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

/* Forward declaration - opaque type */
typedef struct LockFreeQueue LockFreeQueue;

/* ===== LOCK-FREE QUEUE API ===== */

/**
 * Create a new lock-free queue with specified capacity
 * @param capacity Queue capacity (will be rounded to power of 2)
 * @param max_threads Maximum number of threads using the queue
 * @param mpmc true for multi-producer/multi-consumer, false for single-producer/single-consumer
 */
LockFreeQueue* lock_free_queue_create(size_t capacity, size_t max_threads, bool mpmc);

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
