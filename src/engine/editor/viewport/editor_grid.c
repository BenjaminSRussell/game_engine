/*
 * editor_grid.c
 * Infinite ground grid with LOD fade
 *
 * Part of the Editor subsystem
 * Advanced 3D Rendering Engine
 *
 * Implements infinite grid rendering with distance-based fading
 */

#include "editor/viewport/editor_grid.h"
#include <stdint.h>
#include <stdbool.h>
#include <stddef.h>
#include <string.h>
#include <stdlib.h>

/* ============================================================================
 * CONSTANTS
 * ============================================================================ */

#define GRID_MAX_COUNT 8
#define GRID_DEFAULT_SPACING 1.0f
#define GRID_MAJOR_INTERVAL 10
#define GRID_FADE_START 50.0f
#define GRID_FADE_END 200.0f

/* ============================================================================
 * MATH TYPES
 * ============================================================================ */

typedef struct vec3 {
    float x, y, z;
} vec3_t;

typedef struct vec4 {
    float x, y, z, w;
} vec4_t;

/* ============================================================================
 * GRID TYPES
 * ============================================================================ */

typedef struct grid_config {
    float spacing;              // Distance between minor grid lines
    uint32_t major_interval;    // Major grid line every N minor lines
    float fade_start;           // Distance where fade begins
    float fade_end;             // Distance where grid becomes invisible
    
    vec4_t minor_color;         // Color of minor grid lines
    vec4_t major_color;         // Color of major grid lines
    vec4_t axis_x_color;        // Color of X axis line
    vec4_t axis_z_color;        // Color of Z axis line
    
    float line_width;           // Thickness of grid lines
    bool show_axes;             // Show X/Z axis lines
    bool angle_fade;            // Fade at grazing angles
} grid_config_t;

typedef struct editor_grid_internal {
    uint32_t id;
    uint32_t flags;
    grid_config_t config;
    
    // Rendering resources
    uint64_t shader_program;
    uint64_t vertex_buffer;
    
    bool initialized;
    bool dirty;
    uint64_t frame_updated;
} editor_grid_internal_t;

typedef struct editor_grid_context {
    editor_grid_internal_t* items;
    uint32_t count;
    uint32_t capacity;
    bool initialized;
} editor_grid_context_t;

static editor_grid_context_t g_grid_ctx = {0};

/* ============================================================================
 * GRID RENDERING DATA
 * ============================================================================ */

// Grid is rendered as a full-screen quad with procedural grid in fragment shader
// The fragment shader:
// 1. Reconstructs world position from depth buffer
// 2. Projects onto Y=0 plane
// 3. Computes grid pattern based on world XZ coordinates
// 4. Applies distance and angle-based fade

typedef struct grid_uniforms {
    float camera_pos[3];
    float camera_forward[3];
    float spacing;
    uint32_t major_interval;
    float fade_start;
    float fade_end;
    float minor_color[4];
    float major_color[4];
    float axis_x_color[4];
    float axis_z_color[4];
    float line_width;
    bool show_axes;
    bool angle_fade;
} grid_uniforms_t;

/* ============================================================================
 * GRID PATTERN COMPUTATION (for CPU preview/picking)
 * ============================================================================ */

static float compute_grid_intensity(vec3_t world_pos, const grid_config_t* config) {
    float spacing = config->spacing;
    
    // Distance from nearest grid line
    float fx = world_pos.x / spacing;
    float fz = world_pos.z / spacing;
    
    fx = fx - (float)(int)fx;  // Fractional part
    fz = fz - (float)(int)fz;
    
    if (fx < 0) fx += 1.0f;
    if (fz < 0) fz += 1.0f;
    
    // Distance to nearest line
    float dx = (fx < 0.5f) ? fx : (1.0f - fx);
    float dz = (fz < 0.5f) ? fz : (1.0f - fz);
    
    // Convert to world units
    dx *= spacing;
    dz *= spacing;
    
    float line_dist = (dx < dz) ? dx : dz;
    float line_width = config->line_width;
    
    if (line_dist < line_width * 0.5f) {
        return 1.0f;
    } else if (line_dist < line_width) {
        return 1.0f - (line_dist - line_width * 0.5f) / (line_width * 0.5f);
    }
    
    return 0.0f;
}

