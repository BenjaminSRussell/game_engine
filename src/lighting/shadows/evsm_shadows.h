/*
 * evsm_shadows.h
 * Exponential variance shadows
 *
 * Part of the Lighting subsystem
 * Advanced 3D Rendering Engine
 */

#ifndef LIGHTING_EVSM_SHADOWS_H
#define LIGHTING_EVSM_SHADOWS_H

#include <stdint.h>
#include <stdbool.h>
#include <stddef.h>

#ifdef __cplusplus
extern "C" {
#endif

/* ============================================================================
 * TYPES
 * ============================================================================ */

typedef struct lighting_evsm_shadows_handle {
    uint32_t id;
} lighting_evsm_shadows_handle_t;

typedef struct lighting_evsm_shadows_desc {
    uint32_t flags;
    void* user_data;
} lighting_evsm_shadows_desc_t;

typedef struct lighting_evsm_shadows_info {
    uint32_t id;
    uint32_t flags;
    bool initialized;
} lighting_evsm_shadows_info_t;

/* ============================================================================
 * API
 * ============================================================================ */

/* Initialization */
int lighting_evsm_shadows_init(void);
void lighting_evsm_shadows_shutdown(void);

/* Lifecycle */
int lighting_evsm_shadows_create(lighting_evsm_shadows_handle_t* out_handle, const lighting_evsm_shadows_desc_t* desc);
void lighting_evsm_shadows_destroy(lighting_evsm_shadows_handle_t handle);

/* Operations */
int lighting_evsm_shadows_update(lighting_evsm_shadows_handle_t handle, const void* data, size_t size);
bool lighting_evsm_shadows_is_valid(lighting_evsm_shadows_handle_t handle);
int lighting_evsm_shadows_get_info(lighting_evsm_shadows_handle_t handle, lighting_evsm_shadows_info_t* out_info);
void lighting_evsm_shadows_mark_dirty(lighting_evsm_shadows_handle_t handle);
int lighting_evsm_shadows_process_pending(void);

/* Statistics */
uint32_t lighting_evsm_shadows_get_count(void);
size_t lighting_evsm_shadows_get_memory_usage(void);
void lighting_evsm_shadows_debug_print(void);

#ifdef __cplusplus
}
#endif

#endif /* LIGHTING_EVSM_SHADOWS_H */
