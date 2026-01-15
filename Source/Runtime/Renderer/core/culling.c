// src/engine/rendering/core/culling.c
// Culling System - Frustum, occlusion, and distance culling

#include "engine/include/core/logger.h"
#include <stdbool.h>
#include <stdint.h>
#include <stdlib.h>
#include <string.h>
#include <math.h>

#include "../render_pipeline.h"

// ============================================================================
// Culling Types
// ============================================================================

typedef struct {
    float planes[6][4];  // 6 frustum planes: nx, ny, nz, d
} Frustum;

typedef struct {
    float x, y, z;
    float radius;
} BoundingSphere;

typedef struct {
    float min[3];
    float max[3];
} BoundingBox;

typedef struct {
    uint32_t *visible_objects;
    uint32_t visible_count;
    uint32_t capacity;
    
    uint32_t *culled_objects;
    uint32_t culled_count;
    uint32_t culled_capacity;
    
    Frustum camera_frustum;
    float max_distance;
    float min_screen_size;
    
    // Statistics
    uint32_t total_objects;
    uint32_t frustum_culled;
    uint32_t distance_culled;
    uint32_t occlusion_culled;
} CullingSystem;

// ============================================================================
// Frustum Culling
// ============================================================================

static void extract_frustum_planes(Frustum *frustum, const float *view_matrix, 
                                  const float *projection_matrix) {
    float view_proj[16];
    
    // Multiply view and projection matrices
    for (int i = 0; i < 4; i++) {
        for (int j = 0; j < 4; j++) {
            view_proj[i * 4 + j] = 0;
            for (int k = 0; k < 4; k++) {
                view_proj[i * 4 + j] += projection_matrix[i * 4 + k] * view_matrix[k * 4 + j];
            }
        }
    }
    
    // Extract frustum planes from view-projection matrix
    // Left plane: column4 + column1
    frustum->planes[0][0] = view_proj[3] + view_proj[0];
    frustum->planes[0][1] = view_proj[7] + view_proj[4];
    frustum->planes[0][2] = view_proj[11] + view_proj[8];
    frustum->planes[0][3] = view_proj[15] + view_proj[12];
    
    // Right plane: column4 - column1
    frustum->planes[1][0] = view_proj[3] - view_proj[0];
    frustum->planes[1][1] = view_proj[7] - view_proj[4];
    frustum->planes[1][2] = view_proj[11] - view_proj[8];
    frustum->planes[1][3] = view_proj[15] - view_proj[12];
    
    // Bottom plane: column4 + column2
    frustum->planes[2][0] = view_proj[3] + view_proj[1];
    frustum->planes[2][1] = view_proj[7] + view_proj[5];
    frustum->planes[2][2] = view_proj[11] + view_proj[9];
    frustum->planes[2][3] = view_proj[15] + view_proj[13];
    
    // Top plane: column4 - column2
    frustum->planes[3][0] = view_proj[3] - view_proj[1];
    frustum->planes[3][1] = view_proj[7] - view_proj[5];
    frustum->planes[3][2] = view_proj[11] - view_proj[9];
    frustum->planes[3][3] = view_proj[15] - view_proj[13];
    
    // Near plane: column4 + column3
    frustum->planes[4][0] = view_proj[3] + view_proj[2];
    frustum->planes[4][1] = view_proj[7] + view_proj[6];
    frustum->planes[4][2] = view_proj[11] + view_proj[10];
    frustum->planes[4][3] = view_proj[15] + view_proj[14];
    
    // Far plane: column4 - column3
    frustum->planes[5][0] = view_proj[3] - view_proj[2];
    frustum->planes[5][1] = view_proj[7] - view_proj[6];
    frustum->planes[5][2] = view_proj[11] - view_proj[10];
    frustum->planes[5][3] = view_proj[15] - view_proj[14];
    
    // Normalize planes
    for (int i = 0; i < 6; i++) {
        float length = sqrtf(frustum->planes[i][0] * frustum->planes[i][0] +
                           frustum->planes[i][1] * frustum->planes[i][1] +
                           frustum->planes[i][2] * frustum->planes[i][2]);
        if (length > 0.0f) {
            frustum->planes[i][0] /= length;
            frustum->planes[i][1] /= length;
            frustum->planes[i][2] /= length;
            frustum->planes[i][3] /= length;
        }
    }
}