static float compute_fade(vec3_t camera_pos, vec3_t world_pos, 
                           vec3_t camera_forward, const grid_config_t* config) {
    // Distance fade
    vec3_t to_point = {
        world_pos.x - camera_pos.x,
        world_pos.y - camera_pos.y,
        world_pos.z - camera_pos.z
    };
    float dist = sqrtf(to_point.x * to_point.x + to_point.z * to_point.z);
    
    float distance_fade = 1.0f;
    if (dist > config->fade_start) {
        distance_fade = 1.0f - (dist - config->fade_start) / 
                        (config->fade_end - config->fade_start);
        if (distance_fade < 0) distance_fade = 0;
    }
    
    // Angle fade (fade at grazing angles)
    float angle_fade = 1.0f;
    if (config->angle_fade) {
        float view_dot_up = fabsf(camera_forward.y);
        angle_fade = view_dot_up;  // Fade as camera looks more horizontal
        if (angle_fade < 0.1f) angle_fade = 0.1f;  // Minimum visibility
    }
    
    return distance_fade * angle_fade;
}

static float sqrtf(float x) {
    if (x <= 0) return 0;
    float guess = x / 2.0f;
    for (int i = 0; i < 8; i++) {
        guess = (guess + x / guess) / 2.0f;
    }
    return guess;
}

static float fabsf(float x) {
    return x < 0 ? -x : x;
}

/* ============================================================================
 * PUBLIC API
 * ============================================================================ */

int editor_grid_init(void) {
    if (g_grid_ctx.initialized) {
        return 0;
    }

    g_grid_ctx.capacity = GRID_MAX_COUNT;
    g_grid_ctx.items = calloc(g_grid_ctx.capacity, sizeof(editor_grid_internal_t));
    if (!g_grid_ctx.items) {
        return -1;
    }

    g_grid_ctx.count = 0;
    g_grid_ctx.initialized = true;

    return 0;
}

void editor_grid_shutdown(void) {
    if (!g_grid_ctx.initialized) {
        return;
    }

    free(g_grid_ctx.items);
    g_grid_ctx.items = NULL;
    g_grid_ctx.count = 0;
    g_grid_ctx.capacity = 0;
    g_grid_ctx.initialized = false;
}

int editor_grid_create(editor_grid_handle_t* out_handle, 
                        const editor_grid_desc_t* desc) {
    if (!out_handle || !desc) {
        return -1;
    }

    if (!g_grid_ctx.initialized) {
        return -2;
    }

    if (g_grid_ctx.count >= g_grid_ctx.capacity) {
        return -3;
    }

    uint32_t index = g_grid_ctx.count++;
    editor_grid_internal_t* item = &g_grid_ctx.items[index];

    item->id = index;
    item->flags = desc->flags;
    
    // Default grid configuration
    item->config.spacing = GRID_DEFAULT_SPACING;
    item->config.major_interval = GRID_MAJOR_INTERVAL;
    item->config.fade_start = GRID_FADE_START;
    item->config.fade_end = GRID_FADE_END;
    item->config.minor_color = (vec4_t){0.3f, 0.3f, 0.3f, 0.5f};
    item->config.major_color = (vec4_t){0.5f, 0.5f, 0.5f, 0.7f};
    item->config.axis_x_color = (vec4_t){1.0f, 0.2f, 0.2f, 0.8f};
    item->config.axis_z_color = (vec4_t){0.2f, 0.2f, 1.0f, 0.8f};
    item->config.line_width = 0.02f;
    item->config.show_axes = true;
    item->config.angle_fade = true;
    
    item->initialized = true;
    item->dirty = true;
    item->frame_updated = 0;

    out_handle->id = index;
    return 0;
}

