/*
 * mesh_data.h
 * Mesh vertex/index data storage
 *
 * Part of the Geometry subsystem
 * Advanced 3D Rendering Engine
 */

#ifndef GEOMETRY_MESH_DATA_H
#define GEOMETRY_MESH_DATA_H

#include <stdint.h>
#include <stdbool.h>
#include <stddef.h>
#include "geometry/vertex/vertex_format.h"

#ifdef __cplusplus
extern "C" {
#endif

/* ============================================================================
 * TYPES & ENUMS
 * ============================================================================ */

// Index buffer type
typedef enum index_type {
    INDEX_TYPE_UINT16 = 0,
    INDEX_TYPE_UINT32,
    INDEX_TYPE_COUNT
} index_type_t;

// Individual submesh with material binding
typedef struct submesh {
    uint32_t index_offset;    // Offset in index buffer
    uint32_t index_count;     // Number of indices
    uint32_t vertex_offset;   // Offset in vertex buffer
    uint32_t vertex_count;    // Number of vertices
    uint32_t material_id;     // Material binding
    bool visible;             // Visibility flag
} submesh_t;

// Bounding box
typedef struct aabb {
    float min_x, min_y, min_z;
    float max_x, max_y, max_z;
} aabb_t;

// Bounding sphere
typedef struct bounding_sphere {
    float center_x, center_y, center_z;
    float radius;
} bounding_sphere_t;

// Complete mesh data
typedef struct mesh_data {
    void* vertex_data;           // Pointer to vertex data
    void* index_data;            // Pointer to index data
    uint32_t vertex_count;       // Total vertices
    uint32_t index_count;        // Total indices
    uint32_t vertex_stride;      // Stride of each vertex (in bytes)
    size_t vertex_data_size;     // Total size of vertex buffer
    size_t index_data_size;      // Total size of index buffer
    geometry_vertex_format_handle_t vertex_format; // Format handle
    index_type_t index_type;     // Type of indices
    aabb_t bounds;               // Axis-aligned bounding box
    bounding_sphere_t bounding_sphere;
    submesh_t* submeshes;        // Array of submeshes
    uint32_t submesh_count;      // Number of submeshes
    uint32_t flags;              // Usage flags
} mesh_data_t;

// Handle for opaque mesh data
typedef struct geometry_mesh_data_handle {
    uint32_t id;
} geometry_mesh_data_handle_t;

// Descriptor for creating mesh data
typedef struct geometry_mesh_data_desc {
    void* vertex_data;
    size_t vertex_data_size;
    void* index_data;
    size_t index_data_size;
    uint32_t vertex_count;
    uint32_t index_count;
    geometry_vertex_format_handle_t vertex_format;
    index_type_t index_type;
    submesh_t* submeshes;
    uint32_t submesh_count;
    uint32_t flags;
    void* user_data;
} geometry_mesh_data_desc_t;

// Runtime info
typedef struct geometry_mesh_data_info {
    uint32_t id;
    uint32_t vertex_count;
    uint32_t index_count;
    uint32_t submesh_count;
    size_t total_size;
    uint32_t flags;
    bool initialized;
    aabb_t bounds;
} geometry_mesh_data_info_t;

/* ============================================================================
 * API
 * ============================================================================ */

/* Initialization */
int geometry_mesh_data_init(void);
void geometry_mesh_data_shutdown(void);

/* Lifecycle */
int geometry_mesh_data_create(geometry_mesh_data_handle_t* out_handle, const geometry_mesh_data_desc_t* desc);
void geometry_mesh_data_destroy(geometry_mesh_data_handle_t handle);

/* Query & Validation */
bool geometry_mesh_data_is_valid(geometry_mesh_data_handle_t handle);
int geometry_mesh_data_get_info(geometry_mesh_data_handle_t handle, geometry_mesh_data_info_t* out_info);

/* Data Access */
const mesh_data_t* geometry_mesh_data_get_data(geometry_mesh_data_handle_t handle);
int geometry_mesh_data_get_vertex_data(geometry_mesh_data_handle_t handle, const void** out_data, size_t* out_size);
int geometry_mesh_data_get_index_data(geometry_mesh_data_handle_t handle, const void** out_data, size_t* out_size);

/* Submesh Operations */
uint32_t geometry_mesh_data_get_submesh_count(geometry_mesh_data_handle_t handle);
int geometry_mesh_data_get_submesh(geometry_mesh_data_handle_t handle, uint32_t submesh_index, submesh_t* out_submesh);
int geometry_mesh_data_set_submesh_material(geometry_mesh_data_handle_t handle, uint32_t submesh_index, uint32_t material_id);
int geometry_mesh_data_set_submesh_visibility(geometry_mesh_data_handle_t handle, uint32_t submesh_index, bool visible);

/* Bounds Operations */
int geometry_mesh_data_get_bounds(geometry_mesh_data_handle_t handle, aabb_t* out_aabb);
int geometry_mesh_data_get_bounding_sphere(geometry_mesh_data_handle_t handle, bounding_sphere_t* out_sphere);
int geometry_mesh_data_recalculate_bounds(geometry_mesh_data_handle_t handle);

/* Utility Functions */
const char* geometry_index_type_get_name(index_type_t type);
uint32_t geometry_index_type_get_size(index_type_t type);

/* Statistics */
uint32_t geometry_mesh_data_get_count(void);
size_t geometry_mesh_data_get_memory_usage(void);
void geometry_mesh_data_debug_print(void);

#ifdef __cplusplus
}
#endif

#endif /* GEOMETRY_MESH_DATA_H */
