#include "mesh.h"
#include <core/memory.h>
#include <core/logger.h>
#include <math.h>
#include <string.h>
#include <float.h>

// ----------------------------------------------------------------------------
// Convex Hull Approximation
// Using Gift Wrapping (Jarvis March) algorithm simplified for 3D
// ----------------------------------------------------------------------------

typedef struct hull_face {
    u32 indices[3];
    Vec3 normal;
} hull_face_t;

static Vec3 compute_face_normal(Vec3 a, Vec3 b, Vec3 c) {
    Vec3 edge1 = vec3_sub(b, a);
    Vec3 edge2 = vec3_sub(c, a);
    return vec3_normalize(vec3_cross(edge1, edge2));
}

static f32 point_plane_distance(Vec3 point, Vec3 plane_point, Vec3 plane_normal) {
    Vec3 diff = vec3_sub(point, plane_point);
    return vec3_dot(diff, plane_normal);
}

void mesh_calculate_convex_hull(const mesh_t* mesh, Vec3** out_hull_vertices, u32* out_hull_vertex_count) {
    if (!mesh || !out_hull_vertices || !out_hull_vertex_count) return;
    if (mesh->vertex_count < 4) {
        *out_hull_vertices = NULL;
        *out_hull_vertex_count = 0;
        return;
    }

    // For large meshes, this is expensive - simplified approximation
    // Find extreme points in each direction
    u32 extreme_count = 6;
    Vec3* extremes = (Vec3*)MALLOC_GEOMETRY(sizeof(Vec3) * extreme_count);
    
    Vec3 min_x = mesh->vertices[0].position, max_x = min_x;
    Vec3 min_y = min_x, max_y = min_x;
    Vec3 min_z = min_x, max_z = min_x;
    
    for (u32 i = 1; i < mesh->vertex_count; ++i) {
        Vec3 p = mesh->vertices[i].position;
        
        if (p.x < min_x.x) min_x = p;
        if (p.x > max_x.x) max_x = p;
        if (p.y < min_y.y) min_y = p;
        if (p.y > max_y.y) max_y = p;
        if (p.z < min_z.z) min_z = p;
        if (p.z > max_z.z) max_z = p;
    }
    
    extremes[0] = min_x;
    extremes[1] = max_x;
    extremes[2] = min_y;
    extremes[3] = max_y;
    extremes[4] = min_z;
    extremes[5] = max_z;
    
    // Remove duplicates
    u32 unique_count = 0;
    for (u32 i = 0; i < extreme_count; ++i) {
        bool is_unique = true;
        for (u32 j = 0; j < unique_count; ++j) {
            f32 dist = vec3_distance(extremes[i], extremes[j]);
            if (dist < 0.001f) {
                is_unique = false;
                break;
            }
        }
        if (is_unique) {
            extremes[unique_count++] = extremes[i];
        }
    }
    
    *out_hull_vertices = extremes;
    *out_hull_vertex_count = unique_count;
    
    LOG_INFO("Convex hull approximation: %u extreme vertices", unique_count);
}

// ----------------------------------------------------------------------------
// Per-Submesh Bounds Calculation
// ----------------------------------------------------------------------------

void mesh_calculate_submesh_bounds(mesh_t* mesh) {
    if (!mesh || !mesh->submeshes) return;
    
    for (u32 s = 0; s < mesh->submesh_count; ++s) {
        submesh_t* submesh = &mesh->submeshes[s];
        
        if (submesh->vertex_count == 0) {
            memset(&submesh->bounds, 0, sizeof(mesh_bounds_t));
            continue;
        }
        
        Vec3 min = {FLT_MAX, FLT_MAX, FLT_MAX};
        Vec3 max = {-FLT_MAX, -FLT_MAX, -FLT_MAX};
        
        u32 vert_end = submesh->vertex_start + submesh->vertex_count;
        for (u32 i = submesh->vertex_start; i < vert_end && i < mesh->vertex_count; ++i) {
            Vec3 p = mesh->vertices[i].position;
            if (p.x < min.x) min.x = p.x;
            if (p.y < min.y) min.y = p.y;
            if (p.z < min.z) min.z = p.z;
            if (p.x > max.x) max.x = p.x;
            if (p.y > max.y) max.y = p.y;
            if (p.z > max.z) max.z = p.z;
        }
        
        submesh->bounds.min = min;
        submesh->bounds.max = max;
        
        Vec3 center = vec3_mul(vec3_add(min, max), 0.5f);
        submesh->bounds.sphere_center = center;
        
        // Calculate sphere radius
        f32 max_dist_sq = 0.0f;
        for (u32 i = submesh->vertex_start; i < vert_end && i < mesh->vertex_count; ++i) {
            f32 dist_sq = vec3_distance_sq(mesh->vertices[i].position, center);
            if (dist_sq > max_dist_sq) {
                max_dist_sq = dist_sq;
            }
        }
        submesh->bounds.sphere_radius = sqrtf(max_dist_sq);
    }
    
    LOG_INFO("Calculated bounds for %u submeshes of mesh '%s'", 
        mesh->submesh_count, mesh->name);
}

