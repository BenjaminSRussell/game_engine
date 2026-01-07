/*
 * clearcoat_normal.h
 * Clearcoat normal map
 *
 * Part of the Shading subsystem
 * Advanced 3D Rendering Engine
 */

#ifndef SHADING_CLEARCOAT_NORMAL_H
#define SHADING_CLEARCOAT_NORMAL_H

#include <stdint.h>
#include <stdbool.h>
#include <stddef.h>

#ifdef __cplusplus
extern "C" {
#endif

/* ============================================================================
 * TYPES
 * ============================================================================ */

typedef struct shading_clearcoat_normal_handle {
    uint32_t id;
} shading_clearcoat_normal_handle_t;

typedef struct shading_clearcoat_normal_desc {
    uint32_t flags;
    void* user_data;
} shading_clearcoat_normal_desc_t;

typedef struct shading_clearcoat_normal_info {
    uint32_t id;
    uint32_t flags;
    bool initialized;
} shading_clearcoat_normal_info_t;

/* ============================================================================
 * API
 * ============================================================================ */

/* Initialization */
int shading_clearcoat_normal_init(void);
void shading_clearcoat_normal_shutdown(void);

/* Lifecycle */
int shading_clearcoat_normal_create(shading_clearcoat_normal_handle_t* out_handle, const shading_clearcoat_normal_desc_t* desc);
void shading_clearcoat_normal_destroy(shading_clearcoat_normal_handle_t handle);

/* Operations */
int shading_clearcoat_normal_update(shading_clearcoat_normal_handle_t handle, const void* data, size_t size);
bool shading_clearcoat_normal_is_valid(shading_clearcoat_normal_handle_t handle);
int shading_clearcoat_normal_get_info(shading_clearcoat_normal_handle_t handle, shading_clearcoat_normal_info_t* out_info);
void shading_clearcoat_normal_mark_dirty(shading_clearcoat_normal_handle_t handle);
int shading_clearcoat_normal_process_pending(void);

/* Statistics */
uint32_t shading_clearcoat_normal_get_count(void);
size_t shading_clearcoat_normal_get_memory_usage(void);
void shading_clearcoat_normal_debug_print(void);

#ifdef __cplusplus
}
#endif

#endif /* SHADING_CLEARCOAT_NORMAL_H */
