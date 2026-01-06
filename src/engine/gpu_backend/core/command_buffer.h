// Renderer Core - Command Buffer
// Simple stub for command buffer API

#ifndef RENDERER_CORE_COMMAND_BUFFER_H
#define RENDERER_CORE_COMMAND_BUFFER_H

#include "core/types.h"

#ifdef __cplusplus
extern "C" {
#endif

// Command buffer state
typedef enum CommandBufferState {
    CMD_BUFFER_STATE_INITIAL,
    CMD_BUFFER_STATE_RECORDING,
    CMD_BUFFER_STATE_EXECUTABLE,
} CommandBufferState;

// Command buffer (opaque backend handle)
typedef struct CommandBuffer {
    void *backend_handle;        // OpenGL: NULL, Vulkan: VkCommandBuffer
    CommandBufferState state;
    u32 frame_index;
} CommandBuffer;

// Stub functions (to be implemented by backend)
CommandBuffer *cmd_buffer_create(void);
void cmd_buffer_destroy(CommandBuffer *cmd);
void cmd_buffer_begin(CommandBuffer *cmd);
void cmd_buffer_end(CommandBuffer *cmd);

#ifdef __cplusplus
}
#endif

#endif // RENDERER_CORE_COMMAND_BUFFER_H
