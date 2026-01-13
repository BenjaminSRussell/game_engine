/*
 * debug_visualization.h
 * Debug visualization for GPU-driven rendering
 *
 * Part of the Geometry subsystem
 * Advanced 3D Rendering Engine
 */

#ifndef GEOMETRY_DEBUG_VISUALIZATION_H
#define GEOMETRY_DEBUG_VISUALIZATION_H

#include <stdint.h>
#include <stdbool.h>

#ifdef __OBJC__
#import <Metal/Metal.h>
#endif

#ifdef __cplusplus
extern "C" {
#endif

/* ============================================================================
 * FORWARD DECLARATIONS
 * ============================================================================ */

typedef struct instance_buffer instance_buffer_t;
typedef struct gpu_culling_context gpu_culling_context_t;
typedef struct camera_frustum camera_frustum_t;

#ifdef __OBJC__
@class MTLRenderCommandEncoder;
#else
typedef void MTLRenderCommandEncoder;
#endif

/* ============================================================================
 * TYPES & STRUCTURES
 * ============================================================================ */

// Debug visualization mode
typedef enum debug_viz_mode {
    DEBUG_VIZ_OFF = 0,
    DEBUG_VIZ_INSTANCE_BOUNDS,      // Show instance bounding boxes
    DEBUG_VIZ_FRUSTUM_PLANES,       // Show frustum planes
    DEBUG_VIZ_CULLED_INSTANCES,     // Highlight culled instances
    DEBUG_VIZ_VISIBLE_INSTANCES,    // Highlight visible instances
    DEBUG_VIZ_LOD_LEVELS,           // Color by LOD level
    DEBUG_VIZ_HEATMAP_DENSITY,      // Instance density heatmap
    DEBUG_VIZ_DRAW_CALL_BATCHES,    // Color by batch ID
} debug_viz_mode_t;

// Debug visualization settings
typedef struct debug_viz_settings {
    debug_viz_mode_t mode;
    bool show_wireframe;
    bool show_labels;
    float opacity;
    float line_width;
    
    // Colors
    float culled_color[4];          // RGBA for culled instances
    float visible_color[4];         // RGBA for visible instances
    float lod0_color[4];            // LOD 0
    float lod1_color[4];            // LOD 1
    float lod2_color[4];            // LOD 2
} debug_viz_settings_t;

// Debug visualization context
typedef struct debug_viz_context {
    debug_viz_settings_t settings;
    
    // Metal resources for debug rendering
    void* debug_pipeline_state;
    void* debug_lines_buffer;
    void* debug_labels_texture;
    
    uint32_t line_count;
    uint32_t max_lines;
    
    bool initialized;
    char label[64];
} debug_viz_context_t;

/* ============================================================================
 * API - CONTEXT MANAGEMENT
 * ============================================================================ */

/**
 * Create debug visualization context
 */
debug_viz_context_t* debug_viz_create(void);

/**
 * Destroy debug visualization context
 */
void debug_viz_destroy(debug_viz_context_t* context);

/**
 * Set visualization mode
 */
void debug_viz_set_mode(debug_viz_context_t* context, debug_viz_mode_t mode);

/**
 * Update settings
 */
void debug_viz_update_settings(debug_viz_context_t* context, 
                                const debug_viz_settings_t* settings);

/* ============================================================================
 * API - VISUALIZATION RENDERING
 * ============================================================================ */

/**
 * Render frustum visualization
 */
void debug_viz_render_frustum(
    debug_viz_context_t* context,
    MTLRenderCommandEncoder* encoder,
    const camera_frustum_t* frustum,
    const float view_proj_matrix[16]);

/**
 * Render instance bounds
 */
void debug_viz_render_instance_bounds(
    debug_viz_context_t* context,
    MTLRenderCommandEncoder* encoder,
    instance_buffer_t* instances,
    uint32_t instance_count,
    const float view_proj_matrix[16]);

/**
 * Render culling visualization
 */
void debug_viz_render_culling_results(
    debug_viz_context_t* context,
    MTLRenderCommandEncoder* encoder,
    gpu_culling_context_t* culling_context,
    instance_buffer_t* instances,
    const uint32_t* visible_ids,
    uint32_t visible_count,
    const float view_proj_matrix[16]);

/**
 * Render LOD visualization
 */
void debug_viz_render_lod_levels(
    debug_viz_context_t* context,
    MTLRenderCommandEncoder* encoder,
    instance_buffer_t* instances,
    const float camera_position[3],
    const float view_proj_matrix[16]);

/**
 * Render batch visualization
 */
void debug_viz_render_batches(
    debug_viz_context_t* context,
    MTLRenderCommandEncoder* encoder,
    uint32_t batch_count,
    const float view_proj_matrix[16]);

/* ============================================================================
 * API - UTILITIES
 * ============================================================================ */

/**
 * Get default settings
 */
debug_viz_settings_t debug_viz_get_default_settings(void);

/**
 * Add debug line
 */
void debug_viz_add_line(
    debug_viz_context_t* context,
    const float start[3],
    const float end[3],
    const float color[4]);

/**
 * Add debug box (axis-aligned)
 */
void debug_viz_add_box(
    debug_viz_context_t* context,
    const float min[3],
    const float max[3],
    const float color[4]);

/**
 * Add debug sphere
 */
void debug_viz_add_sphere(
    debug_viz_context_t* context,
    const float center[3],
    float radius,
    const float color[4],
    uint32_t segments);

/**
 * Clear all debug primitives
 */
void debug_viz_clear(debug_viz_context_t* context);

/**
 * Print debug statistics
 */
void debug_viz_print_stats(const debug_viz_context_t* context);

#ifdef __cplusplus
}
#endif

#endif /* GEOMETRY_DEBUG_VISUALIZATION_H */
