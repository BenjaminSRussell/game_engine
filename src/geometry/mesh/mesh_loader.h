/*
 * mesh_loader.h
 * Mesh file format loading and processing
 *
 * Part of the Geometry subsystem
 * Advanced 3D Rendering Engine
 */

#ifndef GEOMETRY_MESH_LOADER_H
#define GEOMETRY_MESH_LOADER_H

#include <stdint.h>
#include <stdbool.h>
#include <stddef.h>
#include "../include/common.h"

#ifdef __cplusplus
extern "C" {
#endif

/* ============================================================================
 * TYPES
 * ============================================================================ */

// Supported mesh formats
typedef enum mesh_format_e {
    MESH_FORMAT_OBJ = 0,
    MESH_FORMAT_FBX = 1,
    MESH_FORMAT_GLTF = 2,
    MESH_FORMAT_PLY = 3,
    MESH_FORMAT_STL = 4,
    MESH_FORMAT_CUSTOM = 5
} mesh_format_e;

// Mesh loading options
typedef struct mesh_load_options_t {
    bool calculate_normals;
    bool calculate_tangents;
    bool optimize_vertices;
    bool generate_bounds;
    bool flip_uvs;
    bool merge_vertices;
    f32 merge_tolerance;
    u32 max_vertices;
    bool load_materials;
    bool load_animations;
} mesh_load_options_t;

// Mesh loading statistics
typedef struct mesh_load_stats_t {
    u32 original_vertices;
    u32 original_indices;
    u32 processed_vertices;
    u32 processed_indices;
    u32 merged_vertices;
    u32 submesh_count;
    f32 load_time_ms;
    f32 processing_time_ms;
} mesh_load_stats_t;

// Legacy compatibility types
typedef struct geometry_mesh_loader_handle {
    uint32_t id;
} geometry_mesh_loader_handle_t;

typedef void (*geometry_mesh_loader_callback_t)(geometry_mesh_loader_handle_t handle, void* user_data, int status);

typedef struct geometry_mesh_loader_desc {
    uint32_t flags;
    void* user_data;
} geometry_mesh_loader_desc_t;

typedef struct geometry_mesh_loader_info {
    uint32_t id;
    uint32_t flags;
    bool initialized;
} geometry_mesh_loader_info_t;

/* ============================================================================
 * CONSTANTS
 * ============================================================================ */

extern const mesh_load_options_t MESH_LOAD_OPTIONS_DEFAULT;

/* ============================================================================
 * HIGH-LEVEL MESH LOADING API
 * ============================================================================ */

// Main loading functions
mesh_t* mesh_load_from_file(const char* filepath, const mesh_load_options_t* options);
mesh_t* mesh_load_from_memory(const void* data, size_t size, mesh_format_e format, const mesh_load_options_t* options);
mesh_t* mesh_load_from_stream(FILE* stream, mesh_format_e format, const mesh_load_options_t* options);

// Format-specific loaders
mesh_t* mesh_load_obj(const char* filepath, const mesh_load_options_t* options);
mesh_t* mesh_load_fbx(const char* filepath, const mesh_load_options_t* options);
mesh_t* mesh_load_gltf(const char* filepath, const mesh_load_options_t* options);
mesh_t* mesh_load_ply(const char* filepath, const mesh_load_options_t* options);
mesh_t* mesh_load_stl(const char* filepath, const mesh_load_options_t* options);

// Utility functions
mesh_format_e mesh_detect_format(const char* filepath);
bool mesh_validate_format(const char* filepath, mesh_format_e expected_format);
const char* mesh_format_to_string(mesh_format_e format);
void mesh_get_load_stats(const mesh_t* mesh, mesh_load_stats_t* stats);

/* ============================================================================
 * LEGACY API (for compatibility)
 * ============================================================================ */

/* Initialization */
int geometry_mesh_loader_init(void);
void geometry_mesh_loader_shutdown(void);

/* Lifecycle */
int geometry_mesh_loader_create(geometry_mesh_loader_handle_t* out_handle, const geometry_mesh_loader_desc_t* desc);
void geometry_mesh_loader_destroy(geometry_mesh_loader_handle_t handle);

/* Operations */
int geometry_mesh_loader_update(geometry_mesh_loader_handle_t handle, const void* data, size_t size);
bool geometry_mesh_loader_is_valid(geometry_mesh_loader_handle_t handle);
int geometry_mesh_loader_get_info(geometry_mesh_loader_handle_t handle, geometry_mesh_loader_info_t* out_info);
void geometry_mesh_loader_mark_dirty(geometry_mesh_loader_handle_t handle);
int geometry_mesh_loader_process_pending(void);

/* Async Loading */
int geometry_mesh_loader_load_async(
    geometry_mesh_loader_handle_t handle,
    const char* path,
    geometry_mesh_loader_callback_t callback,
    void* user_data
);

/* Statistics */
uint32_t geometry_mesh_loader_get_count(void);
size_t geometry_mesh_loader_get_memory_usage(void);
void geometry_mesh_loader_debug_print(void);

/* ============================================================================
 * BATCH LOADING
 * ============================================================================ */

typedef struct mesh_batch_t {
    mesh_t** meshes;
    u32 count;
    u32 capacity;
} mesh_batch_t;

mesh_batch_t* mesh_batch_create(void);
void mesh_batch_destroy(mesh_batch_t* batch);
bool mesh_batch_add(mesh_batch_t* batch, mesh_t* mesh);
mesh_batch_t* mesh_load_directory(const char* directory, const mesh_load_options_t* options);
mesh_batch_t* mesh_load_multi_file(const char** filepaths, u32 count, const mesh_load_options_t* options);

/* ============================================================================
 * ASYNC LOADING
 * ============================================================================ */

typedef struct mesh_load_request_t {
    char filepath[256];
    mesh_load_options_t options;
    mesh_t* result_mesh;
    bool completed;
    bool failed;
    mesh_load_stats_t stats;
} mesh_load_request_t;

mesh_load_request_t* mesh_load_async(const char* filepath, const mesh_load_options_t* options);
bool mesh_load_poll(mesh_load_request_t* request);
void mesh_load_wait(mesh_load_request_t* request);

#ifdef __cplusplus
}
#endif

#endif /* GEOMETRY_MESH_LOADER_H */
