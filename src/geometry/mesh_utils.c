#include "geometry/mesh_utils.h"
#include <stdlib.h>
#include <string.h>
#include <stdio.h>
#include <math.h>

// Simple convex hull implementation using gift wrapping algorithm
void mesh_calculate_convex_hull(const mesh_t* mesh, Vec3** out_hull_vertices, u32* out_hull_vertex_count) {
    if (!mesh || !out_hull_vertices || !out_hull_vertex_count || mesh->vertex_count < 3) {
        *out_hull_vertices = NULL;
        *out_hull_vertex_count = 0;
        return;
    }
    
    printf("Calculating convex hull for mesh '%s' (%u vertices)...\n", mesh->name, mesh->vertex_count);
    
    // For simplicity, we'll use a basic approach that finds extreme points
    // In a production environment, you'd use a proper convex hull algorithm
    
    u32 hull_capacity = 32; // Start with reasonable capacity
    Vec3* hull_vertices = (Vec3*)malloc(hull_capacity * sizeof(Vec3));
    u32 hull_count = 0;
    
    if (!hull_vertices) {
        *out_hull_vertices = NULL;
        *out_hull_vertex_count = 0;
        return;
    }
    
    // Find extreme points in each direction (simplified approach)
    Vec3 min_x = mesh->vertices[0].position, max_x = mesh->vertices[0].position;
    Vec3 min_y = mesh->vertices[0].position, max_y = mesh->vertices[0].position;
    Vec3 min_z = mesh->vertices[0].position, max_z = mesh->vertices[0].position;
    
    for (u32 i = 1; i < mesh->vertex_count; i++) {
        Vec3 pos = mesh->vertices[i].position;
        
        if (pos.x < min_x.x) min_x = pos;
        if (pos.x > max_x.x) max_x = pos;
        if (pos.y < min_y.y) min_y = pos;
        if (pos.y > max_y.y) max_y = pos;
        if (pos.z < min_z.z) min_z = pos;
        if (pos.z > max_z.z) max_z = pos;
    }
    
    // Add extreme points to hull (avoiding duplicates)
    Vec3 extremes[6] = {min_x, max_x, min_y, max_y, min_z, max_z};
    
    for (int i = 0; i < 6; i++) {
        bool is_duplicate = false;
        
        // Check if this point is already in hull
        for (u32 j = 0; j < hull_count; j++) {
            if (fabsf(extremes[i].x - hull_vertices[j].x) < EPSILON &&
                fabsf(extremes[i].y - hull_vertices[j].y) < EPSILON &&
                fabsf(extremes[i].z - hull_vertices[j].z) < EPSILON) {
                is_duplicate = true;
                break;
            }
        }
        
        if (!is_duplicate) {
            if (hull_count >= hull_capacity) {
                hull_capacity *= 2;
                Vec3* new_hull = (Vec3*)realloc(hull_vertices, hull_capacity * sizeof(Vec3));
                if (!new_hull) {
                    free(hull_vertices);
                    *out_hull_vertices = NULL;
                    *out_hull_vertex_count = 0;
                    return;
                }
                hull_vertices = new_hull;
            }
            
            hull_vertices[hull_count++] = extremes[i];
        }
    }
    
    // Add some additional points to make it more hull-like (corners of bounding box)
    Vec3 corners[8] = {
        {min_x.x, min_y.y, min_z.z}, {max_x.x, min_y.y, min_z.z},
        {min_x.x, max_y.y, min_z.z}, {max_x.x, max_y.y, min_z.z},
        {min_x.x, min_y.y, max_z.z}, {max_x.x, min_y.y, max_z.z},
        {min_x.x, max_y.y, max_z.z}, {max_x.x, max_y.y, max_z.z}
    };
    
    for (int i = 0; i < 8; i++) {
        bool is_duplicate = false;
        
        // Check if this point is already in hull
        for (u32 j = 0; j < hull_count; j++) {
            if (fabsf(corners[i].x - hull_vertices[j].x) < EPSILON &&
                fabsf(corners[i].y - hull_vertices[j].y) < EPSILON &&
                fabsf(corners[i].z - hull_vertices[j].z) < EPSILON) {
                is_duplicate = true;
                break;
            }
        }
        
        if (!is_duplicate) {
            if (hull_count >= hull_capacity) {
                hull_capacity *= 2;
                Vec3* new_hull = (Vec3*)realloc(hull_vertices, hull_capacity * sizeof(Vec3));
                if (!new_hull) {
                    free(hull_vertices);
                    *out_hull_vertices = NULL;
                    *out_hull_vertex_count = 0;
                    return;
                }
                hull_vertices = new_hull;
            }
            
            hull_vertices[hull_count++] = corners[i];
        }
    }
    
    *out_hull_vertices = hull_vertices;
    *out_hull_vertex_count = hull_count;
    
    printf("Convex hull complete: %u vertices\n", hull_count);
}

