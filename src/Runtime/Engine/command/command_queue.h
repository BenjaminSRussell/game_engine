/*
 * command_queue.h
 * Command queue submission and synchronization
 */

#ifndef COMMAND_QUEUE_H
#define COMMAND_QUEUE_H

#include <stdint.h>
#include <stdbool.h>
#include "core/command/command_buffer.h"

// Forward declarations
typedef struct command_queue command_queue_t;
typedef struct fence fence_t;
typedef struct semaphore semaphore_t;

typedef struct queue_submit_info {
    command_buffer_t** command_buffers;
    uint32_t command_buffer_count;
    
    semaphore_t** wait_semaphores;
    uint64_t* wait_values;
    uint32_t wait_semaphore_count;
    
    semaphore_t** signal_semaphores;
    uint64_t* signal_values;
    uint32_t signal_semaphore_count;
    
    fence_t* signal_fence;
} queue_submit_info_t;

// Queue retrieval (simplification: assume global or device-retrieved)
command_queue_t* command_queue_get(uint32_t queue_family_index, uint32_t queue_index);

// Submission
int command_queue_submit(command_queue_t* queue, const queue_submit_info_t* submit_info);

// Wait idle
void command_queue_wait_idle(command_queue_t* queue);

#endif // COMMAND_QUEUE_H
