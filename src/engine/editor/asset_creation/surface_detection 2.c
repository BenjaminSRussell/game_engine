/**
 * =================================================================================================
 *                           SURFACE DETECTION SYSTEM
 *                               Agent: AGENT_TEMPLATE_1
 * =================================================================================================
 *
 * Implementation of surface detection system for asset placement
 * with ray casting and collision detection.
 *
 * =================================================================================================
 */

#include "asset_placement.h"
#include <core/logger.h>
#include <core/memory.h>
#include <string.h>
#include <stdlib.h>
#include <math.h>

// Surface detection system
#define MAX_SURFACE_SAMPLES 16
#define MAX_RAYCAST_DISTANCE 1000.0f
#define SURFACE_NORMAL_TOLERANCE 0.1f

typedef struct {
    Vec3 position;
    Vec3 normal;
    Vec3 tangent;
    Vec3 bitangent;
    f32 distance;
    bool is_valid;
    SurfaceType surface_type;
    u32 entity_id;
    u32 material_id;
} SurfaceHit;

typedef struct {
    SurfaceHit hits[MAX_SURFACE_SAMPLES];
    u32 hit_count;
    Vec3 average_position;
    Vec3 average_normal;
    f32 confidence;
    bool is_surface_detected;
} SurfaceDetectionResult;

// Surface detection context
typedef struct {
    bool initialized;
    f32 raycast_distance;
    u32 sample_count;
    f32 sample_radius;
    bool enable_normal_filtering;
    f32 normal_tolerance;
    bool enable_height_filtering;
    f32 height_tolerance;
} SurfaceDetectionContext;

static SurfaceDetectionContext g_surface_context = {0};

// Initialize surface detection system
bool surface_detection_init(void) {
    if (g_surface_context.initialized) {
        return true;
    }
    
    memset(&g_surface_context, 0, sizeof(SurfaceDetectionContext));
    g_surface_context.raycast_distance = MAX_RAYCAST_DISTANCE;
    g_surface_context.sample_count = 9; // 3x3 grid
    g_surface_context.sample_radius = 0.1f;
    g_surface_context.enable_normal_filtering = true;
    g_surface_context.normal_tolerance = SURFACE_NORMAL_TOLERANCE;
    g_surface_context.enable_height_filtering = true;
    g_surface_context.height_tolerance = 0.5f;
    g_surface_context.initialized = true;
    
    LOG_INFO("Surface detection system initialized");
    return true;
}

// Perform raycast to detect surface
static bool perform_raycast(Vec3 origin, Vec3 direction, f32 max_distance, SurfaceHit* hit) {
    if (!hit) return false;
    
    // Normalize direction
    direction = vec3_normalize(direction);
    
    // Perform raycast using physics system
    RaycastHit ray_hit;
    if (!physics_raycast(origin, direction, max_distance, &ray_hit)) {
        return false;
    }
    
    // Fill surface hit data
    hit->position = ray_hit.position;
    hit->normal = ray_hit.normal;
    hit->distance = ray_hit.distance;
    hit->is_valid = true;
    hit->entity_id = ray_hit.entity_id;
    
    // Calculate tangent and bitangent
    Vec3 world_up = (Vec3){0.0f, 1.0f, 0.0f};
    
    // Handle case where normal is parallel to world up
    if (fabsf(vec3_dot(hit->normal, world_up)) > 0.99f) {
        world_up = (Vec3){0.0f, 0.0f, 1.0f};
    }
    
    hit->tangent = vec3_normalize(vec3_cross(world_up, hit->normal));
    hit->bitangent = vec3_cross(hit->normal, hit->tangent);
    
    // Determine surface type based on normal
    f32 up_component = hit->normal.y;
    if (up_component > 0.9f) {
        hit->surface_type = SURFACE_TYPE_GROUND;
    } else if (up_component < -0.9f) {
        hit->surface_type = SURFACE_TYPE_CEILING;
    } else if (fabsf(hit->normal.x) > 0.9f) {
        hit->surface_type = (hit->normal.x > 0) ? SURFACE_TYPE_WALL_EAST : SURFACE_TYPE_WALL_WEST;
    } else if (fabsf(hit->normal.z) > 0.9f) {
        hit->surface_type = (hit->normal.z > 0) ? SURFACE_TYPE_WALL_NORTH : SURFACE_TYPE_WALL_SOUTH;
    } else {
        hit->surface_type = SURFACE_TYPE_SLOPE;
    }
    
    // Get material ID from entity
    if (hit->entity_id != 0) {
        hit->material_id = entity_get_material_id(hit->entity_id);
    } else {
        hit->material_id = 0;
    }
    
    return true;
}

