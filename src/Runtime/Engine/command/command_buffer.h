/*
 * command_buffer.h
 * Command buffer recording and management
 */

#ifndef COMMAND_BUFFER_H
#define COMMAND_BUFFER_H

#include <stdint.h>
#include <stdbool.h>
#include "core/command/command_pool.h"

typedef struct command_buffer command_buffer_t;
typedef struct resource_state_tracker resource_state_tracker_t; // Forward declaration

typedef enum command_buffer_state {
    COMMAND_BUFFER_STATE_INITIAL,
    COMMAND_BUFFER_STATE_RECORDING,
    COMMAND_BUFFER_STATE_EXECUTABLE,
    COMMAND_BUFFER_STATE_PENDING,
    COMMAND_BUFFER_STATE_INVALID
} command_buffer_state_t;

typedef enum command_buffer_level {
    COMMAND_BUFFER_LEVEL_PRIMARY,
    COMMAND_BUFFER_LEVEL_SECONDARY
} command_buffer_level_t;

// Structure definition (exposed as requested)
struct command_buffer {
    void* backend_handle;
    command_pool_t* pool;
    command_buffer_state_t state;
    resource_state_tracker_t* state_tracker;
};

// Allocation
command_buffer_t* command_buffer_allocate(command_pool_t* pool, command_buffer_level_t level);

// Lifecycle
void command_buffer_free(command_buffer_t* cmd);
void command_buffer_reset(command_buffer_t* cmd);

// Recording
int command_buffer_begin(command_buffer_t* cmd);
int command_buffer_end(command_buffer_t* cmd);

// Accessors
bool command_buffer_is_recording(command_buffer_t* cmd);

#endif // COMMAND_BUFFER_H
