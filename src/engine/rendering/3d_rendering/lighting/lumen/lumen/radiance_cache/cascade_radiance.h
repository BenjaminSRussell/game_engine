/*
 * cascade_radiance.h
 * Cascaded radiance
 *
 * Part of the Lumen subsystem
 * Advanced 3D Rendering Engine
 */

#ifndef LUMEN_CASCADE_RADIANCE_H
#define LUMEN_CASCADE_RADIANCE_H

#include <stdint.h>
#include <stdbool.h>
#include <stddef.h>

#ifdef __cplusplus
extern "C" {
#endif

/* ============================================================================
 * TYPES
 * ============================================================================ */

typedef struct lumen_cascade_radiance_handle {
    uint32_t id;
} lumen_cascade_radiance_handle_t;

typedef struct lumen_cascade_radiance_desc {
    uint32_t flags;
    void* user_data;
} lumen_cascade_radiance_desc_t;

typedef struct lumen_cascade_radiance_info {
    uint32_t id;
    uint32_t flags;
    bool initialized;
} lumen_cascade_radiance_info_t;

/* ============================================================================
 * API
 * ============================================================================ */

/* Initialization */
int lumen_cascade_radiance_init(void);
void lumen_cascade_radiance_shutdown(void);

/* Lifecycle */
int lumen_cascade_radiance_create(lumen_cascade_radiance_handle_t* out_handle, const lumen_cascade_radiance_desc_t* desc);
void lumen_cascade_radiance_destroy(lumen_cascade_radiance_handle_t handle);

/* Operations */
int lumen_cascade_radiance_update(lumen_cascade_radiance_handle_t handle, const void* data, size_t size);
bool lumen_cascade_radiance_is_valid(lumen_cascade_radiance_handle_t handle);
int lumen_cascade_radiance_get_info(lumen_cascade_radiance_handle_t handle, lumen_cascade_radiance_info_t* out_info);
void lumen_cascade_radiance_mark_dirty(lumen_cascade_radiance_handle_t handle);
int lumen_cascade_radiance_process_pending(void);

/* Statistics */
uint32_t lumen_cascade_radiance_get_count(void);
size_t lumen_cascade_radiance_get_memory_usage(void);
void lumen_cascade_radiance_debug_print(void);

#ifdef __cplusplus
}
#endif

#endif /* LUMEN_CASCADE_RADIANCE_H */
