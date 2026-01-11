// src/engine/rendering/lighting/lighting_system.c
// Lighting System - PBR lighting with multiple light types and shadow support

#include <core/logger.h>
#include <stdbool.h>
#include <stdint.h>
#include <stdlib.h>
#include <string.h>
#include <math.h>

#include "../render_pipeline.h"

// ============================================================================
// Lighting Types
// ============================================================================

typedef enum {
    LIGHT_TYPE_DIRECTIONAL,
    LIGHT_TYPE_POINT,
    LIGHT_TYPE_SPOT,
    LIGHT_TYPE_AREA
} LightType;

typedef enum {
    LIGHT_SHADOW_MODE_NONE,
    LIGHT_SHADOW_MODE_HARD,
    LIGHT_SHADOW_MODE_SOFT,
    LIGHT_SHADOW_MODE_CASCADE
} LightShadowMode;

typedef struct {
    LightType type;
    char name[64];
    
    // Common light properties
    float color[3];
    float intensity;
    bool enabled;
    
    // Transform
    float position[3];
    float direction[3];
    
    // Point/Spot light properties
    float radius;
    float inner_angle;
    float outer_angle;
    
    // Area light properties
    float size[2];
    
    // Shadow properties
    LightShadowMode shadow_mode;
    uint32_t shadow_map_size;
    float shadow_bias;
    float shadow_normal_bias;
    float shadow_softness;
    void *shadow_map;
    float shadow_matrix[16]; // View-projection for shadow rendering
    
    // Light culling
    float bounding_radius;
    uint32_t visibility_mask;
    
    // Rendering data
    uint32_t light_id;
    bool needs_update;
} Light;

typedef struct {
    Light *lights[1024];
    uint32_t light_count;
    uint32_t light_capacity;
    
    // Clustered shading
    uint32_t cluster_count_x;
    uint32_t cluster_count_y;
    uint32_t cluster_count_z;
    float max_distance;
    
    // Light clusters (3D grid)
    uint32_t *light_clusters;
    uint32_t *light_indices;
    uint32_t cluster_capacity;
    
    // Global lighting
    float ambient_color[3];
    float ambient_intensity;
    float environment_intensity;
    
    // Shadow management
    uint32_t shadow_cascade_count;
    float shadow_cascade_distances[8];
    void *shadow_atlas;
    
    // GPU buffers
    void *light_buffer;
    void *cluster_buffer;
    
    // Statistics
    uint32_t visible_lights;
    uint32_t shadow_casting_lights;
    uint32_t total_shadow_pixels;
    
    bool initialized;
    bool clustered_shading_enabled;
} LightingSystem;

static LightingSystem g_lighting_system = {0};

// ============================================================================
// Light Management
// ============================================================================

static float calculate_light_radius(const Light *light) {
    if (light->type == LIGHT_TYPE_DIRECTIONAL) {
        return FLT_MAX;
    }
    
    // Calculate radius based on intensity and a minimum threshold
    const float min_threshold = 0.01f;
    if (light->intensity <= min_threshold) {
        return 0.0f;
    }
    
    // Inverse square law approximation
    float radius = sqrtf(light->intensity / min_threshold);
    
    // Apply some artistic scaling
    radius *= 2.0f;
    
    return radius;
}

static void update_light_bounding_volume(Light *light) {
    switch (light->type) {
        case LIGHT_TYPE_DIRECTIONAL:
            light->bounding_radius = FLT_MAX;
            break;
            
        case LIGHT_TYPE_POINT:
            light->bounding_radius = light->radius > 0.0f ? light->radius : calculate_light_radius(light);
            break;
            
        case LIGHT_TYPE_SPOT:
            light->bounding_radius = light->radius > 0.0f ? light->radius : calculate_light_radius(light);
            break;
            
        case LIGHT_TYPE_AREA:
            // Approximate area light as point light for culling
            light->bounding_radius = sqrtf(light->size[0] * light->size[0] + light->size[1] * light->size[1]) * 0.5f + 
                                     calculate_light_radius(light);
            break;
    }
}

