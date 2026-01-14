/*
 * participating_media.h
 * Participating media simulation
 *
 * Part of the Lighting subsystem
 * Advanced 3D Rendering Engine
 */

#ifndef LIGHTING_PARTICIPATING_MEDIA_H
#define LIGHTING_PARTICIPATING_MEDIA_H

#include <stdint.h>
#include <stdbool.h>
#include <stddef.h>

#ifdef __cplusplus
extern "C" {
#endif

/* ============================================================================
 * TYPES
 * ============================================================================ */

typedef struct lighting_participating_media_handle {
    uint32_t id;
} lighting_participating_media_handle_t;

typedef struct lighting_participating_media_desc {
    uint32_t flags;
    void* user_data;
} lighting_participating_media_desc_t;

typedef struct lighting_participating_media_info {
    uint32_t id;
    uint32_t flags;
    bool initialized;
} lighting_participating_media_info_t;

/* ============================================================================
 * API
 * ============================================================================ */

/* Initialization */
int lighting_participating_media_init(void);
void lighting_participating_media_shutdown(void);

/* Lifecycle */
int lighting_participating_media_create(lighting_participating_media_handle_t* out_handle, const lighting_participating_media_desc_t* desc);
void lighting_participating_media_destroy(lighting_participating_media_handle_t handle);

/* Operations */
int lighting_participating_media_update(lighting_participating_media_handle_t handle, const void* data, size_t size);
bool lighting_participating_media_is_valid(lighting_participating_media_handle_t handle);
int lighting_participating_media_get_info(lighting_participating_media_handle_t handle, lighting_participating_media_info_t* out_info);
void lighting_participating_media_mark_dirty(lighting_participating_media_handle_t handle);
int lighting_participating_media_process_pending(void);

/* Statistics */
uint32_t lighting_participating_media_get_count(void);
size_t lighting_participating_media_get_memory_usage(void);
void lighting_participating_media_debug_print(void);

#ifdef __cplusplus
}
#endif

#endif /* LIGHTING_PARTICIPATING_MEDIA_H */
