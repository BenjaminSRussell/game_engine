#ifndef RENDER_TESTS_H
#define RENDER_TESTS_H

#include "core/integration/render_world.h"
#include <Metal/Metal.h>
#include <stdbool.h>

typedef struct render_test_context {
    id<MTLDevice> device;
    render_world_t* world;
    id<MTLTexture> reference_image;
    float psnr_threshold;
} render_test_context_t;

// Test API
void render_test_init(render_test_context_t* ctx, id<MTLDevice> device);
void render_test_shutdown(render_test_context_t* ctx);
bool render_test_run_unit_tests(render_test_context_t* ctx);

#endif // RENDER_TESTS_H
