/**
 * =================================================================================================
 *                              G-BUFFER PASS IMPLEMENTATION
 * =================================================================================================
 */

#include "rendering/gbuffer/gbuffer_pass.h"
#include <rendering/render_pipeline.h>
#include <rendering/framebuffer.h>
#include <core/memory.h>
#include <core/logger.h>
#include <string.h>

struct GBufferPass {
    GBufferPassDescriptor descriptor;
    u32 shader_id; // Internal shader management
};

extern GBuffer *g_gbuffer; 

GBufferPass* gbuffer_pass_create(const GBufferPassDescriptor *desc) {
    if (!desc || !desc->gbuffer) return NULL;
    
    GBufferPass *pass = (GBufferPass*)memory_allocate(sizeof(GBufferPass), MEMORY_TAG_RENDERER);
    memcpy(&pass->descriptor, desc, sizeof(GBufferPassDescriptor));
    
    // Shader would be loaded here or passed in
    // pass->shader_id = shader_compile_program("assets/shaders/gbuffer.vert", "assets/shaders/gbuffer.frag");
    
    return pass;
}

void gbuffer_pass_destroy(GBufferPass *pass) {
    if (!pass) return;
    memory_free(pass, sizeof(GBufferPass), MEMORY_TAG_RENDERER);
}

void gbuffer_pass_begin(GBufferPass *pass) {
    if (!pass || !pass->descriptor.gbuffer) return;

    GBuffer *gb = pass->descriptor.gbuffer;
    gbuffer_bind(gb);

    // TODO: Use descriptor values for clears
    // framebuffer_clear_color(gb->framebuffer, pass->descriptor.clear_albedo[0], ...);
    // framebuffer_clear_depth(gb->framebuffer, pass->descriptor.clear_depth);
    
    // Setup state (pseudo-code/stubs)
    // render_pipeline_set_depth_state(pass->descriptor.depth_test_enabled, pass->descriptor.depth_write_enabled);
}

void gbuffer_pass_end(GBufferPass *pass) {
    if (!pass || !pass->descriptor.gbuffer) return;
    gbuffer_unbind(pass->descriptor.gbuffer);
}

void pass_gbuffer_execute(void) {
    if (!g_gbuffer) {
        LOG_WARN("G-Buffer pass skipped: G-Buffer not initialized.");
        return;
    }

    // Standard default pass configuration
    GBufferPassDescriptor desc = {0};
    desc.gbuffer = g_gbuffer;
    desc.depth_test_enabled = true;
    desc.depth_write_enabled = true;
    desc.clear_depth = 1.0f;
    
    GBufferPass *pass = gbuffer_pass_create(&desc);
    gbuffer_pass_begin(pass);
    
    // Scene rendering happens here
    // scene_render_opaque(pass->shader_id);
    
    gbuffer_pass_end(pass);
    gbuffer_pass_destroy(pass);
}
