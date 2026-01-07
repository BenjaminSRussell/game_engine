#include "../editor_common.h"
#include "core/logger.h"
#include <stdio.h>

/**
 * =================================================================================================
 *                           MATERIAL EDITOR PREVIEW
 * =================================================================================================
 * 
 * Handles rendering the real-time preview of the material being edited.
 */

// Simulated render state
static struct {
    u32 preview_shader;
    u32 sphere_mesh;
    u32 cube_mesh;
    u32 current_mesh;
    f32 rotation;
} preview_state = {0};

void material_editor_preview_init(EditorContext* ctx) {
    // Generate primitive meshes
    // preview_state.sphere_mesh = render_create_sphere(1.0f, 32, 32);
    // preview_state.cube_mesh = render_create_cube(1.0f);
    preview_state.current_mesh = preview_state.sphere_mesh;
    
    LOG_INFO("Material Preview Initialized");
}

void material_editor_preview_update(f32 delta_time) {
    preview_state.rotation += delta_time * 0.5f;
}

void material_editor_preview_render(EditorContext* ctx, MaterialGraph* graph) {
    if (!graph) return;
    
    // 1. Check if we need to recompile shader
    if (graph->needs_recompile) {
        // material_graph_compile() is called by the UI or update loop
        // Here we would actually define the GPU shader object if the graph compilation succeeded
        if (graph->generated_fragment_shader) {
            // preview_state.preview_shader = render_create_shader(vertex_src, graph->generated_fragment_shader);
            // LOG_INFO("Updated preview shader");
        }
    }
    
    // 2. Setup Preview Viewport
    // Assuming we are rendering to a separate framebuffer or viewport rect within the editor
    
    // 3. Bind Material
    // render_bind_shader(preview_state.preview_shader);
    
    // 4. Set Uniforms
    // render_set_uniform("u_time", get_time());
    // render_set_uniform("u_lightDir", (Vec3){0.5f, 1.0f, 0.3f});
    
    // 5. Draw Mesh
    // mat4 model = mat4_rotate_y(preview_state.rotation);
    // render_set_model_matrix(model);
    // render_draw_mesh(preview_state.current_mesh);
    
    // Placeholder visualization
    // draw_rect((Vec2){10, 10}, (Vec2){200, 200}, (vec4){0, 0, 0, 1}); // Background
}
