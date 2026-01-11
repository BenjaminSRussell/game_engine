#include "geometry/mesh_deform.h"
#include <stdlib.h>
#include <string.h>
#include <stdio.h>

void mesh_add_blend_shape(mesh_t* mesh, const blend_shape_t* shape) {
    if (!mesh || !shape) {
        return;
    }
    
    if (mesh->blend_shape_count >= MAX_BLEND_SHAPES) {
        printf("Warning: Maximum blend shapes reached for mesh '%s'\n", mesh->name);
        return;
    }
    
    // Check if mesh has base vertices backup
    if (!mesh->base_vertices && mesh->vertices) {
        // Create backup of original vertices
        mesh->base_vertices = (vertex_t*)malloc(mesh->vertex_capacity * sizeof(vertex_t));
        if (mesh->base_vertices) {
            memcpy(mesh->base_vertices, mesh->vertices, mesh->vertex_count * sizeof(vertex_t));
        }
    }
    
    // Reallocate blend shapes array
    blend_shape_t* new_shapes = (blend_shape_t*)realloc(mesh->blend_shapes, 
                                                      (mesh->blend_shape_count + 1) * sizeof(blend_shape_t));
    if (!new_shapes) {
        printf("Error: Failed to allocate memory for blend shape\n");
        return;
    }
    
    mesh->blend_shapes = new_shapes;
    
    // Copy blend shape data
    blend_shape_t* new_shape = &mesh->blend_shapes[mesh->blend_shape_count];
    strncpy(new_shape->name, shape->name, MAX_BLEND_SHAPE_NAME - 1);
    new_shape->name[MAX_BLEND_SHAPE_NAME - 1] = '\0';
    new_shape->vertex_count = shape->vertex_count;
    new_shape->weight = 0.0f;
    
    // Allocate and copy delta data
    if (shape->delta_positions && shape->vertex_count > 0) {
        new_shape->delta_positions = (Vec3*)malloc(shape->vertex_count * sizeof(Vec3));
        if (new_shape->delta_positions) {
            memcpy(new_shape->delta_positions, shape->delta_positions, 
                   shape->vertex_count * sizeof(Vec3));
        }
    } else {
        new_shape->delta_positions = NULL;
    }
    
    if (shape->delta_normals && shape->vertex_count > 0) {
        new_shape->delta_normals = (Vec3*)malloc(shape->vertex_count * sizeof(Vec3));
        if (new_shape->delta_normals) {
            memcpy(new_shape->delta_normals, shape->delta_normals, 
                   shape->vertex_count * sizeof(Vec3));
        }
    } else {
        new_shape->delta_normals = NULL;
    }
    
    if (shape->delta_tangents && shape->vertex_count > 0) {
        new_shape->delta_tangents = (Vec3*)malloc(shape->vertex_count * sizeof(Vec3));
        if (new_shape->delta_tangents) {
            memcpy(new_shape->delta_tangents, shape->delta_tangents, 
                   shape->vertex_count * sizeof(Vec3));
        }
    } else {
        new_shape->delta_tangents = NULL;
    }
    
    mesh->blend_shape_count++;
    printf("Added blend shape '%s' to mesh '%s'\n", shape->name, mesh->name);
}

void mesh_remove_blend_shape(mesh_t* mesh, u32 index) {
    if (!mesh || index >= mesh->blend_shape_count) {
        return;
    }
    
    blend_shape_t* shape = &mesh->blend_shapes[index];
    
    // Free delta data
    free(shape->delta_positions);
    free(shape->delta_normals);
    free(shape->delta_tangents);
    
    // Shift remaining blend shapes
    for (u32 i = index; i < mesh->blend_shape_count - 1; i++) {
        mesh->blend_shapes[i] = mesh->blend_shapes[i + 1];
    }
    
    mesh->blend_shape_count--;
    
    // Reallocate array if needed
    if (mesh->blend_shape_count > 0) {
        blend_shape_t* new_shapes = (blend_shape_t*)realloc(mesh->blend_shapes, 
                                                          mesh->blend_shape_count * sizeof(blend_shape_t));
        if (new_shapes) {
            mesh->blend_shapes = new_shapes;
        }
    } else {
        free(mesh->blend_shapes);
        mesh->blend_shapes = NULL;
    }
    
    printf("Removed blend shape %u from mesh '%s'\n", index, mesh->name);
}

void mesh_set_blend_weight(mesh_t* mesh, u32 blend_index, f32 weight) {
    if (!mesh || blend_index >= mesh->blend_shape_count) {
        return;
    }
    
    // Clamp weight to [0, 1]
    weight = weight < 0.0f ? 0.0f : (weight > 1.0f ? 1.0f : weight);
    mesh->blend_shapes[blend_index].weight = weight;
}

