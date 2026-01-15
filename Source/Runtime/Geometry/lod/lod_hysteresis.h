/*
 * lod_hysteresis.h
 * LOD switch hysteresis
 *
 * Part of the Culling subsystem
 * Advanced 3D Rendering Engine
 */

#ifndef CULLING_LOD_HYSTERESIS_H
#define CULLING_LOD_HYSTERESIS_H

#include <stdint.h>
#include <stdbool.h>
#include <stddef.h>

#ifdef __cplusplus
extern "C" {
#endif

/* ============================================================================
 * TYPES
 * ============================================================================ */

typedef struct culling_lod_hysteresis_handle {
    uint32_t id;
} culling_lod_hysteresis_handle_t;

typedef struct culling_lod_hysteresis_desc {
    uint32_t flags;
    void* user_data;
} culling_lod_hysteresis_desc_t;

typedef struct culling_lod_hysteresis_info {
    uint32_t id;
    uint32_t flags;
    bool initialized;
} culling_lod_hysteresis_info_t;

/* ============================================================================
 * API
 * ============================================================================ */

/* Initialization */
int culling_lod_hysteresis_init(void);
void culling_lod_hysteresis_shutdown(void);

/* Lifecycle */
int culling_lod_hysteresis_create(culling_lod_hysteresis_handle_t* out_handle, const culling_lod_hysteresis_desc_t* desc);
void culling_lod_hysteresis_destroy(culling_lod_hysteresis_handle_t handle);

/* Operations */
int culling_lod_hysteresis_update(culling_lod_hysteresis_handle_t handle, const void* data, size_t size);
bool culling_lod_hysteresis_is_valid(culling_lod_hysteresis_handle_t handle);
int culling_lod_hysteresis_get_info(culling_lod_hysteresis_handle_t handle, culling_lod_hysteresis_info_t* out_info);
void culling_lod_hysteresis_mark_dirty(culling_lod_hysteresis_handle_t handle);
int culling_lod_hysteresis_process_pending(void);

/* Statistics */
uint32_t culling_lod_hysteresis_get_count(void);
size_t culling_lod_hysteresis_get_memory_usage(void);
void culling_lod_hysteresis_debug_print(void);

#ifdef __cplusplus
}
#endif

#endif /* CULLING_LOD_HYSTERESIS_H */