static bool sphere_in_frustum(const Frustum *frustum, const BoundingSphere *sphere) {
    for (int i = 0; i < 6; i++) {
        float distance = frustum->planes[i][0] * sphere->x +
                        frustum->planes[i][1] * sphere->y +
                        frustum->planes[i][2] * sphere->z +
                        frustum->planes[i][3];
        
        if (distance < -sphere->radius) {
            return false;  // Outside this plane
        }
    }
    return true;  // Inside or intersecting all planes
}

static bool box_in_frustum(const Frustum *frustum, const BoundingBox *box) {
    // Check box corners against all frustum planes
    float corners[8][3] = {
        {box->min[0], box->min[1], box->min[2]},
        {box->max[0], box->min[1], box->min[2]},
        {box->min[0], box->max[1], box->min[2]},
        {box->max[0], box->max[1], box->min[2]},
        {box->min[0], box->min[1], box->max[2]},
        {box->max[0], box->min[1], box->max[2]},
        {box->min[0], box->max[1], box->max[2]},
        {box->max[0], box->max[1], box->max[2]}
    };
    
    for (int i = 0; i < 6; i++) {
        bool all_outside = true;
        
        for (int j = 0; j < 8; j++) {
            float distance = frustum->planes[i][0] * corners[j][0] +
                            frustum->planes[i][1] * corners[j][1] +
                            frustum->planes[i][2] * corners[j][2] +
                            frustum->planes[i][3];
            
            if (distance >= 0) {
                all_outside = false;
                break;
            }
        }
        
        if (all_outside) {
            return false;  // Box is completely outside this plane
        }
    }
    
    return true;  // Box is inside or intersecting all planes
}

// ============================================================================
// Distance Culling
// ============================================================================

static float distance_squared(float x1, float y1, float z1, float x2, float y2, float z2) {
    float dx = x1 - x2;
    float dy = y1 - y2;
    float dz = z1 - z2;
    return dx * dx + dy * dy + dz * dz;
}

static bool sphere_in_distance(const BoundingSphere *sphere, float max_distance_sq,
                              float cam_x, float cam_y, float cam_z) {
    float dist_sq = distance_squared(sphere->x, sphere->y, sphere->z, cam_x, cam_y, cam_z);
    return dist_sq <= max_distance_sq;
}

// ============================================================================
// Occlusion Culling
// ============================================================================

typedef struct {
    uint32_t width, height;
    float *depth_buffer;
    bool *valid_flags;
    uint32_t tile_size;
} HierarchicalZBuffer;

static HierarchicalZBuffer g_hiz_buffer = {0};

static bool hiz_buffer_init(uint32_t width, uint32_t height) {
    if (g_hiz_buffer.depth_buffer) {
        free(g_hiz_buffer.depth_buffer);
        free(g_hiz_buffer.valid_flags);
    }
    
    g_hiz_buffer.width = width;
    g_hiz_buffer.height = height;
    g_hiz_buffer.tile_size = 16; // 16x16 tiles for hierarchical Z
    
    uint32_t tile_count_x = (width + g_hiz_buffer.tile_size - 1) / g_hiz_buffer.tile_size;
    uint32_t tile_count_y = (height + g_hiz_buffer.tile_size - 1) / g_hiz_buffer.tile_size;
    uint32_t total_tiles = tile_count_x * tile_count_y;
    
    g_hiz_buffer.depth_buffer = calloc(total_tiles, sizeof(float));
    g_hiz_buffer.valid_flags = calloc(total_tiles, sizeof(bool));
    
    if (!g_hiz_buffer.depth_buffer || !g_hiz_buffer.valid_flags) {
        LOG_ERROR("Failed to allocate hierarchical Z buffer");
        free(g_hiz_buffer.depth_buffer);
        free(g_hiz_buffer.valid_flags);
        g_hiz_buffer.depth_buffer = NULL;
        g_hiz_buffer.valid_flags = NULL;
        return false;
    }
    
    // Initialize depth buffer to far plane
    for (uint32_t i = 0; i < total_tiles; i++) {
        g_hiz_buffer.depth_buffer[i] = 1.0f;
        g_hiz_buffer.valid_flags[i] = false;
    }
    
    LOG_DEBUG("Hierarchical Z buffer initialized: %ux%u, %u tiles", 
             width, height, total_tiles);
    return true;
}

