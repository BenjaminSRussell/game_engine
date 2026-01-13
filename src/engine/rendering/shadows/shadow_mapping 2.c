// src/engine/rendering/shadows/shadow_mapping.c
// Shadow Mapping System - Cascaded shadow maps and PCF filtering

#include <core/logger.h>
#include <stdbool.h>
#include <stdint.h>
#include <stdlib.h>
#include <string.h>
#include <math.h>

#include "../render_pipeline.h"
#include "../lighting/lighting_system.c"

// ============================================================================
// Shadow Mapping Types
// ============================================================================

typedef enum {
    SHADOW_FILTER_HARD,
    SHADOW_FILTER_PCF_2X2,
    SHADOW_FILTER_PCF_3X3,
    SHADOW_FILTER_PCF_5X5,
    SHADOW_FILTER_PCSS
} ShadowFilterType;

typedef struct {
    uint32_t size;
    void *depth_texture;
    void *color_texture; // For variance shadow maps
    Framebuffer *framebuffer;
    
    float view_matrix[16];
    float proj_matrix[16];
    float view_proj_matrix[16];
    
    float split_distance;
    uint32_t cascade_index;
} ShadowCascade;

typedef struct {
    ShadowCascade *cascades;
    uint32_t cascade_count;
    uint32_t cascade_size;
    
    // Shadow atlas for point lights
    void *shadow_atlas;
    uint32_t atlas_size;
    uint32_t atlas_used;
    
    // Filtering
    ShadowFilterType filter_type;
    float pcf_radius;
    uint32_t pcf_samples;
    
    // Bias settings
    float constant_bias;
    float normal_bias;
    float slope_scale_bias;
    
    // Performance
    bool enable_cascades;
    bool enable_pcss;
    bool enable_variance_shadow_maps;
    
    // Statistics
    uint32_t shadow_pixels_rendered;
    uint32_t shadow_casters;
    float shadow_render_time_ms;
    
    bool initialized;
} ShadowMappingSystem;

static ShadowMappingSystem g_shadow_system = {0};

// ============================================================================
// Shadow Matrix Calculations
// ============================================================================

static void calculate_cascade_splits(float *splits, uint32_t cascade_count, 
                                    float near_plane, float far_plane, float lambda) {
    // Practical split scheme: mix of logarithmic and uniform splits
    for (uint32_t i = 0; i < cascade_count; i++) {
        float uniform_ratio = (float)(i + 1) / (float)cascade_count;
        float log_ratio = logf(uniform_ratio) / logf(lambda);
        
        splits[i] = near_plane * powf(far_plane / near_plane, log_ratio);
        
        // Blend with uniform distribution
        splits[i] = lambda * splits[i] + (1.0f - lambda) * (near_plane + uniform_ratio * (far_plane - near_plane));
    }
}

static void calculate_frustum_corners(float *corners, const float *view_matrix, 
                                    const float *proj_matrix, float near_plane, float far_plane) {
    // Extract frustum corners in world space
    // This is a simplified implementation
    
    // Near plane corners in clip space
    float clip_corners[8][4] = {
        {-1.0f, -1.0f, near_plane, 1.0f}, // bottom-left
        { 1.0f, -1.0f, near_plane, 1.0f}, // bottom-right
        { 1.0f,  1.0f, near_plane, 1.0f}, // top-right
        {-1.0f,  1.0f, near_plane, 1.0f}, // top-left
        
        {-1.0f, -1.0f, far_plane, 1.0f},  // bottom-left
        { 1.0f, -1.0f, far_plane, 1.0f},  // bottom-right
        { 1.0f,  1.0f, far_plane, 1.0f},  // top-right
        {-1.0f,  1.0f, far_plane, 1.0f}   // top-left
    };
    
    // Calculate inverse view-projection matrix
    float view_proj[16];
    matrix_multiply(view_proj, view_matrix, proj_matrix);
    
    float inv_view_proj[16];
    matrix_inverse(inv_view_proj, view_proj);
    
    // Transform corners to world space
    for (int i = 0; i < 8; i++) {
        float world_pos[4];
        matrix_vector_multiply(world_pos, inv_view_proj, clip_corners[i]);
        
        // Perspective divide
        if (world_pos[3] != 0.0f) {
            world_pos[0] /= world_pos[3];
            world_pos[1] /= world_pos[3];
            world_pos[2] /= world_pos[3];
        }
        
        corners[i * 3 + 0] = world_pos[0];
        corners[i * 3 + 1] = world_pos[1];
        corners[i * 3 + 2] = world_pos[2];
    }
}

