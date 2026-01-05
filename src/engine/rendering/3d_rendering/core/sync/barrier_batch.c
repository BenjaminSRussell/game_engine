/*
 * barrier_batch.c
 * Implementation of resource barrier batching
 */

#include "barrier_batch.h"
#include <stdlib.h>
#include <string.h>

#define MAX_BATCHED_BARRIERS 64

struct barrier_batch {
    buffer_barrier_t buffer_barriers[MAX_BATCHED_BARRIERS];
    uint32_t buffer_count;
    
    image_barrier_t image_barriers[MAX_BATCHED_BARRIERS];
    uint32_t image_count;
    
    // Global memory barrier info
    bool has_global;
};

barrier_batch_t* barrier_batch_create(void) {
    barrier_batch_t* batch = (barrier_batch_t*)malloc(sizeof(barrier_batch_t));
    if (batch) {
        barrier_batch_reset(batch);
    }
    return batch;
}

void barrier_batch_destroy(barrier_batch_t* batch) {
    if (batch) {
        free(batch);
    }
}

void barrier_batch_reset(barrier_batch_t* batch) {
    if (!batch) return;
    batch->buffer_count = 0;
    batch->image_count = 0;
    batch->has_global = false;
}

void barrier_batch_add_buffer(barrier_batch_t* batch, const buffer_barrier_t* barrier) {
    if (!batch || !barrier || batch->buffer_count >= MAX_BATCHED_BARRIERS) return;
    
    batch->buffer_barriers[batch->buffer_count++] = *barrier;
}

void barrier_batch_add_image(barrier_batch_t* batch, const image_barrier_t* barrier) {
    if (!batch || !barrier || batch->image_count >= MAX_BATCHED_BARRIERS) return;
    
    batch->image_barriers[batch->image_count++] = *barrier;
}

void barrier_batch_add_global(barrier_batch_t* batch) {
    if (!batch) return;
    batch->has_global = true;
}

void barrier_batch_flush(barrier_batch_t* batch, command_buffer_t* cmd) {
    if (!batch || !cmd || (batch->buffer_count == 0 && batch->image_count == 0 && !batch->has_global)) return;

    if (!command_buffer_is_recording(cmd)) return;

    // Call backend to issue pipeline barrier
    // backend_cmd_pipeline_barrier(cmd->backend_handle, ... using batch->* ...);
    
    // Reset batch after flush
    barrier_batch_reset(batch);
}