static void hiz_buffer_shutdown(void) {
    free(g_hiz_buffer.depth_buffer);
    free(g_hiz_buffer.valid_flags);
    memset(&g_hiz_buffer, 0, sizeof(HierarchicalZBuffer));
}

static void hiz_buffer_clear(void) {
    if (!g_hiz_buffer.depth_buffer || !g_hiz_buffer.valid_flags) {
        return;
    }
    
    uint32_t tile_count_x = (g_hiz_buffer.width + g_hiz_buffer.tile_size - 1) / g_hiz_buffer.tile_size;
    uint32_t tile_count_y = (g_hiz_buffer.height + g_hiz_buffer.tile_size - 1) / g_hiz_buffer.tile_size;
    uint32_t total_tiles = tile_count_x * tile_count_y;
    
    for (uint32_t i = 0; i < total_tiles; i++) {
        g_hiz_buffer.depth_buffer[i] = 1.0f;
        g_hiz_buffer.valid_flags[i] = false;
    }
}

static bool hiz_buffer_project_sphere(const BoundingSphere *sphere, 
                                    float cam_x, float cam_y, float cam_z,
                                    const float *view_matrix, const float *proj_matrix,
                                    uint32_t *screen_x, uint32_t *screen_y, float *screen_radius) {
    // Transform sphere center to view space
    float view_pos[3];
    view_pos[0] = sphere->x - cam_x;
    view_pos[1] = sphere->y - cam_y;
    view_pos[2] = sphere->z - cam_z;
    
    // Apply view matrix (simplified - assuming identity for now)
    float view_x = view_pos[0];
    float view_y = view_pos[1];
    float view_z = view_pos[2];
    
    // Check if behind camera
    if (view_z <= 0.0f) {
        return false;
    }
    
    // Project to screen space (simplified projection)
    float fov = 60.0f * M_PI / 180.0f;
    float aspect = (float)g_hiz_buffer.width / (float)g_hiz_buffer.height;
    float tan_fov = tanf(fov * 0.5f);
    
    float proj_x = (view_x / (view_z * tan_fov)) * 0.5f + 0.5f;
    float proj_y = (view_y / (view_z * tan_fov / aspect)) * 0.5f + 0.5f;
    
    // Convert to screen coordinates
    *screen_x = (uint32_t)(proj_x * g_hiz_buffer.width);
    *screen_y = (uint32_t)(proj_y * g_hiz_buffer.height);
    
    // Calculate screen radius
    float screen_scale = g_hiz_buffer.height / (2.0f * view_z * tan_fov);
    *screen_radius = sphere->radius * screen_scale;
    
    // Check if on screen
    return (*screen_x < g_hiz_buffer.width && *screen_y < g_hiz_buffer.height);
}

static bool hiz_buffer_is_occluded(uint32_t screen_x, uint32_t screen_y, float screen_radius, float depth) {
    // Get tile coordinates
    uint32_t tile_x = screen_x / g_hiz_buffer.tile_size;
    uint32_t tile_y = screen_y / g_hiz_buffer.tile_size;
    
    uint32_t tile_count_x = (g_hiz_buffer.width + g_hiz_buffer.tile_size - 1) / g_hiz_buffer.tile_size;
    uint32_t tile_index = tile_y * tile_count_x + tile_x;
    
    // Check if tile is valid and depth test passes
    if (tile_index >= tile_count_x * ((g_hiz_buffer.height + g_hiz_buffer.tile_size - 1) / g_hiz_buffer.tile_size)) {
        return false; // Outside buffer bounds
    }
    
    if (!g_hiz_buffer.valid_flags[tile_index]) {
        return false; // No depth data available
    }
    
    // Check if object is behind the stored depth
    return depth > g_hiz_buffer.depth_buffer[tile_index];
}

