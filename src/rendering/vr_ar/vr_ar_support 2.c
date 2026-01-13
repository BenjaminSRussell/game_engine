// src/engine/rendering/vr_ar/vr_ar_support.c
// VR/AR Support - Virtual and Augmented Reality rendering with stereo 3D and tracking

#include <core/logger.h>
#include <stdbool.h>
#include <stdint.h>
#include <stdlib.h>
#include <string.h>
#include <math.h>

#include "../render_pipeline.h"

// Forward declarations for rendering functions
extern void bind_render_target(void *texture);
extern void set_view_matrix(const float *matrix);
extern void set_projection_matrix(const float *matrix);
extern void render_scene();
extern void apply_distortion_shaders(void *left_texture, void *right_texture, 
                                   const float *distortion_k, const float *chromatic_aberration);
extern void* create_texture(uint32_t width, uint32_t height, uint32_t format);
extern void destroy_texture(void *texture);

// ============================================================================
// VR/AR Types
// ============================================================================

typedef enum {
    VR_AR_TYPE_VR,
    VR_AR_TYPE_AR,
    VR_AR_TYPE_MIXED_REALITY,
    VR_AR_TYPE_STEREO_3D,
    VR_AR_TYPE_COUNT
} VRArType;

typedef enum {
    TRACKING_TYPE_NONE,
    TRACKING_TYPE_ROTATION_ONLY,
    TRACKING_TYPE_POSITION,
    TRACKING_TYPE_6DOF,
    TRACKING_TYPE_EYE_TRACKING,
    TRACKING_TYPE_HAND_TRACKING,
    TRACKING_TYPE_COUNT
} TrackingType;

typedef struct {
    // Head tracking
    float head_position[3];
    float head_rotation[4];     // Quaternion
    float head_velocity[3];
    float head_angular_velocity[3];
    
    // Eye tracking
    float left_eye_position[3];
    float right_eye_position[3];
    float left_eye_gaze_direction[3];
    float right_eye_gaze_direction[3];
    float left_eye_pupil_dilation;
    float right_eye_pupil_dilation;
    bool left_eye_blinking;
    bool right_eye_blinking;
    
    // Hand tracking
    float left_hand_position[3];
    float left_hand_rotation[4];
    float right_hand_position[3];
    float right_hand_rotation[4];
    bool left_hand_tracked;
    bool right_hand_tracked;
    float left_hand_confidence;
    float right_hand_confidence;
    
    // Controller tracking
    float left_controller_position[3];
    float left_controller_rotation[4];
    float right_controller_position[3];
    float right_controller_rotation[4];
    bool left_controller_connected;
    bool right_controller_connected;
    
    // Tracking quality
    float tracking_quality;
    bool is_tracking;
    TrackingType tracking_type;
    
    char name[256];
    bool initialized;
} TrackingData;

typedef struct {
    // Display properties
    uint32_t display_width;
    uint32_t display_height;
    float display_refresh_rate;
    float field_of_view_x;
    float field_of_view_y;
    float ipd; // Interpupillary distance
    
    // Lens distortion
    float distortion_k[4];      // Brown's distortion model coefficients
    float chromatic_aberration[4]; // Chromatic aberration coefficients
    
    // Render targets
    void *left_eye_texture;
    void *right_eye_texture;
    void *depth_texture;
    
    // View matrices
    float left_view_matrix[16];
    float right_view_matrix[16];
    float left_projection_matrix[16];
    float right_projection_matrix[16];
    
    // Performance
    bool enable_foveated_rendering;
    float foveation_strength;
    bool enable_fixed_foveated_rendering;
    bool enable_variable_rate_shading;
    
    char name[256];
    bool active;
} VRArDisplay;