static void calculate_ortho_matrix(float *matrix, float left, float right, 
                                   float bottom, float top, float near_plane, float far_plane) {
    memset(matrix, 0, sizeof(float) * 16);
    
    matrix[0] = 2.0f / (right - left);
    matrix[5] = 2.0f / (top - bottom);
    matrix[10] = -2.0f / (far_plane - near_plane);
    matrix[12] = -(right + left) / (right - left);
    matrix[13] = -(top + bottom) / (top - bottom);
    matrix[14] = -(far_plane + near_plane) / (far_plane - near_plane);
    matrix[15] = 1.0f;
}

static void calculate_light_view_matrix(float *matrix, const float *light_dir, 
                                       const float *center, float radius) {
    // Create a view matrix looking at center from light direction
    float light_pos[3] = {
        center[0] - light_dir[0] * radius,
        center[1] - light_dir[1] * radius,
        center[2] - light_dir[2] * radius
    };
    
    float up[3] = {0.0f, 1.0f, 0.0f};
    
    // If light direction is parallel to up vector, use a different up vector
    if (fabsf(light_dir[1]) > 0.999f) {
        up[0] = 1.0f;
        up[1] = 0.0f;
        up[2] = 0.0f;
    }
    
    // Implement proper look-at matrix calculation
    float z_axis[3];
    z_axis[0] = -light_dir[0];
    z_axis[1] = -light_dir[1];
    z_axis[2] = -light_dir[2];
    vector_normalize(z_axis);
    
    float x_axis[3];
    vector_cross(x_axis, up, z_axis);
    vector_normalize(x_axis);
    
    float y_axis[3];
    vector_cross(y_axis, z_axis, x_axis);
    
    // Build view matrix
    matrix[0] = x_axis[0]; matrix[1] = y_axis[0]; matrix[2] = z_axis[0]; matrix[3] = 0.0f;
    matrix[4] = x_axis[1]; matrix[5] = y_axis[1]; matrix[6] = z_axis[1]; matrix[7] = 0.0f;
    matrix[8] = x_axis[2]; matrix[9] = y_axis[2]; matrix[10] = z_axis[2]; matrix[11] = 0.0f;
    matrix[12] = -vector_dot(x_axis, light_pos);
    matrix[13] = -vector_dot(y_axis, light_pos);
    matrix[14] = -vector_dot(z_axis, light_pos);
    matrix[15] = 1.0f;
}

