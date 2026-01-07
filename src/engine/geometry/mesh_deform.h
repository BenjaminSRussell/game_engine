#ifndef GEOMETRY_MESH_DEFORM_H
#define GEOMETRY_MESH_DEFORM_H

#include "geometry_types.h"

/**
 * Mesh Deformation System
 * 
 * Supports:
 * - Blend shapes / morph targets
 * - Skeletal animation setup (data structures)
 */

// Blend Shape Management
void mesh_add_blend_shape(mesh_t* mesh, const blend_shape_t* shape);
void mesh_remove_blend_shape(mesh_t* mesh, u32 index);
void mesh_set_blend_weight(mesh_t* mesh, u32 blend_index, f32 weight);

// Blend Shape Evaluation (CPU)
// Evaluates all blend shapes and writes result to output_vertices
void mesh_evaluate_blend_shapes(mesh_t* mesh, vertex_t* output_vertices);

// Skeletal Animation Support
// Validates bone weights sum to 1.0 for each vertex
bool mesh_validate_bone_weights(const vertex_skinned_t* vertices, u32 count);

// GPU Skinning Setup (prepares data for GPU upload)
// TODO: Will integrate with GPU backend

#endif // GEOMETRY_MESH_DEFORM_H
