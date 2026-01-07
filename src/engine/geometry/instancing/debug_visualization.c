/*
 * debug_visualization.c
 * Debug visualization implementation
 *
 * Part of the Geometry subsystem
 * Advanced 3D Rendering Engine
 */

#include "debug_visualization.h"
#include "instance_data.h"
#include "gpu_culling.h"
#include <stdlib.h>
#include <string.h>
#include <stdio.h>
#include <math.h>

/* ============================================================================
 * DEBUG LINE STRUCTURE
 * ============================================================================ */

typedef struct debug_line {
    float start[3];
    float end[3];
    float color[4];
} debug_line_t;

/* ============================================================================
 * CONTEXT MANAGEMENT
 * ============================================================================ */

debug_viz_context_t* debug_viz_create(void) {
    debug_viz_context_t* context = (debug_viz_context_t*)calloc(
        1, sizeof(debug_viz_context_t));
    if (!context) {
        return NULL;
    }
    
    context->settings = debug_viz_get_default_settings();
    context->max_lines = 10000;
    context->line_count = 0;
    context->initialized = true;
    
    snprintf(context->label, sizeof(context->label), "DebugViz_%p", (void*)context);
    
    printf("[Debug Viz] Created context\n");
    return context;
}

void debug_viz_destroy(debug_viz_context_t* context) {
    if (!context) {
        return;
    }
    
    printf("[Debug Viz] Destroyed context\n");
    free(context);
}

void debug_viz_set_mode(debug_viz_context_t* context, debug_viz_mode_t mode) {
    if (context) {
        context->settings.mode = mode;
        printf("[Debug Viz] Mode set to %d\n", mode);
    }
}

void debug_viz_update_settings(debug_viz_context_t* context,
                                const debug_viz_settings_t* settings) {
    if (context && settings) {
        memcpy(&context->settings, settings, sizeof(debug_viz_settings_t));
    }
}

/* ============================================================================
 * VISUALIZATION RENDERING
 * ============================================================================ */

void debug_viz_render_frustum(
    debug_viz_context_t* context,
    MTLRenderCommandEncoder* encoder,
    const camera_frustum_t* frustum,
    const float view_proj_matrix[16]) {
    
    if (!context || !frustum || context->settings.mode != DEBUG_VIZ_FRUSTUM_PLANES) {
        return;
    }
    
    // Draw frustum planes as colored quads
    float frustum_color[4] = {1.0f, 1.0f, 0.0f, 0.3f}; // Yellow, semi-transparent
    
    // Note: Full implementation would create geometry for each plane
    printf("[Debug Viz] Rendering frustum planes\n");
}

void debug_viz_render_instance_bounds(
    debug_viz_context_t* context,
    MTLRenderCommandEncoder* encoder,
    instance_buffer_t* instances,
    uint32_t instance_count,
    const float view_proj_matrix[16]) {
    
    if (!context || !instances || context->settings.mode != DEBUG_VIZ_INSTANCE_BOUNDS) {
        return;
    }
    
    debug_viz_clear(context);
    
    // Draw bounding box for each instance
    float box_color[4] = {0.0f, 1.0f, 0.0f, 1.0f}; // Green
    
    // Note: Would iterate through instances and extract bounds from transforms
    printf("[Debug Viz] Rendering %u instance bounds\n", instance_count);
}

void debug_viz_render_culling_results(
    debug_viz_context_t* context,
    MTLRenderCommandEncoder* encoder,
    gpu_culling_context_t* culling_context,
    instance_buffer_t* instances,
    const uint32_t* visible_ids,
    uint32_t visible_count,
    const float view_proj_matrix[16]) {
    
    if (!context || !instances) {
        return;
    }
    
    bool show_culled = (context->settings.mode == DEBUG_VIZ_CULLED_INSTANCES);
    bool show_visible = (context->settings.mode == DEBUG_VIZ_VISIBLE_INSTANCES);
    
    if (!show_culled && !show_visible) {
        return;
    }
    
    debug_viz_clear(context);
    
    // Render with different colors based on visibility
    if (show_visible && visible_ids) {
        printf("[Debug Viz] Rendering %u visible instances\n", visible_count);
    }
    
    if (show_culled) {
        uint32_t total = instance_buffer_get_count(instances);
        uint32_t culled = total - visible_count;
        printf("[Debug Viz] Rendering %u culled instances\n", culled);
    }
}

void debug_viz_render_lod_levels(
    debug_viz_context_t* context,
    MTLRenderCommandEncoder* encoder,
    instance_buffer_t* instances,
    const float camera_position[3],
    const float view_proj_matrix[16]) {
    
    if (!context || !instances || context->settings.mode != DEBUG_VIZ_LOD_LEVELS) {
        return;
    }
    
    debug_viz_clear(context);
    
    // Color instances by LOD level
    printf("[Debug Viz] Rendering LOD levels\n");
}

