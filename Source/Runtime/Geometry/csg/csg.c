#include "geometry/csg/csg.h"
#include "geometry/mesh.h"
#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <math.h>

// ============================================================================
// CONSTANTS AND DEFAULTS
// ============================================================================

#define CSG_EPSILON 1e-6f
#define CSG_MAX_INTERSECTION_POINTS 10
#define CSG_DEFAULT_WELD_TOLERANCE 1e-4f
#define CSG_MIN_TRIANGLE_AREA_DEFAULT 1e-6f

const csg_config_t CSG_CONFIG_DEFAULT = {
    .operation = CSG_OPERATION_UNION,
    .weld_tolerance = CSG_DEFAULT_WELD_TOLERANCE,
    .preserve_normals = true,
    .generate_smooth_normals = true,
    .optimize_result = true,
    .min_triangle_area = CSG_MIN_TRIANGLE_AREA_DEFAULT
};

// ============================================================================
// INTERNAL STRUCTURES
// ============================================================================

typedef struct csg_triangle_t {
    Vec3 vertices[3];
    Vec3 normal;
    Vec3 original_vertices[3];  // Keep original for intersection calculations
    bool classified;
    csg_triangle_classification_e classification;
    bool kept;
} csg_triangle_t;

typedef struct csg_mesh_data_t {
    csg_triangle_t* triangles;
    uint32_t triangle_count;
    uint32_t triangle_capacity;
    mesh_bounds_t bounds;
} csg_mesh_data_t;

typedef struct csg_plane_t {
    Vec3 normal;
    float distance;
} csg_plane_t;

// ============================================================================
// UTILITY FUNCTIONS
// ============================================================================

static Vec3 triangle_normal(const Vec3* v0, const Vec3* v1, const Vec3* v2) {
    Vec3 edge1 = vec3_sub(*v1, *v0);
    Vec3 edge2 = vec3_sub(*v2, *v0);
    return vec3_normalize(vec3_cross(edge1, edge2));
}

static float triangle_area(const Vec3* v0, const Vec3* v1, const Vec3* v2) {
    Vec3 edge1 = vec3_sub(*v1, *v0);
    Vec3 edge2 = vec3_sub(*v2, *v0);
    Vec3 cross = vec3_cross(edge1, edge2);
    return vec3_length(cross) * 0.5f;
}

static csg_plane_t triangle_to_plane(const Vec3* v0, const Vec3* v1, const Vec3* v2) {
    csg_plane_t plane;
    plane.normal = triangle_normal(v0, v1, v2);
    plane.distance = vec3_dot(plane.normal, *v0);
    return plane;
}

static float point_plane_distance(const csg_plane_t* plane, const Vec3* point) {
    return vec3_dot(plane->normal, *point) - plane->distance;
}

static bool ray_triangle_intersection(const Vec3* ray_origin, const Vec3* ray_dir,
                                     const Vec3* v0, const Vec3* v1, const Vec3* v2,
                                     float* t, Vec3* intersection) {
    const float epsilon = CSG_EPSILON;
    
    Vec3 edge1 = vec3_sub(*v1, *v0);
    Vec3 edge2 = vec3_sub(*v2, *v0);
    Vec3 h = vec3_cross(*ray_dir, edge2);
    float a = vec3_dot(edge1, h);
    
    if (a > -epsilon && a < epsilon) {
        return false; // Ray is parallel to triangle
    }
    
    float f = 1.0f / a;
    Vec3 s = vec3_sub(*ray_origin, *v0);
    float u = f * vec3_dot(s, h);
    
    if (u < 0.0f || u > 1.0f) {
        return false;
    }
    
    Vec3 q = vec3_cross(s, edge1);
    float v = f * vec3_dot(*ray_dir, q);
    
    if (v < 0.0f || u + v > 1.0f) {
        return false;
    }
    
    float t_val = f * vec3_dot(edge2, q);
    
    if (t_val > epsilon) {
        *t = t_val;
        *intersection = vec3_add(*ray_origin, vec3_mul(*ray_dir, t_val));
        return true;
    }
    
    return false;
}

