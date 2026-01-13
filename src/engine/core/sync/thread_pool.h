#pragma once

#include "engine/include/common.h"
#include <core/types.h>
#include <stddef.h>

/**
 * =================================================================================================
 *                                   THREAD POOL
 * =================================================================================================
 *
 * Purpose: Manages a pool of worker threads for executing tasks concurrently.
 * Provides task queuing, synchronization, and efficient work distribution.
 */

typedef struct ThreadPool ThreadPool;

/**
 * Thread work function signature
 */
typedef void (*ThreadWork)(void *arg);

/* ===== THREAD POOL API ===== */

/**
 * Create a new thread pool with specified number of worker threads
 */
ThreadPool* thread_pool_create(u32 num_threads);

/**
 * Destroy thread pool and wait for all pending tasks
 */
void thread_pool_destroy(ThreadPool *pool);

/**
 * Submit work to the thread pool
 */
void thread_pool_submit(ThreadPool *pool, ThreadWork work, void *arg);

/**
 * Submit work with a priority level
 */
void thread_pool_submit_priority(ThreadPool *pool, ThreadWork work, void *arg, u32 priority);

/**
 * Wait for all pending work to complete
 */
void thread_pool_wait(ThreadPool *pool);

/**
 * Get number of worker threads in the pool
 */
u32 thread_pool_get_size(ThreadPool *pool);

/**
 * Get number of pending tasks in the queue
 */
u32 thread_pool_get_pending_count(ThreadPool *pool);

/**
 * Check if all work is complete
 */
bool thread_pool_is_idle(ThreadPool *pool);

