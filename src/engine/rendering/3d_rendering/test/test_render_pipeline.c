/**
 * @file test_render_pipeline.c
 * @brief Unit tests for the core rendering pipeline
 * @details Tests pipeline creation, frame management, and basic operations
 */

#include "render_test_framework.h"
#include "../3d_rendering.h"
#include <stdio.h>

/* ==================== Test Fixtures ==================== */

static RenderHandle g_pipeline_handle = INVALID_HANDLE;

static bool test_setup(void)
{
    RenderPipelineConfig config = {
        .max_width = 1920,
        .max_height = 1080,
        .max_frame_buffering = 2,
        .max_render_passes = 256,
        .max_resources = 10000,
        .max_meshes = 5000,
        .max_materials = 2000,
        .max_lights = 512,
        .max_shadows = 64,
        .enable_ray_tracing = 1,
        .enable_async_compute = 1,
        .enable_dynamic_resolution = 0,
        .enable_variable_rate_shading = 0,
        .max_memory_mb = 2048,
    };

    g_pipeline_handle = rendering_pipeline_create(&config);
    return g_pipeline_handle != INVALID_HANDLE;
}

static bool test_teardown(void)
{
    if (g_pipeline_handle != INVALID_HANDLE) {
        rendering_pipeline_destroy(g_pipeline_handle);
        g_pipeline_handle = INVALID_HANDLE;
    }
    return true;
}

/* ==================== Test Cases ==================== */

/**
 * @brief Test pipeline creation with valid configuration
 */
static bool test_pipeline_creation_valid(void)
{
    RenderPipelineConfig config = {
        .max_width = 1920,
        .max_height = 1080,
        .max_frame_buffering = 2,
        .max_render_passes = 256,
        .max_resources = 10000,
        .max_meshes = 5000,
        .max_materials = 2000,
        .max_lights = 512,
        .max_shadows = 64,
        .enable_ray_tracing = 1,
        .enable_async_compute = 1,
        .max_memory_mb = 2048,
    };

    RenderHandle handle = rendering_pipeline_create(&config);
    RENDER_TEST_ASSERT_NE(handle, INVALID_HANDLE);

    /* Verify pipeline can be retrieved */
    RenderPipeline* pipeline = rendering_pipeline_get(handle);
    RENDER_TEST_ASSERT_NOT_NULL(pipeline);

    rendering_pipeline_destroy(handle);
    return true;
}

/**
 * @brief Test pipeline creation with NULL config
 */
static bool test_pipeline_creation_null_config(void)
{
    RenderHandle handle = rendering_pipeline_create(NULL);
    RENDER_TEST_ASSERT_EQ(handle, INVALID_HANDLE);
    return true;
}

/**
 * @brief Test pipeline retrieval with invalid handle
 */
static bool test_pipeline_get_invalid_handle(void)
{
    RenderPipeline* pipeline = rendering_pipeline_get(INVALID_HANDLE);
    RENDER_TEST_ASSERT_NULL(pipeline);
    return true;
}

/**
 * @brief Test frame begin/end lifecycle
 */
static bool test_frame_lifecycle(void)
{
    RENDER_TEST_ASSERT_NOT_NULL(g_pipeline_handle);

    /* Begin frame */
    bool result = rendering_frame_begin(g_pipeline_handle, 0.016f);
    RENDER_TEST_ASSERT(result, "Frame begin failed");

    /* End frame */
    result = rendering_frame_end(g_pipeline_handle);
    RENDER_TEST_ASSERT(result, "Frame end failed");

    /* Verify frame index incremented */
    uint32_t frame_index = rendering_frame_get_index(g_pipeline_handle);
    RENDER_TEST_ASSERT_EQ(frame_index, 1U);

    return true;
}

/**
 * @brief Test multiple frame cycles
 */
static bool test_multiple_frames(void)
{
    const uint32_t FRAME_COUNT = 10;

    for (uint32_t i = 0; i < FRAME_COUNT; ++i) {
        bool begin_result = rendering_frame_begin(g_pipeline_handle, 0.016f);
        RENDER_TEST_ASSERT(begin_result, "Frame begin failed");

        bool end_result = rendering_frame_end(g_pipeline_handle);
        RENDER_TEST_ASSERT(end_result, "Frame end failed");
    }

    uint32_t frame_index = rendering_frame_get_index(g_pipeline_handle);
    RENDER_TEST_ASSERT_EQ(frame_index, FRAME_COUNT);

    return true;
}

/**
 * @brief Test resolution setting and getting
 */
