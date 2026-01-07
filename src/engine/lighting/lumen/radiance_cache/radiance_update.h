/*
 * radiance_update.h
 * Radiance cache update
 *
 * Part of the Lumen subsystem
 * Advanced 3D Rendering Engine
 */

#ifndef LUMEN_RADIANCE_UPDATE_H
#define LUMEN_RADIANCE_UPDATE_H

#include <stdint.h>
#include <stdbool.h>
#include <stddef.h>

#ifdef __cplusplus
extern "C" {
#endif

/* ============================================================================
 * TYPES
 * ============================================================================ */

typedef struct lumen_radiance_update_handle {
    uint32_t id;
} lumen_radiance_update_handle_t;

typedef struct lumen_radiance_update_desc {
    uint32_t flags;
    void* user_data;
} lumen_radiance_update_desc_t;

typedef struct lumen_radiance_update_info {
    uint32_t id;
    uint32_t flags;
    bool initialized;
} lumen_radiance_update_info_t;

/* ============================================================================
 * API
 * ============================================================================ */

/* Initialization */
int lumen_radiance_update_init(void);
void lumen_radiance_update_shutdown(void);

/* Lifecycle */
int lumen_radiance_update_create(lumen_radiance_update_handle_t* out_handle, const lumen_radiance_update_desc_t* desc);
void lumen_radiance_update_destroy(lumen_radiance_update_handle_t handle);

/* Operations */
int lumen_radiance_update_update(lumen_radiance_update_handle_t handle, const void* data, size_t size);
bool lumen_radiance_update_is_valid(lumen_radiance_update_handle_t handle);
int lumen_radiance_update_get_info(lumen_radiance_update_handle_t handle, lumen_radiance_update_info_t* out_info);
void lumen_radiance_update_mark_dirty(lumen_radiance_update_handle_t handle);
int lumen_radiance_update_process_pending(void);

/* Statistics */
uint32_t lumen_radiance_update_get_count(void);
size_t lumen_radiance_update_get_memory_usage(void);
void lumen_radiance_update_debug_print(void);

#ifdef __cplusplus
}
#endif

#endif /* LUMEN_RADIANCE_UPDATE_H */
