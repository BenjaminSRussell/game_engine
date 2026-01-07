/*
 * cascade_splits.c
 * Cascade split calculation for shadow mapping
 *
 * Part of the Lighting subsystem
 * Advanced 3D Rendering Engine
 */

#include "cascade_splits.h"
#include <math.h>
#include <string.h>
#include <float.h>

/* ============================================================================
 * MATH HELPERS
 * ============================================================================ */

typedef struct vec3 {
    float x, y, z;
} vec3_t;

typedef struct vec4 {
    float x, y, z, w;
} vec4_t;

static vec3_t vec3_add(vec3_t a, vec3_t b) { return (vec3_t){a.x + b.x, a.y + b.y, a.z + b.z}; }
static vec3_t vec3_sub(vec3_t a, vec3_t b) { return (vec3_t){a.x - b.x, a.y - b.y, a.z - b.z}; }
static vec3_t vec3_scale(vec3_t v, float s) { return (vec3_t){v.x * s, v.y * s, v.z * s}; }

static float vec3_dot(vec3_t a, vec3_t b) { return a.x * b.x + a.y * b.y + a.z * b.z; }

static vec3_t vec3_cross(vec3_t a, vec3_t b) {
    return (vec3_t){
        a.y * b.z - a.z * b.y,
        a.z * b.x - a.x * b.z,
        a.x * b.y - a.y * b.x
    };
}

static vec3_t vec3_normalize(vec3_t v) {
    float len = sqrtf(v.x * v.x + v.y * v.y + v.z * v.z);
    if (len > 1e-6f) return vec3_scale(v, 1.0f / len);
    return v;
}

static void mat4_multiply(const float* a, const float* b, float* out) {
    for (int row = 0; row < 4; row++) {
        for (int col = 0; col < 4; col++) {
            float sum = 0.0f;
            for (int k = 0; k < 4; k++) {
                sum += a[row * 4 + k] * b[k * 4 + col];
            }
            out[row * 4 + col] = sum;
        }
    }
}

static void mat4_ortho(float left, float right, float bottom, float top, float near, float far, float* out) {
    memset(out, 0, sizeof(float) * 16);
    out[0] = 2.0f / (right - left);
    out[5] = 2.0f / (top - bottom);
    out[10] = -2.0f / (far - near);
    out[12] = -(right + left) / (right - left);
    out[13] = -(top + bottom) / (top - bottom);
    out[14] = -(far + near) / (far - near);
    out[15] = 1.0f;
}

static void mat4_look_at(vec3_t eye, vec3_t target, vec3_t up, float* out) {
    vec3_t f = vec3_normalize(vec3_sub(target, eye));
    vec3_t s = vec3_normalize(vec3_cross(f, up));
    vec3_t u = vec3_cross(s, f);

    memset(out, 0, sizeof(float) * 16);
    out[0] = s.x;  out[4] = s.y;  out[8] = s.z;
    out[1] = u.x;  out[5] = u.y;  out[9] = u.z;
    out[2] = -f.x; out[6] = -f.y; out[10] = -f.z;
    out[12] = -vec3_dot(s, eye);
    out[13] = -vec3_dot(u, eye);
    out[14] = vec3_dot(f, eye);
    out[15] = 1.0f;
}

/* ============================================================================
 * IMPLEMENTATION
 * ============================================================================ */