// Detect surface at given position
SurfaceDetectionResult surface_detection_detect_at_position(Vec3 position) {
    SurfaceDetectionResult result = {0};
    
    if (!g_surface_context.initialized) {
        LOG_ERROR("Surface detection system not initialized");
        return result;
    }
    
    // Cast rays in a pattern around the position
    u32 sample_count = g_surface_context.sample_count;
    f32 sample_radius = g_surface_context.sample_radius;
    
    // Calculate sample grid dimensions
    u32 grid_size = (u32)sqrtf((f32)sample_count);
    if (grid_size * grid_size > sample_count) {
        grid_size--;
    }
    
    f32 step_size = (sample_radius * 2.0f) / (f32)(grid_size - 1);
    f32 start_offset = -sample_radius;
    
    // Cast rays from above the position
    Vec3 ray_origin = (Vec3){position.x, position.y + 10.0f, position.z};
    Vec3 ray_direction = (Vec3){0.0f, -1.0f, 0.0f}; // Downward
    
    // Perform raycasts
    for (u32 x = 0; x < grid_size; x++) {
        for (u32 z = 0; z < grid_size; z++) {
            if (result.hit_count >= MAX_SURFACE_SAMPLES) break;
            
            Vec3 sample_offset = {
                start_offset + (f32)x * step_size,
                0.0f,
                start_offset + (f32)z * step_size
            };
            
            Vec3 sample_origin = vec3_add(ray_origin, sample_offset);
            SurfaceHit hit = {0};
            
            if (perform_raycast(sample_origin, ray_direction, g_surface_context.raycast_distance, &hit)) {
                result.hits[result.hit_count++] = hit;
            }
        }
    }
    
    // Analyze results
    if (result.hit_count == 0) {
        LOG_DEBUG("No surface detected at position (%.2f, %.2f, %.2f)", 
                 position.x, position.y, position.z);
        return result;
    }
    
    // Calculate average position and normal
    Vec3 position_sum = {0.0f, 0.0f, 0.0f};
    Vec3 normal_sum = {0.0f, 0.0f, 0.0f};
    
    for (u32 i = 0; i < result.hit_count; i++) {
        position_sum = vec3_add(position_sum, result.hits[i].position);
        normal_sum = vec3_add(normal_sum, result.hits[i].normal);
    }
    
    result.average_position = vec3_multiply(position_sum, 1.0f / (f32)result.hit_count);
    result.average_normal = vec3_normalize(normal_sum);
    
    // Filter results by normal consistency
    if (g_surface_context.enable_normal_filtering) {
        u32 valid_hits = 0;
        f32 normal_tolerance = g_surface_context.normal_tolerance;
        
        for (u32 i = 0; i < result.hit_count; i++) {
            f32 dot_product = vec3_dot(result.average_normal, result.hits[i].normal);
            if (dot_product >= (1.0f - normal_tolerance)) {
                result.hits[valid_hits++] = result.hits[i];
            }
        }
        
        result.hit_count = valid_hits;
        
        // Recalculate averages if we filtered out hits
        if (valid_hits > 0 && valid_hits < result.hit_count) {
            position_sum = (Vec3){0.0f, 0.0f, 0.0f};
            normal_sum = (Vec3){0.0f, 0.0f, 0.0f};
            
            for (u32 i = 0; i < valid_hits; i++) {
                position_sum = vec3_add(position_sum, result.hits[i].position);
                normal_sum = vec3_add(normal_sum, result.hits[i].normal);
            }
            
            result.average_position = vec3_multiply(position_sum, 1.0f / (f32)valid_hits);
            result.average_normal = vec3_normalize(normal_sum);
        }
    }
    
    // Filter results by height consistency
    if (g_surface_context.enable_height_filtering && result.hit_count > 1) {
        f32 height_tolerance = g_surface_context.height_tolerance;
        f32 avg_height = result.average_position.y;
        
        u32 valid_hits = 0;
        for (u32 i = 0; i < result.hit_count; i++) {
            if (fabsf(result.hits[i].position.y - avg_height) <= height_tolerance) {
                result.hits[valid_hits++] = result.hits[i];
            }
        }
        
        result.hit_count = valid_hits;
        
        // Recalculate averages if we filtered out hits
        if (valid_hits > 0 && valid_hits < result.hit_count) {
            position_sum = (Vec3){0.0f, 0.0f, 0.0f};
            normal_sum = (Vec3){0.0f, 0.0f, 0.0f};
            
            for (u32 i = 0; i < valid_hits; i++) {
                position_sum = vec3_add(position_sum, result.hits[i].position);
                normal_sum = vec3_add(normal_sum, result.hits[i].normal);
            }
            
            result.average_position = vec3_multiply(position_sum, 1.0f / (f32)valid_hits);
            result.average_normal = vec3_normalize(normal_sum);
        }
    }
    
    // Calculate confidence based on hit consistency
    if (result.hit_count > 0) {
        f32 position_variance = 0.0f;
        f32 normal_variance = 0.0f;
        
        for (u32 i = 0; i < result.hit_count; i++) {
            f32 pos_diff = vec3_length(vec3_subtract(result.hits[i].position, result.average_position));
            f32 normal_diff = 1.0f - vec3_dot(result.hits[i].normal, result.average_normal);
            
            position_variance += pos_diff * pos_diff;
            normal_variance += normal_diff * normal_diff;
        }
        
        position_variance /= (f32)result.hit_count;
        normal_variance /= (f32)result.hit_count;
        
        // Higher confidence for lower variance
        f32 position_confidence = expf(-position_variance * 10.0f);
        f32 normal_confidence = expf(-normal_variance * 20.0f);
        
        result.confidence = (position_confidence + normal_confidence) * 0.5f;
        result.is_surface_detected = result.confidence > 0.5f;
    } else {
        result.confidence = 0.0f;
        result.is_surface_detected = false;
    }
    
    LOG_DEBUG("Surface detection: %u hits, confidence=%.2f, detected=%s", 
             result.hit_count, result.confidence, result.is_surface_detected ? "YES" : "NO");
    
    return result;
}