static bool point_inside_triangle(const Vec3* point, const Vec3* v0, const Vec3* v1, const Vec3* v2) {
    Vec3 edge0 = vec3_sub(*v1, *v0);
    Vec3 edge1 = vec3_sub(*v2, *v1);
    Vec3 edge2 = vec3_sub(*v0, *v2);
    
    Vec3 c0 = vec3_sub(*point, *v0);
    Vec3 c1 = vec3_sub(*point, *v1);
    Vec3 c2 = vec3_sub(*point, *v2);
    
    Vec3 n = triangle_normal(v0, v1, v2);
    
    float d0 = vec3_dot(vec3_cross(edge0, c0), n);
    float d1 = vec3_dot(vec3_cross(edge1, c1), n);
    float d2 = vec3_dot(vec3_cross(edge2, c2), n);
    
    return (d0 >= 0.0f && d1 >= 0.0f && d2 >= 0.0f) ||
           (d0 <= 0.0f && d1 <= 0.0f && d2 <= 0.0f);
}

// ============================================================================
// MESH DATA CONVERSION
// ============================================================================

static csg_mesh_data_t* csg_create_mesh_data(const mesh_t* mesh) {
    if (!mesh) return NULL;
    
    csg_mesh_data_t* data = calloc(1, sizeof(csg_mesh_data_t));
    if (!data) return NULL;
    
    data->triangle_capacity = mesh->index_count / 3;
    data->triangles = malloc(data->triangle_capacity * sizeof(csg_triangle_t));
    if (!data->triangles) {
        free(data);
        return NULL;
    }
    
    // Convert mesh triangles to CSG triangles
    for (uint32_t i = 0; i < mesh->index_count; i += 3) {
        if (data->triangle_count >= data->triangle_capacity) break;
        
        uint32_t i0 = mesh->indices[i];
        uint32_t i1 = mesh->indices[i + 1];
        uint32_t i2 = mesh->indices[i + 2];
        
        if (i0 >= mesh->vertex_count || i1 >= mesh->vertex_count || i2 >= mesh->vertex_count) {
            continue; // Skip invalid indices
        }
        
        csg_triangle_t* tri = &data->triangles[data->triangle_count];
        
        tri->vertices[0] = mesh->vertices[i0].position;
        tri->vertices[1] = mesh->vertices[i1].position;
        tri->vertices[2] = mesh->vertices[i2].position;
        
        tri->original_vertices[0] = tri->vertices[0];
        tri->original_vertices[1] = tri->vertices[1];
        tri->original_vertices[2] = tri->vertices[2];
        
        tri->normal = triangle_normal(&tri->vertices[0], &tri->vertices[1], &tri->vertices[2]);
        tri->classified = false;
        tri->kept = false;
        
        data->triangle_count++;
    }
    
    // Calculate bounds
    if (data->triangle_count > 0) {
        data->bounds.min = data->triangles[0].vertices[0];
        data->bounds.max = data->triangles[0].vertices[0];
        
        for (uint32_t i = 0; i < data->triangle_count; i++) {
            for (int j = 0; j < 3; j++) {
                Vec3 v = data->triangles[i].vertices[j];
                data->bounds.min.x = fminf(data->bounds.min.x, v.x);
                data->bounds.min.y = fminf(data->bounds.min.y, v.y);
                data->bounds.min.z = fminf(data->bounds.min.z, v.z);
                data->bounds.max.x = fmaxf(data->bounds.max.x, v.x);
                data->bounds.max.y = fmaxf(data->bounds.max.y, v.y);
                data->bounds.max.z = fmaxf(data->bounds.max.z, v.z);
            }
        }
        
        // Calculate sphere bounds
        Vec3 center = vec3_mul(vec3_add(data->bounds.min, data->bounds.max), 0.5f);
        float max_dist_sq = 0.0f;
        
        for (uint32_t i = 0; i < data->triangle_count; i++) {
            for (int j = 0; j < 3; j++) {
                Vec3 diff = vec3_sub(data->triangles[i].vertices[j], center);
                float dist_sq = vec3_dot(diff, diff);
                max_dist_sq = fmaxf(max_dist_sq, dist_sq);
            }
        }
        
        data->bounds.sphere_center = center;
        data->bounds.sphere_radius = sqrtf(max_dist_sq);
    }
    
    return data;
}

static void csg_destroy_mesh_data(csg_mesh_data_t* data) {
    if (!data) return;
    free(data->triangles);
    free(data);
}

// ============================================================================
// TRIANGLE CLASSIFICATION
// ============================================================================

