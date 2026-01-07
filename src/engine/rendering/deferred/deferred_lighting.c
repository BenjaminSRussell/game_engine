/**
 * =================================================================================================
 *                              DEFERRED LIGHTING IMPLEMENTATION
 * =================================================================================================
 */

#include "deferred_lighting.h"
#include <gpu_backend/render_pipeline.h>
#include <core/logger/logger.h>

static u32 s_lighting_shader_id = 0;
// static void *s_fullscreen_mesh = 0; // Assuming a shared fullscreen quad/triangle exists

void deferred_lighting_init(void) {
    LOG_INFO("Initializing Deferred Lighting System...");

    // 1. Compile Lighting Shader
    s_lighting_shader_id = shader_compile_fragment("assets/shaders/deferred_lighting.frag");
    // Vertex shader is likely standard fullscreen triangle
    u32 vs_id = shader_compile_vertex("assets/shaders/fullscreen.vert"); // Assuming this exists
    
    // Link (in a real engine this might effectively create a pipeline state object)
    shader_link_program(vs_id, s_lighting_shader_id);
    
    LOG_INFO("Deferred lighting shader compiled. ID: %d", s_lighting_shader_id);
}

void deferred_lighting_shutdown(void) {
    // Release resources
}

void deferred_lighting_execute(GBuffer *gbuffer, void *output_target) {
    if (!gbuffer || !output_target) return;

    // 1. Bind Output Render Target (Accumulation Buffer or Swapchain Image)
    render_target_bind(output_target);
    // Maybe clear if not blending? usually we overwrite or clear before.

    // 2. Bind Lighting Shader
    shader_bind(s_lighting_shader_id);

    // 3. Bind G-Buffer Textures as Inputs
    // Assuming specific binding slots defined in shader
    shader_set_texture(s_lighting_shader_id, "texAlbedo", gbuffer->texture_albedo);
    shader_set_texture(s_lighting_shader_id, "texNormal", gbuffer->texture_normal);
    shader_set_texture(s_lighting_shader_id, "texMaterial", gbuffer->texture_material);
    shader_set_texture(s_lighting_shader_id, "texDepth", gbuffer->texture_depth);
    shader_set_texture(s_lighting_shader_id, "texEmissive", gbuffer->texture_emissive);

    // 4. Bind Light Data (Uniform Buffers / SSBOs)
    // shader_set_buffer(s_lighting_shader_id, "LightBuffer", light_manager_get_buffer());
    // shader_set_uniforms(...) for camera pos, etc.

    // 5. Draw Fullscreen Triangle
    // draw_fullscreen_triangle(); 
    // Or if we need a mesh:
    // mesh_render(s_fullscreen_mesh);
    
    // Placeholder using a generic draw command
    // draw_arrays(3); // 3 vertices for full screen triangle
}

void deferred_lighting_update_settings(void *settings) {
    // Update global uniforms
}
