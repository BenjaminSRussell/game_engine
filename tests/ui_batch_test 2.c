/*
 * ui_batch_test.c
 * Comprehensive tests for UI batching system
 *
 * Advanced 3D Rendering Engine
 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <assert.h>

/* Include UI batching modules */
#include "editor/ui/canvas/ui_batch.h"
#include "editor/ui/canvas/ui_batch_gpu.h"
#include "editor/ui/canvas/ui_batch_text.h"
#include "editor/ui/canvas/ui_batch_effects.h"
#include "editor/ui/canvas/ui_batch_optimize.h"

/* ============================================================================
 * TEST UTILITIES
 * ============================================================================ */

static int tests_passed = 0;
static int tests_failed = 0;

#define TEST_ASSERT(condition, message) \
    if (!(condition)) { \
        fprintf(stderr, "FAIL: %s\n", message); \
        tests_failed++; \
    } else { \
        tests_passed++; \
    }

/* ============================================================================
 * CORE BATCHING TESTS
 * ============================================================================ */

static void test_batch_init_shutdown(void) {
    fprintf(stdout, "Test: Batch initialization and shutdown\n");

    int result = ui_rendering_ui_batch_init();
    TEST_ASSERT(result == 0, "Batch init should succeed");

    uint32_t count = ui_rendering_ui_batch_get_count();
    TEST_ASSERT(count == 0, "Initial batch count should be 0");

    ui_rendering_ui_batch_shutdown();
}

static void test_batch_creation(void) {
    fprintf(stdout, "Test: Batch creation and destruction\n");

    ui_rendering_ui_batch_init();

    ui_rendering_ui_batch_desc_t desc = {
        .flags = 0,
        .max_vertices = 1024,
        .max_indices = 2048,
        .user_data = NULL,
    };

    ui_rendering_ui_batch_handle_t handle;
    int result = ui_rendering_ui_batch_create(&handle, &desc);
    TEST_ASSERT(result == 0, "Batch creation should succeed");
    TEST_ASSERT(handle.id == 0, "First batch should have id 0");

    bool valid = ui_rendering_ui_batch_is_valid(handle);
    TEST_ASSERT(valid, "Created batch should be valid");

    ui_rendering_ui_batch_destroy(handle);

    ui_rendering_ui_batch_shutdown();
}

static void test_geometry_addition(void) {
    fprintf(stdout, "Test: Geometry addition to batch\n");

    ui_rendering_ui_batch_init();

    ui_rendering_ui_batch_desc_t desc = {
        .flags = 0,
        .max_vertices = 1024,
        .max_indices = 2048,
    };

    ui_rendering_ui_batch_handle_t handle;
    ui_rendering_ui_batch_create(&handle, &desc);

    /* Create quad geometry */
    ui_rendering_vertex_t vertices[4] = {
        { 0, 0, 0, 0, 0, 0xFFFFFFFF, 0, 0 },
        { 100, 0, 0, 1, 0, 0xFFFFFFFF, 0, 0 },
        { 100, 100, 0, 1, 1, 0xFFFFFFFF, 0, 0 },
        { 0, 100, 0, 0, 1, 0xFFFFFFFF, 0, 0 },
    };

    uint32_t indices[6] = { 0, 1, 2, 0, 2, 3 };

    int result = ui_rendering_ui_batch_add_geometry(handle, vertices, 4, indices, 6);
    TEST_ASSERT(result == 0, "Geometry addition should succeed");

    ui_rendering_ui_batch_info_t info;
    result = ui_rendering_ui_batch_get_info(handle, &info);
    TEST_ASSERT(result == 0, "Get info should succeed");
    TEST_ASSERT(info.vertex_count == 4, "Vertex count should be 4");
    TEST_ASSERT(info.index_count == 6, "Index count should be 6");

    ui_rendering_ui_batch_destroy(handle);
    ui_rendering_ui_batch_shutdown();
}