static csg_triangle_classification_e csg_classify_triangle_against_mesh(
    const csg_triangle_t* triangle, const csg_mesh_data_t* mesh) {
    
    bool has_inside_point = false;
    bool has_outside_point = false;
    
    // Test multiple points on the triangle
    Vec3 test_points[7];
    
    // Triangle vertices
    test_points[0] = triangle->vertices[0];
    test_points[1] = triangle->vertices[1];
    test_points[2] = triangle->vertices[2];
    
    // Edge midpoints
    test_points[3] = vec3_mul(vec3_add(triangle->vertices[0], triangle->vertices[1]), 0.5f);
    test_points[4] = vec3_mul(vec3_add(triangle->vertices[1], triangle->vertices[2]), 0.5f);
    test_points[5] = vec3_mul(vec3_add(triangle->vertices[2], triangle->vertices[0]), 0.5f);
    
    // Triangle center
    test_points[6] = vec3_mul(vec3_add(vec3_add(triangle->vertices[0], triangle->vertices[1]), triangle->vertices[2]), 1.0f / 3.0f);
    
    // Cast rays from each test point
    for (int i = 0; i < 7; i++) {
        int intersection_count = 0;
        
        // Cast ray in positive Y direction
        Vec3 ray_dir = {0.0f, 1.0f, 0.0f};
        Vec3 ray_origin = test_points[i];
        ray_origin.y += 0.1f; // Small offset to avoid self-intersection
        
        for (uint32_t j = 0; j < mesh->triangle_count; j++) {
            const csg_triangle_t* other_tri = &mesh->triangles[j];
            
            float t;
            Vec3 intersection;
            if (ray_triangle_intersection(&ray_origin, &ray_dir,
                                       &other_tri->vertices[0], &other_tri->vertices[1], &other_tri->vertices[2],
                                       &t, &intersection)) {
                intersection_count++;
            }
        }
        
        // Odd number of intersections means point is inside
        bool inside = (intersection_count % 2) == 1;
        
        if (inside) {
            has_inside_point = true;
        } else {
            has_outside_point = true;
        }
        
        // If we have both inside and outside points, triangle is intersecting
        if (has_inside_point && has_outside_point) {
            return CSG_TRIANGLE_INTERSECTING;
        }
    }
    
    return has_inside_point ? CSG_TRIANGLE_INSIDE : CSG_TRIANGLE_OUTSIDE;
}

// ============================================================================
// CSG OPERATIONS
// ============================================================================

static mesh_t* csg_build_result_mesh(const csg_mesh_data_t* mesh_a, const csg_mesh_data_t* mesh_b,
                                     const csg_config_t* config) {
    // Count triangles to keep
    uint32_t total_triangles = 0;
    
    for (uint32_t i = 0; i < mesh_a->triangle_count; i++) {
        if (mesh_a->triangles[i].kept) {
            total_triangles++;
        }
    }
    
    for (uint32_t i = 0; i < mesh_b->triangle_count; i++) {
        if (mesh_b->triangles[i].kept) {
            total_triangles++;
        }
    }
    
    if (total_triangles == 0) {
        return NULL; // Empty result
    }
    
    // Create result mesh
    mesh_t* result = mesh_create("csg_result");
    if (!result) return NULL;
    
    uint32_t vertex_count = total_triangles * 3;
    uint32_t index_count = total_triangles * 3;
    
    mesh_allocate_buffers(result, vertex_count, index_count);
    
    // Add triangles
    uint32_t vertex_index = 0;
    uint32_t index_index = 0;
    
    for (uint32_t i = 0; i < mesh_a->triangle_count; i++) {
        if (mesh_a->triangles[i].kept) {
            const csg_triangle_t* tri = &mesh_a->triangles[i];
            
            for (int j = 0; j < 3; j++) {
                result->vertices[vertex_index].position = tri->vertices[j];
                result->vertices[vertex_index].normal = tri->normal;
                result->vertices[vertex_index].uv = (Vec2){0.0f, 0.0f}; // Default UV
                result->vertices[vertex_index].tangent = (Vec4){1.0f, 0.0f, 0.0f, 1.0f}; // Default tangent
                
                result->indices[index_index++] = vertex_index;
                vertex_index++;
            }
        }
    }
    
    for (uint32_t i = 0; i < mesh_b->triangle_count; i++) {
        if (mesh_b->triangles[i].kept) {
            const csg_triangle_t* tri = &mesh_b->triangles[i];
            
            for (int j = 0; j < 3; j++) {
                result->vertices[vertex_index].position = tri->vertices[j];
                result->vertices[vertex_index].normal = tri->normal;
                result->vertices[vertex_index].uv = (Vec2){0.0f, 0.0f}; // Default UV
                result->vertices[vertex_index].tangent = (Vec4){1.0f, 0.0f, 0.0f, 1.0f}; // Default tangent
                
                result->indices[index_index++] = vertex_index;
                vertex_index++;
            }
        }
    }
    
    result->vertex_count = vertex_index;
    result->index_count = index_index;
    
    // Calculate bounds
    mesh_calculate_bounds(result);
    
    // Generate smooth normals if requested
    if (config && config->generate_smooth_normals) {
        mesh_calculate_normals(result);
    }
    
    return result;
}

