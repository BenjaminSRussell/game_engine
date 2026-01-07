#include "light_culling.h"
#include "../../include/math/aabb.h"
#include <stdlib.h>
#include <string.h>
#include <stdio.h>
#include <math.h>

#define MAX_LIGHTS_PER_TILE 256

LightCullingSystem* LightCulling_Create(uint32_t screen_width, uint32_t screen_height) {
    LightCullingSystem* system = (LightCullingSystem*)malloc(sizeof(LightCullingSystem));
    if (!system) return NULL;
    
    // Calculate grid dimensions
    system->x_count = (screen_width + CULLING_TILE_SIZE - 1) / CULLING_TILE_SIZE;
    system->y_count = (screen_height + CULLING_TILE_SIZE - 1) / CULLING_TILE_SIZE;
    system->z_count = CULLING_GRID_Z;
    system->max_lights_per_tile = MAX_LIGHTS_PER_TILE;
    
    uint32_t total_tiles = system->x_count * system->y_count * system->z_count;
    
    // Allocate buffers
    // light_indices: flattened array of MAX_LIGHTS_PER_TILE * total_tiles
    // This consumes significant memory (e.g. 1920/16 * 1080/16 * 24 * 256 * 4 ~= 200MB)
    // Optimization: Use a linked list or offset buffer. For now, flat array.
    size_t indices_size = total_tiles * MAX_LIGHTS_PER_TILE * sizeof(uint32_t);
    system->light_indices = (uint32_t*)malloc(indices_size);
    memset(system->light_indices, 0, indices_size);
    
    system->light_counts = (uint32_t*)calloc(total_tiles, sizeof(uint32_t));
    
    printf("[LightCulling] Initialized grid: %ux%ux%u (%u tiles)\n", 
           system->x_count, system->y_count, system->z_count, total_tiles);
    
    return system;
}

void LightCulling_Destroy(LightCullingSystem* system) {
    if (system) {
        if (system->light_indices) free(system->light_indices);
        if (system->light_counts) free(system->light_counts);
        free(system);
    }
}

// Helper to get view space AABB of a tile
// This is a simplified approach. Accurate approach uses frustum planes.
static AABB CalculateTileAABB(const Mat4* proj, float near_z, float far_z, 
                             uint32_t tx, uint32_t ty, uint32_t tz, 
                             uint32_t grid_x, uint32_t grid_y, uint32_t grid_z) {
    // Determine min/max depth for this splice
    // We use a logarithmic distribution or linear. Let's use linear for simplicity first, 
    // or if we have near/far, we can split.
    // 24 slices.
    
    // Using logarithmic depth usually better.
    float n = near_z;
    float f = far_z;
    float ratio = f / n;
    
    float z_min = n * powf(ratio, (float)tz / grid_z);
    float z_max = n * powf(ratio, (float)(tz + 1) / grid_z);
    
    // View space Z is negative.
    // Let's assume right-handed coordinate system looking down -Z.
    float view_z_min = -z_max; // Further from camera
    float view_z_max = -z_min; // Closer to camera
    
    // We need bounds in X and Y at the furthest depth (view_z_min).
    // In view space, x = z * (ndc_x / proj[0][0]), y = z * (ndc_y / proj[1][1]) normally.
    // Normalized Device Coordinates: -1 to 1.
    
    float tile_percent_x_min = (float)tx / grid_x;
    float tile_percent_x_max = (float)(tx + 1) / grid_x;
    float tile_percent_y_min = (float)ty / grid_y;
    float tile_percent_y_max = (float)(ty + 1) / grid_y;
    
    // NDC
    float ndc_min_x = tile_percent_x_min * 2.0f - 1.0f;
    float ndc_max_x = tile_percent_x_max * 2.0f - 1.0f;
    float ndc_min_y = tile_percent_y_min * 2.0f - 1.0f; // Note: verify Y flip
    float ndc_max_y = tile_percent_y_max * 2.0f - 1.0f;
    
    // Projection parameters (assume specific matrix layout, e.g. m00 = 1/tan(fov/2)/aspect)
    float m00 = proj->m00;
    float m11 = proj->m11;

    // View space X/Y
    // x = z * ndc_x / m00
    // y = z * ndc_y / m11 (or similar)
    
    // We calculate corners at both Z depths to form AABB
    // Note: This AABB will be loose because frustum is a truncated pyramid, AABB is a box.
    // But it's enough for conservative culling.
    
    // At z_max (view_z_min)
    float x_min_far = view_z_min * ndc_max_x / m00; // Multiply by z (negative) swaps order?
    float x_max_far = view_z_min * ndc_min_x / m00;
    
    // Wait, if z is negative:
    // x = z * slope.
    // If z = -10. ndc_x = 1. x = -10/m00.
    // If z = -10. ndc_x = -1. x = 10/m00.
    // So X range is inverted if z is negative?
    // Let's rely on finding min/max of derived points.
    
    float corners_x[4];
    float corners_y[4];
    float corners_z[2] = {view_z_min, view_z_max};
    
    // Min/Max X/Y at Z far
    corners_x[0] = view_z_min * ndc_min_x / m00; // Actually simpler: just project 4 rays?
    // Let's trust logic: ViewX = ViewZ * NDCX / Proj00.
    
    AABB aabb;
    aabb.min = vec3(FLT_MAX, FLT_MAX, FLT_MAX);
    aabb.max = vec3(-FLT_MAX, -FLT_MAX, -FLT_MAX);
    
    for (int z_idx = 0; z_idx < 2; z_idx++) {
        float z = corners_z[z_idx];
        float x1 = z * ndc_min_x / m00;
        float x2 = z * ndc_max_x / m00;
        float y1 = z * ndc_min_y / m11;
        float y2 = z * ndc_max_y / m11;
        
        // Add all 4 points at this Z plane
        // Actually only need min/max of x1,x2, etc.
        aabb.min.x = fminf(aabb.min.x, fminf(x1, x2));
        aabb.max.x = fmaxf(aabb.max.x, fmaxf(x1, x2));
        aabb.min.y = fminf(aabb.min.y, fminf(y1, y2));
        aabb.max.y = fmaxf(aabb.max.y, fmaxf(y1, y2));
        aabb.min.z = fminf(aabb.min.z, z);
        aabb.max.z = fmaxf(aabb.max.z, z);
    }
    
    return aabb;
}