typedef struct {
    VRArDisplay *displays[8];
    uint32_t display_count;
    uint32_t display_capacity;
    
    TrackingData *tracking_data[4];
    uint32_t tracking_count;
    uint32_t tracking_capacity;
    
    // Global settings
    VRArType system_type;
    float world_scale;
    bool enable_depth_buffer;
    bool enable_stereo_rendering;
    bool enable_time_warp;
    
    // Performance
    uint64_t total_render_time_ms;
    uint64_t total_tracking_time_ms;
    uint32_t frames_rendered;
    float average_fps;
    float latency_ms;
    
    // Thread safety
    pthread_mutex_t vr_ar_mutex;
    
    bool initialized;
} VRArSystem;

static VRArSystem g_vr_ar_system = {0};

// ============================================================================
// VR/AR Mathematics
// ============================================================================

static void quaternion_multiply(const float *q1, const float *q2, float *result) {
    result[0] = q1[0] * q2[0] - q1[1] * q2[1] - q1[2] * q2[2] - q1[3] * q2[3];
    result[1] = q1[0] * q2[1] + q1[1] * q2[0] + q1[2] * q2[3] - q1[3] * q2[2];
    result[2] = q1[0] * q2[2] - q1[1] * q2[3] + q1[2] * q2[0] + q1[3] * q2[1];
    result[3] = q1[0] * q2[3] + q1[1] * q2[2] - q1[2] * q2[1] + q1[3] * q2[0];
}

static void quaternion_normalize(float *q) {
    float length = sqrtf(q[0] * q[0] + q[1] * q[1] + q[2] * q[2] + q[3] * q[3]);
    if (length > 0.0f) {
        q[0] /= length;
        q[1] /= length;
        q[2] /= length;
        q[3] /= length;
    }
}

static void quaternion_to_matrix(const float *q, float *matrix) {
    float xx = q[0] * q[0];
    float xy = q[0] * q[1];
    float xz = q[0] * q[2];
    float xw = q[0] * q[3];
    float yy = q[1] * q[1];
    float yz = q[1] * q[2];
    float yw = q[1] * q[3];
    float zz = q[2] * q[2];
    float zw = q[2] * q[3];
    
    matrix[0] = 1.0f - 2.0f * (yy + zz);
    matrix[1] = 2.0f * (xy - zw);
    matrix[2] = 2.0f * (xz + yw);
    matrix[3] = 0.0f;
    
    matrix[4] = 2.0f * (xy + zw);
    matrix[5] = 1.0f - 2.0f * (xx + zz);
    matrix[6] = 2.0f * (yz - xw);
    matrix[7] = 0.0f;
    
    matrix[8] = 2.0f * (xz - yw);
    matrix[9] = 2.0f * (yz + xw);
    matrix[10] = 1.0f - 2.0f * (xx + yy);
    matrix[11] = 0.0f;
    
    matrix[12] = 0.0f;
    matrix[13] = 0.0f;
    matrix[14] = 0.0f;
    matrix[15] = 1.0f;
}

static void matrix_perspective(float fov, float aspect, float near_plane, float far_plane, float *matrix) {
    float f = 1.0f / tanf(fov * 0.5f);
    
    matrix[0] = f / aspect;
    matrix[1] = 0.0f;
    matrix[2] = 0.0f;
    matrix[3] = 0.0f;
    
    matrix[4] = 0.0f;
    matrix[5] = f;
    matrix[6] = 0.0f;
    matrix[7] = 0.0f;
    
    matrix[8] = 0.0f;
    matrix[9] = 0.0f;
    matrix[10] = (far_plane + near_plane) / (near_plane - far_plane);
    matrix[11] = -1.0f;
    
    matrix[12] = 0.0f;
    matrix[13] = 0.0f;
    matrix[14] = (2.0f * far_plane * near_plane) / (near_plane - far_plane);
    matrix[15] = 0.0f;
}

static void matrix_multiply_4x4(const float *m1, const float *m2, float *result) {
    for (int i = 0; i < 4; i++) {
        for (int j = 0; j < 4; j++) {
            result[i * 4 + j] = 0.0f;
            for (int k = 0; k < 4; k++) {
                result[i * 4 + j] += m1[i * 4 + k] * m2[k * 4 + j];
            }
        }
    }
}

