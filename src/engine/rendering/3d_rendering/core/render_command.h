/**
 * @file render_command.h
 * @brief Rendering command recording and submission
 * @details Manages command buffer recording, command queuing, and GPU submission
 */

#ifndef RENDER_COMMAND_H
#define RENDER_COMMAND_H

#ifdef __cplusplus
extern "C" {
#endif

#include <stdint.h>
#include <stdbool.h>

/* Forward declarations */
typedef struct RenderCommandBuffer RenderCommandBuffer;
typedef struct RenderCommandQueue RenderCommandQueue;

/**
 * @brief Command buffer state
 */
typedef enum {
    COMMAND_BUFFER_STATE_INITIAL = 0,
    COMMAND_BUFFER_STATE_RECORDING = 1,
    COMMAND_BUFFER_STATE_EXECUTABLE = 2,
    COMMAND_BUFFER_STATE_PENDING = 3,
    COMMAND_BUFFER_STATE_INVALID = 4,
} RenderCommandBufferState;

/**
 * @brief Queue type for command submission
 */
typedef enum {
    QUEUE_TYPE_GRAPHICS = 0,
    QUEUE_TYPE_COMPUTE = 1,
    QUEUE_TYPE_TRANSFER = 2,
    QUEUE_TYPE_PRESENT = 3,
} QueueType;

/**
 * @brief Command buffer info
 */
typedef struct {
    RenderCommandBufferState state;
    uint32_t command_count;
    size_t buffer_size;
    size_t buffer_used;
} RenderCommandBufferInfo;

/**
 * @brief Create a command buffer
 * @param[in] queue_type Type of queue for this buffer
 * @param[in] max_commands Maximum commands this buffer can hold
 * @return Pointer to created buffer, NULL on failure
 */
RenderCommandBuffer* render_command_buffer_create(QueueType queue_type, uint32_t max_commands);

/**
 * @brief Destroy a command buffer
 * @param[in] cmd_buffer Buffer to destroy
 */
void render_command_buffer_destroy(RenderCommandBuffer* cmd_buffer);

/**
 * @brief Begin recording commands
 * @param[in] cmd_buffer Buffer to begin recording to
 * @return true on success
 */
bool render_command_buffer_begin(RenderCommandBuffer* cmd_buffer);

/**
 * @brief End recording commands
 * @param[in] cmd_buffer Buffer to end recording for
 * @return true on success
 */
bool render_command_buffer_end(RenderCommandBuffer* cmd_buffer);

/**
 * @brief Reset command buffer to initial state
 * @param[in] cmd_buffer Buffer to reset
 * @return true on success
 */
bool render_command_buffer_reset(RenderCommandBuffer* cmd_buffer);

/**
 * @brief Get command buffer info
 * @param[in] cmd_buffer Buffer to query
 * @return Info structure
 */
RenderCommandBufferInfo render_command_buffer_get_info(RenderCommandBuffer* cmd_buffer);

/**
 * @brief Create a command queue
 * @param[in] queue_type Type of queue
 * @param[in] max_submissions Maximum pending submissions
 * @return Pointer to created queue, NULL on failure
 */
RenderCommandQueue* render_command_queue_create(QueueType queue_type, uint32_t max_submissions);

/**
 * @brief Destroy a command queue
 * @param[in] queue Queue to destroy
 */
void render_command_queue_destroy(RenderCommandQueue* queue);

/**
 * @brief Submit command buffer to queue
 * @param[in] queue Queue to submit to
 * @param[in] cmd_buffer Buffer to submit
 * @return true on success
 */
bool render_command_queue_submit(RenderCommandQueue* queue, RenderCommandBuffer* cmd_buffer);

/**
 * @brief Wait for queue to complete all submissions
 * @param[in] queue Queue to wait for
 * @return true on success
 */
bool render_command_queue_wait(RenderCommandQueue* queue);

#ifdef __cplusplus
}
#endif

#endif /* RENDER_COMMAND_H */
