#include "assets/resources/resource_management/resource_lifetime.h"
#include <stdlib.h>
#include <stdio.h>

#define MAX_PENDING_DELETES_PER_FRAME 1024
#define MAX_FRAME_LATENCY 3  // Triple buffering max

typedef struct {
    resource_handle_t handle;
    resource_destroy_fn destroy_fn;
    void* ctx;
} pending_delete_t;

typedef struct {
    pending_delete_t items[MAX_PENDING_DELETES_PER_FRAME];
    uint32_t count;
} frame_deletion_queue_t;

static struct {
    frame_deletion_queue_t queues[MAX_FRAME_LATENCY];
    uint32_t current_frame_index;
    uint32_t frame_latency;
    bool initialized;
} g_lifetime_ctx = {0};

void resource_lifetime_init(uint32_t frame_latency) {
    if (frame_latency > MAX_FRAME_LATENCY) frame_latency = MAX_FRAME_LATENCY;
    if (frame_latency < 1) frame_latency = 1;

    g_lifetime_ctx.frame_latency = frame_latency;
    g_lifetime_ctx.current_frame_index = 0;
    
    for (uint32_t i = 0; i < MAX_FRAME_LATENCY; ++i) {
        g_lifetime_ctx.queues[i].count = 0;
    }
    
    g_lifetime_ctx.initialized = true;
}

static void process_queue(uint32_t queue_index) {
    frame_deletion_queue_t* queue = &g_lifetime_ctx.queues[queue_index];
    
    for (uint32_t i = 0; i < queue->count; ++i) {
        pending_delete_t* item = &queue->items[i];
        if (item->destroy_fn) {
            item->destroy_fn(item->ctx, item->handle);
        }
    }
    
    queue->count = 0;
}

void resource_lifetime_shutdown(void) {
    if (!g_lifetime_ctx.initialized) return;

    // Flush all queues
    for (uint32_t i = 0; i < MAX_FRAME_LATENCY; ++i) {
        process_queue(i);
    }
    
    g_lifetime_ctx.initialized = false;
}

void resource_lifetime_next_frame(void) {
    if (!g_lifetime_ctx.initialized) return;

    // Identify the queue for the next frame that we are about to overwrite.
    // In a ring of size N, if we are at index I, the queue at (I) contains items
    // from N frames ago (since we cycle through them).
    // So we process the current index before making it the new "current" frame's bucket.
    
    // Advance frame index first, then process the queue we are about to reuse.
    // This ensures that the queue we process contains items from 'frame_latency' frames ago.
    g_lifetime_ctx.current_frame_index = (g_lifetime_ctx.current_frame_index + 1) % g_lifetime_ctx.frame_latency;
    
    // Process the queue at the new index (which holds old data) before we start adding new data to it.
    process_queue(g_lifetime_ctx.current_frame_index);
}

void resource_lifetime_defer_free(resource_handle_t handle, resource_destroy_fn destroy_fn, void* ctx) {
    if (!g_lifetime_ctx.initialized) {
        // Fallback: immediate free if system not init (or leak? better to free)
        if (destroy_fn) destroy_fn(ctx, handle);
        return;
    }

    // Add to current frame's queue. These will be processed when we wrap around back to this index.
    // e.g., if latency is 3. We are at frame 0. We add to queue 0.
    // Frame 1: process queue 1 (empty), add to queue 1.
    // Frame 2: process queue 2 (empty), add to queue 2.
    // Frame 3: process queue 0 (EXECUTE DELETES from frame 0), add to new queue 0.
    
    frame_deletion_queue_t* queue = &g_lifetime_ctx.queues[g_lifetime_ctx.current_frame_index];
    
    if (queue->count >= MAX_PENDING_DELETES_PER_FRAME) {
        // Emergency overflow: execute immediately or warn.
        // Immediate execution is unsafe for GPU, but better than memory leak?
        // Ideally we'd have a dynamic list, but fixed array is simpler/faster.
        if (destroy_fn) destroy_fn(ctx, handle);
        return;
    }

    pending_delete_t* item = &queue->items[queue->count++];
    item->handle = handle;
    item->destroy_fn = destroy_fn;
    item->ctx = ctx;
}