static void matrix_translate(const float *matrix, const float *translation, float *result) {
    memcpy(result, matrix, 16 * sizeof(float));
    result[12] = translation[0];
    result[13] = translation[1];
    result[14] = translation[2];
}

// ============================================================================
// VR/AR Rendering Functions
// ============================================================================

static void apply_lens_distortion(const float *uv, const float *distortion_k, float *result) {
    float r2 = uv[0] * uv[0] + uv[1] * uv[1];
    float r4 = r2 * r2;
    float r6 = r4 * r2;
    
    float distortion = 1.0f + distortion_k[0] * r2 + distortion_k[1] * r4 + distortion_k[2] * r6;
    
    result[0] = uv[0] * distortion;
    result[1] = uv[1] * distortion;
}

static void update_view_matrices(VRArDisplay *display, const TrackingData *tracking) {
    if (!display || !tracking) return;
    
    // Create view matrices from head position and rotation
    float rotation_matrix[16];
    quaternion_to_matrix(tracking->head_rotation, rotation_matrix);
    
    // Left eye view (offset by half IPD)
    float left_eye_offset[3] = {-display->ipd * 0.5f, 0.0f, 0.0f};
    float left_eye_position[3];
    left_eye_position[0] = tracking->head_position[0] + left_eye_offset[0];
    left_eye_position[1] = tracking->head_position[1] + left_eye_offset[1];
    left_eye_position[2] = tracking->head_position[2] + left_eye_offset[2];
    
    matrix_translate(rotation_matrix, left_eye_position, display->left_view_matrix);
    
    // Right eye view (offset by half IPD)
    float right_eye_offset[3] = {display->ipd * 0.5f, 0.0f, 0.0f};
    float right_eye_position[3];
    right_eye_position[0] = tracking->head_position[0] + right_eye_offset[0];
    right_eye_position[1] = tracking->head_position[1] + right_eye_offset[1];
    right_eye_position[2] = tracking->head_position[2] + right_eye_offset[2];
    
    matrix_translate(rotation_matrix, right_eye_position, display->right_view_matrix);
}

static void update_projection_matrices(VRArDisplay *display) {
    if (!display) return;
    
    float aspect = (float)display->display_width / (float)display->display_height;
    float fov_x = display->field_of_view_x;
    float fov_y = display->field_of_view_y;
    
    // Left eye projection
    matrix_perspective(fov_y, aspect, 0.1f, 1000.0f, display->left_projection_matrix);
    
    // Right eye projection (same as left for symmetric setup)
    memcpy(display->right_projection_matrix, display->left_projection_matrix, 16 * sizeof(float));
}

static void render_stereo_pair(VRArDisplay *display) {
    if (!display || !display->active) return;
    
    uint64_t start_time = get_time_nanos();
    
    // Render left eye
    bind_render_target(display->left_eye_texture);
    
    // Set left eye view and projection matrices
    set_view_matrix(display->left_view_matrix);
    set_projection_matrix(display->left_projection_matrix);
    
    // Render scene for left eye
    render_scene();
    
    // Bind right eye render target
    bind_render_target(display->right_eye_texture);
    
    // Set right eye view and projection matrices
    set_view_matrix(display->right_view_matrix);
    set_projection_matrix(display->right_projection_matrix);
    
    // Render scene for right eye
    render_scene();
    
    // Apply lens distortion correction
    // Apply distortion shaders to both render targets
    apply_distortion_shaders(display->left_eye_texture, display->right_eye_texture, 
                          display->distortion_k, display->chromatic_aberration);
    
    uint64_t end_time = get_time_nanos();
    g_vr_ar_system.total_render_time_ms += nanos_to_ms(end_time - start_time);
    g_vr_ar_system.frames_rendered++;
    
    LOG_DEBUG("Rendered stereo pair: %s (%.2f ms)", display->name, g_vr_ar_system.total_render_time_ms);
}

// ============================================================================
// VR/AR Display Management
// ============================================================================

