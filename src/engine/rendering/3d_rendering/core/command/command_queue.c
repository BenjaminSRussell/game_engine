/*
 * command_queue.c
 * Implementation of command queue submission
 */

#include "command_queue.h"
#include <stdlib.h>
#include <stdio.h>

struct command_queue {
    void* backend_handle; // VkQueue
    uint32_t family_index;
    uint32_t index;
};

// Global queue registry (simplification)
static command_queue_t g_main_queue = {0};

command_queue_t* command_queue_get(uint32_t queue_family_index, uint32_t queue_index) {
    // In a real engine, this would retrieve from the logical device
    if (g_main_queue.backend_handle == NULL) {
        g_main_queue.backend_handle = (void*)0x01234567; // Fake handle
        g_main_queue.family_index = queue_family_index;
        g_main_queue.index = queue_index;
    }
    return &g_main_queue;
}

int command_queue_submit(command_queue_t* queue, const queue_submit_info_t* submit_info) {
    if (!queue || !submit_info) return -1;

    // Backend submission logic:
    // VkSubmitInfo vk_submit = { ... };
    // vkQueueSubmit(queue->backend_handle, 1, &vk_submit, fence);

    // Validate states
    for (uint32_t i = 0; i < submit_info->command_buffer_count; ++i) {
        command_buffer_t* cmd = submit_info->command_buffers[i];
        if (cmd->state != COMMAND_BUFFER_STATE_EXECUTABLE) {
            // Error: buffer not ready
            return -2;
        }
        cmd->state = COMMAND_BUFFER_STATE_PENDING;
    }

    return 0;
}

void command_queue_wait_idle(command_queue_t* queue) {
    if (!queue) return;
    // backend_queue_wait_idle(queue->backend_handle);
}
