/*
 * editor_camera.c
 * Editor camera controls (fly, orbit, focus)
 *
 * Part of the Editor subsystem
 * Advanced 3D Rendering Engine
 *
 * Implements fly camera, orbit camera, and focus-on-selection
 */

#include "editor_camera.h"
#include <stdint.h>
#include <stdbool.h>
#include <stddef.h>
#include <string.h>
#include <stdlib.h>

/* ============================================================================
 * CONSTANTS
 * ============================================================================ */

#define CAMERA_MAX_COUNT 16
#define CAMERA_DEFAULT_CAPACITY 4
#define CAMERA_FLY_SPEED 10.0f
#define CAMERA_ORBIT_SPEED 0.005f
#define CAMERA_ZOOM_SPEED 1.0f
#define CAMERA_SMOOTH_FACTOR 10.0f
#define CAMERA_MIN_PITCH -1.5f
#define CAMERA_MAX_PITCH 1.5f

/* ============================================================================
 * MATH TYPES
 * ============================================================================ */

typedef struct vec2 {
    float x, y;
} vec2_t;

typedef struct vec3 {
    float x, y, z;
} vec3_t;

typedef struct mat4 {
    float m[16];
} mat4_t;

/* ============================================================================
 * CAMERA TYPES
 * ============================================================================ */

typedef enum camera_mode {
    CAMERA_MODE_FLY = 0,        // WASD + mouse look
    CAMERA_MODE_ORBIT,          // Orbit around pivot
    CAMERA_MODE_PAN             // Pan on view plane
} camera_mode_t;

typedef struct camera_state {
    // Position and orientation
    vec3_t position;
    vec3_t target;              // Look-at target (for orbit mode)
    float yaw;                  // Horizontal rotation (radians)
    float pitch;                // Vertical rotation (radians)
    float roll;                 // Roll (usually 0)
    
    // Orbit mode
    float orbit_distance;       // Distance from target
    float orbit_yaw;
    float orbit_pitch;
    
    // Smooth interpolation targets
    vec3_t target_position;
    vec3_t target_look_at;
    float target_yaw;
    float target_pitch;
    float target_orbit_distance;
    
    // Movement
    vec3_t velocity;
    float move_speed;
    float look_sensitivity;
    float zoom_sensitivity;
    
    // Input state
    bool moving_forward;
    bool moving_backward;
    bool moving_left;
    bool moving_right;
    bool moving_up;
    bool moving_down;
    bool shift_held;            // Speed boost
    
    // View properties
    float fov;
    float near_plane;
    float far_plane;
    float aspect_ratio;
    
    // Mode
    camera_mode_t mode;
    bool smooth_movement;
} camera_state_t;

typedef struct editor_camera_internal {
    uint32_t id;
    uint32_t flags;
    camera_state_t state;
    mat4_t view_matrix;
    mat4_t projection_matrix;
    bool matrices_dirty;
    bool initialized;
    uint64_t frame_updated;
} editor_camera_internal_t;

typedef struct editor_camera_context {
    editor_camera_internal_t* items;
    uint32_t count;
    uint32_t capacity;
    bool initialized;
} editor_camera_context_t;

static editor_camera_context_t g_camera_ctx = {0};

/* ============================================================================
 * MATH HELPERS
 * ============================================================================ */

static float clampf(float v, float min, float max) {
    if (v < min) return min;
    if (v > max) return max;
    return v;
}

static float lerpf(float a, float b, float t) {
    return a + (b - a) * t;
}

static float sinf_approx(float x);
static float cosf_approx(float x);

static float sinf_approx(float x) {
    // Normalize to [-PI, PI]
    while (x > 3.14159f) x -= 6.28318f;
    while (x < -3.14159f) x += 6.28318f;
    float x2 = x * x;
    return x * (1.0f - x2 / 6.0f * (1.0f - x2 / 20.0f));
}

static float cosf_approx(float x) {
    while (x > 3.14159f) x -= 6.28318f;
    while (x < -3.14159f) x += 6.28318f;
    float x2 = x * x;
    return 1.0f - x2 / 2.0f * (1.0f - x2 / 12.0f);
}

static float tanf_approx(float x) {
    float c = cosf_approx(x);
    if (c < 0.0001f && c > -0.0001f) return 0;
    return sinf_approx(x) / c;
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
    float len_sq = vec3_dot(v, v);
    if (len_sq <= 0) return 0;
    float guess = len_sq / 2.0f;
    for (int i = 0; i < 8; i++) {
        guess = (guess + len_sq / guess) / 2.0f;
    }
    return guess;
}