// Get surface type at position
SurfaceType surface_detection_get_surface_type(Vec3 position) {
    SurfaceDetectionResult result = surface_detection_detect_at_position(position);
    
    if (!result.is_surface_detected || result.hit_count == 0) {
        return SURFACE_TYPE_UNKNOWN;
    }
    
    // Return the most common surface type
    u32 type_counts[SURFACE_TYPE_COUNT] = {0};
    
    for (u32 i = 0; i < result.hit_count; i++) {
        if (result.hits[i].surface_type < SURFACE_TYPE_COUNT) {
            type_counts[result.hits[i].surface_type]++;
        }
    }
    
    u32 max_count = 0;
    SurfaceType most_common = SURFACE_TYPE_UNKNOWN;
    
    for (u32 i = 0; i < SURFACE_TYPE_COUNT; i++) {
        if (type_counts[i] > max_count) {
            max_count = type_counts[i];
            most_common = (SurfaceType)i;
        }
    }
    
    return most_common;
}

// Get surface normal at position
Vec3 surface_detection_get_normal(Vec3 position) {
    SurfaceDetectionResult result = surface_detection_detect_at_position(position);
    
    if (!result.is_surface_detected) {
        return (Vec3){0.0f, 1.0f, 0.0f}; // Default to up
    }
    
    return result.average_normal;
}

// Get surface height at position (for ground snapping)
f32 surface_detection_get_height(Vec3 position) {
    SurfaceDetectionResult result = surface_detection_detect_at_position(position);
    
    if (!result.is_surface_detected) {
        return FLT_MAX;
    }
    
    return result.average_position.y;
}

// Check if position is on valid surface
bool surface_detection_is_valid_surface(Vec3 position) {
    SurfaceDetectionResult result = surface_detection_detect_at_position(position);
    
    return result.is_surface_detected && result.confidence > 0.7f;
}

// Configure surface detection parameters
void surface_detection_set_sample_count(u32 count) {
    if (count > 0 && count <= MAX_SURFACE_SAMPLES) {
        g_surface_context.sample_count = count;
        LOG_DEBUG("Set surface detection sample count to %u", count);
    }
}

void surface_detection_set_sample_radius(f32 radius) {
    if (radius > 0.0f && radius <= 10.0f) {
        g_surface_context.sample_radius = radius;
        LOG_DEBUG("Set surface detection sample radius to %.2f", radius);
    }
}

void surface_detection_set_normal_tolerance(f32 tolerance) {
    if (tolerance >= 0.0f && tolerance <= 1.0f) {
        g_surface_context.normal_tolerance = tolerance;
        LOG_DEBUG("Set surface detection normal tolerance to %.2f", tolerance);
    }
}

void surface_detection_set_height_tolerance(f32 tolerance) {
    if (tolerance >= 0.0f && tolerance <= 10.0f) {
        g_surface_context.height_tolerance = tolerance;
        LOG_DEBUG("Set surface detection height tolerance to %.2f", tolerance);
    }
}

void surface_detection_enable_normal_filtering(bool enabled) {
    g_surface_context.enable_normal_filtering = enabled;
    LOG_DEBUG("Surface detection normal filtering %s", enabled ? "enabled" : "disabled");
}

void surface_detection_enable_height_filtering(bool enabled) {
    g_surface_context.enable_height_filtering = enabled;
    LOG_DEBUG("Surface detection height filtering %s", enabled ? "enabled" : "disabled");
}

// Public API functions
bool surface_detection_is_initialized(void) {
    return g_surface_context.initialized;
}

void surface_detection_cleanup(void) {
    memset(&g_surface_context, 0, sizeof(SurfaceDetectionContext));
    LOG_INFO("Surface detection system cleaned up");
}
