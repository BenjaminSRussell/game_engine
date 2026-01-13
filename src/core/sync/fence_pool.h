/*
 * fence_pool.h
 * Fence allocation and tracking
 */

#ifndef FENCE_POOL_H
#define FENCE_POOL_H

#include <stdint.h>
#include <stdbool.h>

typedef struct fence {
    void* backend_handle;
    uint64_t signal_value;
    bool signaled;
} fence_t;

typedef struct fence_pool fence_pool_t;

// Initialization
fence_pool_t* fence_pool_create(void);
void fence_pool_destroy(fence_pool_t* pool);

// Allocation
fence_t* fence_pool_acquire(fence_pool_t* pool);
void fence_pool_release(fence_pool_t* pool, fence_t* fence);

// Operations
void fence_wait(fence_t* fence, uint64_t timeout_ns);
void fence_reset(fence_t* fence);
bool fence_is_signaled(fence_t* fence);

#endif // FENCE_POOL_H