static void test_draw_commands(void) {
    fprintf(stdout, "Test: Draw command handling\n");

    ui_rendering_ui_batch_init();

    ui_rendering_ui_batch_desc_t desc = {
        .flags = 0,
        .max_vertices = 1024,
        .max_indices = 2048,
    };

    ui_rendering_ui_batch_handle_t handle;
    ui_rendering_ui_batch_create(&handle, &desc);

    ui_rendering_draw_command_t cmd = {
        .vertex_offset = 0,
        .vertex_count = 4,
        .index_offset = 0,
        .index_count = 6,
        .material_id = 1,
        .texture_id = 0,
        .blend_mode = 0,
        .z_order = 0.0f,
    };

    int result = ui_rendering_ui_batch_add_draw_command(handle, &cmd);
    TEST_ASSERT(result == 0, "Add draw command should succeed");

    ui_rendering_draw_command_t* commands = NULL;
    uint32_t command_count = 0;
    result = ui_rendering_ui_batch_get_draw_commands(handle, &commands, &command_count);
    TEST_ASSERT(result == 0, "Get draw commands should succeed");
    TEST_ASSERT(command_count == 1, "Should have 1 command");

    ui_rendering_ui_batch_destroy(handle);
    ui_rendering_ui_batch_shutdown();
}

static void test_batch_sorting(void) {
    fprintf(stdout, "Test: Batch sorting and optimization\n");

    ui_rendering_ui_batch_init();

    ui_rendering_ui_batch_desc_t desc = {
        .flags = 0,
        .max_vertices = 4096,
        .max_indices = 8192,
    };

    ui_rendering_ui_batch_handle_t handle;
    ui_rendering_ui_batch_create(&handle, &desc);

    /* Add multiple draw commands with different z-orders */
    ui_rendering_draw_command_t cmd1 = {
        .vertex_offset = 0, .vertex_count = 4,
        .index_offset = 0, .index_count = 6,
        .material_id = 1, .texture_id = 0,
        .blend_mode = 0, .z_order = 0.5f,
    };

    ui_rendering_draw_command_t cmd2 = {
        .vertex_offset = 4, .vertex_count = 4,
        .index_offset = 6, .index_count = 6,
        .material_id = 2, .texture_id = 1,
        .blend_mode = 0, .z_order = 0.3f,
    };

    ui_rendering_ui_batch_add_draw_command(handle, &cmd1);
    ui_rendering_ui_batch_add_draw_command(handle, &cmd2);

    int result = ui_rendering_ui_batch_sort(handle);
    TEST_ASSERT(result == 0, "Sort should succeed");

    ui_rendering_ui_batch_destroy(handle);
    ui_rendering_ui_batch_shutdown();
}

/* ============================================================================
 * TEXT RENDERING TESTS
 * ============================================================================ */

static void test_text_system(void) {
    fprintf(stdout, "Test: Text rendering system\n");

    ui_batch_text_init();

    ui_batch_text_font_desc_t font_desc = {
        .font_name = "Arial",
        .font_size = 16,
        .atlas_width = 512,
        .atlas_height = 512,
        .font_data = NULL,
        .font_data_size = 0,
    };

    ui_batch_text_font_handle_t font_handle;
    int result = ui_batch_text_create_font(&font_handle, &font_desc);
    TEST_ASSERT(result == 0, "Font creation should succeed");

    /* Test glyph retrieval */
    ui_batch_text_glyph_metrics_t glyph;
    result = ui_batch_text_get_glyph(font_handle, 'A', &glyph);
    TEST_ASSERT(result == 0, "Glyph retrieval should succeed");

    /* Test text measurement */
    float width, height;
    result = ui_batch_text_measure(font_handle, "Hello", &width, &height);
    TEST_ASSERT(result == 0, "Text measurement should succeed");
    TEST_ASSERT(width > 0, "Text width should be positive");

    ui_batch_text_destroy_font(font_handle);
    ui_batch_text_shutdown();
}