static vec3_t vec3_normalize(vec3_t v) {
    float len = vec3_length(v);
    if (len < 0.0001f) return (vec3_t){0, 0, -1};
    return vec3_scale(v, 1.0f / len);
}

static vec3_t vec3_cross(vec3_t a, vec3_t b) {
    return (vec3_t){
        a.y * b.z - a.z * b.y,
        a.z * b.x - a.x * b.z,
        a.x * b.y - a.y * b.x
    };
}

static vec3_t vec3_lerp(vec3_t a, vec3_t b, float t) {
    return (vec3_t){
        lerpf(a.x, b.x, t),
        lerpf(a.y, b.y, t),
        lerpf(a.z, b.z, t)
    };
}

/* ============================================================================
 * MATRIX HELPERS
 * ============================================================================ */

static void mat4_look_at(mat4_t* m, vec3_t eye, vec3_t target, vec3_t up) {
    vec3_t f = vec3_normalize(vec3_sub(target, eye));
    vec3_t r = vec3_normalize(vec3_cross(f, up));
    vec3_t u = vec3_cross(r, f);
    
    m->m[0] = r.x;  m->m[4] = r.y;  m->m[8]  = r.z;  m->m[12] = -vec3_dot(r, eye);
    m->m[1] = u.x;  m->m[5] = u.y;  m->m[9]  = u.z;  m->m[13] = -vec3_dot(u, eye);
    m->m[2] = -f.x; m->m[6] = -f.y; m->m[10] = -f.z; m->m[14] = vec3_dot(f, eye);
    m->m[3] = 0;    m->m[7] = 0;    m->m[11] = 0;    m->m[15] = 1;
}

static void mat4_perspective(mat4_t* m, float fov, float aspect, float near, float far) {
    float tan_half_fov = tanf_approx(fov / 2.0f);
    
    memset(m->m, 0, sizeof(m->m));
    m->m[0] = 1.0f / (aspect * tan_half_fov);
    m->m[5] = 1.0f / tan_half_fov;
    m->m[10] = -(far + near) / (far - near);
    m->m[11] = -1.0f;
    m->m[14] = -(2.0f * far * near) / (far - near);
}

/* ============================================================================
 * CAMERA DIRECTION HELPERS
 * ============================================================================ */

static vec3_t camera_get_forward(camera_state_t* cam) {
    float cy = cosf_approx(cam->yaw);
    float sy = sinf_approx(cam->yaw);
    float cp = cosf_approx(cam->pitch);
    float sp = sinf_approx(cam->pitch);
    
    return (vec3_t){
        sy * cp,
        -sp,
        -cy * cp
    };
}

static vec3_t camera_get_right(camera_state_t* cam) {
    float cy = cosf_approx(cam->yaw);
    float sy = sinf_approx(cam->yaw);
    return (vec3_t){cy, 0, sy};
}

static vec3_t camera_get_up(camera_state_t* cam) {
    vec3_t forward = camera_get_forward(cam);
    vec3_t right = camera_get_right(cam);
    return vec3_cross(right, forward);
}

/* ============================================================================
 * CAMERA UPDATE
 * ============================================================================ */

static void camera_update_fly(editor_camera_internal_t* cam_item, float dt) {
    camera_state_t* cam = &cam_item->state;
    
    vec3_t move_dir = {0, 0, 0};
    vec3_t forward = camera_get_forward(cam);
    vec3_t right = camera_get_right(cam);
    vec3_t up = {0, 1, 0};
    
    if (cam->moving_forward)  move_dir = vec3_add(move_dir, forward);
    if (cam->moving_backward) move_dir = vec3_sub(move_dir, forward);
    if (cam->moving_right)    move_dir = vec3_add(move_dir, right);
    if (cam->moving_left)     move_dir = vec3_sub(move_dir, right);
    if (cam->moving_up)       move_dir = vec3_add(move_dir, up);
    if (cam->moving_down)     move_dir = vec3_sub(move_dir, up);
    
    float len = vec3_length(move_dir);
    if (len > 0.001f) {
        move_dir = vec3_scale(move_dir, 1.0f / len);
        
        float speed = cam->move_speed;
        if (cam->shift_held) speed *= 3.0f;
        
        cam->target_position = vec3_add(cam->target_position, 
                                         vec3_scale(move_dir, speed * dt));
    }
    
    // Smooth position interpolation
    if (cam->smooth_movement) {
        float t = 1.0f - expf(-CAMERA_SMOOTH_FACTOR * dt);
        cam->position = vec3_lerp(cam->position, cam->target_position, t);
    } else {
        cam->position = cam->target_position;
    }
    
    cam_item->matrices_dirty = true;
}

