/**
 * @file 3d_rendering.c
 * @brief Main 3D rendering pipeline implementation
 * @details Core implementation of the rendering pipeline initialization and management
 */

#include "3d_rendering.h"
#include "core/render_device.h"
#include "core/render_pipeline_core.h"
#include "core/render_command.h"
#include "resource_management/render_resource.h"
#include <stdlib.h>
#include <string.h>

/* ==================== Private Data Structures ==================== */

typedef struct RenderPipeline {
    RenderHandle handle;
    RenderPipelineConfig config;
    RenderDevice* device;
    RenderPipelineCore* core;
    RenderResourceManager* resource_manager;
    RenderCommandQueue* graphics_queue;
    RenderCommandQueue* compute_queue;
    RenderCommandQueue* transfer_queue;

    uint32_t frame_index;
    bool is_initialized;
    bool is_recording;
} RenderPipeline;

/* ==================== Static Global State ==================== */

#define MAX_PIPELINES 16
static RenderPipeline* g_pipelines[MAX_PIPELINES] = {0};
static uint32_t g_pipeline_count = 0;

/* ==================== Internal Functions ==================== */

static uint32_t allocate_pipeline_handle(void)
{
    if (g_pipeline_count >= MAX_PIPELINES) {
        return INVALID_HANDLE;
    }
    return g_pipeline_count;
}

static RenderPipeline* get_pipeline(RenderHandle handle)
{
    if (handle >= MAX_PIPELINES || g_pipelines[handle] == NULL) {
        return NULL;
    }
    return g_pipelines[handle];
}

/* ==================== Public API Implementation ==================== */

RenderHandle rendering_pipeline_create(const RenderPipelineConfig* config)
{
    if (config == NULL) {
        return INVALID_HANDLE;
    }

    RenderHandle handle = allocate_pipeline_handle();
    if (handle == INVALID_HANDLE) {
        return INVALID_HANDLE;
    }

    RenderPipeline* pipeline = (RenderPipeline*)malloc(sizeof(RenderPipeline));
    if (pipeline == NULL) {
        return INVALID_HANDLE;
    }

    memset(pipeline, 0, sizeof(RenderPipeline));
    memcpy(&pipeline->config, config, sizeof(RenderPipelineConfig));
    pipeline->handle = handle;
    pipeline->frame_index = 0;

    /* Create graphics device - prefer Metal on macOS, Vulkan on others */
#ifdef __APPLE__
    pipeline->device = render_device_create(RENDER_API_METAL);
#else
    pipeline->device = render_device_create(RENDER_API_VULKAN);
#endif

    if (pipeline->device == NULL) {
        free(pipeline);
        return INVALID_HANDLE;
    }

    /* Create core pipeline */
    pipeline->core = (RenderPipelineCore*)malloc(sizeof(RenderPipelineCore));
    if (pipeline->core == NULL) {
        render_device_destroy(pipeline->device);
        free(pipeline);
        return INVALID_HANDLE;
    }

    if (!render_pipeline_core_init(pipeline->core, config->max_width, config->max_height,
                                   config->max_frame_buffering)) {
        free(pipeline->core);
        render_device_destroy(pipeline->device);
        free(pipeline);
        return INVALID_HANDLE;
    }

    /* Create resource manager */
    pipeline->resource_manager = render_resource_manager_create(config->max_resources);
    if (pipeline->resource_manager == NULL) {
        render_pipeline_core_destroy(pipeline->core);
        free(pipeline->core);
        render_device_destroy(pipeline->device);
        free(pipeline);
        return INVALID_HANDLE;
    }

    /* Create command queues */
    pipeline->graphics_queue = render_command_queue_create(QUEUE_TYPE_GRAPHICS, 64);
    pipeline->compute_queue = render_command_queue_create(QUEUE_TYPE_COMPUTE, 32);
    pipeline->transfer_queue = render_command_queue_create(QUEUE_TYPE_TRANSFER, 32);

    if (pipeline->graphics_queue == NULL || pipeline->compute_queue == NULL ||
        pipeline->transfer_queue == NULL) {
        if (pipeline->graphics_queue) render_command_queue_destroy(pipeline->graphics_queue);
        if (pipeline->compute_queue) render_command_queue_destroy(pipeline->compute_queue);
        if (pipeline->transfer_queue) render_command_queue_destroy(pipeline->transfer_queue);
        render_resource_manager_destroy(pipeline->resource_manager);
        render_pipeline_core_destroy(pipeline->core);
        free(pipeline->core);
        render_device_destroy(pipeline->device);
        free(pipeline);
        return INVALID_HANDLE;
    }

    pipeline->is_initialized = true;
    g_pipelines[handle] = pipeline;
    g_pipeline_count++;

    return handle;
}

void rendering_pipeline_destroy(RenderHandle pipeline_handle)
{
    RenderPipeline* pipeline = get_pipeline(pipeline_handle);
    if (pipeline == NULL) {
        return;
    }

    /* Clean up resources */
    if (pipeline->graphics_queue) {
        render_command_queue_destroy(pipeline->graphics_queue);
    }
    if (pipeline->compute_queue) {
        render_command_queue_destroy(pipeline->compute_queue);
    }
    if (pipeline->transfer_queue) {
        render_command_queue_destroy(pipeline->transfer_queue);
    }

    if (pipeline->resource_manager) {
        render_resource_manager_destroy(pipeline->resource_manager);
    }

    if (pipeline->core) {
        render_pipeline_core_destroy(pipeline->core);
        free(pipeline->core);
    }

    if (pipeline->device) {
        render_device_destroy(pipeline->device);
    }

    free(pipeline);
    g_pipelines[pipeline_handle] = NULL;
}