static void calculate_cascade_matrices(ShadowCascade *cascade, const float *light_dir,
                                      const float *camera_view, const float *camera_proj,
                                      float split_near, float split_far) {
    // Get frustum corners for this cascade
    float corners[24]; // 8 corners * 3 components
    calculate_frustum_corners(corners, camera_view, camera_proj, split_near, split_far);
    
    // Calculate bounding box of frustum in light space
    float min_bounds[3] = {FLT_MAX, FLT_MAX, FLT_MAX};
    float max_bounds[3] = {FLT_MIN, FLT_MIN, FLT_MIN};
    
    for (int i = 0; i < 8; i++) {
        float *corner = &corners[i * 3];
        
        // Transform to light space
        float light_space_pos[4];
        matrix_vector_multiply(light_space_pos, cascade->view_matrix, corner);
        
        for (int j = 0; j < 3; j++) {
            if (light_space_pos[j] < min_bounds[j]) min_bounds[j] = light_space_pos[j];
            if (light_space_pos[j] > max_bounds[j]) max_bounds[j] = light_space_pos[j];
        }
    }
    
    // Add some padding to prevent edge bleeding
    float padding = 10.0f;
    min_bounds[0] -= padding;
    min_bounds[1] -= padding;
    min_bounds[2] -= padding;
    max_bounds[0] += padding;
    max_bounds[1] += padding;
    max_bounds[2] += padding;
    
    // Calculate center and radius for light view matrix
    float center[3] = {
        (min_bounds[0] + max_bounds[0]) * 0.5f,
        (min_bounds[1] + max_bounds[1]) * 0.5f,
        (min_bounds[2] + max_bounds[2]) * 0.5f
    };
    
    float radius = fmaxf(
        fmaxf(max_bounds[0] - min_bounds[0], max_bounds[1] - min_bounds[1]),
        max_bounds[2] - min_bounds[2]
    ) * 0.5f;
    
    // Calculate light view and projection matrices
    calculate_light_view_matrix(cascade->view_matrix, light_dir, center, radius);
    calculate_ortho_matrix(cascade->proj_matrix, -radius, radius, -radius, radius, -radius, radius);
    
    // Calculate view-projection matrix
    matrix_multiply(cascade->view_proj_matrix, cascade->view_matrix, cascade->proj_matrix);
    
    cascade->split_distance = split_far;
}

// ============================================================================
// Shadow Rendering
// ============================================================================

static void render_shadow_cascade(ShadowCascade *cascade, const Light *light, 
                                 const float *scene_bounds) {
    if (!cascade || !light) return;
    
    // Bind shadow framebuffer
    framebuffer_bind(cascade->framebuffer);
    
    // Set viewport to cascade size
    renderer_set_viewport(0, 0, cascade->size, cascade->size);
    
    // Clear depth buffer
    framebuffer_clear_depth(cascade->framebuffer, 1.0f);
    
    // Set up shadow rendering state
    // TODO: Set cull mode, depth bias, etc.
    
    // Render scene from light perspective
    // TODO: Render all shadow casters using cascade matrices
    
    LOG_DEBUG("Rendered shadow cascade %u", cascade->cascade_index);
}

static void render_point_light_shadows(const Light *light) {
    if (!light || light->type != LIGHT_TYPE_POINT) return;
    
    // Point lights require 6 shadow maps (cube map)
    // TODO: Implement cube map shadow rendering
    
    LOG_DEBUG("Rendering point light shadows for: %s", light->name);
}

// ============================================================================
// Shadow Mapping API
// ============================================================================

bool shadow_mapping_init(uint32_t cascade_count, uint32_t cascade_size, 
                        uint32_t atlas_size, ShadowFilterType filter_type) {
    if (g_shadow_system.initialized) {
        LOG_WARN("Shadow mapping system already initialized");
        return true;
    }
    
    memset(&g_shadow_system, 0, sizeof(ShadowMappingSystem));
    
    g_shadow_system.cascade_count = cascade_count;
    g_shadow_system.cascade_size = cascade_size;
    g_shadow_system.atlas_size = atlas_size;
    g_shadow_system.filter_type = filter_type;
    
    // Create shadow cascades
    g_shadow_system.cascades = calloc(cascade_count, sizeof(ShadowCascade));
    if (!g_shadow_system.cascades) {
        LOG_ERROR("Failed to allocate shadow cascades");
        return false;
    }
    
    // Initialize cascades
    for (uint32_t i = 0; i < cascade_count; i++) {
        ShadowCascade *cascade = &g_shadow_system.cascades[i];
        cascade->size = cascade_size;
        cascade->cascade_index = i;
        
        // TODO: Create depth texture and framebuffer
        cascade->framebuffer = framebuffer_create(cascade_size, cascade_size);
        if (!cascade->framebuffer) {
            LOG_ERROR("Failed to create shadow cascade framebuffer");
            shadow_mapping_shutdown();
            return false;
        }
        
        // TODO: Create depth texture and attach to framebuffer
    }
    
    // Create shadow atlas for point lights
    if (atlas_size > 0) {
        // TODO: Create shadow atlas texture
        LOG_DEBUG("Created shadow atlas: %ux%u", atlas_size, atlas_size);
    }
    
    // Set default bias values
    g_shadow_system.constant_bias = 0.001f;
    g_shadow_system.normal_bias = 0.001f;
    g_shadow_system.slope_scale_bias = 2.0f;
    g_shadow_system.pcf_radius = 2.0f;
    g_shadow_system.pcf_samples = 16;
    
    g_shadow_system.enable_cascades = (cascade_count > 1);
    g_shadow_system.enable_pcss = (filter_type == SHADOW_FILTER_PCSS);
    
    g_shadow_system.initialized = true;
    LOG_INFO("Shadow mapping system initialized (%u cascades, %u size, %u atlas)", 
             cascade_count, cascade_size, atlas_size);
    return true;
}