static void calculate_shadow_matrix(Light *light, const float *view_matrix, const float *proj_matrix) {
    if (light->shadow_mode == LIGHT_SHADOW_MODE_NONE) {
        return;
    }
    
    // Create view matrix for light
    float light_view[16];
    float light_proj[16];
    
    switch (light->type) {
        case LIGHT_TYPE_DIRECTIONAL: {
            // Orthographic projection for directional lights
            float light_pos[3] = {
                -light->direction[0] * 50.0f,
                -light->direction[1] * 50.0f,
                -light->direction[2] * 50.0f
            };
            
            // Simple look-at matrix
            float up[3] = {0.0f, 1.0f, 0.0f};
            if (fabsf(light->direction[1]) > 0.999f) {
                up[0] = 1.0f;
                up[1] = 0.0f;
                up[2] = 0.0f;
            }
            
            // TODO: Implement proper matrix math
            memset(light_view, 0, sizeof(light_view));
            memset(light_proj, 0, sizeof(light_proj));
            
            // Orthographic projection
            light_proj[0] = 2.0f / 100.0f;  // width
            light_proj[5] = 2.0f / 100.0f;  // height
            light_proj[10] = -2.0f / 200.0f; // depth
            light_proj[12] = 0.0f;
            light_proj[13] = 0.0f;
            light_proj[14] = 0.0f;
            light_proj[15] = 1.0f;
            break;
        }
        
        case LIGHT_TYPE_POINT: {
            // Perspective projection for point lights
            // TODO: Implement cube map shadow matrix calculation
            memset(light_view, 0, sizeof(light_view));
            memset(light_proj, 0, sizeof(light_proj));
            break;
        }
        
        case LIGHT_TYPE_SPOT: {
            // Perspective projection for spot lights
            float aspect = 1.0f;
            float fov = light->outer_angle * 2.0f;
            float near_plane = 0.1f;
            float far_plane = light->bounding_radius;
            
            // TODO: Implement perspective matrix
            memset(light_view, 0, sizeof(light_view));
            memset(light_proj, 0, sizeof(light_proj));
            break;
        }
        
        default:
            memset(light_view, 0, sizeof(light_view));
            memset(light_proj, 0, sizeof(light_proj));
            break;
    }
    
    // Multiply view and projection to get shadow matrix
    // TODO: Implement matrix multiplication
    memcpy(light->shadow_matrix, light_proj, sizeof(light->shadow_matrix));
}

// ============================================================================
// Clustered Shading
// ============================================================================

static void initialize_clusters(uint32_t cluster_x, uint32_t cluster_y, uint32_t cluster_z) {
    g_lighting_system.cluster_count_x = cluster_x;
    g_lighting_system.cluster_count_y = cluster_y;
    g_lighting_system.cluster_count_z = cluster_z;
    
    uint32_t total_clusters = cluster_x * cluster_y * cluster_z;
    g_lighting_system.cluster_capacity = total_clusters * 16; // 16 lights per cluster average
    
    g_lighting_system.light_clusters = calloc(total_clusters, sizeof(uint32_t));
    g_lighting_system.light_indices = calloc(g_lighting_system.cluster_capacity, sizeof(uint32_t));
    
    if (!g_lighting_system.light_clusters || !g_lighting_system.light_indices) {
        LOG_ERROR("Failed to allocate light clusters");
        free(g_lighting_system.light_clusters);
        free(g_lighting_system.light_indices);
        g_lighting_system.light_clusters = NULL;
        g_lighting_system.light_indices = NULL;
    }
}

static void assign_lights_to_clusters(const float *view_matrix, const float *proj_matrix) {
    if (!g_lighting_system.clustered_shading_enabled || !g_lighting_system.light_clusters) {
        return;
    }
    
    // Clear clusters
    uint32_t total_clusters = g_lighting_system.cluster_count_x * 
                             g_lighting_system.cluster_count_y * 
                             g_lighting_system.cluster_count_z;
    memset(g_lighting_system.light_clusters, 0, total_clusters * sizeof(uint32_t));
    
    uint32_t index_offset = 0;
    
    // Assign each light to clusters it affects
    for (uint32_t i = 0; i < g_lighting_system.light_count; i++) {
        Light *light = g_lighting_system.lights[i];
        if (!light->enabled) continue;
        
        // Transform light position to view space
        float view_pos[3];
        // TODO: Transform position using view matrix
        memcpy(view_pos, light->position, sizeof(view_pos));
        
        // Calculate affected clusters (simplified)
        // In a real implementation, this would be more sophisticated
        uint32_t cluster_x = (uint32_t)(view_pos[0] / g_lighting_system.max_distance * g_lighting_system.cluster_count_x);
        uint32_t cluster_y = (uint32_t)(view_pos[1] / g_lighting_system.max_distance * g_lighting_system.cluster_count_y);
        uint32_t cluster_z = (uint32_t)(view_pos[2] / g_lighting_system.max_distance * g_lighting_system.cluster_count_z);
        
        // Clamp to valid range
        cluster_x = (cluster_x < g_lighting_system.cluster_count_x) ? cluster_x : g_lighting_system.cluster_count_x - 1;
        cluster_y = (cluster_y < g_lighting_system.cluster_count_y) ? cluster_y : g_lighting_system.cluster_count_y - 1;
        cluster_z = (cluster_z < g_lighting_system.cluster_count_z) ? cluster_z : g_lighting_system.cluster_count_z - 1;
        
        uint32_t cluster_index = cluster_z * g_lighting_system.cluster_count_x * g_lighting_system.cluster_count_y +
                                cluster_y * g_lighting_system.cluster_count_x + cluster_x;
        
        // Add light to cluster
        if (index_offset < g_lighting_system.cluster_capacity) {
            g_lighting_system.light_indices[index_offset] = i;
            g_lighting_system.light_clusters[cluster_index] = index_offset;
            index_offset++;
        }
    }
}

