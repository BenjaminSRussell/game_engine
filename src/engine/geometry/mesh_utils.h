#ifndef GEOMETRY_MESH_UTILS_H
#define GEOMETRY_MESH_UTILS_H

#include "geometry_types.h"

// Convex Hull Approximation
// Returns array of extreme hull vertices (caller must free)
void mesh_calculate_convex_hull(const mesh_t* mesh, Vec3** out_hull_vertices, u32* out_hull_vertex_count);

// Per-Submesh Bounds
void mesh_calculate_submesh_bounds(mesh_t* mesh);

// Transform-Aware Bounds Update
// transform_matrix should be 4x4 column-major matrix (16 floats)
void mesh_update_bounds_transform_matrix(mesh_t* mesh, const float* transform_matrix);

#endif // GEOMETRY_MESH_UTILS_H