void debug_viz_render_batches(
    debug_viz_context_t* context,
    MTLRenderCommandEncoder* encoder,
    uint32_t batch_count,
    const float view_proj_matrix[16]) {
    
    if (!context || context->settings.mode != DEBUG_VIZ_DRAW_CALL_BATCHES) {
        return;
    }
    
    printf("[Debug Viz] Rendering %u batches\n", batch_count);
}

/* ============================================================================
 * UTILITIES
 * ============================================================================ */

debug_viz_settings_t debug_viz_get_default_settings(void) {
    debug_viz_settings_t settings = {0};
    
    settings.mode = DEBUG_VIZ_OFF;
    settings.show_wireframe = true;
    settings.show_labels = false;
    settings.opacity = 0.5f;
    settings.line_width = 1.0f;
    
    // Default colors
    settings.culled_color[0] = 1.0f; // Red
    settings.culled_color[1] = 0.0f;
    settings.culled_color[2] = 0.0f;
    settings.culled_color[3] = 0.5f;
    
    settings.visible_color[0] = 0.0f; // Green
    settings.visible_color[1] = 1.0f;
    settings.visible_color[2] = 0.0f;
    settings.visible_color[3] = 1.0f;
    
    settings.lod0_color[0] = 0.0f; // Blue
    settings.lod0_color[1] = 0.0f;
    settings.lod0_color[2] = 1.0f;
    settings.lod0_color[3] = 1.0f;
    
    settings.lod1_color[0] = 0.0f; // Cyan
    settings.lod1_color[1] 1.0f;
    settings.lod1_color[2] = 1.0f;
    settings.lod1_color[3] = 1.0f;
    
    settings.lod2_color[0] = 1.0f; // Magenta
    settings.lod2_color[1] = 0.0f;
    settings.lod2_color[2] = 1.0f;
    settings.lod2_color[3] = 1.0f;
    
    return settings;
}

void debug_viz_add_line(
    debug_viz_context_t* context,
    const float start[3],
    const float end[3],
    const float color[4]) {
    
    if (!context || context->line_count >= context->max_lines) {
        return;
    }
    
    // Note: Would add to debug_lines_buffer
    context->line_count++;
}

void debug_viz_add_box(
    debug_viz_context_t* context,
    const float min[3],
    const float max[3],
    const float color[4]) {
    
    if (!context) {
        return;
    }
    
    // Create 12 lines for box edges
    float vertices[8][3] = {
        {min[0], min[1], min[2]}, // 0
        {max[0], min[1], min[2]}, // 1
        {max[0], max[1], min[2]}, // 2
        {min[0], max[1], min[2]}, // 3
        {min[0], min[1], max[2]}, // 4
        {max[0], min[1], max[2]}, // 5
        {max[0], max[1], max[2]}, // 6
        {min[0], max[1], max[2]}, // 7
    };
    
    // Bottom face
    debug_viz_add_line(context, vertices[0], vertices[1], color);
    debug_viz_add_line(context, vertices[1], vertices[2], color);
    debug_viz_add_line(context, vertices[2], vertices[3], color);
    debug_viz_add_line(context, vertices[3], vertices[0], color);
    
    // Top face
    debug_viz_add_line(context, vertices[4], vertices[5], color);
    debug_viz_add_line(context, vertices[5], vertices[6], color);
    debug_viz_add_line(context, vertices[6], vertices[7], color);
    debug_viz_add_line(context, vertices[7], vertices[4], color);
    
    // Vertical edges
    debug_viz_add_line(context, vertices[0], vertices[4], color);
    debug_viz_add_line(context, vertices[1], vertices[5], color);
    debug_viz_add_line(context, vertices[2], vertices[6], color);
    debug_viz_add_line(context, vertices[3], vertices[7], color);
}

void debug_viz_add_sphere(
    debug_viz_context_t* context,
    const float center[3],
    float radius,
    const float color[4],
    uint32_t segments) {
    
    if (!context || segments == 0) {
        return;
    }
    
    // Draw sphere as circles in 3 planes
    for (uint32_t i = 0; i < segments; i++) {
        float angle1 = (i * 2.0f * M_PI) / segments;
        float angle2 = ((i + 1) * 2.0f * M_PI) / segments;
        
        // XY plane
        float start[3] = {
            center[0] + radius * cosf(angle1),
            center[1] + radius * sinf(angle1),
            center[2]
        };
        float end[3] = {
            center[0] + radius * cosf(angle2),
            center[1] + radius * sinf(angle2),
            center[2]
        };
        debug_viz_add_line(context, start, end, color);
    }
}

void debug_viz_clear(debug_viz_context_t* context) {
    if (context) {
        context->line_count = 0;
    }
}

void debug_viz_print_stats(const debug_viz_context_t* context) {
    if (!context) {
        return;
    }
    
    printf("=== Debug Visualization Stats ===\n");
    printf("  Mode: %d\n", context->settings.mode);
    printf("  Lines: %u / %u\n", context->line_count, context->max_lines);
    printf("  Wireframe: %s\n", context->settings.show_wireframe ? "ON" : "OFF");
    printf("  Opacity: %.2f\n", context->settings.opacity);
}