// ----------------------------------------------------------------------------
// Matrix-based Bounds Transform
// ----------------------------------------------------------------------------

typedef struct mat4 {
    f32 m[16]; // Column-major 4x4 matrix
} mat4_t;

static Vec3 mat4_transform_point(const mat4_t* mat, Vec3 p) {
    Vec3 result;
    result.x = mat->m[0] * p.x + mat->m[4] * p.y + mat->m[8] * p.z + mat->m[12];
    result.y = mat->m[1] * p.x + mat->m[5] * p.y + mat->m[9] * p.z + mat->m[13];
    result.z = mat->m[2] * p.x + mat->m[6] * p.y + mat->m[10] * p.z + mat->m[14];
    return result;
}

void mesh_update_bounds_transform_matrix(mesh_t* mesh, const float* transform_matrix) {
    if (!mesh || !transform_matrix) return;
    
    const mat4_t* mat = (const mat4_t*)transform_matrix;
    
    // Transform all 8 corners of the AABB
    Vec3 corners[8] = {
        {mesh->bounds.min.x, mesh->bounds.min.y, mesh->bounds.min.z},
        {mesh->bounds.max.x, mesh->bounds.min.y, mesh->bounds.min.z},
        {mesh->bounds.min.x, mesh->bounds.max.y, mesh->bounds.min.z},
        {mesh->bounds.max.x, mesh->bounds.max.y, mesh->bounds.min.z},
        {mesh->bounds.min.x, mesh->bounds.min.y, mesh->bounds.max.z},
        {mesh->bounds.max.x, mesh->bounds.min.y, mesh->bounds.max.z},
        {mesh->bounds.min.x, mesh->bounds.max.y, mesh->bounds.max.z},
        {mesh->bounds.max.x, mesh->bounds.max.y, mesh->bounds.max.z},
    };
    
    Vec3 new_min = {FLT_MAX, FLT_MAX, FLT_MAX};
    Vec3 new_max = {-FLT_MAX, -FLT_MAX, -FLT_MAX};
    
    for (u32 i = 0; i < 8; ++i) {
        Vec3 transformed = mat4_transform_point(mat, corners[i]);
        
        if (transformed.x < new_min.x) new_min.x = transformed.x;
        if (transformed.y < new_min.y) new_min.y = transformed.y;
        if (transformed.z < new_min.z) new_min.z = transformed.z;
        if (transformed.x > new_max.x) new_max.x = transformed.x;
        if (transformed.y > new_max.y) new_max.y = transformed.y;
        if (transformed.z > new_max.z) new_max.z = transformed.z;
    }
    
    mesh->bounds.min = new_min;
    mesh->bounds.max = new_max;
    mesh->bounds.sphere_center = vec3_mul(vec3_add(new_min, new_max), 0.5f);
    
    // Recalculate sphere radius
    f32 max_dist = 0.0f;
    for (u32 i = 0; i < 8; ++i) {
        Vec3 transformed = mat4_transform_point(mat, corners[i]);
        f32 dist = vec3_distance(transformed, mesh->bounds.sphere_center);
        if (dist > max_dist) max_dist = dist;
    }
    mesh->bounds.sphere_radius = max_dist;
    
    mesh->bounds_dirty = false;
}