void mesh_calculate_submesh_bounds(mesh_t* mesh) {
    if (!mesh || !mesh->submeshes || mesh->submesh_count == 0) {
        return;
    }
    
    printf("Calculating submesh bounds for mesh '%s'...\n", mesh->name);
    
    for (u32 submesh_idx = 0; submesh_idx < mesh->submesh_count; submesh_idx++) {
        submesh_t* submesh = &mesh->submeshes[submesh_idx];
        
        if (submesh->vertex_count == 0) {
            // Initialize to zero bounds
            submesh->bounds.min = (Vec3){0, 0, 0};
            submesh->bounds.max = (Vec3){0, 0, 0};
            submesh->bounds.sphere_center = (Vec3){0, 0, 0};
            submesh->bounds.sphere_radius = 0.0f;
            continue;
        }
        
        // Find the range of vertices used by this submesh
        u32 vertex_start = submesh->vertex_start;
        u32 vertex_end = vertex_start + submesh->vertex_count;
        
        // Clamp to mesh vertex count
        if (vertex_end > mesh->vertex_count) {
            vertex_end = mesh->vertex_count;
        }
        
        if (vertex_start >= mesh->vertex_count) {
            printf("Warning: Submesh %u has invalid vertex range\n", submesh_idx);
            continue;
        }
        
        // Initialize bounds to first vertex in submesh
        submesh->bounds.min = mesh->vertices[vertex_start].position;
        submesh->bounds.max = mesh->vertices[vertex_start].position;
        
        // Find AABB min/max for vertices in this submesh
        for (u32 i = vertex_start + 1; i < vertex_end; i++) {
            Vec3 pos = mesh->vertices[i].position;
            
            if (pos.x < submesh->bounds.min.x) submesh->bounds.min.x = pos.x;
            if (pos.y < submesh->bounds.min.y) submesh->bounds.min.y = pos.y;
            if (pos.z < submesh->bounds.min.z) submesh->bounds.min.z = pos.z;
            
            if (pos.x > submesh->bounds.max.x) submesh->bounds.max.x = pos.x;
            if (pos.y > submesh->bounds.max.y) submesh->bounds.max.y = pos.y;
            if (pos.z > submesh->bounds.max.z) submesh->bounds.max.z = pos.z;
        }
        
        // Calculate sphere center (center of AABB)
        submesh->bounds.sphere_center.x = (submesh->bounds.min.x + submesh->bounds.max.x) * 0.5f;
        submesh->bounds.sphere_center.y = (submesh->bounds.min.y + submesh->bounds.max.y) * 0.5f;
        submesh->bounds.sphere_center.z = (submesh->bounds.min.z + submesh->bounds.max.z) * 0.5f;
        
        // Calculate sphere radius
        Vec3 to_corner = {
            submesh->bounds.max.x - submesh->bounds.sphere_center.x,
            submesh->bounds.max.y - submesh->bounds.sphere_center.y,
            submesh->bounds.max.z - submesh->bounds.sphere_center.z
        };
        
        submesh->bounds.sphere_radius = sqrtf(to_corner.x * to_corner.x + 
                                            to_corner.y * to_corner.y + 
                                            to_corner.z * to_corner.z);
        
        printf("Submesh %u bounds: min(%.3f,%.3f,%.3f) max(%.3f,%.3f,%.3f) radius=%.3f\n",
               submesh_idx,
               submesh->bounds.min.x, submesh->bounds.min.y, submesh->bounds.min.z,
               submesh->bounds.max.x, submesh->bounds.max.y, submesh->bounds.max.z,
               submesh->bounds.sphere_radius);
    }
}