// ============================================================================
// Lighting System API
// ============================================================================

bool lighting_system_init(uint32_t max_lights, bool enable_clustered_shading) {
    if (g_lighting_system.initialized) {
        LOG_WARN("Lighting system already initialized");
        return true;
    }
    
    memset(&g_lighting_system, 0, sizeof(LightingSystem));
    
    g_lighting_system.light_capacity = max_lights;
    g_lighting_system.lights = calloc(max_lights, sizeof(Light*));
    
    if (!g_lighting_system.lights) {
        LOG_ERROR("Failed to allocate lights array");
        return false;
    }
    
    // Set default ambient lighting
    g_lighting_system.ambient_color[0] = 0.1f;
    g_lighting_system.ambient_color[1] = 0.1f;
    g_lighting_system.ambient_color[2] = 0.1f;
    g_lighting_system.ambient_intensity = 0.1f;
    g_lighting_system.environment_intensity = 1.0f;
    
    // Initialize shadow cascades
    g_lighting_system.shadow_cascade_count = 4;
    g_lighting_system.shadow_cascade_distances[0] = 5.0f;
    g_lighting_system.shadow_cascade_distances[1] = 15.0f;
    g_lighting_system.shadow_cascade_distances[2] = 40.0f;
    g_lighting_system.shadow_cascade_distances[3] = 100.0f;
    
    // Initialize clustered shading
    g_lighting_system.clustered_shading_enabled = enable_clustered_shading;
    if (enable_clustered_shading) {
        initialize_clusters(16, 8, 24); // 16x8x24 clusters
    }
    
    // TODO: Create GPU buffers
    // g_lighting_system.light_buffer = create_light_buffer(max_lights);
    // g_lighting_system.cluster_buffer = create_cluster_buffer();
    
    g_lighting_system.initialized = true;
    LOG_INFO("Lighting system initialized (max lights: %u, clustered: %s)", 
             max_lights, enable_clustered_shading ? "yes" : "no");
    return true;
}

void lighting_system_shutdown(void) {
    if (!g_lighting_system.initialized)
        return;
    
    // Destroy all lights
    for (uint32_t i = 0; i < g_lighting_system.light_count; i++) {
        if (g_lighting_system.lights[i]) {
            free(g_lighting_system.lights[i]);
        }
    }
    
    // Cleanup clusters
    free(g_lighting_system.light_clusters);
    free(g_lighting_system.light_indices);
    
    // TODO: Destroy GPU buffers
    // destroy_buffer(g_lighting_system.light_buffer);
    // destroy_buffer(g_lighting_system.cluster_buffer);
    
    free(g_lighting_system.lights);
    memset(&g_lighting_system, 0, sizeof(LightingSystem));
    
    LOG_INFO("Lighting system shutdown");
}

Light *lighting_create_light(const char *name, LightType type) {
    if (!g_lighting_system.initialized || !name) {
        LOG_ERROR("Lighting system not initialized or invalid name");
        return NULL;
    }
    
    if (g_lighting_system.light_count >= g_lighting_system.light_capacity) {
        LOG_ERROR("Too many lights in system");
        return NULL;
    }
    
    Light *light = calloc(1, sizeof(Light));
    if (!light) {
        LOG_ERROR("Failed to allocate light");
        return NULL;
    }
    
    strncpy(light->name, name, sizeof(light->name) - 1);
    light->type = type;
    light->enabled = true;
    light->intensity = 1.0f;
    light->color[0] = 1.0f;
    light->color[1] = 1.0f;
    light->color[2] = 1.0f;
    light->shadow_mode = LIGHT_SHADOW_MODE_NONE;
    light->shadow_map_size = 1024;
    light->shadow_bias = 0.005f;
    light->shadow_normal_bias = 0.001f;
    light->shadow_softness = 1.0f;
    light->visibility_mask = 0xFFFFFFFF;
    light->light_id = g_lighting_system.light_count;
    
    // Set default direction for directional lights
    if (type == LIGHT_TYPE_DIRECTIONAL) {
        light->direction[0] = 0.0f;
        light->direction[1] = -1.0f;
        light->direction[2] = 0.0f;
    }
    
    update_light_bounding_volume(light);
    
    g_lighting_system.lights[g_lighting_system.light_count++] = light;
    
    LOG_DEBUG("Created light: %s (type: %d)", name, (int)type);
    return light;
}

