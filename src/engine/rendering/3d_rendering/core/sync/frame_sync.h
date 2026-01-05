/*
 * frame_sync.h
 * Frame-in-flight synchronization
 */

#ifndef FRAME_SYNC_H
#define FRAME_SYNC_H

#include <stdint.h>
#include <stdbool.h>
#include "fence_pool.h"
#include "semaphore_manager.h"

#define MAX_FRAMES_IN_FLIGHT 3

typedef struct frame_sync frame_sync_t;

// Lifecycle
frame_sync_t* frame_sync_create(uint32_t max_frames);
void frame_sync_destroy(frame_sync_t* sync);

// Per-frame operations
// Call at start of frame: waits for previous frame's fence
void frame_sync_begin_frame(frame_sync_t* sync);

// Call at end of frame: signals fence and advances frame index
void frame_sync_end_frame(frame_sync_t* sync);

// Accessors for current frame resources
uint32_t frame_sync_get_frame_index(frame_sync_t* sync);
fence_t* frame_sync_get_current_fence(frame_sync_t* sync);
semaphore_t* frame_sync_get_image_available_semaphore(frame_sync_t* sync);
semaphore_t* frame_sync_get_render_finished_semaphore(frame_sync_t* sync);

#endif // FRAME_SYNC_H
