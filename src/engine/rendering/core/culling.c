// src/engine/rendering/core/culling.c
// Culling System - Frustum, occlusion, and distance culling

#include <core/logger.h>
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
        
        // TODO: Occlusion culling
        // if (visible && !occlusion_test(sphere)) {
        //     visible = false;
        //     system->occlusion_culled++;
        // }
        
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
