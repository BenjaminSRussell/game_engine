/*
 * frame_sync.c
 * Implementation of frame-in-flight synchronization
 */

#include "frame_sync.h"
#include <stdlib.h>

struct frame_sync {
    uint32_t frame_index;
    uint32_t max_frames;
    
    fence_t* in_flight_fences[MAX_FRAMES_IN_FLIGHT];
    semaphore_t* image_available_semaphores[MAX_FRAMES_IN_FLIGHT];
    semaphore_t* render_finished_semaphores[MAX_FRAMES_IN_FLIGHT];
    
    fence_pool_t* fence_pool; // internal ownership
};

frame_sync_t* frame_sync_create(uint32_t max_frames) {
    if (max_frames > MAX_FRAMES_IN_FLIGHT) max_frames = MAX_FRAMES_IN_FLIGHT;
    
    frame_sync_t* sync = (frame_sync_t*)malloc(sizeof(frame_sync_t));
    if (!sync) return NULL;

    sync->frame_index = 0;
    sync->max_frames = max_frames;
    
    // Create internal pool (or could take one as arg)
    sync->fence_pool = fence_pool_create();

    for (uint32_t i = 0; i < max_frames; ++i) {
        sync->in_flight_fences[i] = fence_pool_acquire(sync->fence_pool);
        // Start signaled so first wait doesn't block? Or reset on end?
        // Usually fences are creaeted specific to signaled state. 
        // Here assuming we only wait if previously submitted. 
        // For simplicity, let's assume fence_acquire returns unsignaled.
        // We'll manually signal it initially or skip wait on first frame frame logic
        // Standard vulkan: create fence signaled for first frame wait.
        ((fence_t*)sync->in_flight_fences[i])->signaled = true; // Hack: direct access or API? API doesn't have set signaled.

        sync->image_available_semaphores[i] = semaphore_create(0);
        sync->render_finished_semaphores[i] = semaphore_create(0);
    }
    
    return sync;
}

void frame_sync_destroy(frame_sync_t* sync) {
    if (!sync) return;
    
    for (uint32_t i = 0; i < sync->max_frames; ++i) {
        // fence_pool_release(sync->fence_pool, sync->in_flight_fences[i]); // Requires correct pool logic
        semaphore_destroy(sync->image_available_semaphores[i]);
        semaphore_destroy(sync->render_finished_semaphores[i]);
    }
    fence_pool_destroy(sync->fence_pool);
    free(sync);
}

void frame_sync_begin_frame(frame_sync_t* sync) {
    if (!sync) return;
    
    fence_t* fence = sync->in_flight_fences[sync->frame_index];
    fence_wait(fence, UINT64_MAX);
    fence_reset(fence);
}

void frame_sync_end_frame(frame_sync_t* sync) {
    if (!sync) return;
    
    sync->frame_index = (sync->frame_index + 1) % sync->max_frames;
}

uint32_t frame_sync_get_frame_index(frame_sync_t* sync) {
    return sync ? sync->frame_index : 0;
}

fence_t* frame_sync_get_current_fence(frame_sync_t* sync) {
    return sync ? sync->in_flight_fences[sync->frame_index] : NULL;
}

semaphore_t* frame_sync_get_image_available_semaphore(frame_sync_t* sync) {
    return sync ? sync->image_available_semaphores[sync->frame_index] : NULL;
}

semaphore_t* frame_sync_get_render_finished_semaphore(frame_sync_t* sync) {
    return sync ? sync->render_finished_semaphores[sync->frame_index] : NULL;
}
