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
void mesh_calculate_normals(mesh_t* mesh);
void mesh_calculate_tangents(mesh_t* mesh);

// GPU Interop (To be implemented with Backend)
bool mesh_upload(mesh_t* mesh);
void mesh_unload(mesh_t* mesh);

// Debug
void mesh_print_stats(const mesh_t* mesh);
bool mesh_validate(const mesh_t* mesh);

#endif // GEOMETRY_MESH_H