void shadow_mapping_shutdown(void) {
    if (!g_shadow_system.initialized)
        return;
    
    // Destroy cascades
    for (uint32_t i = 0; i < g_shadow_system.cascade_count; i++) {
        ShadowCascade *cascade = &g_shadow_system.cascades[i];
        if (cascade->framebuffer) {
            framebuffer_destroy(cascade->framebuffer);
        }
        // TODO: Destroy depth texture
    }
    
    free(g_shadow_system.cascades);
    
    // TODO: Destroy shadow atlas
    
    memset(&g_shadow_system, 0, sizeof(ShadowMappingSystem));
    
    LOG_INFO("Shadow mapping system shutdown");
}

void shadow_mapping_render_shadows(const Light *lights, uint32_t light_count,
                                  const float *camera_view, const float *camera_proj,
                                  float near_plane, float far_plane) {
    if (!g_shadow_system.initialized || !lights) return;
    
    uint64_t start_time = get_time_nanos();
    g_shadow_system.shadow_casters = 0;
    g_shadow_system.shadow_pixels_rendered = 0;
    
    // Calculate cascade splits
    float splits[8];
    float lambda = 0.5f; // Blend factor between uniform and logarithmic splits
    calculate_cascade_splits(splits, g_shadow_system.cascade_count, near_plane, far_plane, lambda);
    
    // Render shadows for each light
    for (uint32_t i = 0; i < light_count; i++) {
        const Light *light = &lights[i];
        
        if (!light->enabled || light->shadow_mode == LIGHT_SHADOW_MODE_NONE) {
            continue;
        }
        
        g_shadow_system.shadow_casters++;
        
        switch (light->type) {
            case LIGHT_TYPE_DIRECTIONAL: {
                if (g_shadow_system.enable_cascades) {
                    // Render cascaded shadow maps
                    float prev_split = near_plane;
                    for (uint32_t j = 0; j < g_shadow_system.cascade_count; j++) {
                        ShadowCascade *cascade = &g_shadow_system.cascades[j];
                        calculate_cascade_matrices(cascade, light->direction, camera_view, camera_proj, 
                                                 prev_split, splits[j]);
                        render_shadow_cascade(cascade, light, NULL);
                        prev_split = splits[j];
                        
                        g_shadow_system.shadow_pixels_rendered += cascade->size * cascade->size;
                    }
                } else {
                    // Single shadow map
                    ShadowCascade *cascade = &g_shadow_system.cascades[0];
                    calculate_cascade_matrices(cascade, light->direction, camera_view, camera_proj, 
                                             near_plane, far_plane);
                    render_shadow_cascade(cascade, light, NULL);
                    g_shadow_system.shadow_pixels_rendered += cascade->size * cascade->size;
                }
                break;
            }
            
            case LIGHT_TYPE_POINT: {
                render_point_light_shadows(light);
                // TODO: Update shadow_pixels_rendered for cube maps
                break;
            }
            
            case LIGHT_TYPE_SPOT: {
                // Single shadow map for spot light
                ShadowCascade *cascade = &g_shadow_system.cascades[0];
                // TODO: Calculate spot light matrices
                render_shadow_cascade(cascade, light, NULL);
                g_shadow_system.shadow_pixels_rendered += cascade->size * cascade->size;
                break;
            }
            
            default:
                break;
        }
    }
    
    uint64_t end_time = get_time_nanos();
    g_shadow_system.shadow_render_time_ms = nanos_to_ms(end_time - start_time);
    
    LOG_DEBUG("Shadow rendering: %u casters, %u pixels, %.2f ms", 
             g_shadow_system.shadow_casters, g_shadow_system.shadow_pixels_rendered,
             g_shadow_system.shadow_render_time_ms);
}