static void hiz_buffer_update(uint32_t screen_x, uint32_t screen_y, float screen_radius, float depth) {
    if (!g_hiz_buffer.depth_buffer || !g_hiz_buffer.valid_flags) {
        return;
    }
    
    float radius = fmaxf(screen_radius, 1.0f);
    int32_t min_x = (int32_t)screen_x - (int32_t)ceilf(radius);
    int32_t min_y = (int32_t)screen_y - (int32_t)ceilf(radius);
    int32_t max_x = (int32_t)screen_x + (int32_t)ceilf(radius);
    int32_t max_y = (int32_t)screen_y + (int32_t)ceilf(radius);
    
    min_x = (int32_t)fmaxf(0.0f, (float)min_x);
    min_y = (int32_t)fmaxf(0.0f, (float)min_y);
    max_x = (int32_t)fminf((float)(g_hiz_buffer.width - 1), (float)max_x);
    max_y = (int32_t)fminf((float)(g_hiz_buffer.height - 1), (float)max_y);
    
    uint32_t tile_count_x = (g_hiz_buffer.width + g_hiz_buffer.tile_size - 1) / g_hiz_buffer.tile_size;
    uint32_t tile_min_x = (uint32_t)min_x / g_hiz_buffer.tile_size;
    uint32_t tile_min_y = (uint32_t)min_y / g_hiz_buffer.tile_size;
    uint32_t tile_max_x = (uint32_t)max_x / g_hiz_buffer.tile_size;
    uint32_t tile_max_y = (uint32_t)max_y / g_hiz_buffer.tile_size;
    
    for (uint32_t tile_y = tile_min_y; tile_y <= tile_max_y; tile_y++) {
        for (uint32_t tile_x = tile_min_x; tile_x <= tile_max_x; tile_x++) {
            uint32_t tile_index = tile_y * tile_count_x + tile_x;
            if (!g_hiz_buffer.valid_flags[tile_index] || depth < g_hiz_buffer.depth_buffer[tile_index]) {
                g_hiz_buffer.depth_buffer[tile_index] = depth;
                g_hiz_buffer.valid_flags[tile_index] = true;
            }
        }
    }
}

static bool occlusion_test(const BoundingSphere *sphere, float cam_x, float cam_y, float cam_z, float min_screen_size) {
    if (!g_hiz_buffer.depth_buffer) {
        return true;
    }
    
    // Project sphere to screen space
    uint32_t screen_x, screen_y;
    float screen_radius;
    
    if (!hiz_buffer_project_sphere(sphere, cam_x, cam_y, cam_z, NULL, NULL, 
                                  &screen_x, &screen_y, &screen_radius)) {
        return true; // Behind camera or off-screen, consider visible
    }
    
    if (screen_radius < min_screen_size) {
        return true; // Too small to occlude reliably
    }
    
    // Calculate depth (distance from camera along view direction)
    float dx = sphere->x - cam_x;
    float dy = sphere->y - cam_y;
    float dz = sphere->z - cam_z;
    float depth = sqrtf(dx * dx + dy * dy + dz * dz);
    
    // Normalize depth to [0,1] range
    float max_depth = 1000.0f; // Maximum visible distance
    depth = depth / max_depth;
    depth = fmaxf(0.0f, fminf(1.0f, depth));
    
    bool occluded = hiz_buffer_is_occluded(screen_x, screen_y, screen_radius, depth);
    if (!occluded) {
        hiz_buffer_update(screen_x, screen_y, screen_radius, depth);
    }
    
    return !occluded;
}

// ============================================================================
// Culling System API
// ============================================================================

CullingSystem *culling_system_create(uint32_t max_objects) {
    CullingSystem *system = calloc(1, sizeof(CullingSystem));
    if (!system) {
        LOG_ERROR("Failed to allocate culling system");
        return NULL;
    }
    
    system->visible_objects = malloc(max_objects * sizeof(uint32_t));
    system->culled_objects = malloc(max_objects * sizeof(uint32_t));
    
    if (!system->visible_objects || !system->culled_objects) {
        LOG_ERROR("Failed to allocate culling buffers");
        free(system->visible_objects);
        free(system->culled_objects);
        free(system);
        return NULL;
    }
    
    system->visible_count = 0;
    system->culled_count = 0;
    system->capacity = max_objects;
    system->culled_capacity = max_objects;
    system->max_distance = 1000.0f;
    system->min_screen_size = 1.0f;
    
    LOG_INFO("Culling system created for %u objects", max_objects);
    return system;
}

