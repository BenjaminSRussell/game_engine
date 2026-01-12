/**
 * =================================================================================================
 *                           SURFACE SNAPPING SYSTEM
 *                               Agent: AGENT_TEMPLATE_1
 * =================================================================================================
 *
 * Implementation of surface snapping system for precise asset placement
 * with automatic surface detection and alignment.
 *
 * =================================================================================================
 */

#include "asset_placement.h"
#include "surface_detection.h"
#include <core/logger.h>
#include <core/memory.h>
#include <string.h>
#include <stdlib.h>
#include <math.h>

// Surface snapping system
#define MAX_SNAP_POINTS 32
#define SNAP_THRESHOLD 0.1f
#define ALIGNMENT_THRESHOLD 0.98f

typedef struct {
    Vec3 position;
    Vec3 normal;
    Vec3 up_vector;
    Vec3 forward_vector;
    Vec3 right_vector;
    SurfaceType surface_type;
    f32 confidence;
    bool is_valid;
} SnapPoint;

typedef struct {
    SnapPoint points[MAX_SNAP_POINTS];
    u32 point_count;
    Vec3 best_position;
    Vec3 best_normal;
    Vec3 best_up;
    Vec3 best_forward;
    SurfaceType best_surface_type;
    f32 best_confidence;
    bool has_snap_target;
} SnapResult;

// Surface snapping context
typedef struct {
    bool initialized;
    bool enable_surface_snapping;
    bool enable_normal_alignment;
    bool enable_grid_snapping;
    bool enable_edge_snapping;
    bool enable_vertex_snapping;
    f32 snap_strength;
    f32 snap_threshold;
    f32 alignment_threshold;
    u32 max_snap_distance;
} SurfaceSnappingContext;

static SurfaceSnappingContext g_snap_context = {0};

// Initialize surface snapping system
bool surface_snapping_init(void) {
    if (g_snap_context.initialized) {
        return true;
    }
    
    memset(&g_snap_context, 0, sizeof(SurfaceSnappingContext));
    g_snap_context.enable_surface_snapping = true;
    g_snap_context.enable_normal_alignment = true;
    g_snap_context.enable_grid_snapping = false;
    g_snap_context.enable_edge_snapping = true;
    g_snap_context.enable_vertex_snapping = true;
    g_snap_context.snap_strength = 1.0f;
    g_snap_context.snap_threshold = SNAP_THRESHOLD;
    g_snap_context.alignment_threshold = ALIGNMENT_THRESHOLD;
    g_snap_context.max_snap_distance = 10;
    g_snap_context.initialized = true;
    
    LOG_INFO("Surface snapping system initialized");
    return true;
}

// Calculate orientation vectors from normal
static void calculate_orientation_from_normal(Vec3 normal, Vec3* up, Vec3* forward, Vec3* right) {
    if (!up || !forward || !right) return;
    
    // Default up vector
    *up = normal;
    
    // Calculate forward vector (project world forward onto surface plane)
    Vec3 world_forward = (Vec3){0.0f, 0.0f, 1.0f};
    f32 dot_product = vec3_dot(world_forward, normal);
    *forward = vec3_normalize(vec3_subtract(world_forward, vec3_scale(normal, dot_product)));
    
    // Handle case where forward is parallel to normal
    if (vec3_length(*forward) < 0.1f) {
        world_forward = (Vec3){1.0f, 0.0f, 0.0f};
        dot_product = vec3_dot(world_forward, normal);
        *forward = vec3_normalize(vec3_subtract(world_forward, vec3_scale(normal, dot_product)));
    }
    
    // Calculate right vector
    *right = vec3_cross(*forward, *up);
}

