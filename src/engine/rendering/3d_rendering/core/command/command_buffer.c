/*
 * command_buffer.c
 * Implementation of command buffer recording and management
 */

#include "command_buffer.h"
#include <stdlib.h>
#include <string.h>

// Helper to simulate backend creation
static void* create_backend_command_buffer(void* pool_handle, command_buffer_level_t level) {
    // Integrate with Vulkan/DirectX here
    return (void*)((uintptr_t)pool_handle + (uintptr_t)level + 0x1000); // Fake handle
}

command_buffer_t* command_buffer_allocate(command_pool_t* pool, command_buffer_level_t level) {
    if (!pool) return NULL;

    command_buffer_t* cmd = (command_buffer_t*)malloc(sizeof(command_buffer_t));
    if (!cmd) return NULL;

    cmd->pool = pool;
    cmd->state = COMMAND_BUFFER_STATE_INITIAL;
    cmd->state_tracker = NULL; // TODO: Initialize tracker if needed

    void* pool_handle = command_pool_get_handle(pool);
    cmd->backend_handle = create_backend_command_buffer(pool_handle, level);

    return cmd;
}

void command_buffer_free(command_buffer_t* cmd) {
    if (!cmd) return;

    // Backend free logic
    // backend_free_command_buffer(cmd->pool->backend_handle, cmd->backend_handle);

    free(cmd);
}

void command_buffer_reset(command_buffer_t* cmd) {
    if (!cmd) return;

    // Backend reset logic
    // backend_reset_command_buffer(cmd->backend_handle);

    cmd->state = COMMAND_BUFFER_STATE_INITIAL;
    // Reset state tracker if attached
}

int command_buffer_begin(command_buffer_t* cmd) {
    if (!cmd) return -1;
    if (cmd->state == COMMAND_BUFFER_STATE_RECORDING) return -2;

    // Backend begin logic
    // backend_begin_command_buffer(cmd->backend_handle);

    cmd->state = COMMAND_BUFFER_STATE_RECORDING;
    return 0;
}

int command_buffer_end(command_buffer_t* cmd) {
    if (!cmd) return -1;
    if (cmd->state != COMMAND_BUFFER_STATE_RECORDING) return -2;

    // Backend end logic
    // backend_end_command_buffer(cmd->backend_handle);

    cmd->state = COMMAND_BUFFER_STATE_EXECUTABLE;
    return 0;
}

bool command_buffer_is_recording(command_buffer_t* cmd) {
    return cmd && cmd->state == COMMAND_BUFFER_STATE_RECORDING;
}