void cascade_calculate_matrix(const cascade_camera_t* camera,
                             const float* light_dir_ptr,
                             float near_z,
                             float far_z,
                             float* out_matrix) {
    vec3_t light_dir = {light_dir_ptr[0], light_dir_ptr[1], light_dir_ptr[2]};
    vec3_t cam_pos = {camera->position[0], camera->position[1], camera->position[2]};
    vec3_t cam_fwd = {camera->forward[0], camera->forward[1], camera->forward[2]};
    vec3_t cam_up = {camera->up[0], camera->up[1], camera->up[2]};
    vec3_t cam_right = vec3_normalize(vec3_cross(cam_fwd, cam_up));

    // Calculate frustum corners in world space
    float aspect = camera->aspect_ratio;
    float tan_fov = tanf(camera->fov * 0.5f);
    
    float near_height = 2.0f * tan_fov * near_z;
    float near_width = near_height * aspect;
    float far_height = 2.0f * tan_fov * far_z;
    float far_width = far_height * aspect;

    vec3_t fc = vec3_add(cam_pos, vec3_scale(cam_fwd, far_z));
    vec3_t nc = vec3_add(cam_pos, vec3_scale(cam_fwd, near_z));

    vec3_t corners[8];
    // Near plane
    corners[0] = vec3_add(vec3_sub(nc, vec3_scale(cam_up, near_height * 0.5f)), vec3_scale(cam_right, near_width * 0.5f)); // TR
    corners[1] = vec3_add(vec3_add(nc, vec3_scale(cam_up, near_height * 0.5f)), vec3_scale(cam_right, near_width * 0.5f)); // BR
    corners[2] = vec3_add(vec3_sub(nc, vec3_scale(cam_up, near_height * 0.5f)), vec3_scale(cam_right, -near_width * 0.5f)); // TL
    corners[3] = vec3_add(vec3_add(nc, vec3_scale(cam_up, near_height * 0.5f)), vec3_scale(cam_right, -near_width * 0.5f)); // BL
    
    // Far plane
    corners[4] = vec3_add(vec3_sub(fc, vec3_scale(cam_up, far_height * 0.5f)), vec3_scale(cam_right, far_width * 0.5f)); // TR
    corners[5] = vec3_add(vec3_add(fc, vec3_scale(cam_up, far_height * 0.5f)), vec3_scale(cam_right, far_width * 0.5f)); // BR
    corners[6] = vec3_add(vec3_sub(fc, vec3_scale(cam_up, far_height * 0.5f)), vec3_scale(cam_right, -far_width * 0.5f)); // TL
    corners[7] = vec3_add(vec3_add(fc, vec3_scale(cam_up, far_height * 0.5f)), vec3_scale(cam_right, -far_width * 0.5f)); // BL

    // Calculate frustum center
    vec3_t center = {0};
    for(int i=0; i<8; i++) center = vec3_add(center, corners[i]);
    center = vec3_scale(center, 1.0f/8.0f);

    // Light view matrix
    float view_mat[16];
    vec3_t light_up = {0, 1, 0};
    if (fabsf(light_dir.y) > 0.99f) light_up = (vec3_t){1, 0, 0};
    
    // Look from "sun" direction towards center
    // We position the light far away for the view matrix, but it's orthographic so position relative to target only matters for Z range
    vec3_t light_pos = vec3_add(center, vec3_scale(light_dir, -1.0f)); 
    mat4_look_at(light_pos, center, light_up, view_mat);

    // Transform corners to light space to find AABB
    float min_x = FLT_MAX, max_x = -FLT_MAX;
    float min_y = FLT_MAX, max_y = -FLT_MAX;
    float min_z = FLT_MAX, max_z = -FLT_MAX;

    for (int i = 0; i < 8; i++) {
        vec3_t c = corners[i];
        // Multiply by view matrix
        float x = view_mat[0] * c.x + view_mat[4] * c.y + view_mat[8] * c.z + view_mat[12];
        float y = view_mat[1] * c.x + view_mat[5] * c.y + view_mat[9] * c.z + view_mat[13];
        float z = view_mat[2] * c.x + view_mat[6] * c.y + view_mat[10] * c.z + view_mat[14];
        
        if (x < min_x) min_x = x; if (x > max_x) max_x = x;
        if (y < min_y) min_y = y; if (y > max_y) max_y = y;
        if (z < min_z) min_z = z; if (z > max_z) max_z = z;
    }
    
    // Stabilize shadow edges (snap to texels ideally, but omitting for simplicity as resolution isn't passed here)
    // float world_units_per_texel = (max_x - min_x) / shadow_map_size; ...

    // Orthographic projection
    // We extend z range to include potential blockers
    float z_mult = 10.0f;
    min_z = (min_z < 0) ? min_z * z_mult : min_z / z_mult;
    max_z = (max_z > 0) ? max_z * z_mult : max_z / z_mult;

    float proj_mat[16];
    mat4_ortho(min_x, max_x, min_y, max_y, -max_z, -min_z, proj_mat); // Negative because light looks down -Z

    mat4_multiply(proj_mat, view_mat, out_matrix);
}

void cascade_splits_calculate(const cascade_camera_t* camera,
                             const float* light_dir,
                             uint32_t cascade_count,
                             float shadow_distance,
                             float lambda,
                             cascade_split_info_t* out_cascades) {
    if (!camera || !out_cascades || cascade_count == 0) return;

    float near_clip = camera->near_plane;
    float far_clip = shadow_distance; // We stop at shadow distance, not camera far plane

    float clip_range = far_clip - near_clip;
    float min_z = near_clip;
    float max_z = near_clip + clip_range;

    float range = max_z - min_z;
    float ratio = max_z / min_z;

    for (uint32_t i = 0; i < cascade_count; i++) {
        float p = (float)(i + 1) / (float)cascade_count;
        float log_split = min_z * powf(ratio, p);
        float uniform_split = min_z + range * p;
        
        float d = lambda * log_split + (1.0f - lambda) * uniform_split;
        float prev_d = (i == 0) ? min_z : out_cascades[i-1].split_far;
        
        out_cascades[i].split_near = prev_d;
        out_cascades[i].split_far = d;
        
        cascade_calculate_matrix(camera, light_dir, prev_d, d, out_cascades[i].view_proj);
    }
}