// Snap to surface
static SnapPoint snap_to_surface(Vec3 position) {
    SnapPoint snap = {0};
    
    if (!g_snap_context.enable_surface_snapping) {
        return snap;
    }
    
    // Detect surface at position
    SurfaceDetectionResult surface_result = surface_detection_detect_at_position(position);
    
    if (!surface_result.is_surface_detected || surface_result.confidence < 0.5f) {
        return snap;
    }
    
    snap.position = surface_result.average_position;
    snap.normal = surface_result.average_normal;
    snap.surface_type = surface_result.hits[0].surface_type;
    snap.confidence = surface_result.confidence;
    snap.is_valid = true;
    
    // Calculate orientation
    calculate_orientation_from_normal(snap.normal, &snap.up_vector, &snap.forward_vector, &snap.right_vector);
    
    return snap;
}

// Snap to grid
static SnapPoint snap_to_grid(Vec3 position) {
    SnapPoint snap = {0};
    
    if (!g_snap_context.enable_grid_snapping) {
        return snap;
    }
    
    // Get grid settings from asset placement context
    AssetPlacementContext* placement_ctx = asset_placement_get_context();
    if (!placement_ctx || !placement_ctx->grid_snapping_enabled) {
        return snap;
    }
    
    f32 grid_size = placement_ctx->grid_size;
    
    // Snap position to grid
    snap.position.x = roundf(position.x / grid_size) * grid_size;
    snap.position.y = position.y; // Keep Y unchanged for now
    snap.position.z = roundf(position.z / grid_size) * grid_size;
    
    // Get surface normal at snapped position
    snap.normal = surface_detection_get_normal(snap.position);
    snap.surface_type = surface_detection_get_surface_type(snap.position);
    snap.confidence = 0.8f; // Grid snapping has moderate confidence
    snap.is_valid = true;
    
    // Calculate orientation
    calculate_orientation_from_normal(snap.normal, &snap.up_vector, &snap.forward_vector, &snap.right_vector);
    
    return snap;
}

// Snap to edges
static SnapPoint snap_to_edges(Vec3 position) {
    SnapPoint snap = {0};
    
    if (!g_snap_context.enable_edge_snapping) {
        return snap;
    }
    
    // Find nearby edges in the scene
    // This would query the scene graph for edge geometry
    // For now, we'll implement a simplified version
    
    // Cast rays in multiple directions to find edges
    Vec3 directions[8] = {
        {1.0f, 0.0f, 0.0f}, {-1.0f, 0.0f, 0.0f},
        {0.0f, 0.0f, 1.0f}, {0.0f, 0.0f, -1.0f},
        {1.0f, 0.0f, 1.0f}, {-1.0f, 0.0f, -1.0f},
        {1.0f, 0.0f, -1.0f}, {-1.0f, 0.0f, 1.0f}
    };
    
    f32 closest_distance = g_snap_context.max_snap_distance;
    Vec3 closest_point = position;
    Vec3 closest_normal = (Vec3){0.0f, 1.0f, 0.0f};
    bool found_edge = false;
    
    for (u32 i = 0; i < 8; i++) {
        RaycastHit hit;
        if (physics_raycast(position, directions[i], g_snap_context.max_snap_distance, &hit)) {
            if (hit.distance < closest_distance) {
                closest_distance = hit.distance;
                closest_point = hit.position;
                closest_normal = hit.normal;
                found_edge = true;
            }
        }
    }
    
    if (found_edge && closest_distance < g_snap_context.snap_threshold) {
        snap.position = closest_point;
        snap.normal = closest_normal;
        snap.surface_type = SURFACE_TYPE_WALL; // Assume edge is on a wall
        snap.confidence = 0.9f; // Edge snapping has high confidence
        snap.is_valid = true;
        
        // Calculate orientation
        calculate_orientation_from_normal(snap.normal, &snap.up_vector, &snap.forward_vector, &snap.right_vector);
    }
    
    return snap;
}

