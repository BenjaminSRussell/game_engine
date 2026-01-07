/*
 * bvh_compaction.h
 * BVH memory optimization
 *
 * Part of the Geometry subsystem
 * Advanced 3D Rendering Engine
 */

#ifndef GEOMETRY_BVH_COMPACTION_H
#define GEOMETRY_BVH_COMPACTION_H

#include <stdint.h>
#include <stdbool.h>
#include <stddef.h>

#ifdef __cplusplus
extern "C" {
#endif

/* ============================================================================
 * TYPES
 * ============================================================================ */

typedef struct geometry_bvh_compaction_handle {
    uint32_t id;
} geometry_bvh_compaction_handle_t;

typedef struct geometry_bvh_compaction_desc {
    uint32_t flags;
    void* user_data;
} geometry_bvh_compaction_desc_t;

typedef struct geometry_bvh_compaction_info {
    uint32_t id;
    uint32_t flags;
    bool initialized;
} geometry_bvh_compaction_info_t;

/* ============================================================================
 * API
 * ============================================================================ */

/* Initialization */
int geometry_bvh_compaction_init(void);
void geometry_bvh_compaction_shutdown(void);

/* Lifecycle */
int geometry_bvh_compaction_create(geometry_bvh_compaction_handle_t* out_handle, const geometry_bvh_compaction_desc_t* desc);
void geometry_bvh_compaction_destroy(geometry_bvh_compaction_handle_t handle);

/* Operations */
int geometry_bvh_compaction_update(geometry_bvh_compaction_handle_t handle, const void* data, size_t size);
bool geometry_bvh_compaction_is_valid(geometry_bvh_compaction_handle_t handle);
int geometry_bvh_compaction_get_info(geometry_bvh_compaction_handle_t handle, geometry_bvh_compaction_info_t* out_info);
void geometry_bvh_compaction_mark_dirty(geometry_bvh_compaction_handle_t handle);
int geometry_bvh_compaction_process_pending(void);

/* Statistics */
uint32_t geometry_bvh_compaction_get_count(void);
size_t geometry_bvh_compaction_get_memory_usage(void);
void geometry_bvh_compaction_debug_print(void);

#ifdef __cplusplus
}
#endif

#endif /* GEOMETRY_BVH_COMPACTION_H */
