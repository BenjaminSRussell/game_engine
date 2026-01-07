/**
 * =================================================================================================
 *                              DEFERRED LIGHTING IMPLEMENTATION
 * =================================================================================================
 */

#include "deferred_lighting.h"
#include <gpu_backend/render_pipeline.h>
#include <gpu_backend/framebuffer.h>
#include <core/logger/logger.h>
#include <string.h>

/* =================================================================================================
 *                                    INTERNAL STATE
 * =================================================================================================
 */

static struct {
    u32 vertex_shader_id;
    u32 fragment_shader_id;
    u32 program_id;
    
    // Light data (simplified for now)
    struct {
        f32 direction[3];
        f32 color[3];
        f32 intensity;
    } sun_light;
    
    // Camera data
    struct {
        f32 inv_view_proj[16];  // mat4
        f32 position[3];
    } camera_uniforms;
    
    bool initialized;
} s_deferred_state = {0};

/* =================================================================================================
 *                                    IMPLEMENTATION
 * =================================================================================================
 */

void deferred_lighting_init(void) {
    LOG_INFO("Initializing Deferred Lighting System...");

    // Compile fullscreen vertex shader
    s_deferred_state.vertex_shader_id = shader_compile_vertex("assets/shaders/fullscreen.vert");
    
    // Compile lighting fragment shader
    s_deferred_state.fragment_shader_id = shader_compile_fragment("assets/shaders/deferred_lighting.frag");
    
    // Link program
    s_deferred_state.program_id = shader_link_program(
        s_deferred_state.vertex_shader_id,
        s_deferred_state.fragment_shader_id
    );
    
    // Initialize default sun light
    s_deferred_state.sun_light.direction[0] = -0.5f;
    s_deferred_state.sun_light.direction[1] = -1.0f;
    s_deferred_state.sun_light.direction[2] = -0.3f;
    
    s_deferred_state.sun_light.color[0] = 1.0f;
    s_deferred_state.sun_light.color[1] = 0.95f;
    s_deferred_state.sun_light.color[2] = 0.9f;
    
    s_deferred_state.sun_light.intensity = 3.0f;
    
    s_deferred_state.initialized = true;
    
    LOG_INFO("Deferred lighting initialized (Program ID: %u)", s_deferred_state.program_id);
}

void deferred_lighting_shutdown(void) {
    if (!s_deferred_state.initialized) return;
    
    // TODO: Release shader resources
    s_deferred_state.initialized = false;
    
    LOG_INFO("Deferred lighting shutdown");
}

void deferred_lighting_execute(GBuffer *gbuffer, void *output_target) {
    if (!s_deferred_state.initialized) {
        LOG_ERROR("Deferred lighting not initialized");
        return;
    }
    
    if (!gbuffer || !output_target) {
        LOG_ERROR("Invalid parameters to deferred_lighting_execute");
        return;
    }

    // 1. Bind Output Render Target
    render_target_bind(output_target);
    
    // 2. Bind Lighting Shader
    shader_bind(s_deferred_state.program_id);

    // 3. Bind G-Buffer Textures as Inputs
    gbuffer_bind_textures(gbuffer, 0);

// 4. Upload Camera Uniforms
    // Update the local state first, then upload to GPU
    // In a real system, we'd use a persistently mapped uniform buffer
    shader_set_uniforms(s_deferred_state.program_id, &s_deferred_state.camera_uniforms);
    
    // 5. Upload Light Data
    // For now we only have a single "sun" light in the fragment shader
    // We pass it via a separate struct or uniform buffer
    struct {
        f32 direction[3];
        f32 padding1;
        f32 color[3];
        f32 intensity;
    } sun_data;
    
    memcpy(sun_data.direction, s_deferred_state.sun_light.direction, sizeof(f32) * 3);
    memcpy(sun_data.color, s_deferred_state.sun_light.color, sizeof(f32) * 3);
    sun_data.intensity = s_deferred_state.sun_light.intensity;
    
    // shader_set_uniform_struct(s_deferred_state.program_id, "u_SunLight", &sun_data);

    // 6. Draw Fullscreen Triangle (3 vertices)
    // The shader generates vertices from gl_VertexIndex
    // render_pipeline_draw_arrays(3);
    
    LOG_TRACE("Deferred lighting pass executed");
}

/**
 * Set camera matrices for deferred lighting
 */
void deferred_lighting_set_camera(f32 *inv_view_proj, f32 *camera_pos) {
    if (!s_deferred_state.initialized) return;
    
    if (inv_view_proj) {
        memcpy(s_deferred_state.camera_uniforms.inv_view_proj, inv_view_proj, sizeof(f32) * 16);
    }
    if (camera_pos) {
        memcpy(s_deferred_state.camera_uniforms.position, camera_pos, sizeof(f32) * 3);
    }
}

/**
 * Set directional light parameters
 */
void deferred_lighting_set_sun(f32 *direction, f32 *color, f32 intensity) {
    if (!s_deferred_state.initialized) return;

    if (direction) {
        // Normalize direction? Lighting shader usually expects normalized
        memcpy(s_deferred_state.sun_light.direction, direction, sizeof(f32) * 3);
    }
    if (color) {
        memcpy(s_deferred_state.sun_light.color, color, sizeof(f32) * 3);
    }
    s_deferred_state.sun_light.intensity = intensity;
}