static float expf(float x) {
    // Approximation of e^x
    float result = 1.0f + x + x*x/2.0f + x*x*x/6.0f;
    return result;
}

static void camera_update_orbit(editor_camera_internal_t* cam_item, float dt) {
    camera_state_t* cam = &cam_item->state;
    
    // Smooth orbit parameters
    if (cam->smooth_movement) {
        float t = 1.0f - expf(-CAMERA_SMOOTH_FACTOR * dt);
        cam->orbit_distance = lerpf(cam->orbit_distance, cam->target_orbit_distance, t);
        cam->target = vec3_lerp(cam->target, cam->target_look_at, t);
    } else {
        cam->orbit_distance = cam->target_orbit_distance;
        cam->target = cam->target_look_at;
    }
    
    // Calculate position from orbit parameters
    float cy = cosf_approx(cam->orbit_yaw);
    float sy = sinf_approx(cam->orbit_yaw);
    float cp = cosf_approx(cam->orbit_pitch);
    float sp = sinf_approx(cam->orbit_pitch);
    
    vec3_t offset = {
        sy * cp * cam->orbit_distance,
        sp * cam->orbit_distance,
        cy * cp * cam->orbit_distance
    };
    
    cam->position = vec3_add(cam->target, offset);
    
    // Update yaw/pitch to look at target
    vec3_t to_target = vec3_normalize(vec3_sub(cam->target, cam->position));
    cam->yaw = atan2f_approx(to_target.x, -to_target.z);
    cam->pitch = asinf_approx(-to_target.y);
    
    cam_item->matrices_dirty = true;
}

static float atan2f_approx(float y, float x) {
    // Approximation of atan2
    if (x == 0 && y == 0) return 0;
    
    float abs_x = x < 0 ? -x : x;
    float abs_y = y < 0 ? -y : y;
    
    float a = (abs_x < abs_y) ? (abs_x / abs_y) : (abs_y / abs_x);
    float s = a * a;
    float r = ((-0.0464964749f * s + 0.15931422f) * s - 0.327622764f) * s * a + a;
    
    if (abs_y > abs_x) r = 1.57079637f - r;
    if (x < 0) r = 3.14159265f - r;
    if (y < 0) r = -r;
    
    return r;
}

static float asinf_approx(float x) {
    x = clampf(x, -1.0f, 1.0f);
    return x + x*x*x/6.0f;
}

static void camera_update_matrices(editor_camera_internal_t* cam_item) {
    if (!cam_item->matrices_dirty) return;
    
    camera_state_t* cam = &cam_item->state;
    
    // Calculate view matrix
    vec3_t forward = camera_get_forward(cam);
    vec3_t look_at = vec3_add(cam->position, forward);
    mat4_look_at(&cam_item->view_matrix, cam->position, look_at, (vec3_t){0, 1, 0});
    
    // Calculate projection matrix
    mat4_perspective(&cam_item->projection_matrix, cam->fov, cam->aspect_ratio,
                     cam->near_plane, cam->far_plane);
    
    cam_item->matrices_dirty = false;
}

/* ============================================================================
 * PUBLIC API
 * ============================================================================ */

int editor_camera_init(void) {
    if (g_camera_ctx.initialized) {
        return 0;
    }

    g_camera_ctx.capacity = CAMERA_DEFAULT_CAPACITY;
    g_camera_ctx.items = calloc(g_camera_ctx.capacity, sizeof(editor_camera_internal_t));
    if (!g_camera_ctx.items) {
        return -1;
    }

    g_camera_ctx.count = 0;
    g_camera_ctx.initialized = true;

    return 0;
}

void editor_camera_shutdown(void) {
    if (!g_camera_ctx.initialized) {
        return;
    }

    free(g_camera_ctx.items);
    g_camera_ctx.items = NULL;
    g_camera_ctx.count = 0;
    g_camera_ctx.capacity = 0;
    g_camera_ctx.initialized = false;
}

