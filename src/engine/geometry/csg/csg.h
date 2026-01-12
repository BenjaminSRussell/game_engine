/*
 * csg.h
 * Constructive Solid Geometry operations
 *
 * Part of the Geometry subsystem
 * Advanced 3D Rendering Engine
 */

#ifndef GEOMETRY_CSG_H
#define GEOMETRY_CSG_H

#include <stdint.h>
#include <stdbool.h>
#include <stddef.h>
#include "geometry/geometry_types.h"

#ifdef __cplusplus
extern "C" {
#endif

/* ============================================================================
 * TYPES
 * ============================================================================ */

// CSG operation types
typedef enum csg_operation_e {
    CSG_OPERATION_UNION = 0,
    CSG_OPERATION_INTERSECTION = 1,
    CSG_OPERATION_DIFFERENCE = 2,
    CSG_OPERATION_XOR = 3
} csg_operation_e;

// CSG operation configuration
typedef struct csg_config_t {
    csg_operation_e operation;
    float weld_tolerance;        // Vertex welding tolerance
    bool preserve_normals;       // Preserve original normals where possible
    bool generate_smooth_normals; // Generate smooth normals for result
    bool optimize_result;        // Optimize resulting mesh
    float min_triangle_area;     // Minimum triangle area to keep
} csg_config_t;

// CSG operation statistics
typedef struct csg_stats_t {
    uint32_t input_vertices_a;
    uint32_t input_vertices_b;
    uint32_t input_triangles_a;
    uint32_t input_triangles_b;
    uint32_t output_vertices;
    uint32_t output_triangles;
    float operation_time_ms;
    uint32_t triangles_classified_inside;
    uint32_t triangles_classified_outside;
    uint32_t triangles_classified_intersecting;
} csg_stats_t;

/* ============================================================================
 * CONSTANTS
 * ============================================================================ */

extern const csg_config_t CSG_CONFIG_DEFAULT;

/* ============================================================================
 * MAIN CSG OPERATIONS
 * ============================================================================ */

// Perform CSG operation between two meshes
mesh_t* csg_perform_operation(const mesh_t* mesh_a, const mesh_t* mesh_b, 
                             const csg_config_t* config, csg_stats_t* stats);

// Convenience functions for common operations
mesh_t* csg_union(const mesh_t* mesh_a, const mesh_t* mesh_b);
mesh_t* csg_intersection(const mesh_t* mesh_a, const mesh_t* mesh_b);
mesh_t* csg_difference(const mesh_t* mesh_a, const mesh_t* mesh_b);
mesh_t* csg_xor(const mesh_t* mesh_a, const mesh_t* mesh_b);

// Advanced operations with configuration
mesh_t* csg_union_advanced(const mesh_t* mesh_a, const mesh_t* mesh_b, 
                           const csg_config_t* config, csg_stats_t* stats);
mesh_t* csg_intersection_advanced(const mesh_t* mesh_a, const mesh_t* mesh_b, 
                                 const csg_config_t* config, csg_stats_t* stats);
mesh_t* csg_difference_advanced(const mesh_t* mesh_a, const mesh_t* mesh_b, 
                               const csg_config_t* config, csg_stats_t* stats);
mesh_t* csg_xor_advanced(const mesh_t* mesh_a, const mesh_t* mesh_b, 
                        const csg_config_t* config, csg_stats_t* stats);

/* ============================================================================
 * UTILITY FUNCTIONS
 * ============================================================================ */

// Get default configuration
csg_config_t csg_get_default_config(void);

// Validate mesh for CSG operations
bool csg_validate_mesh(const mesh_t* mesh);

// Check if two meshes intersect
bool csg_meshes_intersect(const mesh_t* mesh_a, const mesh_t* mesh_b);

// Get bounding box intersection
bool csg_bounds_intersect(const mesh_bounds_t* bounds_a, const mesh_bounds_t* bounds_b);

/* ============================================================================
 * BOOLEAN OPERATIONS
 * ============================================================================ */

// Point-in-mesh testing
bool csg_point_inside_mesh(const Vec3* point, const mesh_t* mesh);

// Triangle classification
typedef enum csg_triangle_classification_e {
    CSG_TRIANGLE_INSIDE = 0,
    CSG_TRIANGLE_OUTSIDE = 1,
    CSG_TRIANGLE_INTERSECTING = 2
} csg_triangle_classification_e;

csg_triangle_classification_e csg_classify_triangle(const mesh_t* mesh, const mesh_t* other_mesh,
                                                   uint32_t triangle_index);

/* ============================================================================
 * MESH PROCESSING
 * ============================================================================ */

// Triangle intersection
typedef struct csg_triangle_intersection_t {
    bool intersects;
    Vec3 intersection_points[6];  // Max 6 intersection points for triangle-triangle
    uint32_t intersection_count;
    Vec3 normal;
} csg_triangle_intersection_t;

bool csg_triangles_intersect(const mesh_t* mesh_a, const mesh_t* mesh_b,
                             uint32_t tri_a, uint32_t tri_b,
                             csg_triangle_intersection_t* result);

// Mesh subdivision for CSG
mesh_t* csg_subdivide_mesh_at_intersections(const mesh_t* mesh_a, const mesh_t* mesh_b);

/* ============================================================================
 * DEBUGGING AND VALIDATION
 * ============================================================================ */

// Validate CSG result
bool csg_validate_result(const mesh_t* result, const mesh_t* mesh_a, const mesh_t* mesh_b);

// Print CSG statistics
void csg_print_stats(const csg_stats_t* stats);

// Debug visualization helpers
void csg_debug_print_triangle_info(const mesh_t* mesh, uint32_t triangle_index);
void csg_debug_print_classification(const mesh_t* mesh, const mesh_t* other_mesh);

#ifdef __cplusplus
}
#endif

#endif /* GEOMETRY_CSG_H */
