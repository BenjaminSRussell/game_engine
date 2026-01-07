#include "geometry/mesh_deform.h"
#include "geometry/mesh.h"
#include <core/memory.h>
#include <core/logger.h>
#include <string.h>
#include <include/math/math.h>

// ----------------------------------------------------------------------------
// Blend Shape Management
// ----------------------------------------------------------------------------

void mesh_add_blend_shape(mesh_t* mesh, const blend_shape_t* shape) {
    if (!mesh || !shape) return;
    
    // Allocate morph targets if not present
    if (!mesh->morph_targets) {
        mesh->morph_targets = (morph_target_t*)MALLOC_GEOMETRY(sizeof(morph_target_t));
        memset(mesh->morph_targets, 0, sizeof(morph_target_t));
    }
    
    if (mesh->morph_targets->shape_count >= MESH_MAX_BLEND_SHAPES) {
        LOG_ERROR("Cannot add blend shape: Maximum limit reached (%d)", MESH_MAX_BLEND_SHAPES);
        return;
    }
    
    // Validate vertex count matches
    if (shape->vertex_count != mesh->vertex_count) {
        LOG_ERROR("Blend shape vertex count (%u) doesn't match mesh (%u)", 
            shape->vertex_count, mesh->vertex_count);
        return;
    }
    
    // Deep copy the blend shape
    u32 index = mesh->morph_targets->shape_count;
    blend_shape_t* target = &mesh->morph_targets->shapes[index];
    
    strncpy(target->name, shape->name, 63);
    target->name[63] = '\0';
    target->vertex_count = shape->vertex_count;
    target->weight = shape->weight;
    
    // Copy position deltas
    target->position_deltas = (Vec3*)MALLOC_GEOMETRY(sizeof(Vec3) * shape->vertex_count);
    memcpy(target->position_deltas, shape->position_deltas, sizeof(Vec3) * shape->vertex_count);
    
    // Copy normal deltas if present
    if (shape->normal_deltas) {
        target->normal_deltas = (Vec3*)MALLOC_GEOMETRY(sizeof(Vec3) * shape->vertex_count);
        memcpy(target->normal_deltas, shape->normal_deltas, sizeof(Vec3) * shape->vertex_count);
    } else {
        target->normal_deltas = NULL;
    }
    
    mesh->morph_targets->shape_count++;
    mesh->flags |= MESH_FLAG_HAS_BLEND_SHAPES;
    
    LOG_INFO("Added blend shape '%s' to mesh '%s' (index %u)", 
        shape->name, mesh->name, index);
}

void mesh_remove_blend_shape(mesh_t* mesh, u32 index) {
    if (!mesh || !mesh->morph_targets) return;
    if (index >= mesh->morph_targets->shape_count) return;
    
    blend_shape_t* shape = &mesh->morph_targets->shapes[index];
    
    if (shape->position_deltas) {
        FREE(shape->position_deltas);
    }
    if (shape->normal_deltas) {
        FREE(shape->normal_deltas);
    }
    
    // Shift remaining shapes down
    for (u32 i = index; i < mesh->morph_targets->shape_count - 1; ++i) {
        mesh->morph_targets->shapes[i] = mesh->morph_targets->shapes[i + 1];
    }
    
    mesh->morph_targets->shape_count--;
    
    if (mesh->morph_targets->shape_count == 0) {
        mesh->flags &= ~MESH_FLAG_HAS_BLEND_SHAPES;
    }
}

void mesh_set_blend_weight(mesh_t* mesh, u32 blend_index, f32 weight) {
    if (!mesh || !mesh->morph_targets) return;
    if (blend_index >= mesh->morph_targets->shape_count) return;
    
    // Clamp weight to [0, 1]
    if (weight < 0.0f) weight = 0.0f;
    if (weight > 1.0f) weight = 1.0f;
    
    mesh->morph_targets->shapes[blend_index].weight = weight;
}

// ----------------------------------------------------------------------------
// Blend Shape Evaluation (CPU)
// ----------------------------------------------------------------------------

void mesh_evaluate_blend_shapes(mesh_t* mesh, vertex_t* output_vertices) {
    if (!mesh || !output_vertices || !mesh->morph_targets) return;
    
    // Start with base mesh vertices
    memcpy(output_vertices, mesh->vertices, sizeof(vertex_t) * mesh->vertex_count);
    
    // Apply each blend shape
    for (u32 shape_idx = 0; shape_idx < mesh->morph_targets->shape_count; ++shape_idx) {
        blend_shape_t* shape = &mesh->morph_targets->shapes[shape_idx];
        
        if (shape->weight <= 0.0f) continue; // Skip inactive shapes
        
        for (u32 v = 0; v < mesh->vertex_count; ++v) {
            // Apply position delta
            output_vertices[v].position.x += shape->position_deltas[v].x * shape->weight;
            output_vertices[v].position.y += shape->position_deltas[v].y * shape->weight;
            output_vertices[v].position.z += shape->position_deltas[v].z * shape->weight;
            
            // Apply normal delta if present
            if (shape->normal_deltas) {
                output_vertices[v].normal.x += shape->normal_deltas[v].x * shape->weight;
                output_vertices[v].normal.y += shape->normal_deltas[v].y * shape->weight;
                output_vertices[v].normal.z += shape->normal_deltas[v].z * shape->weight;
            }
        }
    }
    
    // Re-normalize normals after blending
    for (u32 v = 0; v < mesh->vertex_count; ++v) {
        f32 len = sqrtf(
            output_vertices[v].normal.x * output_vertices[v].normal.x +
            output_vertices[v].normal.y * output_vertices[v].normal.y +
            output_vertices[v].normal.z * output_vertices[v].normal.z
        );
        
        if (len > 0.0001f) {
            output_vertices[v].normal.x /= len;
            output_vertices[v].normal.y /= len;
            output_vertices[v].normal.z /= len;
        }
    }
}

// ----------------------------------------------------------------------------
// Skeletal Animation Support
// ----------------------------------------------------------------------------

bool mesh_validate_bone_weights(const vertex_skinned_t* vertices, u32 count) {
    if (!vertices) return false;
    
    for (u32 i = 0; i < count; ++i) {
        f32 sum = vertices[i].weights.x + 
                  vertices[i].weights.y +
                  vertices[i].weights.z +
                  vertices[i].weights.w;
        
        // Allow small tolerance
        if (fabsf(sum - 1.0f) > 0.001f) {
            LOG_ERROR("Vertex %u has invalid bone weights (sum = %.3f)", i, sum);
            return false;
        }
    }
    
    return true;
}
