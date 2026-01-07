/*
 * barrier_batch.h
 * Resource barrier batching
 */

#ifndef BARRIER_BATCH_H
#define BARRIER_BATCH_H

#include <stdint.h>
#include <stdbool.h>
#include "command_buffer.h"

typedef struct barrier_batch barrier_batch_t;

// Simplified barrier structures
typedef struct buffer_barrier {
    void* buffer;
    // offsets, sizes, access masks
} buffer_barrier_t;

typedef struct image_barrier {
    void* image;
    // layouts, subresource ranges
} image_barrier_t;

// Lifecycle
barrier_batch_t* barrier_batch_create(void);
void barrier_batch_destroy(barrier_batch_t* batch);
void barrier_batch_reset(barrier_batch_t* batch);

// Accumulation
void barrier_batch_add_buffer(barrier_batch_t* batch, const buffer_barrier_t* barrier);
void barrier_batch_add_image(barrier_batch_t* batch, const image_barrier_t* barrier);
void barrier_batch_add_global(barrier_batch_t* batch); // Memory barrier

// Flush
void barrier_batch_flush(barrier_batch_t* batch, command_buffer_t* cmd);

#endif // BARRIER_BATCH_H