int editor_camera_create(editor_camera_handle_t* out_handle, 
                          const editor_camera_desc_t* desc) {
    if (!out_handle || !desc) {
        return -1;
    }

    if (!g_camera_ctx.initialized) {
        return -2;
    }

    if (g_camera_ctx.count >= g_camera_ctx.capacity) {
        return -3;
    }

    uint32_t index = g_camera_ctx.count++;
    editor_camera_internal_t* item = &g_camera_ctx.items[index];

    item->id = index;
    item->flags = desc->flags;
    
    // Initialize camera state with defaults
    camera_state_t* cam = &item->state;
    cam->position = (vec3_t){0, 5, 10};
    cam->target = (vec3_t){0, 0, 0};
    cam->target_position = cam->position;
    cam->target_look_at = cam->target;
    cam->yaw = 0;
    cam->pitch = 0;
    cam->roll = 0;
    cam->target_yaw = 0;
    cam->target_pitch = 0;
    cam->orbit_distance = 10.0f;
    cam->target_orbit_distance = 10.0f;
    cam->orbit_yaw = 0;
    cam->orbit_pitch = 0.3f;
    cam->velocity = (vec3_t){0, 0, 0};
    cam->move_speed = CAMERA_FLY_SPEED;
    cam->look_sensitivity = CAMERA_ORBIT_SPEED;
    cam->zoom_sensitivity = CAMERA_ZOOM_SPEED;
    cam->fov = 1.047f;  // 60 degrees
    cam->near_plane = 0.1f;
    cam->far_plane = 1000.0f;
    cam->aspect_ratio = 16.0f / 9.0f;
    cam->mode = CAMERA_MODE_ORBIT;
    cam->smooth_movement = true;
    
    item->matrices_dirty = true;
    item->initialized = true;
    item->frame_updated = 0;

    out_handle->id = index;
    return 0;
}

void editor_camera_destroy(editor_camera_handle_t handle) {
    if (handle.id >= g_camera_ctx.count) {
        return;
    }

    g_camera_ctx.items[handle.id].initialized = false;
}

int editor_camera_set_mode(editor_camera_handle_t handle, camera_mode_t mode) {
    if (handle.id >= g_camera_ctx.count) {
        return -1;
    }
    
    g_camera_ctx.items[handle.id].state.mode = mode;
    return 0;
}

int editor_camera_look(editor_camera_handle_t handle, float delta_yaw, float delta_pitch) {
    if (handle.id >= g_camera_ctx.count) {
        return -1;
    }
    
    camera_state_t* cam = &g_camera_ctx.items[handle.id].state;
    
    if (cam->mode == CAMERA_MODE_FLY) {
        cam->yaw += delta_yaw * cam->look_sensitivity;
        cam->pitch += delta_pitch * cam->look_sensitivity;
        cam->pitch = clampf(cam->pitch, CAMERA_MIN_PITCH, CAMERA_MAX_PITCH);
    } else if (cam->mode == CAMERA_MODE_ORBIT) {
        cam->orbit_yaw += delta_yaw * cam->look_sensitivity;
        cam->orbit_pitch += delta_pitch * cam->look_sensitivity;
        cam->orbit_pitch = clampf(cam->orbit_pitch, -1.4f, 1.4f);
    }
    
    g_camera_ctx.items[handle.id].matrices_dirty = true;
    return 0;
}

int editor_camera_zoom(editor_camera_handle_t handle, float delta) {
    if (handle.id >= g_camera_ctx.count) {
        return -1;
    }
    
    camera_state_t* cam = &g_camera_ctx.items[handle.id].state;
    
    if (cam->mode == CAMERA_MODE_ORBIT) {
        cam->target_orbit_distance -= delta * cam->zoom_sensitivity;
        if (cam->target_orbit_distance < 0.5f) cam->target_orbit_distance = 0.5f;
        if (cam->target_orbit_distance > 500.0f) cam->target_orbit_distance = 500.0f;
    } else if (cam->mode == CAMERA_MODE_FLY) {
        // Move forward/backward
        vec3_t forward = camera_get_forward(cam);
        cam->target_position = vec3_add(cam->target_position, 
                                         vec3_scale(forward, delta * cam->zoom_sensitivity));
    }
    
    g_camera_ctx.items[handle.id].matrices_dirty = true;
    return 0;
}