VRArDisplay *vr_ar_display_create(const char *name, uint32_t width, uint32_t height, float refresh_rate, float fov_x, float fov_y, float ipd) {
    if (!g_vr_ar_system.initialized || !name) {
        LOG_ERROR("VR/AR system not initialized or invalid name");
        return NULL;
    }
    
    pthread_mutex_lock(&g_vr_ar_system.vr_ar_mutex);
    
    if (g_vr_ar_system.display_count >= g_vr_ar_system.display_capacity) {
        LOG_ERROR("Too many VR/AR displays");
        pthread_mutex_unlock(&g_vr_ar_system.vr_ar_mutex);
        return NULL;
    }
    
    VRArDisplay *display = calloc(1, sizeof(VRArDisplay));
    if (!display) {
        LOG_ERROR("Failed to allocate VR/AR display");
        pthread_mutex_unlock(&g_vr_ar_system.vr_ar_mutex);
        return NULL;
    }
    
    strncpy(display->name, name, sizeof(display->name) - 1);
    display->display_width = width;
    display->display_height = height;
    display->display_refresh_rate = refresh_rate;
    display->field_of_view_x = fov_x;
    display->field_of_view_y = fov_y;
    display->ipd = ipd;
    
    // Initialize lens distortion (typical values)
    display->distortion_k[0] = 0.22f;
    display->distortion_k[1] = 0.24f;
    display->distortion_k[2] = 0.0f;
    display->distortion_k[3] = 0.0f;
    
    display->chromatic_aberration[0] = 0.0f;
    display->chromatic_aberration[1] = 0.001f;
    display->chromatic_aberration[2] = 0.001f;
    display->chromatic_aberration[3] = 0.0f;
    
    // Enable advanced features
    display->enable_foveated_rendering = true;
    display->foveation_strength = 0.5f;
    display->enable_fixed_foveated_rendering = false;
    display->enable_variable_rate_shading = true;
    
    // Create render targets
    // Create left and right eye textures
    extern void* create_texture(uint32_t width, uint32_t height, uint32_t format);
    
    display->left_eye_texture = create_texture(width, height, 4); // RGBA8
    display->right_eye_texture = create_texture(width, height, 4); // RGBA8
    display->depth_texture = create_texture(width, height, 1); // R8 (depth)
    
    if (!display->left_eye_texture || !display->right_eye_texture || !display->depth_texture) {
        LOG_ERROR("Failed to create VR/AR render targets");
        free(display);
        pthread_mutex_unlock(&g_vr_ar_system.vr_ar_mutex);
        return NULL;
    }
    
    // Initialize matrices
    update_projection_matrices(display);
    
    display->active = true;
    
    g_vr_ar_system.displays[g_vr_ar_system.display_count++] = display;
    
    pthread_mutex_unlock(&g_vr_ar_system.vr_ar_mutex);
    
    LOG_INFO("Created VR/AR display: %s (%ux%u, %.1f Hz, FOV: %.1fx%.1f, IPD: %.3f)",
             name, width, height, refresh_rate, fov_x, fov_y, ipd);
    return display;
}

void vr_ar_display_destroy(VRArDisplay *display) {
    if (!display) return;
    
    pthread_mutex_lock(&g_vr_ar_system.vr_ar_mutex);
    
    // Remove from system
    for (uint32_t i = 0; i < g_vr_ar_system.display_count; i++) {
        if (g_vr_ar_system.displays[i] == display) {
            g_vr_ar_system.displays[i] = g_vr_ar_system.displays[g_vr_ar_system.display_count - 1];
            g_vr_ar_system.display_count--;
            break;
        }
    }
    
    // Destroy render targets
    if (display->left_eye_texture) {
        destroy_texture(display->left_eye_texture);
        display->left_eye_texture = NULL;
    }
    if (display->right_eye_texture) {
        destroy_texture(display->right_eye_texture);
        display->right_eye_texture = NULL;
    }
    if (display->depth_texture) {
        destroy_texture(display->depth_texture);
        display->depth_texture = NULL;
    }
    
    free(display);
    
    pthread_mutex_unlock(&g_vr_ar_system.vr_ar_mutex);
    
    LOG_DEBUG("Destroyed VR/AR display: %s", display->name);
}

