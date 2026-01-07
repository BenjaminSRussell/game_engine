/*
 * trail_uvs.h
 * Trail UV coordinates
 *
 * Part of the Effects subsystem
 * Advanced 3D Rendering Engine
 */

#ifndef EFFECTS_TRAIL_UVS_H
#define EFFECTS_TRAIL_UVS_H

#include <stdint.h>
#include <stdbool.h>
#include <stddef.h>

#ifdef __cplusplus
extern "C" {
#endif

/* ============================================================================
 * TYPES
 * ============================================================================ */

typedef struct effects_trail_uvs_handle {
    uint32_t id;
} effects_trail_uvs_handle_t;

typedef struct effects_trail_uvs_desc {
    uint32_t flags;
    void* user_data;
} effects_trail_uvs_desc_t;

typedef struct effects_trail_uvs_info {
    uint32_t id;
    uint32_t flags;
    bool initialized;
} effects_trail_uvs_info_t;

/* ============================================================================
 * API
 * ============================================================================ */

/* Initialization */
int effects_trail_uvs_init(void);
void effects_trail_uvs_shutdown(void);

/* Lifecycle */
int effects_trail_uvs_create(effects_trail_uvs_handle_t* out_handle, const effects_trail_uvs_desc_t* desc);
void effects_trail_uvs_destroy(effects_trail_uvs_handle_t handle);

/* Operations */
int effects_trail_uvs_update(effects_trail_uvs_handle_t handle, const void* data, size_t size);
bool effects_trail_uvs_is_valid(effects_trail_uvs_handle_t handle);
int effects_trail_uvs_get_info(effects_trail_uvs_handle_t handle, effects_trail_uvs_info_t* out_info);
void effects_trail_uvs_mark_dirty(effects_trail_uvs_handle_t handle);
int effects_trail_uvs_process_pending(void);

/* Statistics */
uint32_t effects_trail_uvs_get_count(void);
size_t effects_trail_uvs_get_memory_usage(void);
void effects_trail_uvs_debug_print(void);

#ifdef __cplusplus
}
#endif

#endif /* EFFECTS_TRAIL_UVS_H */
