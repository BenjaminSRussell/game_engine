#include "render_tests.h"
#include <stdio.h>

void render_test_init(render_test_context_t* ctx, id<MTLDevice> device) {
    if (!ctx) return;
    ctx->device = device;
    ctx->world = render_world_create(device, 1920, 1080);
    ctx->psnr_threshold = 30.0f; // dB
}

void render_test_shutdown(render_test_context_t* ctx) {
    if (!ctx) return;
    if (ctx->world) {
        render_world_destroy(ctx->world);
        ctx->world = NULL;
    }
}

// Simple assertion helper
#define TEST_ASSERT(cond) \
    if (!(cond)) { \
        printf("Test Failed: %s at line %d\n", #cond, __LINE__); \
        return false; \
    }

bool test_initialization(render_test_context_t* ctx) {
    TEST_ASSERT(ctx->world != NULL);
    TEST_ASSERT(ctx->world->device == ctx->device);
    TEST_ASSERT(ctx->world->width == 1920);
    return true;
}

bool test_resize(render_test_context_t* ctx) {
    render_world_resize(ctx->world, 1280, 720);
    TEST_ASSERT(ctx->world->width == 1280);
    TEST_ASSERT(ctx->world->height == 720);
    return true;
}

bool render_test_run_unit_tests(render_test_context_t* ctx) {
    bool success = true;
    printf("Running Render Unit Tests...\n");
    
    if (!test_initialization(ctx)) success = false;
    else printf("  [PASS] Initialization\n");

    if (!test_resize(ctx)) success = false;
    else printf("  [PASS] Resize\n");

    return success;
}