RenderPipeline* rendering_pipeline_get(RenderHandle pipeline_handle)
{
    return get_pipeline(pipeline_handle);
}

bool rendering_frame_begin(RenderHandle pipeline_handle, float delta_time)
{
    RenderPipeline* pipeline = get_pipeline(pipeline_handle);
    if (pipeline == NULL) {
        return false;
    }

    pipeline->is_recording = true;
    return render_pipeline_core_begin_frame(pipeline->core, delta_time);
}

bool rendering_frame_end(RenderHandle pipeline_handle)
{
    RenderPipeline* pipeline = get_pipeline(pipeline_handle);
    if (pipeline == NULL) {
        return false;
    }

    pipeline->is_recording = false;
    pipeline->frame_index++;
    return render_pipeline_core_submit(pipeline->core);
}

bool rendering_frame_wait(RenderHandle pipeline_handle)
{
    RenderPipeline* pipeline = get_pipeline(pipeline_handle);
    if (pipeline == NULL) {
        return false;
    }

    return render_pipeline_core_wait(pipeline->core);
}

uint32_t rendering_frame_get_index(RenderHandle pipeline_handle)
{
    RenderPipeline* pipeline = get_pipeline(pipeline_handle);
    if (pipeline == NULL) {
        return 0;
    }

    return pipeline->frame_index;
}

bool rendering_set_resolution(RenderHandle pipeline_handle, uint32_t width, uint32_t height)
{
    RenderPipeline* pipeline = get_pipeline(pipeline_handle);
    if (pipeline == NULL) {
        return false;
    }

    return render_pipeline_core_resize(pipeline->core, width, height);
}

void rendering_get_resolution(RenderHandle pipeline_handle, uint32_t* width, uint32_t* height)
{
    RenderPipeline* pipeline = get_pipeline(pipeline_handle);
    if (pipeline == NULL) {
        return;
    }

    if (width) *width = pipeline->core->viewport_width;
    if (height) *height = pipeline->core->viewport_height;
}

bool rendering_set_viewport(RenderHandle pipeline_handle,
                           uint32_t x, uint32_t y,
                           uint32_t width, uint32_t height,
                           float min_depth, float max_depth)
{
    RenderPipeline* pipeline = get_pipeline(pipeline_handle);
    if (pipeline == NULL || !pipeline->is_recording) {
        return false;
    }

    /* Viewport validation */
    if (width == 0 || height == 0) {
        return false;
    }

    /* TODO: Record viewport change command to command buffer */
    return true;
}

RenderHandle rendering_scene_create(RenderHandle pipeline_handle, const char* name)
{
    RenderPipeline* pipeline = get_pipeline(pipeline_handle);
    if (pipeline == NULL || name == NULL) {
        return INVALID_HANDLE;
    }

    /* TODO: Implement scene creation */
    return 0;  /* Placeholder */
}

void rendering_scene_destroy(RenderHandle pipeline_handle, RenderHandle scene_handle)
{
    RenderPipeline* pipeline = get_pipeline(pipeline_handle);
    if (pipeline == NULL) {
        return;
    }

    /* TODO: Implement scene destruction */
}

RenderScene* rendering_scene_get(RenderHandle pipeline_handle, RenderHandle scene_handle)
{
    RenderPipeline* pipeline = get_pipeline(pipeline_handle);
    if (pipeline == NULL) {
        return NULL;
    }

    /* TODO: Implement scene retrieval */
    return NULL;
}

RenderHandle rendering_camera_set(RenderHandle pipeline_handle,
                                  RenderHandle scene_handle,
                                  const CameraParameters* camera_params)
{
    RenderPipeline* pipeline = get_pipeline(pipeline_handle);
    if (pipeline == NULL || camera_params == NULL) {
        return INVALID_HANDLE;
    }

    /* TODO: Implement camera setup */
    return 0;  /* Placeholder */
}

bool rendering_camera_update(RenderHandle pipeline_handle,
                             RenderHandle camera_handle,
                             const CameraParameters* camera_params)
{
    RenderPipeline* pipeline = get_pipeline(pipeline_handle);
    if (pipeline == NULL || camera_params == NULL) {
        return false;
    }

    /* TODO: Implement camera update */
    return true;
}

bool rendering_execute(RenderHandle pipeline_handle, RenderHandle scene_handle)
{
    RenderPipeline* pipeline = get_pipeline(pipeline_handle);
    if (pipeline == NULL || !pipeline->is_recording) {
        return false;
    }

    /* TODO: Execute rendering pipeline for scene */
    return true;
}

bool rendering_pass_execute(RenderHandle pipeline_handle, PassHandle pass_handle)
{
    RenderPipeline* pipeline = get_pipeline(pipeline_handle);
    if (pipeline == NULL) {
        return false;
    }

    /* TODO: Execute specific rendering pass */
    return true;
}