/* ============================================================================
 * EFFECTS TESTS
 * ============================================================================ */

static void test_gradient_effects(void) {
    fprintf(stdout, "Test: Gradient effects\n");

    ui_rendering_ui_batch_init();
    ui_batch_effect_init();

    ui_rendering_ui_batch_desc_t desc = {
        .flags = 0,
        .max_vertices = 1024,
        .max_indices = 2048,
    };

    ui_rendering_ui_batch_handle_t handle;
    ui_rendering_ui_batch_create(&handle, &desc);

    ui_batch_gradient_t gradient;
    int result = ui_batch_effect_create_linear_gradient(45.0f, 0xFF0000FF, 0x00FF00FF, &gradient);
    TEST_ASSERT(result == 0, "Create gradient should succeed");

    result = ui_batch_effect_add_gradient(handle, 10, 10, 100, 100, &gradient);
    TEST_ASSERT(result == 0, "Add gradient should succeed");

    ui_rendering_ui_batch_destroy(handle);
    ui_batch_effect_shutdown();
    ui_rendering_ui_batch_shutdown();
}

static void test_shadow_effects(void) {
    fprintf(stdout, "Test: Shadow effects\n");

    ui_rendering_ui_batch_init();
    ui_batch_effect_init();

    ui_rendering_ui_batch_desc_t desc = {
        .flags = 0,
        .max_vertices = 1024,
        .max_indices = 2048,
    };

    ui_rendering_ui_batch_handle_t handle;
    ui_rendering_ui_batch_create(&handle, &desc);

    ui_batch_shadow_t shadow = {
        .type = UI_EFFECT_SHADOW,
        .blur_radius = 5.0f,
        .offset_x = 2.0f,
        .offset_y = 2.0f,
        .color = 0xFF000000,
        .opacity = 0.5f,
    };

    int result = ui_batch_effect_add_shadow(handle, 10, 10, 100, 100, &shadow);
    TEST_ASSERT(result == 0, "Add shadow should succeed");

    ui_rendering_ui_batch_destroy(handle);
    ui_batch_effect_shutdown();
    ui_rendering_ui_batch_shutdown();
}

/* ============================================================================
 * GPU AND OPTIMIZATION TESTS
 * ============================================================================ */

static void test_optimization(void) {
    fprintf(stdout, "Test: Batch optimization\n");

    ui_batch_optimize_options_t opts = {
        .enable_pooling = true,
        .enable_simd = true,
        .enable_caching = true,
        .enable_compression = false,
        .simd_path = 0,
    };

    int result = ui_batch_optimize_init(&opts);
    TEST_ASSERT(result == 0, "Optimize init should succeed");

    uint32_t caps = ui_batch_simd_get_capabilities();
    TEST_ASSERT(caps >= 0, "SIMD capabilities should be valid");

    ui_batch_optimize_shutdown();
}

/* ============================================================================
 * MAIN TEST RUNNER
 * ============================================================================ */

int main(void) {
    fprintf(stdout, "=== UI Batch System Test Suite ===\n\n");

    fprintf(stdout, "--- Core Batching Tests ---\n");
    test_batch_init_shutdown();
    test_batch_creation();
    test_geometry_addition();
    test_draw_commands();
    test_batch_sorting();

    fprintf(stdout, "\n--- Text Rendering Tests ---\n");
    test_text_system();

    fprintf(stdout, "\n--- Effects Tests ---\n");
    test_gradient_effects();
    test_shadow_effects();

    fprintf(stdout, "\n--- Optimization Tests ---\n");
    test_optimization();

    fprintf(stdout, "\n=== Test Results ===\n");
    fprintf(stdout, "Passed: %d\n", tests_passed);
    fprintf(stdout, "Failed: %d\n", tests_failed);

    return tests_failed == 0 ? 0 : 1;
}

/* End of ui_batch_test.c */