static void csg_classify_triangles(const csg_mesh_data_t* mesh_a, const csg_mesh_data_t* mesh_b,
                                  csg_operation_e operation) {
    // Classify triangles of mesh A against mesh B
    for (uint32_t i = 0; i < mesh_a->triangle_count; i++) {
        csg_triangle_t* tri = &mesh_a->triangles[i];
        tri->classification = csg_classify_triangle_against_mesh(tri, mesh_b);
        tri->classified = true;
        
        // Determine if triangle should be kept based on operation
        switch (operation) {
            case CSG_OPERATION_UNION:
                tri->kept = (tri->classification != CSG_TRIANGLE_INSIDE);
                break;
            case CSG_OPERATION_INTERSECTION:
                tri->kept = (tri->classification == CSG_TRIANGLE_INSIDE);
                break;
            case CSG_OPERATION_DIFFERENCE:
                tri->kept = (tri->classification != CSG_TRIANGLE_INSIDE);
                break;
            case CSG_OPERATION_XOR:
                tri->kept = (tri->classification == CSG_TRIANGLE_OUTSIDE);
                break;
        }
    }
    
    // Classify triangles of mesh B against mesh A
    for (uint32_t i = 0; i < mesh_b->triangle_count; i++) {
        csg_triangle_t* tri = &mesh_b->triangles[i];
        tri->classification = csg_classify_triangle_against_mesh(tri, mesh_a);
        tri->classified = true;
        
        // Determine if triangle should be kept based on operation
        switch (operation) {
            case CSG_OPERATION_UNION:
                tri->kept = (tri->classification != CSG_TRIANGLE_INSIDE);
                break;
            case CSG_OPERATION_INTERSECTION:
                tri->kept = (tri->classification == CSG_TRIANGLE_INSIDE);
                break;
            case CSG_OPERATION_DIFFERENCE:
                tri->kept = (tri->classification == CSG_TRIANGLE_INSIDE); // Inverted for difference
                break;
            case CSG_OPERATION_XOR:
                tri->kept = (tri->classification == CSG_TRIANGLE_OUTSIDE);
                break;
        }
    }
}

// ============================================================================
// PUBLIC API IMPLEMENTATION
// ============================================================================

csg_config_t csg_get_default_config(void) {
    return CSG_CONFIG_DEFAULT;
}

bool csg_validate_mesh(const mesh_t* mesh) {
    if (!mesh || mesh->vertex_count < 3 || mesh->index_count < 3) {
        return false;
    }
    
    if (mesh->index_count % 3 != 0) {
        return false; // Not triangulated
    }
    
    // Check for degenerate triangles
    for (uint32_t i = 0; i < mesh->index_count; i += 3) {
        uint32_t i0 = mesh->indices[i];
        uint32_t i1 = mesh->indices[i + 1];
        uint32_t i2 = mesh->indices[i + 2];
        
        if (i0 >= mesh->vertex_count || i1 >= mesh->vertex_count || i2 >= mesh->vertex_count) {
            return false; // Invalid index
        }
        
        Vec3 v0 = mesh->vertices[i0].position;
        Vec3 v1 = mesh->vertices[i1].position;
        Vec3 v2 = mesh->vertices[i2].position;
        
        if (triangle_area(&v0, &v1, &v2) < CSG_MIN_TRIANGLE_AREA_DEFAULT) {
            return false; // Degenerate triangle
        }
    }
    
    return true;
}

bool csg_bounds_intersect(const mesh_bounds_t* bounds_a, const mesh_bounds_t* bounds_b) {
    if (!bounds_a || !bounds_b) return false;
    
    // AABB intersection test
    if (bounds_a->max.x < bounds_b->min.x || bounds_b->max.x < bounds_a->min.x) return false;
    if (bounds_a->max.y < bounds_b->min.y || bounds_b->max.y < bounds_a->min.y) return false;
    if (bounds_a->max.z < bounds_b->min.z || bounds_b->max.z < bounds_a->min.z) return false;
    
    return true;
}

