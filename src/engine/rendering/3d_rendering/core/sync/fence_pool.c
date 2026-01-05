/*
 * fence_pool.c
 * Implementation of fence pool management
 */

#include "fence_pool.h"
#include <stdlib.h>

struct fence_pool {
    // Simple implementation: just keep track of allocated count or a free list
    // For this phase, we'll alloc on demand
    uint32_t active_count;
};

fence_pool_t* fence_pool_create(void) {
    fence_pool_t* pool = (fence_pool_t*)malloc(sizeof(fence_pool_t));
    if (pool) {
        pool->active_count = 0;
    }
    return pool;
}

void fence_pool_destroy(fence_pool_t* pool) {
    if (pool) {
        free(pool);
    }
}

fence_t* fence_pool_acquire(fence_pool_t* pool) {
    if (!pool) return NULL;

    fence_t* fence = (fence_t*)malloc(sizeof(fence_t));
    if (fence) {
        // Init backend fence
        fence->backend_handle = (void*)0xFE11C3;
        fence->signal_value = 0;
        fence->signaled = false;
        pool->active_count++;
    }
    return fence;
}

void fence_pool_release(fence_pool_t* pool, fence_t* fence) {
    if (!pool || !fence) return;

    // Backend destroy or cache for reuse
    free(fence);
    pool->active_count--;
}

void fence_wait(fence_t* fence, uint64_t timeout_ns) {
    if (!fence) return;
    // backend_fence_wait(fence->backend_handle, timeout_ns);
}

void fence_reset(fence_t* fence) {
    if (!fence) return;
    // backend_fence_reset(fence->backend_handle);
    fence->signaled = false;
}

bool fence_is_signaled(fence_t* fence) {
    if (!fence) return false;
    // return backend_fence_status(fence->backend_handle) == SIGNALED;
    return fence->signaled;
}