static bool test_resolution_management(void)
{
    const uint32_t TEST_WIDTH = 3840;
    const uint32_t TEST_HEIGHT = 2160;

    bool result = rendering_set_resolution(g_pipeline_handle, TEST_WIDTH, TEST_HEIGHT);
    RENDER_TEST_ASSERT(result, "Set resolution failed");

    uint32_t width = 0, height = 0;
    rendering_get_resolution(g_pipeline_handle, &width, &height);

    RENDER_TEST_ASSERT_EQ(width, TEST_WIDTH);
    RENDER_TEST_ASSERT_EQ(height, TEST_HEIGHT);

    return true;
}

/**
 * @brief Test resolution with invalid dimensions
 */
static bool test_resolution_invalid_dimensions(void)
{
    /* Test zero width */
    bool result = rendering_set_resolution(g_pipeline_handle, 0, 1080);
    RENDER_TEST_ASSERT(!result, "Zero width should fail");

    /* Test zero height */
    result = rendering_set_resolution(g_pipeline_handle, 1920, 0);
    RENDER_TEST_ASSERT(!result, "Zero height should fail");

    return true;
}

/**
 * @brief Test viewport setting
 */
static bool test_viewport_setting(void)
{
    /* Begin frame for viewport recording */
    rendering_frame_begin(g_pipeline_handle, 0.016f);

    bool result = rendering_set_viewport(g_pipeline_handle, 0, 0, 1920, 1080, 0.0f, 1.0f);
    RENDER_TEST_ASSERT(result, "Set viewport failed");

    rendering_frame_end(g_pipeline_handle);
    return true;
}

/**
 * @brief Test scene creation and retrieval
 */
static bool test_scene_creation(void)
{
    RenderHandle scene_handle = rendering_scene_create(g_pipeline_handle, "TestScene");
    RENDER_TEST_ASSERT_NE(scene_handle, INVALID_HANDLE);

    RenderScene* scene = rendering_scene_get(g_pipeline_handle, scene_handle);
    RENDER_TEST_ASSERT_NOT_NULL(scene);

    rendering_scene_destroy(g_pipeline_handle, scene_handle);
    return true;
}

/**
 * @brief Test camera setup
 */
static bool test_camera_setup(void)
{
    RenderHandle scene_handle = rendering_scene_create(g_pipeline_handle, "CameraTest");
    RENDER_TEST_ASSERT_NE(scene_handle, INVALID_HANDLE);

    CameraParameters camera = {
        .position = {0.0f, 5.0f, 10.0f},
        .forward = {0.0f, 0.0f, -1.0f},
        .up = {0.0f, 1.0f, 0.0f},
        .fov_y = 45.0f,
        .near_plane = 0.1f,
        .far_plane = 1000.0f,
        .aspect_ratio = 16.0f / 9.0f,
    };

    RenderHandle camera_handle = rendering_camera_set(g_pipeline_handle, scene_handle, &camera);
    RENDER_TEST_ASSERT_NE(camera_handle, INVALID_HANDLE);

    rendering_scene_destroy(g_pipeline_handle, scene_handle);
    return true;
}

/**
 * @brief Test rendering execution
 */
static bool test_rendering_execution(void)
{
    rendering_frame_begin(g_pipeline_handle, 0.016f);

    RenderHandle scene_handle = rendering_scene_create(g_pipeline_handle, "RenderTest");
    bool result = rendering_execute(g_pipeline_handle, scene_handle);
    RENDER_TEST_ASSERT(result, "Rendering execute failed");

    rendering_scene_destroy(g_pipeline_handle, scene_handle);
    rendering_frame_end(g_pipeline_handle);

    return true;
}

/* ==================== Test Suite Registration ==================== */

/**
 * @brief Register all rendering pipeline tests
 */
void register_render_pipeline_tests(void)
{
    uint32_t suite_id = render_test_suite_create("RenderPipeline");
    render_test_suite_set_fixtures(suite_id, test_setup, test_teardown);

    render_test_register(suite_id, "PipelineCreationValid", test_pipeline_creation_valid);
    render_test_register(suite_id, "PipelineCreationNullConfig", test_pipeline_creation_null_config);
    render_test_register(suite_id, "PipelineGetInvalidHandle", test_pipeline_get_invalid_handle);
    render_test_register(suite_id, "FrameLifecycle", test_frame_lifecycle);
    render_test_register(suite_id, "MultipleFrames", test_multiple_frames);
    render_test_register(suite_id, "ResolutionManagement", test_resolution_management);
    render_test_register(suite_id, "ResolutionInvalidDimensions", test_resolution_invalid_dimensions);
    render_test_register(suite_id, "ViewportSetting", test_viewport_setting);
    render_test_register(suite_id, "SceneCreation", test_scene_creation);
    render_test_register(suite_id, "CameraSetup", test_camera_setup);
    render_test_register(suite_id, "RenderingExecution", test_rendering_execution);
}