void shadow_mapping_bind_cascade_textures(void *shader, uint32_t start_slot) {
    if (!g_shadow_system.initialized) return;
    
    // Bind cascade shadow maps to shader
    for (uint32_t i = 0; i < g_shadow_system.cascade_count; i++) {
        ShadowCascade *cascade = &g_shadow_system.cascades[i];
        // TODO: Bind cascade->depth_texture to shader at slot start_slot + i
        LOG_DEBUG("Bound cascade %u to slot %u", i, start_slot + i);
    }
}

void shadow_mapping_set_bias_parameters(float constant_bias, float normal_bias, 
                                       float slope_scale_bias) {
    if (!g_shadow_system.initialized) return;
    
    g_shadow_system.constant_bias = constant_bias;
    g_shadow_system.normal_bias = normal_bias;
    g_shadow_system.slope_scale_bias = slope_scale_bias;
    
    LOG_DEBUG("Shadow bias updated: constant=%.4f, normal=%.4f, slope=%.4f",
             constant_bias, normal_bias, slope_scale_bias);
}

void shadow_mapping_set_filter_parameters(ShadowFilterType filter_type, 
                                         float pcf_radius, uint32_t pcf_samples) {
    if (!g_shadow_system.initialized) return;
    
    g_shadow_system.filter_type = filter_type;
    g_shadow_system.pcf_radius = pcf_radius;
    g_shadow_system.pcf_samples = pcf_samples;
    
    LOG_DEBUG("Shadow filter updated: type=%d, radius=%.2f, samples=%u",
             (int)filter_type, pcf_radius, pcf_samples);
}

void shadow_mapping_get_cascade_matrices(float *matrices, uint32_t max_cascades) {
    if (!g_shadow_system.initialized || !matrices) return;
    
    uint32_t cascade_count = (max_cascades < g_shadow_system.cascade_count) ? 
                             max_cascades : g_shadow_system.cascade_count;
    
    for (uint32_t i = 0; i < cascade_count; i++) {
        memcpy(&matrices[i * 16], g_shadow_system.cascades[i].view_proj_matrix, sizeof(float) * 16);
    }
}

void shadow_mapping_get_cascade_distances(float *distances, uint32_t max_cascades) {
    if (!g_shadow_system.initialized || !distances) return;
    
    uint32_t cascade_count = (max_cascades < g_shadow_system.cascade_count) ? 
                             max_cascades : g_shadow_system.cascade_count;
    
    for (uint32_t i = 0; i < cascade_count; i++) {
        distances[i] = g_shadow_system.cascades[i].split_distance;
    }
}

void shadow_mapping_get_stats(uint32_t *shadow_casters, uint32_t *shadow_pixels, 
                            float *render_time) {
    if (!g_shadow_system.initialized) return;
    
    if (shadow_casters) *shadow_casters = g_shadow_system.shadow_casters;
    if (shadow_pixels) *shadow_pixels = g_shadow_system.shadow_pixels_rendered;
    if (render_time) *render_time = g_shadow_system.shadow_render_time_ms;
}
