/**
 * @file render_pipeline_core.h
 * @brief Core rendering pipeline implementation
 * @details Manages the main rendering pipeline state, command recording,
 *          and GPU submission logic.
 */

#ifndef RENDER_PIPELINE_CORE_H
#define RENDER_PIPELINE_CORE_H

#ifdef __cplusplus
extern "C" {
#endif

#include <stdint.h>
#include <stdbool.h>

/* Forward declarations */
typedef struct RenderDevice RenderDevice;
typedef struct RenderCommandBuffer RenderCommandBuffer;
typedef struct RenderSynchronization RenderSynchronization;

/**
 * @brief Core pipeline state structure
 */
typedef struct {
    uint32_t frame_index;
    uint32_t frame_count;
    float delta_time;
    uint32_t viewport_width;
    uint32_t viewport_height;

    RenderDevice* device;
    RenderCommandBuffer** command_buffers;
    uint32_t num_command_buffers;
    RenderSynchronization* sync;

    bool is_recording;
    bool needs_recreation;
} RenderPipelineCore;

/**
 * @brief Initialize core pipeline
 */
bool render_pipeline_core_init(RenderPipelineCore* core,
                               uint32_t max_width,
                               uint32_t max_height,
                               uint32_t frame_buffering);

/**
 * @brief Destroy core pipeline
 */
void render_pipeline_core_destroy(RenderPipelineCore* core);

/**
 * @brief Begin recording commands for current frame
 */
bool render_pipeline_core_begin_frame(RenderPipelineCore* core, float delta_time);

/**
 * @brief Submit recorded commands to GPU
 */
bool render_pipeline_core_submit(RenderPipelineCore* core);

/**
 * @brief Wait for GPU to complete previous frame
 */
bool render_pipeline_core_wait(RenderPipelineCore* core);

/**
 * @brief Handle resolution changes
 */
bool render_pipeline_core_resize(RenderPipelineCore* core, uint32_t width, uint32_t height);

#ifdef __cplusplus
}
#endif

#endif /* RENDER_PIPELINE_CORE_H */