TrackingData *tracking_data_create(const char *name, TrackingType tracking_type) {
    if (!g_vr_ar_system.initialized || !name) {
        LOG_ERROR("VR/AR system not initialized or invalid name");
        return NULL;
    }
    
    pthread_mutex_lock(&g_vr_ar_system.vr_ar_mutex);
    
    if (g_vr_ar_system.tracking_count >= g_vr_ar_system.tracking_capacity) {
        LOG_ERROR("Too many tracking data objects");
        pthread_mutex_unlock(&g_vr_ar_system.vr_ar_mutex);
        return NULL;
    }
    
    TrackingData *tracking = calloc(1, sizeof(TrackingData));
    if (!tracking) {
        LOG_ERROR("Failed to allocate tracking data");
        pthread_mutex_unlock(&g_vr_ar_system.vr_ar_mutex);
        return NULL;
    }
    
    strncpy(tracking->name, name, sizeof(tracking->name) - 1);
    tracking->tracking_type = tracking_type;
    tracking->is_tracking = true;
    tracking->tracking_quality = 1.0f;
    
    // Initialize head tracking
    tracking->head_position[0] = 0.0f;
    tracking->head_position[1] = 1.7f; // Average head height
    tracking->head_position[2] = 2.0f;
    
    tracking->head_rotation[0] = 0.0f;
    tracking->head_rotation[1] = 0.0f;
    tracking->head_rotation[2] = 0.0f;
    tracking->head_rotation[3] = 1.0f; // Identity quaternion
    
    g_vr_ar_system.tracking_data[g_vr_ar_system.tracking_count++] = tracking;
    
    pthread_mutex_unlock(&g_vr_ar_system.vr_ar_mutex);
    
    LOG_INFO("Created tracking data: %s (type: %d)", name, (int)tracking_type);
    return tracking;
}

void tracking_data_destroy(TrackingData *tracking) {
    if (!tracking) return;
    
    pthread_mutex_lock(&g_vr_ar_system.vr_ar_mutex);
    
    // Remove from system
    for (uint32_t i = 0; i < g_vr_ar_system.tracking_count; i++) {
        if (g_vr_ar_system.tracking_data[i] == tracking) {
            g_vr_ar_system.tracking_data[i] = g_vr_ar_system.tracking_data[g_vr_ar_system.tracking_count - 1];
            g_vr_ar_system.tracking_count--;
            break;
        }
    }
    
    free(tracking);
    
    pthread_mutex_unlock(&g_vr_ar_system.vr_ar_mutex);
    
    LOG_DEBUG("Destroyed tracking data: %s", tracking->name);
}

void vr_ar_system_update(float dt) {
    if (!g_vr_ar_system.initialized) return;
    
    uint64_t start_time = get_time_nanos();
    
    pthread_mutex_lock(&g_vr_ar_system.vr_ar_mutex);
    
    // Update all tracking data
    for (uint32_t i = 0; i < g_vr_ar_system.tracking_count; i++) {
        TrackingData *tracking = g_vr_ar_system.tracking_data[i];
        
        update_head_tracking(tracking, dt);
        update_eye_tracking(tracking, dt);
        update_hand_tracking(tracking, dt);
    }
    
    // Update all displays
    for (uint32_t i = 0; i < g_vr_ar_system.display_count; i++) {
        VRArDisplay *display = g_vr_ar_system.displays[i];
        if (!display->active) continue;
        
        // Update view matrices based on tracking
        if (g_vr_ar_system.tracking_count > 0) {
            update_view_matrices(display, g_vr_ar_system.tracking_data[0]);
        }
        
        // Render stereo pair
        if (g_vr_ar_system.enable_stereo_rendering) {
            render_stereo_pair(display);
        }
    }
    
    pthread_mutex_unlock(&g_vr_ar_system.vr_ar_mutex);
    
    uint64_t end_time = get_time_nanos();
    g_vr_ar_system.total_tracking_time_ms += nanos_to_ms(end_time - start_time);
    g_vr_ar_system.average_fps = 1000.0f / (g_vr_ar_system.total_render_time_ms / (g_vr_ar_system.frames_rendered + 1));
    g_vr_ar_system.latency_ms = g_vr_ar_system.total_tracking_time_ms + g_vr_ar_system.total_render_time_ms;
    
    LOG_DEBUG("VR/AR update: tracking=%.2f ms, render=%.2f ms, fps=%.1f, latency=%.2f ms",
             g_vr_ar_system.total_tracking_time_ms, g_vr_ar_system.total_render_time_ms, g_vr_ar_system.average_fps, g_vr_ar_system.latency_ms);
}