void lighting_destroy_light(Light *light) {
    if (!light || !g_lighting_system.initialized)
        return;
    
    // Remove from system
    for (uint32_t i = 0; i < g_lighting_system.light_count; i++) {
        if (g_lighting_system.lights[i] == light) {
            g_lighting_system.lights[i] = g_lighting_system.lights[g_lighting_system.light_count - 1];
            g_lighting_system.light_count--;
            break;
        }
    }
    
    // Cleanup shadow resources
    if (light->shadow_map) {
        // TODO: Destroy shadow map texture
        light->shadow_map = NULL;
    }
    
    free(light);
    LOG_DEBUG("Destroyed light: %s", light->name);
}

void lighting_set_light_position(Light *light, float x, float y, float z) {
    if (!light) return;
    
    light->position[0] = x;
    light->position[1] = y;
    light->position[2] = z;
    light->needs_update = true;
    update_light_bounding_volume(light);
}

void lighting_set_light_direction(Light *light, float x, float y, float z) {
    if (!light) return;
    
    float length = sqrtf(x*x + y*y + z*z);
    if (length > 0.0f) {
        light->direction[0] = x / length;
        light->direction[1] = y / length;
        light->direction[2] = z / length;
        light->needs_update = true;
    }
}

void lighting_set_light_color(Light *light, float r, float g, float b) {
    if (!light) return;
    
    light->color[0] = r;
    light->color[1] = g;
    light->color[2] = b;
    light->needs_update = true;
}

void lighting_set_light_intensity(Light *light, float intensity) {
    if (!light) return;
    
    light->intensity = intensity;
    light->needs_update = true;
    update_light_bounding_volume(light);
}

void lighting_set_light_shadow_mode(Light *light, LightShadowMode mode) {
    if (!light) return;
    
    light->shadow_mode = mode;
    light->needs_update = true;
    
    if (mode != LIGHT_SHADOW_MODE_NONE && !light->shadow_map) {
        // TODO: Create shadow map texture
        LOG_DEBUG("Created shadow map for light: %s", light->name);
    }
}

void lighting_update_lights(const float *view_matrix, const float *proj_matrix) {
    if (!g_lighting_system.initialized)
        return;
    
    g_lighting_system.visible_lights = 0;
    g_lighting_system.shadow_casting_lights = 0;
    
    // Update individual lights
    for (uint32_t i = 0; i < g_lighting_system.light_count; i++) {
        Light *light = g_lighting_system.lights[i];
        
        if (!light->enabled) continue;
        
        g_lighting_system.visible_lights++;
        
        if (light->shadow_mode != LIGHT_SHADOW_MODE_NONE) {
            g_lighting_system.shadow_casting_lights++;
            calculate_shadow_matrix(light, view_matrix, proj_matrix);
        }
        
        light->needs_update = false;
    }
    
    // Update light clusters
    assign_lights_to_clusters(view_matrix, proj_matrix);
    
    // TODO: Update GPU buffers
    // update_light_buffer();
    // update_cluster_buffer();
}

void lighting_set_ambient_lighting(float r, float g, float b, float intensity) {
    if (!g_lighting_system.initialized) return;
    
    g_lighting_system.ambient_color[0] = r;
    g_lighting_system.ambient_color[1] = g;
    g_lighting_system.ambient_color[2] = b;
    g_lighting_system.ambient_intensity = intensity;
}

Light *lighting_find_light(const char *name) {
    if (!g_lighting_system.initialized || !name) return NULL;
    
    for (uint32_t i = 0; i < g_lighting_system.light_count; i++) {
        if (strcmp(g_lighting_system.lights[i]->name, name) == 0) {
            return g_lighting_system.lights[i];
        }
    }
    
    return NULL;
}

void lighting_get_stats(uint32_t *total_lights, uint32_t *visible_lights, 
                       uint32_t *shadow_casters) {
    if (!g_lighting_system.initialized) return;
    
    if (total_lights) *total_lights = g_lighting_system.light_count;
    if (visible_lights) *visible_lights = g_lighting_system.visible_lights;
    if (shadow_casters) *shadow_casters = g_lighting_system.shadow_casting_lights;
}