bool csg_meshes_intersect(const mesh_t* mesh_a, const mesh_t* mesh_b) {
    if (!mesh_a || !mesh_b) return false;
    
    // Quick bounds test first
    if (!csg_bounds_intersect(&mesh_a->bounds, &mesh_b->bounds)) {
        return false;
    }
    
    // More detailed triangle intersection test could be added here
    // For now, assume they intersect if bounds intersect
    return true;
}

mesh_t* csg_perform_operation(const mesh_t* mesh_a, const mesh_t* mesh_b, 
                             const csg_config_t* config, csg_stats_t* stats) {
    if (!mesh_a || !mesh_b) return NULL;
    
    if (!csg_validate_mesh(mesh_a) || !csg_validate_mesh(mesh_b)) {
        printf("Error: Invalid mesh for CSG operation\n");
        return NULL;
    }
    
    const csg_config_t* cfg = config ? config : &CSG_CONFIG_DEFAULT;
    
    // Create CSG mesh data
    csg_mesh_data_t* csg_a = csg_create_mesh_data(mesh_a);
    csg_mesh_data_t* csg_b = csg_create_mesh_data(mesh_b);
    
    if (!csg_a || !csg_b) {
        csg_destroy_mesh_data(csg_a);
        csg_destroy_mesh_data(csg_b);
        return NULL;
    }
    
    // Classify triangles
    csg_classify_triangles(csg_a, csg_b, cfg->operation);
    
    // Build result mesh
    mesh_t* result = csg_build_result_mesh(csg_a, csg_b, cfg);
    
    // Fill statistics if requested
    if (stats) {
        memset(stats, 0, sizeof(csg_stats_t));
        stats->input_vertices_a = mesh_a->vertex_count;
        stats->input_vertices_b = mesh_b->vertex_count;
        stats->input_triangles_a = mesh_a->index_count / 3;
        stats->input_triangles_b = mesh_b->index_count / 3;
        
        if (result) {
            stats->output_vertices = result->vertex_count;
            stats->output_triangles = result->index_count / 3;
        }
        
        for (uint32_t i = 0; i < csg_a->triangle_count; i++) {
            switch (csg_a->triangles[i].classification) {
                case CSG_TRIANGLE_INSIDE: stats->triangles_classified_inside++; break;
                case CSG_TRIANGLE_OUTSIDE: stats->triangles_classified_outside++; break;
                case CSG_TRIANGLE_INTERSECTING: stats->triangles_classified_intersecting++; break;
            }
        }
    }
    
    // Cleanup
    csg_destroy_mesh_data(csg_a);
    csg_destroy_mesh_data(csg_b);
    
    return result;
}

mesh_t* csg_union(const mesh_t* mesh_a, const mesh_t* mesh_b) {
    csg_config_t config = CSG_CONFIG_DEFAULT;
    config.operation = CSG_OPERATION_UNION;
    return csg_perform_operation(mesh_a, mesh_b, &config, NULL);
}

mesh_t* csg_intersection(const mesh_t* mesh_a, const mesh_t* mesh_b) {
    csg_config_t config = CSG_CONFIG_DEFAULT;
    config.operation = CSG_OPERATION_INTERSECTION;
    return csg_perform_operation(mesh_a, mesh_b, &config, NULL);
}

mesh_t* csg_difference(const mesh_t* mesh_a, const mesh_t* mesh_b) {
    csg_config_t config = CSG_CONFIG_DEFAULT;
    config.operation = CSG_OPERATION_DIFFERENCE;
    return csg_perform_operation(mesh_a, mesh_b, &config, NULL);
}

mesh_t* csg_xor(const mesh_t* mesh_a, const mesh_t* mesh_b) {
    csg_config_t config = CSG_CONFIG_DEFAULT;
    config.operation = CSG_OPERATION_XOR;
    return csg_perform_operation(mesh_a, mesh_b, &config, NULL);
}

mesh_t* csg_union_advanced(const mesh_t* mesh_a, const mesh_t* mesh_b, 
                           const csg_config_t* config, csg_stats_t* stats) {
    csg_config_t cfg = *config;
    cfg.operation = CSG_OPERATION_UNION;
    return csg_perform_operation(mesh_a, mesh_b, &cfg, stats);
}

mesh_t* csg_intersection_advanced(const mesh_t* mesh_a, const mesh_t* mesh_b, 
                                 const csg_config_t* config, csg_stats_t* stats) {
    csg_config_t cfg = *config;
    cfg.operation = CSG_OPERATION_INTERSECTION;
    return csg_perform_operation(mesh_a, mesh_b, &cfg, stats);
}

