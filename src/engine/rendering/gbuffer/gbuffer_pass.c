/**
 * =================================================================================================
 *                              G-BUFFER PASS
 * =================================================================================================
 */

#include "gbuffer.h"
#include <gpu_backend/render_pipeline.h>
#include <core/logger/logger.h>

// Global instance constraint for simplicity in this pass, 
// or this function would likely take the GBuffer as an argument or context.
extern GBuffer *g_gbuffer; // Assuming a global or system-managed instance for now

void pass_gbuffer_execute(void) {
    if (!g_gbuffer) {
        LOG_WARN("G-Buffer pass skipped: G-Buffer not initialized.");
        return;
    }

    gbuffer_bind(g_gbuffer);

    // 1. Clear G-Buffer
    // We need to clear at least Depth and probably Albedo/others to black/default.
    // render_target_clear_all(...);

    // 2. Setup Render State
    // Enable Depth Test
    // Enable Backface Culling
    
    // 3. Render Geometry
    // Iterate over visible entities/meshes and draw them using the G-Buffer shader.
    // 
    // Use `shader_bind(gbuffer_shader_id)`
    // For each mesh:
    //   `material_bind(...)`
    //   `mesh_draw(...)` or `draw_command_submit(...)`
    
    // Since this is a high-level pass function, it delegates to the scene renderer 
    // to actually issue draw calls into the currently bound render targets.
    // scene_render_geometry(SCENE_RENDER_LAYER_OPAQUE);

    gbuffer_unbind(g_gbuffer);
}
