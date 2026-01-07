#ifndef GEOMETRY_MESH_H
#define GEOMETRY_MESH_H

#include "geometry_types.h"

// Lifecycle
mesh_t* mesh_create(const char* name);
void mesh_destroy(mesh_t* mesh);

// Buffer Management
// Allocates CPU memory for vertices/indices
void mesh_allocate_buffers(mesh_t* mesh, u32 vertex_count, u32 index_count);
// Sets specific vertex/index data
void mesh_set_vertices(mesh_t* mesh, const vertex_t* vertices, u32 count, u32 offset);
void mesh_set_indices(mesh_t* mesh, const u32* indices, u32 count, u32 offset);

// Submesh Management
void mesh_add_submesh(mesh_t* mesh, submesh_t submesh);
void mesh_clear_submeshes(mesh_t* mesh);
void mesh_set_material(mesh_t* mesh, u32 slot, u32 material_id);

// Operations
void mesh_calculate_bounds(mesh_t* mesh);
void mesh_calculate_obb(mesh_t* mesh);
Vec3 mesh_calculate_centroid(const mesh_t* mesh);
void mesh_calculate_normals(mesh_t* mesh);
void mesh_calculate_tangents(mesh_t* mesh);
void mesh_update_bounds_transform(mesh_t* mesh, const float* transform_matrix);

// Statistics
void mesh_calculate_stats(mesh_t* mesh);
u32 mesh_get_triangle_count(const mesh_t* mesh);
u64 mesh_get_memory_usage(const mesh_t* mesh);

// Optimization
void mesh_deduplicate_vertices(mesh_t* mesh, f32 epsilon);
void mesh_optimize_indices(mesh_t* mesh);
void mesh_optimize_vertex_cache(mesh_t* mesh);

// Blend Shapes / Deformation
void mesh_add_blend_shape(mesh_t* mesh, const blend_shape_t* shape);
void mesh_evaluate_blend_shapes(mesh_t* mesh, vertex_t* output_vertices);

// Cloning
mesh_t* mesh_clone(const mesh_t* mesh);

// GPU Interop (To be implemented with Backend)
bool mesh_upload(mesh_t* mesh);
void mesh_unload(mesh_t* mesh);
bool mesh_update_gpu(mesh_t* mesh);

// Debug & Validation
void mesh_print_stats(const mesh_t* mesh);
bool mesh_validate(const mesh_t* mesh);
mesh_validation_error_e mesh_validate_detailed(const mesh_t* mesh, char* error_msg, u32 msg_size);

#endif // GEOMETRY_MESH_H