mesh_t* csg_difference_advanced(const mesh_t* mesh_a, const mesh_t* mesh_b, 
                               const csg_config_t* config, csg_stats_t* stats) {
    csg_config_t cfg = *config;
    cfg.operation = CSG_OPERATION_DIFFERENCE;
    return csg_perform_operation(mesh_a, mesh_b, &cfg, stats);
}

mesh_t* csg_xor_advanced(const mesh_t* mesh_a, const mesh_t* mesh_b, 
                        const csg_config_t* config, csg_stats_t* stats) {
    csg_config_t cfg = *config;
    cfg.operation = CSG_OPERATION_XOR;
    return csg_perform_operation(mesh_a, mesh_b, &cfg, stats);
}

bool csg_point_inside_mesh(const Vec3* point, const mesh_t* mesh) {
    if (!point || !mesh) return false;
    
    int intersection_count = 0;
    
    // Cast ray in positive Y direction
    Vec3 ray_dir = {0.0f, 1.0f, 0.0f};
    Vec3 ray_origin = *point;
    ray_origin.y += 0.1f; // Small offset to avoid self-intersection
    
    for (uint32_t i = 0; i < mesh->index_count; i += 3) {
        uint32_t i0 = mesh->indices[i];
        uint32_t i1 = mesh->indices[i + 1];
        uint32_t i2 = mesh->indices[i + 2];
        
        if (i0 >= mesh->vertex_count || i1 >= mesh->vertex_count || i2 >= mesh->vertex_count) {
            continue;
        }
        
        Vec3 v0 = mesh->vertices[i0].position;
        Vec3 v1 = mesh->vertices[i1].position;
        Vec3 v2 = mesh->vertices[i2].position;
        
        float t;
        Vec3 intersection;
        if (ray_triangle_intersection(&ray_origin, &ray_dir, &v0, &v1, &v2, &t, &intersection)) {
            intersection_count++;
        }
    }
    
    return (intersection_count % 2) == 1;
}

csg_triangle_classification_e csg_classify_triangle(const mesh_t* mesh, const mesh_t* other_mesh,
                                                   uint32_t triangle_index) {
    if (!mesh || !other_mesh || triangle_index * 3 >= mesh->index_count) {
        return CSG_TRIANGLE_OUTSIDE;
    }
    
    // Create temporary CSG mesh data for classification
    csg_mesh_data_t* csg_mesh = csg_create_mesh_data(mesh);
    csg_mesh_data_t* csg_other = csg_create_mesh_data(other_mesh);
    
    if (!csg_mesh || !csg_other) {
        csg_destroy_mesh_data(csg_mesh);
        csg_destroy_mesh_data(csg_other);
        return CSG_TRIANGLE_OUTSIDE;
    }
    
    csg_triangle_classification_e result = CSG_TRIANGLE_OUTSIDE;
    
    if (triangle_index < csg_mesh->triangle_count) {
        result = csg_classify_triangle_against_mesh(&csg_mesh->triangles[triangle_index], csg_other);
    }
    
    csg_destroy_mesh_data(csg_mesh);
    csg_destroy_mesh_data(csg_other);
    
    return result;
}

void csg_print_stats(const csg_stats_t* stats) {
    if (!stats) return;
    
    printf("CSG Operation Statistics:\n");
    printf("  Input Mesh A: %u vertices, %u triangles\n", 
           stats->input_vertices_a, stats->input_triangles_a);
    printf("  Input Mesh B: %u vertices, %u triangles\n", 
           stats->input_vertices_b, stats->input_triangles_b);
    printf("  Output Mesh: %u vertices, %u triangles\n", 
           stats->output_vertices, stats->output_triangles);
    printf("  Triangle Classification:\n");
    printf("    Inside: %u\n", stats->triangles_classified_inside);
    printf("    Outside: %u\n", stats->triangles_classified_outside);
    printf("    Intersecting: %u\n", stats->triangles_classified_intersecting);
    printf("  Operation Time: %.2f ms\n", stats->operation_time_ms);
}

bool csg_validate_result(const mesh_t* result, const mesh_t* mesh_a, const mesh_t* mesh_b) {
    if (!result) return false;
    
    // Basic validation
    if (!csg_validate_mesh(result)) {
        return false;
    }
    
    // Result should have vertices from both input meshes
    if (result->vertex_count == 0) {
        return false;
    }
    
    // Additional validation could be added here
    return true;
}