void editor_grid_destroy(editor_grid_handle_t handle) {
    if (handle.id >= g_grid_ctx.count) {
        return;
    }

    g_grid_ctx.items[handle.id].initialized = false;
}

int editor_grid_set_spacing(editor_grid_handle_t handle, float spacing) {
    if (handle.id >= g_grid_ctx.count || spacing <= 0) {
        return -1;
    }
    
    g_grid_ctx.items[handle.id].config.spacing = spacing;
    g_grid_ctx.items[handle.id].dirty = true;
    return 0;
}

int editor_grid_set_fade(editor_grid_handle_t handle, 
                          float fade_start, float fade_end) {
    if (handle.id >= g_grid_ctx.count) {
        return -1;
    }
    
    g_grid_ctx.items[handle.id].config.fade_start = fade_start;
    g_grid_ctx.items[handle.id].config.fade_end = fade_end;
    g_grid_ctx.items[handle.id].dirty = true;
    return 0;
}

int editor_grid_set_colors(editor_grid_handle_t handle,
                            vec4_t minor_color, vec4_t major_color) {
    if (handle.id >= g_grid_ctx.count) {
        return -1;
    }
    
    g_grid_ctx.items[handle.id].config.minor_color = minor_color;
    g_grid_ctx.items[handle.id].config.major_color = major_color;
    g_grid_ctx.items[handle.id].dirty = true;
    return 0;
}

int editor_grid_render(editor_grid_handle_t handle,
                        vec3_t camera_pos, vec3_t camera_forward) {
    if (handle.id >= g_grid_ctx.count) {
        return -1;
    }
    
    editor_grid_internal_t* item = &g_grid_ctx.items[handle.id];
    if (!item->initialized) {
        return -2;
    }
    
    // In a real implementation, this would:
    // 1. Bind fullscreen quad
    // 2. Set grid shader uniforms from config
    // 3. Draw quad (grid computed procedurally in fragment shader)
    
    // grid_uniforms_t uniforms = {...};
    // set_uniforms(item->shader_program, &uniforms);
    // draw_fullscreen_quad();
    
    item->frame_updated++;
    return 0;
}

int editor_grid_update(editor_grid_handle_t handle, const void* data, size_t size) {
    if (handle.id >= g_grid_ctx.count) {
        return -1;
    }

    g_grid_ctx.items[handle.id].dirty = true;
    return 0;
}

bool editor_grid_is_valid(editor_grid_handle_t handle) {
    if (handle.id >= g_grid_ctx.count) {
        return false;
    }
    return g_grid_ctx.items[handle.id].initialized;
}

int editor_grid_get_info(editor_grid_handle_t handle, editor_grid_info_t* out_info) {
    if (!out_info) {
        return -1;
    }

    if (handle.id >= g_grid_ctx.count) {
        return -2;
    }

    const editor_grid_internal_t* item = &g_grid_ctx.items[handle.id];
    out_info->id = item->id;
    out_info->flags = item->flags;
    out_info->initialized = item->initialized;

    return 0;
}

void editor_grid_mark_dirty(editor_grid_handle_t handle) {
    if (handle.id < g_grid_ctx.count) {
        g_grid_ctx.items[handle.id].dirty = true;
    }
}

int editor_grid_process_pending(void) {
    int processed = 0;
    for (uint32_t i = 0; i < g_grid_ctx.count; i++) {
        editor_grid_internal_t* item = &g_grid_ctx.items[i];
        if (item->initialized && item->dirty) {
            item->dirty = false;
            processed++;
        }
    }

    return processed;
}

uint32_t editor_grid_get_count(void) {
    return g_grid_ctx.count;
}

size_t editor_grid_get_memory_usage(void) {
    size_t total = sizeof(g_grid_ctx);
    total += g_grid_ctx.capacity * sizeof(editor_grid_internal_t);
    return total;
}

void editor_grid_debug_print(void) {
    // Debug output
}

/* End of editor_grid.c */