// Snap to vertices
static SnapPoint snap_to_vertices(Vec3 position) {
    SnapPoint snap = {0};
    
    if (!g_snap_context.enable_vertex_snapping) {
        return snap;
    }
    
    // Find nearby vertices in the scene
    // This would query the scene graph for vertex positions
    // For now, we'll implement a simplified version
    
    // Get all entities in range
    u32 entity_count = 0;
    u32* entities = physics_get_entities_in_sphere(position, g_snap_context.max_snap_distance, &entity_count);
    
    if (!entities || entity_count == 0) {
        return snap;
    }
    
    f32 closest_distance = g_snap_context.max_snap_distance;
    Vec3 closest_vertex = position;
    bool found_vertex = false;
    
    // Check each entity for vertices
    for (u32 i = 0; i < entity_count && i < 16; i++) { // Limit to 16 entities for performance
        u32 vertex_count = 0;
        Vec3* vertices = entity_get_vertices(entities[i], &vertex_count);
        
        if (!vertices || vertex_count == 0) continue;
        
        // Find closest vertex
        for (u32 j = 0; j < vertex_count && j < 64; j++) { // Limit to 64 vertices per entity
            f32 distance = vec3_length(vec3_subtract(vertices[j], position));
            if (distance < closest_distance) {
                closest_distance = distance;
                closest_vertex = vertices[j];
                found_vertex = true;
            }
        }
    }
    
    if (found_vertex && closest_distance < g_snap_context.snap_threshold) {
        snap.position = closest_vertex;
        snap.normal = surface_detection_get_normal(closest_vertex);
        snap.surface_type = surface_detection_get_surface_type(closest_vertex);
        snap.confidence = 0.95f; // Vertex snapping has very high confidence
        snap.is_valid = true;
        
        // Calculate orientation
        calculate_orientation_from_normal(snap.normal, &snap.up_vector, &snap.forward_vector, &snap.right_vector);
    }
    
    if (entities) {
        free(entities);
    }
    
    return snap;
}

// Perform surface snapping
SnapResult surface_snapping_perform_snap(Vec3 position, Vec3 current_up, Vec3 current_forward) {
    SnapResult result = {0};
    
    if (!g_snap_context.initialized) {
        LOG_ERROR("Surface snapping system not initialized");
        return result;
    }
    
    // Collect all potential snap points
    SnapPoint surface_snap = snap_to_surface(position);
    SnapPoint grid_snap = snap_to_grid(position);
    SnapPoint edge_snap = snap_to_edges(position);
    SnapPoint vertex_snap = snap_to_vertices(position);
    
    // Add valid snap points to result
    if (surface_snap.is_valid) {
        result.points[result.point_count++] = surface_snap;
    }
    if (grid_snap.is_valid) {
        result.points[result.point_count++] = grid_snap;
    }
    if (edge_snap.is_valid) {
        result.points[result.point_count++] = edge_snap;
    }
    if (vertex_snap.is_valid) {
        result.points[result.point_count++] = vertex_snap;
    }
    
    if (result.point_count == 0) {
        LOG_DEBUG("No valid snap points found");
        return result;
    }
    
    // Find best snap point based on confidence and distance
    f32 best_score = 0.0f;
    u32 best_index = 0;
    
    for (u32 i = 0; i < result.point_count; i++) {
        SnapPoint* point = &result.points[i];
        
        // Calculate score based on confidence and distance
        f32 distance = vec3_length(vec3_subtract(point->position, position));
        f32 distance_score = expf(-distance * g_snap_context.snap_strength);
        f32 score = point->confidence * distance_score;
        
        // Bonus for normal alignment if enabled
        if (g_snap_context.enable_normal_alignment) {
            f32 alignment = vec3_dot(point->up_vector, current_up);
            if (alignment > g_snap_context.alignment_threshold) {
                score *= 1.2f; // 20% bonus for good alignment
            }
        }
        
        if (score > best_score) {
            best_score = score;
            best_index = i;
        }
    }
    
    // Set best result
    SnapPoint* best_point = &result.points[best_index];
    result.best_position = best_point->position;
    result.best_normal = best_point->normal;
    result.best_up = best_point->up_vector;
    result.best_forward = best_point->forward_vector;
    result.best_surface_type = best_point->surface_type;
    result.best_confidence = best_point->confidence;
    result.has_snap_target = true;
    
    LOG_DEBUG("Surface snapping: best_score=%.2f, confidence=%.2f, type=%d", 
             best_score, result.best_confidence, result.best_surface_type);
    
    return result;
}