int editor_camera_focus(editor_camera_handle_t handle, vec3_t target, float distance) {
    if (handle.id >= g_camera_ctx.count) {
        return -1;
    }
    
    camera_state_t* cam = &g_camera_ctx.items[handle.id].state;
    cam->target_look_at = target;
    cam->target_orbit_distance = distance;
    
    g_camera_ctx.items[handle.id].matrices_dirty = true;
    return 0;
}

int editor_camera_set_movement(editor_camera_handle_t handle,
                                 bool forward, bool backward,
                                 bool left, bool right,
                                 bool up, bool down) {
    if (handle.id >= g_camera_ctx.count) {
        return -1;
    }
    
    camera_state_t* cam = &g_camera_ctx.items[handle.id].state;
    cam->moving_forward = forward;
    cam->moving_backward = backward;
    cam->moving_left = left;
    cam->moving_right = right;
    cam->moving_up = up;
    cam->moving_down = down;
    
    return 0;
}

int editor_camera_tick(editor_camera_handle_t handle, float dt) {
    if (handle.id >= g_camera_ctx.count) {
        return -1;
    }
    
    editor_camera_internal_t* item = &g_camera_ctx.items[handle.id];
    if (!item->initialized) {
        return -2;
    }
    
    switch (item->state.mode) {
        case CAMERA_MODE_FLY:
            camera_update_fly(item, dt);
            break;
        case CAMERA_MODE_ORBIT:
            camera_update_orbit(item, dt);
            break;
        default:
            break;
    }
    
    camera_update_matrices(item);
    item->frame_updated++;
    
    return 0;
}

int editor_camera_get_matrices(editor_camera_handle_t handle,
                                 mat4_t* out_view, mat4_t* out_projection) {
    if (handle.id >= g_camera_ctx.count) {
        return -1;
    }
    
    editor_camera_internal_t* item = &g_camera_ctx.items[handle.id];
    
    if (out_view) *out_view = item->view_matrix;
    if (out_projection) *out_projection = item->projection_matrix;
    
    return 0;
}

vec3_t editor_camera_get_position(editor_camera_handle_t handle) {
    if (handle.id >= g_camera_ctx.count) {
        return (vec3_t){0, 0, 0};
    }
    return g_camera_ctx.items[handle.id].state.position;
}

vec3_t editor_camera_get_forward(editor_camera_handle_t handle) {
    if (handle.id >= g_camera_ctx.count) {
        return (vec3_t){0, 0, -1};
    }
    return camera_get_forward(&g_camera_ctx.items[handle.id].state);
}

int editor_camera_update(editor_camera_handle_t handle, const void* data, size_t size) {
    if (handle.id >= g_camera_ctx.count) {
        return -1;
    }

    g_camera_ctx.items[handle.id].matrices_dirty = true;
    return 0;
}

bool editor_camera_is_valid(editor_camera_handle_t handle) {
    if (handle.id >= g_camera_ctx.count) {
        return false;
    }
    return g_camera_ctx.items[handle.id].initialized;
}

int editor_camera_get_info(editor_camera_handle_t handle, editor_camera_info_t* out_info) {
    if (!out_info) {
        return -1;
    }

    if (handle.id >= g_camera_ctx.count) {
        return -2;
    }

    const editor_camera_internal_t* item = &g_camera_ctx.items[handle.id];
    out_info->id = item->id;
    out_info->flags = item->flags;
    out_info->initialized = item->initialized;

    return 0;
}

void editor_camera_mark_dirty(editor_camera_handle_t handle) {
    if (handle.id < g_camera_ctx.count) {
        g_camera_ctx.items[handle.id].matrices_dirty = true;
    }
}

int editor_camera_process_pending(void) {
    int processed = 0;
    for (uint32_t i = 0; i < g_camera_ctx.count; i++) {
        editor_camera_internal_t* item = &g_camera_ctx.items[i];
        if (item->initialized && item->matrices_dirty) {
            camera_update_matrices(item);
            processed++;
        }
    }

    return processed;
}

uint32_t editor_camera_get_count(void) {
    return g_camera_ctx.count;
}

size_t editor_camera_get_memory_usage(void) {
    size_t total = sizeof(g_camera_ctx);
    total += g_camera_ctx.capacity * sizeof(editor_camera_internal_t);
    return total;
}

void editor_camera_debug_print(void) {
    // Debug output
}

/* End of editor_camera.c */