static bool CheckPointLightIntersection(const Light* light, AABB tile_aabb, Mat4 view) {
    // Transform light position to view space
    Vec3 view_pos = mat4_transform_point_optimized(view, light->point.position);
    float r = light->point.range;
    
    // Sphere-AABB intersection
    // Find closest point on AABB to sphere center
    Vec3 closest = vec3_clamp(view_pos, tile_aabb.min, tile_aabb.max);
    float dist_sq = vec3_distance_sq(view_pos, closest);
    return dist_sq <= (r * r);
    // Note: This is AABB-Sphere. AABB is aligned to view axes.
}

static bool CheckSpotLightIntersection(const Light* light, AABB tile_aabb, Mat4 view) {
    // Bounding sphere approximation for spot light
    // Improvement: Cone-AABB intersection
    Vec3 view_pos = mat4_transform_point_optimized(view, light->spot.position);
    float r = light->spot.range;
    
    // Simple sphere check
    Vec3 closest = vec3_clamp(view_pos, tile_aabb.min, tile_aabb.max);
    float dist_sq = vec3_distance_sq(view_pos, closest);
    return dist_sq <= (r * r);
}

void LightCulling_Update(LightCullingSystem* system, LightManager* light_manager, 
                        const Mat4* view, const Mat4* proj, 
                        float near_z, float far_z) {
    if (!system || !light_manager) return;
    
    // Clear counts
    uint32_t total_tiles = system->x_count * system->y_count * system->z_count;
    memset(system->light_counts, 0, total_tiles * sizeof(uint32_t));
    
    // Get all lights
    uint32_t active_light_count = LightManager_GetLightCount(light_manager); // Not strictly needed if iterating max
    const Light* lights = LightManager_GetLightsArray(light_manager);
    
    float z_factor = (float)system->z_count / logf(far_z / near_z);
    
    for (uint32_t i = 0; i < MAX_LIGHTS; i++) {
        const Light* l = &lights[i];
        if (!l->base.enabled || l->base.id == 0) continue;
        
        if (l->base.type == LIGHT_TYPE_DIRECTIONAL) {
            // Directional lights are global
            continue; 
        }
        
        // View space position and radius
        Vec3 pos_world;
        float radius;
        if (l->base.type == LIGHT_TYPE_POINT) {
            pos_world = l->point.position;
            radius = l->point.range;
        } else if (l->base.type == LIGHT_TYPE_SPOT) {
            pos_world = l->spot.position;
            radius = l->spot.range;
        } else {
            continue;
        }
        
        Vec3 view_pos = mat4_transform_point_optimized(*view, pos_world);
        
        // Depth bounds check
        // View Space Z is negative. Positive distance from camera is -view_pos.z
        float dist = -view_pos.z;
        float min_z = dist - radius;
        float max_z = dist + radius;
        
        if (min_z > far_z || max_z < near_z) continue;
        
        // Z Slices
        // Clamp to near/far
        min_z = MAX(min_z, near_z);
        max_z = MIN(max_z, far_z);
        
        uint32_t z_start = (uint32_t)(logf(min_z / near_z) * z_factor);
        uint32_t z_end = (uint32_t)(logf(max_z / near_z) * z_factor);
        
        z_start = MAX(0, MIN(z_start, system->z_count - 1));
        z_end = MAX(0, MIN(z_end, system->z_count - 1));
        
        // Screen space bounds
        // Project bounding sphere approximation
        // Assume spherical bound for simplicity
        
        // Bounds in View Space (at z=1)
        // x_proj = x / |z| * p00
        // y_proj = y / |z| * p11
        
        // We iterate tiles in [z_start, z_end]
        // For each slice, the light covers a certain screen area.
        // Actually, conservative bound: project sphere to screen AABB.
        
        // Simple screen space AABB of sphere:
        // Proper way involves finding extrema of sphere on screen.
        // Simplified: Project center, add radius projected?
        // Projected radius varies with Z. Use closest Z (min_z) for max screen size.
        
        // Project center
        Vec3 clip_pos = mat4_transform_point_optimized(*proj, view_pos); // This assumes w=1
        // Usually: vec4 clip = proj * view
        // Manual projection:
        float w = -view_pos.z; // Perspective division factor usually
        // Actually utilize mat4_transform_point but handle w?
        // mat4_transform_point does division by w if w!=1? No, usually not.
        
        // Let's do raw projection manually using proj matrix diagonals for simplicity, assuming standard perspective
        // p00 = 1/(aspect*tan), p11 = 1/tan
        float p00 = proj->m00;
        float p11 = proj->m11;
        
        // Center on screen (NDC)
        float cx = view_pos.x * p00 / -view_pos.z;
        float cy = view_pos.y * p11 / -view_pos.z;
        
        // Radius on screen (approx) at closest depth (conservative)
        // This is tricky. Sphere projected is an ellipse.
        // Conservative square: radius * p00 / min_z
        float cr_x = radius * p00 / min_z;
        float cr_y = radius * p11 / min_z;
        
        // Convert NDC to Tile Coords
        // NDC [-1, 1] -> [0, x_count]
        
        uint32_t tx_min = (uint32_t)(((cx - cr_x) * 0.5f + 0.5f) * system->x_count);
        uint32_t tx_max = (uint32_t)(((cx + cr_x) * 0.5f + 0.5f) * system->x_count);
        uint32_t ty_min = (uint32_t)(((cy - cr_y) * 0.5f + 0.5f) * system->y_count);
        uint32_t ty_max = (uint32_t)(((cy + cr_y) * 0.5f + 0.5f) * system->y_count);
        
        // Check clamps (careful with unsigned underflow)
        // If cx - cr_x < -1, (negative)*count is bad cast.
        // Use floats first.
        
        float ftx_min = ((cx - cr_x) * 0.5f + 0.5f) * system->x_count;
        float ftx_max = ((cx + cr_x) * 0.5f + 0.5f) * system->x_count;
        float fty_min = ((cy - cr_y) * 0.5f + 0.5f) * system->y_count;
        float fty_max = ((cy + cr_y) * 0.5f + 0.5f) * system->y_count;
        
        int itx_min = MAX(0, MIN((int)ftx_min, (int)system->x_count - 1));
        int itx_max = MAX(0, MIN((int)ftx_max, (int)system->x_count - 1));
        int ity_min = MAX(0, MIN((int)fty_min, (int)system->y_count - 1));
        int ity_max = MAX(0, MIN((int)fty_max, (int)system->y_count - 1));
        
        for (int z = z_start; z <= z_end; z++) {
            for (int y = ity_min; y <= ity_max; y++) {
                for (int x = itx_min; x <= itx_max; x++) {
                    uint32_t tile_idx = (z * system->y_count + y) * system->x_count + x;
                    
                    if (system->light_counts[tile_idx] < system->max_lights_per_tile) {
                        uint32_t offset = tile_idx * system->max_lights_per_tile + system->light_counts[tile_idx];
                        system->light_indices[offset] = l->base.id; // Store ID (1-based)
                        system->light_counts[tile_idx]++;
                    }
                }
            }
        }
    }
}

const uint32_t* LightCulling_GetLightIndices(LightCullingSystem* system) {
    return system ? system->light_indices : NULL;
}

const uint32_t* LightCulling_GetLightCounts(LightCullingSystem* system) {
    return system ? system->light_counts : NULL;
}
