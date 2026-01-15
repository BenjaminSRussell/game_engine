/*
 * aerial_perspective.h
 * Aerial perspective
 *
 * Part of the Atmosphere subsystem
 * Advanced 3D Rendering Engine
 */

#ifndef ATMOSPHERE_AERIAL_PERSPECTIVE_H
#define ATMOSPHERE_AERIAL_PERSPECTIVE_H

#include <stdint.h>
#include <stdbool.h>
#include <stddef.h>

#ifdef __cplusplus
extern "C" {
#endif

/* ============================================================================
 * TYPES
 * ============================================================================ */

typedef struct atmosphere_aerial_perspective_handle {
    uint32_t id;
} atmosphere_aerial_perspective_handle_t;

typedef struct atmosphere_aerial_perspective_desc {
    uint32_t flags;
    void* user_data;
} atmosphere_aerial_perspective_desc_t;

typedef struct atmosphere_aerial_perspective_info {
    uint32_t id;
    uint32_t flags;
    bool initialized;
} atmosphere_aerial_perspective_info_t;

/* ============================================================================
 * API
 * ============================================================================ */

/* Initialization */
int atmosphere_aerial_perspective_init(void);
void atmosphere_aerial_perspective_shutdown(void);

/* Lifecycle */
int atmosphere_aerial_perspective_create(atmosphere_aerial_perspective_handle_t* out_handle, const atmosphere_aerial_perspective_desc_t* desc);
void atmosphere_aerial_perspective_destroy(atmosphere_aerial_perspective_handle_t handle);

/* Operations */
int atmosphere_aerial_perspective_update(atmosphere_aerial_perspective_handle_t handle, const void* data, size_t size);
bool atmosphere_aerial_perspective_is_valid(atmosphere_aerial_perspective_handle_t handle);
int atmosphere_aerial_perspective_get_info(atmosphere_aerial_perspective_handle_t handle, atmosphere_aerial_perspective_info_t* out_info);
void atmosphere_aerial_perspective_mark_dirty(atmosphere_aerial_perspective_handle_t handle);
int atmosphere_aerial_perspective_process_pending(void);

/* Statistics */
uint32_t atmosphere_aerial_perspective_get_count(void);
size_t atmosphere_aerial_perspective_get_memory_usage(void);
void atmosphere_aerial_perspective_debug_print(void);

#ifdef __cplusplus
}
#endif

#endif /* ATMOSPHERE_AERIAL_PERSPECTIVE_H */
