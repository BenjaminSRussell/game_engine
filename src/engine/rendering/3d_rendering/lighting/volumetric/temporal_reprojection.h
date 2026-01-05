/*
 * temporal_reprojection.h
 * Volumetric temporal AA
 *
 * Part of the Lighting subsystem
 * Advanced 3D Rendering Engine
 */

#ifndef LIGHTING_TEMPORAL_REPROJECTION_H
#define LIGHTING_TEMPORAL_REPROJECTION_H

#include <stdint.h>
#include <stdbool.h>
#include <stddef.h>

#ifdef __cplusplus
extern "C" {
#endif

/* ============================================================================
 * TYPES
 * ============================================================================ */

typedef struct lighting_temporal_reprojection_handle {
    uint32_t id;
} lighting_temporal_reprojection_handle_t;

typedef struct lighting_temporal_reprojection_desc {
    uint32_t flags;
    void* user_data;
} lighting_temporal_reprojection_desc_t;

typedef struct lighting_temporal_reprojection_info {
    uint32_t id;
    uint32_t flags;
    bool initialized;
} lighting_temporal_reprojection_info_t;

/* ============================================================================
 * API
 * ============================================================================ */

/* Initialization */
int lighting_temporal_reprojection_init(void);
void lighting_temporal_reprojection_shutdown(void);

/* Lifecycle */
int lighting_temporal_reprojection_create(lighting_temporal_reprojection_handle_t* out_handle, const lighting_temporal_reprojection_desc_t* desc);
void lighting_temporal_reprojection_destroy(lighting_temporal_reprojection_handle_t handle);

/* Operations */
int lighting_temporal_reprojection_update(lighting_temporal_reprojection_handle_t handle, const void* data, size_t size);
bool lighting_temporal_reprojection_is_valid(lighting_temporal_reprojection_handle_t handle);
int lighting_temporal_reprojection_get_info(lighting_temporal_reprojection_handle_t handle, lighting_temporal_reprojection_info_t* out_info);
void lighting_temporal_reprojection_mark_dirty(lighting_temporal_reprojection_handle_t handle);
int lighting_temporal_reprojection_process_pending(void);

/* Statistics */
uint32_t lighting_temporal_reprojection_get_count(void);
size_t lighting_temporal_reprojection_get_memory_usage(void);
void lighting_temporal_reprojection_debug_print(void);

#ifdef __cplusplus
}
#endif

#endif /* LIGHTING_TEMPORAL_REPROJECTION_H */
