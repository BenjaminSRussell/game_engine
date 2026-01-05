/*
 * cascade_blending.h
 * Inter-cascade blending
 *
 * Part of the Lighting subsystem
 * Advanced 3D Rendering Engine
 */

#ifndef LIGHTING_CASCADE_BLENDING_H
#define LIGHTING_CASCADE_BLENDING_H

#include <stdint.h>
#include <stdbool.h>
#include <stddef.h>

#ifdef __cplusplus
extern "C" {
#endif

/* ============================================================================
 * TYPES
 * ============================================================================ */

typedef struct lighting_cascade_blending_handle {
    uint32_t id;
} lighting_cascade_blending_handle_t;

typedef struct lighting_cascade_blending_desc {
    uint32_t flags;
    void* user_data;
} lighting_cascade_blending_desc_t;

typedef struct lighting_cascade_blending_info {
    uint32_t id;
    uint32_t flags;
    bool initialized;
} lighting_cascade_blending_info_t;

/* ============================================================================
 * API
 * ============================================================================ */

/* Initialization */
int lighting_cascade_blending_init(void);
void lighting_cascade_blending_shutdown(void);

/* Lifecycle */
int lighting_cascade_blending_create(lighting_cascade_blending_handle_t* out_handle, const lighting_cascade_blending_desc_t* desc);
void lighting_cascade_blending_destroy(lighting_cascade_blending_handle_t handle);

/* Operations */
int lighting_cascade_blending_update(lighting_cascade_blending_handle_t handle, const void* data, size_t size);
bool lighting_cascade_blending_is_valid(lighting_cascade_blending_handle_t handle);
int lighting_cascade_blending_get_info(lighting_cascade_blending_handle_t handle, lighting_cascade_blending_info_t* out_info);
void lighting_cascade_blending_mark_dirty(lighting_cascade_blending_handle_t handle);
int lighting_cascade_blending_process_pending(void);

/* Statistics */
uint32_t lighting_cascade_blending_get_count(void);
size_t lighting_cascade_blending_get_memory_usage(void);
void lighting_cascade_blending_debug_print(void);

#ifdef __cplusplus
}
#endif

#endif /* LIGHTING_CASCADE_BLENDING_H */
