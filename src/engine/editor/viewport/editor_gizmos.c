/*
 * editor_gizmos.c
 * Transform gizmos for translate, rotate, scale operations
 *
 * Part of the Editor subsystem
 * Advanced 3D Rendering Engine
 *
 * Implements interactive 3D manipulation handles
 */

#include "editor/viewport/editor_gizmos.h"
#include <stdint.h>
#include <stdbool.h>
#include <stddef.h>
#include <string.h>
#include <stdlib.h>

/* ============================================================================
 * CONSTANTS
 * ============================================================================ */

#define GIZMO_MAX_COUNT 64
#define GIZMO_DEFAULT_CAPACITY 8
#define GIZMO_SCREEN_SIZE 0.15f
#define GIZMO_PICK_THRESHOLD 0.05f

/* ============================================================================
 * MATH TYPES
 * ============================================================================ */

typedef struct vec2 {
    float x, y;
} vec2_t;

typedef struct vec3 {
    float x, y, z;
} vec3_t;

typedef struct vec4 {
    float x, y, z, w;
} vec4_t;

typedef struct mat4 {
    float m[16];
} mat4_t;

typedef struct ray {
    vec3_t origin;
    vec3_t direction;
} ray_t;

/* ============================================================================
 * GIZMO TYPES
 * ============================================================================ */

typedef enum gizmo_mode {
    GIZMO_MODE_TRANSLATE = 0,
    GIZMO_MODE_ROTATE,
    GIZMO_MODE_SCALE
} gizmo_mode_t;

typedef enum gizmo_space {
    GIZMO_SPACE_WORLD = 0,
    GIZMO_SPACE_LOCAL
} gizmo_space_t;

typedef enum gizmo_axis {
    AXIS_NONE = 0,
    AXIS_X = 1,
    AXIS_Y = 2,
    AXIS_Z = 3,
    AXIS_XY = 4,
    AXIS_XZ = 5,
    AXIS_YZ = 6,
    AXIS_XYZ = 7
} gizmo_axis_t;

// Colors for each axis
static const vec4_t AXIS_COLORS[] = {
    {0.5f, 0.5f, 0.5f, 1.0f},  // NONE - gray
    {1.0f, 0.2f, 0.2f, 1.0f},  // X - red
    {0.2f, 1.0f, 0.2f, 1.0f},  // Y - green
    {0.2f, 0.2f, 1.0f, 1.0f},  // Z - blue
    {1.0f, 1.0f, 0.2f, 1.0f},  // XY - yellow
    {1.0f, 0.2f, 1.0f, 1.0f},  // XZ - magenta
    {0.2f, 1.0f, 1.0f, 1.0f},  // YZ - cyan
    {1.0f, 1.0f, 1.0f, 1.0f}   // XYZ - white
};

static const vec4_t HOVER_COLOR = {1.0f, 1.0f, 0.0f, 1.0f};

typedef struct gizmo_state {
    gizmo_mode_t mode;
    gizmo_space_t space;
    
    // Interaction state
    gizmo_axis_t hovered_axis;
    gizmo_axis_t active_axis;
    bool is_dragging;
    
    // Transform being edited
    vec3_t object_position;
    vec3_t object_rotation;     // Euler angles in radians
    vec3_t object_scale;
    mat4_t object_transform;
    
    // Drag state
    vec3_t drag_start_position;
    vec3_t drag_start_rotation;
    vec3_t drag_start_scale;
    vec2_t drag_start_mouse;
    ray_t drag_start_ray;
    
    // Visual size
    float screen_scale;
} gizmo_state_t;

typedef struct editor_gizmo_internal {
    uint32_t id;
    uint32_t flags;
    gizmo_state_t state;
    bool initialized;
    bool dirty;
    uint64_t frame_updated;
} editor_gizmo_internal_t;

typedef struct editor_gizmo_context {
    editor_gizmo_internal_t* items;
    uint32_t count;
    uint32_t capacity;
    
    // Shared rendering resources (mesh handles for arrows, circles, etc.)
    uint64_t arrow_mesh;
    uint64_t circle_mesh;
    uint64_t cube_mesh;
    
    bool initialized;
} editor_gizmo_context_t;

static editor_gizmo_context_t g_gizmo_ctx = {0};

/* ============================================================================
 * MATH HELPERS
 * ============================================================================ */