void vr_ar_display_get_view_matrices(VRArDisplay *display, float *left_view, float *right_view) {
    if (!display || !left_view || !right_view) return;
    
    memcpy(left_view, display->left_view_matrix, 16 * sizeof(float));
    memcpy(right_view, display->right_view_matrix, 16 * sizeof(float));
}

void vr_ar_display_get_projection_matrices(VRArDisplay *display, float *left_proj, float *right_proj) {
    if (!display || !left_proj || !right_proj) return;
    
    memcpy(left_proj, display->left_projection_matrix, 16 * sizeof(float));
    memcpy(right_proj, display->right_projection_matrix, 16 * sizeof(float));
}

void tracking_data_get_head_pose(TrackingData *tracking, float *position, float *rotation) {
    if (!tracking || !position || !rotation) return;
    
    memcpy(position, tracking->head_position, 3 * sizeof(float));
    memcpy(rotation, tracking->head_rotation, 4 * sizeof(float));
}

void tracking_data_get_eye_gaze(TrackingData *tracking, float *left_gaze, float *right_gaze) {
    if (!tracking || !left_gaze || !right_gaze) return;
    
    memcpy(left_gaze, tracking->left_eye_gaze_direction, 3 * sizeof(float));
    memcpy(right_gaze, tracking->right_eye_gaze_direction, 3 * sizeof(float));
}

void tracking_data_get_hand_positions(TrackingData *tracking, float *left_hand, float *right_hand) {
    if (!tracking || !left_hand || !right_hand) return;
    
    memcpy(left_hand, tracking->left_hand_position, 3 * sizeof(float));
    memcpy(right_hand, tracking->right_hand_position, 3 * sizeof(float));
}

void vr_ar_system_set_world_scale(float scale) {
    if (!g_vr_ar_system.initialized) return;
    
    g_vr_ar_system.world_scale = scale;
    
    LOG_DEBUG("Updated world scale: %.2f", scale);
}

void vr_ar_system_enable_time_warp(bool enable) {
    if (!g_vr_ar_system.initialized) return;
    
    g_vr_ar_system.enable_time_warp = enable;
    
    LOG_DEBUG("Time warp: %s", enable ? "enabled" : "disabled");
}

void vr_ar_system_get_stats(float *render_time, float *tracking_time, uint32_t *frames_rendered, float *latency) {
    if (!g_vr_ar_system.initialized) return;
    
    if (render_time) *render_time = g_vr_ar_system.total_render_time_ms;
    if (tracking_time) *tracking_time = g_vr_ar_system.total_tracking_time_ms;
    if (frames_rendered) *frames_rendered = g_vr_ar_system.frames_rendered;
    if (latency) *latency = g_vr_ar_system.latency_ms;
    
    LOG_DEBUG("VR/AR stats: render=%.2f ms, tracking=%.2f ms, frames=%u, latency=%.2f ms",
             *render_time, *tracking_time, *frames_rendered, *latency);
}

bool vr_ar_system_is_initialized(void) {
    return g_vr_ar_system.initialized;
}
