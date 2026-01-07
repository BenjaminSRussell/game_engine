/**
 * =================================================================================================
 *                              G-BUFFER PASS IMPLEMENTATION
 * =================================================================================================
 */

#include "gbuffer.h"
#include <gpu_backend/render_pipeline.h>
#include <core/logger/logger.h>

// Early-Z Optimization Notes:
// 1. Ensure depth testing and depth writes are enabled for the opaque pass.
// 2. Sort meshes front-to-back before submission to maximize early-Z rejection.
// 3. For complex scenes, use a depth pre-pass (Z-prepass) to fill the depth buffer first.

extern GBuffer *g_gbuffer; 

void pass_gbuffer_execute(void) {
    if (!g_gbuffer) {
        LOG_WARN("G-Buffer pass skipped: G-Buffer not initialized.");
        return;
    }

    // 1. Bind G-Buffer Framebuffer
    gbuffer_bind(g_gbuffer);

    // 2. Clear Attachments
    // Clear color targets to black, depth to 1.0 (far plane)
    // framebuffer_clear(g_gbuffer->framebuffer, CLEAR_COLOR_BIT | CLEAR_DEPTH_BIT);

    // 3. Setup Render State for OPAQUE rendering
    // These calls would typically interface with the hardware abstraction layer
    // render_state_set_depth_test(true);
    // render_state_set_depth_write(true);
    // render_state_set_cull_mode(CULL_MODE_BACK);
    // render_state_set_blend_mode(BLEND_MODE_NONE);

    // 4. Render Opaque Geometry
    // Opaque objects must be rendered front-to-back for optimal early-Z performance.
    LOG_TRACE("Executing G-buffer geometry pass...");
    
    // shader_bind(gbuffer_shader_id);
    // scene_render_opaque_front_to_back();

    // 5. Unbind
    gbuffer_unbind(g_gbuffer);
}
