/*
 * instance_batching.h
 * Instance draw batching
 *
 * Part of the Geometry subsystem
 * Advanced 3D Rendering Engine
 */

#ifndef GEOMETRY_INSTANCE_BATCHING_H
#define GEOMETRY_INSTANCE_BATCHING_H

#include <stdint.h>
#include <stdbool.h>
#include <stddef.h>

#ifdef __cplusplus
extern "C" {
#endif

/* ============================================================================
 * TYPES
 * ============================================================================ */

typedef struct geometry_instance_batching_handle {
    uint32_t id;
} geometry_instance_batching_handle_t;

typedef struct geometry_instance_batching_desc {
    uint32_t flags;
    void* user_data;
} geometry_instance_batching_desc_t;

typedef struct geometry_instance_batching_info {
    uint32_t id;
    uint32_t flags;
    bool initialized;
} geometry_instance_batching_info_t;

/* ============================================================================
 * API
 * ============================================================================ */

/* Initialization */
int geometry_instance_batching_init(void);
void geometry_instance_batching_shutdown(void);

/* Lifecycle */
int geometry_instance_batching_create(geometry_instance_batching_handle_t* out_handle, const geometry_instance_batching_desc_t* desc);
void geometry_instance_batching_destroy(geometry_instance_batching_handle_t handle);

/* Operations */
int geometry_instance_batching_update(geometry_instance_batching_handle_t handle, const void* data, size_t size);
bool geometry_instance_batching_is_valid(geometry_instance_batching_handle_t handle);
int geometry_instance_batching_get_info(geometry_instance_batching_handle_t handle, geometry_instance_batching_info_t* out_info);
void geometry_instance_batching_mark_dirty(geometry_instance_batching_handle_t handle);
int geometry_instance_batching_process_pending(void);

/* Statistics */
uint32_t geometry_instance_batching_get_count(void);
size_t geometry_instance_batching_get_memory_usage(void);
void geometry_instance_batching_debug_print(void);

/* GPU Integration - Additional Functions */
typedef struct geometry_instance_batch_data {
    uint32_t* batch_indices;        // Instance indices in this batch
    uint32_t batch_count;           // Number of instances in batch
    uint32_t mesh_id;               // Mesh for this batch
    uint32_t material_id;           // Material for this batch
    uint8_t lod_level;              // LOD level for this batch
} geometry_instance_batch_data_t;

/* Get batch data for GPU upload */
int geometry_instance_batching_get_batch_data(geometry_instance_batching_handle_t handle,
                                               uint32_t batch_index,
                                               geometry_instance_batch_data_t* out_data);

/* Get total instance count across all batches */
uint32_t geometry_instance_batching_get_instance_count(geometry_instance_batching_handle_t handle);

/* Get batch count for the handle */
uint32_t geometry_instance_batching_get_batch_count(geometry_instance_batching_handle_t handle);

/* Add instance with GPU data preparation */
int geometry_instance_batching_add_instance(geometry_instance_batching_handle_t handle,
                                             uint32_t instance_id,
                                             uint32_t mesh_id,
                                             uint32_t material_id,
                                             uint32_t lod_level,
                                             uint32_t shader_variant,
                                             float depth);

/* Clear all batches */
int geometry_instance_batching_clear(geometry_instance_batching_handle_t handle);

/* Sort batches for rendering */
int geometry_instance_batching_sort(geometry_instance_batching_handle_t handle, bool transparent);

#ifdef __cplusplus
}
#endif

#endif /* GEOMETRY_INSTANCE_BATCHING_H */