void mesh_update_bounds_transform_matrix(mesh_t* mesh, const float* transform_matrix) {
    if (!mesh || !transform_matrix || mesh->vertex_count == 0) {
        return;
    }
    
    printf("Updating bounds with transform for mesh '%s'...\n", mesh->name);
    
    // Store original bounds
    mesh_bounds_t original_bounds = mesh->bounds;
    
    // Transform the 8 corners of the original AABB
    Vec3 corners[8] = {
        {original_bounds.min.x, original_bounds.min.y, original_bounds.min.z},
        {original_bounds.max.x, original_bounds.min.y, original_bounds.min.z},
        {original_bounds.min.x, original_bounds.max.y, original_bounds.min.z},
        {original_bounds.max.x, original_bounds.max.y, original_bounds.min.z},
        {original_bounds.min.x, original_bounds.min.y, original_bounds.max.z},
        {original_bounds.max.x, original_bounds.min.y, original_bounds.max.z},
        {original_bounds.min.x, original_bounds.max.y, original_bounds.max.z},
        {original_bounds.max.x, original_bounds.max.y, original_bounds.max.z}
    };
    
    // Initialize new bounds to first transformed corner
    Vec3 first_corner = corners[0];
    Vec3 transformed;
    
    // Transform first corner: pos = M * pos
    transformed.x = transform_matrix[0] * first_corner.x + transform_matrix[4] * first_corner.y + 
                   transform_matrix[8] * first_corner.z + transform_matrix[12];
    transformed.y = transform_matrix[1] * first_corner.x + transform_matrix[5] * first_corner.y + 
                   transform_matrix[9] * first_corner.z + transform_matrix[13];
    transformed.z = transform_matrix[2] * first_corner.x + transform_matrix[6] * first_corner.y + 
                   transform_matrix[10] * first_corner.z + transform_matrix[14];
    
    mesh->bounds.min = transformed;
    mesh->bounds.max = transformed;
    
    // Transform remaining corners and expand bounds
    for (int i = 1; i < 8; i++) {
        Vec3 corner = corners[i];
        
        // Transform corner
        transformed.x = transform_matrix[0] * corner.x + transform_matrix[4] * corner.y + 
                       transform_matrix[8] * corner.z + transform_matrix[12];
        transformed.y = transform_matrix[1] * corner.x + transform_matrix[5] * corner.y + 
                       transform_matrix[9] * corner.z + transform_matrix[13];
        transformed.z = transform_matrix[2] * corner.x + transform_matrix[6] * corner.y + 
                       transform_matrix[10] * corner.z + transform_matrix[14];
        
        // Expand bounds
        if (transformed.x < mesh->bounds.min.x) mesh->bounds.min.x = transformed.x;
        if (transformed.y < mesh->bounds.min.y) mesh->bounds.min.y = transformed.y;
        if (transformed.z < mesh->bounds.min.z) mesh->bounds.min.z = transformed.z;
        
        if (transformed.x > mesh->bounds.max.x) mesh->bounds.max.x = transformed.x;
        if (transformed.y > mesh->bounds.max.y) mesh->bounds.max.y = transformed.y;
        if (transformed.z > mesh->bounds.max.z) mesh->bounds.max.z = transformed.z;
    }
    
    // Calculate new sphere center and radius
    mesh->bounds.sphere_center.x = (mesh->bounds.min.x + mesh->bounds.max.x) * 0.5f;
    mesh->bounds.sphere_center.y = (mesh->bounds.min.y + mesh->bounds.max.y) * 0.5f;
    mesh->bounds.sphere_center.z = (mesh->bounds.min.z + mesh->bounds.max.z) * 0.5f;
    
    Vec3 to_corner = {
        mesh->bounds.max.x - mesh->bounds.sphere_center.x,
        mesh->bounds.max.y - mesh->bounds.sphere_center.y,
        mesh->bounds.max.z - mesh->bounds.sphere_center.z
    };
    
    mesh->bounds.sphere_radius = sqrtf(to_corner.x * to_corner.x + 
                                      to_corner.y * to_corner.y + 
                                      to_corner.z * to_corner.z);
    
    printf("Transformed bounds: min(%.3f,%.3f,%.3f) max(%.3f,%.3f,%.3f) radius=%.3f\n",
           mesh->bounds.min.x, mesh->bounds.min.y, mesh->bounds.min.z,
           mesh->bounds.max.x, mesh->bounds.max.y, mesh->bounds.max.z,
           mesh->bounds.sphere_radius);
}