void mesh_evaluate_blend_shapes(mesh_t* mesh, vertex_t* output_vertices) {
    if (!mesh || !output_vertices || !mesh->base_vertices) {
        return;
    }
    
    if (mesh->blend_shape_count == 0) {
        // No blend shapes, copy base vertices
        memcpy(output_vertices, mesh->base_vertices, mesh->vertex_count * sizeof(vertex_t));
        return;
    }
    
    // Start with base vertices
    memcpy(output_vertices, mesh->base_vertices, mesh->vertex_count * sizeof(vertex_t));
    
    // Apply each blend shape
    for (u32 shape_idx = 0; shape_idx < mesh->blend_shape_count; shape_idx++) {
        blend_shape_t* shape = &mesh->blend_shapes[shape_idx];
        
        if (shape->weight <= 0.0f || !shape->delta_positions) {
            continue; // Skip inactive shapes
        }
        
        u32 vertices_to_process = MIN(shape->vertex_count, mesh->vertex_count);
        
        for (u32 vertex_idx = 0; vertex_idx < vertices_to_process; vertex_idx++) {
            // Apply position delta
            output_vertices[vertex_idx].position.x += shape->delta_positions[vertex_idx].x * shape->weight;
            output_vertices[vertex_idx].position.y += shape->delta_positions[vertex_idx].y * shape->weight;
            output_vertices[vertex_idx].position.z += shape->delta_positions[vertex_idx].z * shape->weight;
            
            // Apply normal delta if available
            if (shape->delta_normals) {
                output_vertices[vertex_idx].normal.x += shape->delta_normals[vertex_idx].x * shape->weight;
                output_vertices[vertex_idx].normal.y += shape->delta_normals[vertex_idx].y * shape->weight;
                output_vertices[vertex_idx].normal.z += shape->delta_normals[vertex_idx].z * shape->weight;
                
                // Renormalize normal
                Vec3 normal = output_vertices[vertex_idx].normal;
                f32 length = sqrtf(normal.x * normal.x + normal.y * normal.y + normal.z * normal.z);
                if (length > EPSILON) {
                    output_vertices[vertex_idx].normal.x /= length;
                    output_vertices[vertex_idx].normal.y /= length;
                    output_vertices[vertex_idx].normal.z /= length;
                }
            }
            
            // Apply tangent delta if available
            if (shape->delta_tangents) {
                output_vertices[vertex_idx].tangent.x += shape->delta_tangents[vertex_idx].x * shape->weight;
                output_vertices[vertex_idx].tangent.y += shape->delta_tangents[vertex_idx].y * shape->weight;
                output_vertices[vertex_idx].tangent.z += shape->delta_tangents[vertex_idx].z * shape->weight;
                
                // Renormalize tangent
                Vec3 tangent = {output_vertices[vertex_idx].tangent.x, 
                              output_vertices[vertex_idx].tangent.y, 
                              output_vertices[vertex_idx].tangent.z};
                f32 length = sqrtf(tangent.x * tangent.x + tangent.y * tangent.y + tangent.z * tangent.z);
                if (length > EPSILON) {
                    output_vertices[vertex_idx].tangent.x /= length;
                    output_vertices[vertex_idx].tangent.y /= length;
                    output_vertices[vertex_idx].tangent.z /= length;
                }
            }
        }
    }
}

bool mesh_validate_bone_weights(const vertex_skinned_t* vertices, u32 count) {
    if (!vertices || count == 0) {
        return true; // Empty is valid
    }
    
    for (u32 i = 0; i < count; i++) {
        const vertex_skinned_t* vertex = &vertices[i];
        
        f32 total_weight = vertex->weights.x + vertex->weights.y + 
                          vertex->weights.z + vertex->weights.w;
        
        // Check if weights sum to approximately 1.0
        if (fabsf(total_weight - 1.0f) > 0.001f) {
            printf("Validation Error: Vertex %u bone weights sum to %.6f (expected 1.0)\n", 
                   i, total_weight);
            return false;
        }
        
        // Check bone indices are within reasonable range
        for (u32 j = 0; j < 4; j++) {
            if (vertex->indices[j] >= MAX_BONES) {
                printf("Validation Error: Vertex %u bone index %u out of range (%u >= %u)\n", 
                       i, j, vertex->indices[j], MAX_BONES);
                return false;
            }
        }
        
        // Check weights are non-negative
        if (vertex->weights.x < 0.0f || vertex->weights.y < 0.0f || 
            vertex->weights.z < 0.0f || vertex->weights.w < 0.0f) {
            printf("Validation Error: Vertex %u has negative bone weights\n", i);
            return false;
        }
    }
    
    return true;
}