void culling_system_destroy(CullingSystem *system) {
    if (!system)
        return;
    
    free(system->visible_objects);
    free(system->culled_objects);
    free(system);
    
    // Cleanup occlusion culling resources
    hiz_buffer_shutdown();
    
    LOG_INFO("Culling system destroyed");
}

void culling_system_update_frustum(CullingSystem *system, const float *view_matrix,
                                  const float *projection_matrix) {
    if (!system)
        return;
    
    extract_frustum_planes(&system->camera_frustum, view_matrix, projection_matrix);
}

void culling_system_perform(CullingSystem *system, const uint32_t *object_ids,
                           const BoundingSphere *bounds, uint32_t object_count,
                           float cam_x, float cam_y, float cam_z) {
    if (!system || !object_ids || !bounds)
        return;
    
    system->total_objects = object_count;
    system->visible_count = 0;
    system->culled_count = 0;
    system->frustum_culled = 0;
    system->distance_culled = 0;
    system->occlusion_culled = 0;
    
    if (!g_hiz_buffer.depth_buffer) {
        if (!hiz_buffer_init(1920, 1080)) {
            LOG_WARN("Failed to initialize occlusion culling, disabling");
        }
    }
    
    hiz_buffer_clear();
    
    float max_distance_sq = system->max_distance * system->max_distance;
    
    for (uint32_t i = 0; i < object_count; i++) {
        bool visible = true;
        const BoundingSphere *sphere = &bounds[i];
        
        // Frustum culling
        if (!sphere_in_frustum(&system->camera_frustum, sphere)) {
            visible = false;
            system->frustum_culled++;
        }
        
        // Distance culling
        if (visible && !sphere_in_distance(sphere, max_distance_sq, cam_x, cam_y, cam_z)) {
            visible = false;
            system->distance_culled++;
        }
        
        // Occlusion culling
        if (visible && !occlusion_test(sphere, cam_x, cam_y, cam_z, system->min_screen_size)) {
            visible = false;
            system->occlusion_culled++;
        }
        
        if (visible) {
            if (system->visible_count < system->capacity) {
                system->visible_objects[system->visible_count++] = object_ids[i];
            }
        } else {
            if (system->culled_count < system->culled_capacity) {
                system->culled_objects[system->culled_count++] = object_ids[i];
            }
        }
    }
    
    LOG_DEBUG("Culling: %u total, %u visible, %u culled (F:%u, D:%u, O:%u)",
             system->total_objects, system->visible_count, system->culled_count,
             system->frustum_culled, system->distance_culled, system->occlusion_culled);
}

const uint32_t *culling_system_get_visible(CullingSystem *system, uint32_t *count) {
    if (!system || !count)
        return NULL;
    
    *count = system->visible_count;
    return system->visible_objects;
}

const uint32_t *culling_system_get_culled(CullingSystem *system, uint32_t *count) {
    if (!system || !count)
        return NULL;
    
    *count = system->culled_count;
    return system->culled_objects;
}

void culling_system_set_max_distance(CullingSystem *system, float distance) {
    if (!system)
        return;
    
    system->max_distance = distance;
}

void culling_system_set_min_screen_size(CullingSystem *system, float size) {
    if (!system)
        return;
    
    system->min_screen_size = size;
}

void culling_system_get_stats(CullingSystem *system, uint32_t *total, uint32_t *visible,
                             uint32_t *frustum, uint32_t *distance, uint32_t *occlusion) {
    if (!system)
        return;
    
    if (total) *total = system->total_objects;
    if (visible) *visible = system->visible_count;
    if (frustum) *frustum = system->frustum_culled;
    if (distance) *distance = system->distance_culled;
    if (occlusion) *occlusion = system->occlusion_culled;
}