static float sqrtf_approx(float x) {
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

static vec3_t vec3_add(vec3_t a, vec3_t b) {
    return (vec3_t){a.x + b.x, a.y + b.y, a.z + b.z};
}

static vec3_t vec3_sub(vec3_t a, vec3_t b) {
    return (vec3_t){a.x - b.x, a.y - b.y, a.z - b.z};
}

static vec3_t vec3_scale(vec3_t v, float s) {
    return (vec3_t){v.x * s, v.y * s, v.z * s};
}

static float vec3_dot(vec3_t a, vec3_t b) {
    return a.x * b.x + a.y * b.y + a.z * b.z;
}

static float vec3_length(vec3_t v) {
    return sqrtf_approx(vec3_dot(v, v));
}

static vec3_t vec3_normalize(vec3_t v) {
    float len = vec3_length(v);
    if (len < 0.0001f) return (vec3_t){0, 0, 1};
    return vec3_scale(v, 1.0f / len);
}

static vec3_t vec3_cross(vec3_t a, vec3_t b) {
    return (vec3_t){
        a.y * b.z - a.z * b.y,
        a.z * b.x - a.x * b.z,
        a.x * b.y - a.y * b.x
    };
}

// Ray-plane intersection
static bool ray_plane_intersect(ray_t ray, vec3_t plane_normal, vec3_t plane_point, 
                                  float* out_t, vec3_t* out_point) {
    float denom = vec3_dot(ray.direction, plane_normal);
    if (fabsf(denom) < 0.0001f) return false;
    
    vec3_t to_plane = vec3_sub(plane_point, ray.origin);
    *out_t = vec3_dot(to_plane, plane_normal) / denom;
    
    if (*out_t < 0) return false;
    
    *out_point = vec3_add(ray.origin, vec3_scale(ray.direction, *out_t));
    return true;
}

// Ray-cylinder intersection (for axis arrows)
static bool ray_cylinder_intersect(ray_t ray, vec3_t start, vec3_t end, 
                                    float radius, float* out_t) {
    vec3_t axis = vec3_sub(end, start);
    float axis_len = vec3_length(axis);
    if (axis_len < 0.0001f) return false;
    
    vec3_t axis_norm = vec3_scale(axis, 1.0f / axis_len);
    vec3_t oc = vec3_sub(ray.origin, start);
    
    float d_dot_a = vec3_dot(ray.direction, axis_norm);
    float oc_dot_a = vec3_dot(oc, axis_norm);
    
    vec3_t d_perp = vec3_sub(ray.direction, vec3_scale(axis_norm, d_dot_a));
    vec3_t oc_perp = vec3_sub(oc, vec3_scale(axis_norm, oc_dot_a));
    
    float a = vec3_dot(d_perp, d_perp);
    float b = 2.0f * vec3_dot(d_perp, oc_perp);
    float c = vec3_dot(oc_perp, oc_perp) - radius * radius;
    
    float discriminant = b * b - 4 * a * c;
    if (discriminant < 0) return false;
    
    float sqrt_disc = sqrtf_approx(discriminant);
    float t = (-b - sqrt_disc) / (2 * a);
    
    if (t < 0) {
        t = (-b + sqrt_disc) / (2 * a);
        if (t < 0) return false;
    }
    
    // Check if intersection is within cylinder bounds
    vec3_t hit = vec3_add(ray.origin, vec3_scale(ray.direction, t));
    float proj = vec3_dot(vec3_sub(hit, start), axis_norm);
    
    if (proj < 0 || proj > axis_len) return false;
    
    *out_t = t;
    return true;
}

/* ============================================================================
 * GIZMO PICKING
 * ============================================================================ */

static gizmo_axis_t gizmo_pick_translate(gizmo_state_t* gizmo, ray_t mouse_ray) {
    vec3_t pos = gizmo->object_position;
    float size = gizmo->screen_scale;
    float pick_radius = size * 0.05f;
    
    float best_t = 1e10f;
    gizmo_axis_t best_axis = AXIS_NONE;
    float t;
    
    // Test X axis (red arrow)
    if (ray_cylinder_intersect(mouse_ray, pos, 
                                vec3_add(pos, (vec3_t){size, 0, 0}), 
                                pick_radius, &t)) {
        if (t < best_t) { best_t = t; best_axis = AXIS_X; }
    }
    
    // Test Y axis (green arrow)
    if (ray_cylinder_intersect(mouse_ray, pos, 
                                vec3_add(pos, (vec3_t){0, size, 0}), 
                                pick_radius, &t)) {
        if (t < best_t) { best_t = t; best_axis = AXIS_Y; }
    }
    
    // Test Z axis (blue arrow)
    if (ray_cylinder_intersect(mouse_ray, pos, 
                                vec3_add(pos, (vec3_t){0, 0, size}), 
                                pick_radius, &t)) {
        if (t < best_t) { best_t = t; best_axis = AXIS_Z; }
    }
    
    // Test XY plane quad
    vec3_t hit_point;
    if (ray_plane_intersect(mouse_ray, (vec3_t){0, 0, 1}, pos, &t, &hit_point)) {
        vec3_t local = vec3_sub(hit_point, pos);
        if (local.x > 0 && local.x < size * 0.3f && 
            local.y > 0 && local.y < size * 0.3f && t < best_t) {
            best_t = t;
            best_axis = AXIS_XY;
        }
    }
    
    // Test XZ plane quad  
    if (ray_plane_intersect(mouse_ray, (vec3_t){0, 1, 0}, pos, &t, &hit_point)) {
        vec3_t local = vec3_sub(hit_point, pos);
        if (local.x > 0 && local.x < size * 0.3f && 
            local.z > 0 && local.z < size * 0.3f && t < best_t) {
            best_t = t;
            best_axis = AXIS_XZ;
        }
    }
    
    // Test YZ plane quad
    if (ray_plane_intersect(mouse_ray, (vec3_t){1, 0, 0}, pos, &t, &hit_point)) {
        vec3_t local = vec3_sub(hit_point, pos);
        if (local.y > 0 && local.y < size * 0.3f && 
            local.z > 0 && local.z < size * 0.3f && t < best_t) {
            best_t = t;
            best_axis = AXIS_YZ;
        }
    }
    
    return best_axis;
}

/* ============================================================================
 * GIZMO DRAGGING
 * ============================================================================ */

static vec3_t gizmo_compute_translate_delta(gizmo_state_t* gizmo, 
                                              ray_t current_ray) {
    vec3_t delta = {0, 0, 0};
    vec3_t plane_normal = {0, 1, 0};
    vec3_t pos = gizmo->drag_start_position;
    
    // Choose constraint plane based on active axis
    switch (gizmo->active_axis) {
        case AXIS_X:
        case AXIS_XY:
        case AXIS_XZ:
            plane_normal = (vec3_t){0, 1, 0};  // Constrain to XZ or XY plane
            if (gizmo->active_axis == AXIS_XY) plane_normal = (vec3_t){0, 0, 1};
            break;
        case AXIS_Y:
        case AXIS_YZ:
            plane_normal = (vec3_t){1, 0, 0};  // Constrain to YZ plane
            break;
        case AXIS_Z:
            plane_normal = (vec3_t){0, 1, 0};  // Constrain to XZ plane
            break;
        default:
            return delta;
    }
    
    float t_start, t_current;
    vec3_t hit_start, hit_current;
    
    if (ray_plane_intersect(gizmo->drag_start_ray, plane_normal, pos, &t_start, &hit_start) &&
        ray_plane_intersect(current_ray, plane_normal, pos, &t_current, &hit_current)) {
        
        delta = vec3_sub(hit_current, hit_start);
        
        // Constrain to single axis if needed
        if (gizmo->active_axis == AXIS_X) {
            delta.y = 0; delta.z = 0;
        } else if (gizmo->active_axis == AXIS_Y) {
            delta.x = 0; delta.z = 0;
        } else if (gizmo->active_axis == AXIS_Z) {
            delta.x = 0; delta.y = 0;
        }
    }
    
    return delta;
}

/* ============================================================================
 * PUBLIC API
 * ============================================================================ */

int editor_gizmo_init(void) {
    if (g_gizmo_ctx.initialized) {
        return 0;
    }

    g_gizmo_ctx.capacity = GIZMO_DEFAULT_CAPACITY;
    g_gizmo_ctx.items = calloc(g_gizmo_ctx.capacity, sizeof(editor_gizmo_internal_t));
    if (!g_gizmo_ctx.items) {
        return -1;
    }

    // Create shared gizmo meshes
    // g_gizmo_ctx.arrow_mesh = create_arrow_mesh();
    // g_gizmo_ctx.circle_mesh = create_circle_mesh();
    // g_gizmo_ctx.cube_mesh = create_cube_mesh();

    g_gizmo_ctx.count = 0;
    g_gizmo_ctx.initialized = true;

    return 0;
}

void editor_gizmo_shutdown(void) {
    if (!g_gizmo_ctx.initialized) {
        return;
    }

    free(g_gizmo_ctx.items);
    g_gizmo_ctx.items = NULL;
    g_gizmo_ctx.count = 0;
    g_gizmo_ctx.capacity = 0;
    g_gizmo_ctx.initialized = false;
}

int editor_gizmo_create(editor_gizmo_handle_t* out_handle, 
                         const editor_gizmo_desc_t* desc) {
    if (!out_handle || !desc) {
        return -1;
    }

    if (!g_gizmo_ctx.initialized) {
        return -2;
    }

    if (g_gizmo_ctx.count >= g_gizmo_ctx.capacity) {
        return -3;
    }

    uint32_t index = g_gizmo_ctx.count++;
    editor_gizmo_internal_t* item = &g_gizmo_ctx.items[index];

    item->id = index;
    item->flags = desc->flags;
    
    // Initialize gizmo state
    item->state.mode = GIZMO_MODE_TRANSLATE;
    item->state.space = GIZMO_SPACE_WORLD;
    item->state.hovered_axis = AXIS_NONE;
    item->state.active_axis = AXIS_NONE;
    item->state.is_dragging = false;
    item->state.object_position = (vec3_t){0, 0, 0};
    item->state.object_rotation = (vec3_t){0, 0, 0};
    item->state.object_scale = (vec3_t){1, 1, 1};
    item->state.screen_scale = 1.0f;
    
    item->initialized = true;
    item->dirty = true;
    item->frame_updated = 0;

    out_handle->id = index;
    return 0;
}

void editor_gizmo_destroy(editor_gizmo_handle_t handle) {
    if (handle.id >= g_gizmo_ctx.count) {
        return;
    }

    g_gizmo_ctx.items[handle.id].initialized = false;
}

int editor_gizmo_set_mode(editor_gizmo_handle_t handle, gizmo_mode_t mode) {
    if (handle.id >= g_gizmo_ctx.count) {
        return -1;
    }
    
    g_gizmo_ctx.items[handle.id].state.mode = mode;
    return 0;
}

int editor_gizmo_set_space(editor_gizmo_handle_t handle, gizmo_space_t space) {
    if (handle.id >= g_gizmo_ctx.count) {
        return -1;
    }
    
    g_gizmo_ctx.items[handle.id].state.space = space;
    return 0;
}

int editor_gizmo_set_transform(editor_gizmo_handle_t handle,
                                 vec3_t position, vec3_t rotation, vec3_t scale) {
    if (handle.id >= g_gizmo_ctx.count) {
        return -1;
    }
    
    gizmo_state_t* state = &g_gizmo_ctx.items[handle.id].state;
    state->object_position = position;
    state->object_rotation = rotation;
    state->object_scale = scale;
    
    return 0;
}

int editor_gizmo_update_hover(editor_gizmo_handle_t handle, ray_t mouse_ray) {
    if (handle.id >= g_gizmo_ctx.count) {
        return -1;
    }
    
    gizmo_state_t* state = &g_gizmo_ctx.items[handle.id].state;
    
    if (!state->is_dragging) {
        switch (state->mode) {
            case GIZMO_MODE_TRANSLATE:
                state->hovered_axis = gizmo_pick_translate(state, mouse_ray);
                break;
            case GIZMO_MODE_ROTATE:
                // Similar picking for rotation rings
                break;
            case GIZMO_MODE_SCALE:
                // Similar picking for scale handles
                break;
        }
    }
    
    return 0;
}

int editor_gizmo_begin_drag(editor_gizmo_handle_t handle, ray_t mouse_ray, vec2_t mouse_pos) {
    if (handle.id >= g_gizmo_ctx.count) {
        return -1;
    }
    
    gizmo_state_t* state = &g_gizmo_ctx.items[handle.id].state;
    
    if (state->hovered_axis != AXIS_NONE) {
        state->is_dragging = true;
        state->active_axis = state->hovered_axis;
        state->drag_start_position = state->object_position;
        state->drag_start_rotation = state->object_rotation;
        state->drag_start_scale = state->object_scale;
        state->drag_start_mouse = mouse_pos;
        state->drag_start_ray = mouse_ray;
        return 0;
    }
    
    return -2;  // Nothing to drag
}

int editor_gizmo_update_drag(editor_gizmo_handle_t handle, ray_t mouse_ray,
                               vec3_t* out_position, vec3_t* out_rotation, vec3_t* out_scale) {
    if (handle.id >= g_gizmo_ctx.count) {
        return -1;
    }
    
    gizmo_state_t* state = &g_gizmo_ctx.items[handle.id].state;
    
    if (!state->is_dragging) {
        return -2;
    }
    
    switch (state->mode) {
        case GIZMO_MODE_TRANSLATE: {
            vec3_t delta = gizmo_compute_translate_delta(state, mouse_ray);
            state->object_position = vec3_add(state->drag_start_position, delta);
            break;
        }
        case GIZMO_MODE_ROTATE:
            // Rotation drag implementation
            break;
        case GIZMO_MODE_SCALE:
            // Scale drag implementation  
            break;
    }
    
    if (out_position) *out_position = state->object_position;
    if (out_rotation) *out_rotation = state->object_rotation;
    if (out_scale) *out_scale = state->object_scale;
    
    return 0;
}

int editor_gizmo_end_drag(editor_gizmo_handle_t handle) {
    if (handle.id >= g_gizmo_ctx.count) {
        return -1;
    }
    
    gizmo_state_t* state = &g_gizmo_ctx.items[handle.id].state;
    state->is_dragging = false;
    state->active_axis = AXIS_NONE;
    
    return 0;
}

void editor_gizmo_cancel_drag(editor_gizmo_handle_t handle) {
    if (handle.id >= g_gizmo_ctx.count) {
        return;
    }
    
    gizmo_state_t* state = &g_gizmo_ctx.items[handle.id].state;
    
    if (state->is_dragging) {
        // Restore original transform
        state->object_position = state->drag_start_position;
        state->object_rotation = state->drag_start_rotation;
        state->object_scale = state->drag_start_scale;
        state->is_dragging = false;
        state->active_axis = AXIS_NONE;
    }
}

gizmo_axis_t editor_gizmo_get_hovered_axis(editor_gizmo_handle_t handle) {
    if (handle.id >= g_gizmo_ctx.count) {
        return AXIS_NONE;
    }
    return g_gizmo_ctx.items[handle.id].state.hovered_axis;
}

bool editor_gizmo_is_dragging(editor_gizmo_handle_t handle) {
    if (handle.id >= g_gizmo_ctx.count) {
        return false;
    }
    return g_gizmo_ctx.items[handle.id].state.is_dragging;
}

int editor_gizmo_update(editor_gizmo_handle_t handle, const void* data, size_t size) {
    if (handle.id >= g_gizmo_ctx.count) {
        return -1;
    }

    g_gizmo_ctx.items[handle.id].dirty = true;
    return 0;
}

bool editor_gizmo_is_valid(editor_gizmo_handle_t handle) {
    if (handle.id >= g_gizmo_ctx.count) {
        return false;
    }
    return g_gizmo_ctx.items[handle.id].initialized;
}

int editor_gizmo_get_info(editor_gizmo_handle_t handle, editor_gizmo_info_t* out_info) {
    if (!out_info) {
        return -1;
    }

    if (handle.id >= g_gizmo_ctx.count) {
        return -2;
    }

    const editor_gizmo_internal_t* item = &g_gizmo_ctx.items[handle.id];
    out_info->id = item->id;
    out_info->flags = item->flags;
    out_info->initialized = item->initialized;

    return 0;
}

void editor_gizmo_mark_dirty(editor_gizmo_handle_t handle) {
    if (handle.id < g_gizmo_ctx.count) {
        g_gizmo_ctx.items[handle.id].dirty = true;
    }
}

int editor_gizmo_process_pending(void) {
    int processed = 0;
    for (uint32_t i = 0; i < g_gizmo_ctx.count; i++) {
        editor_gizmo_internal_t* item = &g_gizmo_ctx.items[i];
        if (item->initialized && item->dirty) {
            item->dirty = false;
            processed++;
        }
    }

    return processed;
}

uint32_t editor_gizmo_get_count(void) {
    return g_gizmo_ctx.count;
}

size_t editor_gizmo_get_memory_usage(void) {
    size_t total = sizeof(g_gizmo_ctx);
    total += g_gizmo_ctx.capacity * sizeof(editor_gizmo_internal_t);
    return total;
}

void editor_gizmo_debug_print(void) {
    // Debug output
}

/* End of editor_gizmos.c */