// Get snapped position
Vec3 surface_snapping_get_snapped_position(Vec3 position, Vec3 current_up, Vec3 current_forward) {
    SnapResult result = surface_snapping_perform_snap(position, current_up, current_forward);
    
    if (!result.has_snap_target) {
        return position;
    }
    
    return result.best_position;
}

// Get snapped orientation
void surface_snapping_get_snapped_orientation(Vec3 position, Vec3 current_up, Vec3 current_forward,
                                               Vec3* out_up, Vec3* out_forward, Vec3* out_right) {
    if (!out_up || !out_forward || !out_right) return;
    
    SnapResult result = surface_snapping_perform_snap(position, current_up, current_forward);
    
    if (!result.has_snap_target) {
        *out_up = current_up;
        *out_forward = current_forward;
        *out_right = vec3_cross(current_forward, current_up);
        return;
    }
    
    *out_up = result.best_up;
    *out_forward = result.best_forward;
    *out_right = vec3_cross(result.best_forward, result.best_up);
}

// Check if position would snap
bool surface_snapping_would_snap(Vec3 position, Vec3 current_up, Vec3 current_forward) {
    SnapResult result = surface_snapping_perform_snap(position, current_up, current_forward);
    return result.has_snap_target;
}

// Configure surface snapping parameters
void surface_snapping_enable_surface_snapping(bool enabled) {
    g_snap_context.enable_surface_snapping = enabled;
    LOG_DEBUG("Surface snapping %s", enabled ? "enabled" : "disabled");
}

void surface_snapping_enable_normal_alignment(bool enabled) {
    g_snap_context.enable_normal_alignment = enabled;
    LOG_DEBUG("Normal alignment %s", enabled ? "enabled" : "disabled");
}

void surface_snapping_enable_grid_snapping(bool enabled) {
    g_snap_context.enable_grid_snapping = enabled;
    LOG_DEBUG("Grid snapping %s", enabled ? "enabled" : "disabled");
}

void surface_snapping_enable_edge_snapping(bool enabled) {
    g_snap_context.enable_edge_snapping = enabled;
    LOG_DEBUG("Edge snapping %s", enabled ? "enabled" : "disabled");
}

void surface_snapping_enable_vertex_snapping(bool enabled) {
    g_snap_context.enable_vertex_snapping = enabled;
    LOG_DEBUG("Vertex snapping %s", enabled ? "enabled" : "disabled");
}

void surface_snapping_set_snap_strength(f32 strength) {
    if (strength >= 0.0f && strength <= 10.0f) {
        g_snap_context.snap_strength = strength;
        LOG_DEBUG("Set snap strength to %.2f", strength);
    }
}

void surface_snapping_set_snap_threshold(f32 threshold) {
    if (threshold >= 0.01f && threshold <= 5.0f) {
        g_snap_context.snap_threshold = threshold;
        LOG_DEBUG("Set snap threshold to %.2f", threshold);
    }
}

void surface_snapping_set_max_snap_distance(u32 distance) {
    if (distance > 0 && distance <= 100) {
        g_snap_context.max_snap_distance = distance;
        LOG_DEBUG("Set max snap distance to %u", distance);
    }
}

// Public API functions
bool surface_snapping_is_initialized(void) {
    return g_snap_context.initialized;
}

void surface_snapping_cleanup(void) {
    memset(&g_snap_context, 0, sizeof(SurfaceSnappingContext));
    LOG_INFO("Surface snapping system cleaned up");
}
