/*
 * fresnel.h
 * Fresnel equations
 *
 * Part of the Shading subsystem
 * Advanced 3D Rendering Engine
 */

#ifndef SHADING_FRESNEL_H
#define SHADING_FRESNEL_H

#include <stdint.h>
#include <stdbool.h>
#include <stddef.h>

#ifdef __cplusplus
extern "C" {
#endif

/* ============================================================================
 * TYPES
 * ============================================================================ */

typedef struct shading_fresnel_handle {
    uint32_t id;
} shading_fresnel_handle_t;

typedef struct shading_fresnel_desc {
    uint32_t flags;
    void* user_data;
} shading_fresnel_desc_t;

typedef struct shading_fresnel_info {
    uint32_t id;
    uint32_t flags;
    bool initialized;
} shading_fresnel_info_t;

/* ============================================================================
 * API
 * ============================================================================ */

/* Initialization */
int shading_fresnel_init(void);
void shading_fresnel_shutdown(void);

/* Lifecycle */
int shading_fresnel_create(shading_fresnel_handle_t* out_handle, const shading_fresnel_desc_t* desc);
void shading_fresnel_destroy(shading_fresnel_handle_t handle);

/* Operations */
int shading_fresnel_update(shading_fresnel_handle_t handle, const void* data, size_t size);
bool shading_fresnel_is_valid(shading_fresnel_handle_t handle);
int shading_fresnel_get_info(shading_fresnel_handle_t handle, shading_fresnel_info_t* out_info);
void shading_fresnel_mark_dirty(shading_fresnel_handle_t handle);
int shading_fresnel_process_pending(void);

/* Statistics */
uint32_t shading_fresnel_get_count(void);
size_t shading_fresnel_get_memory_usage(void);
void shading_fresnel_debug_print(void);

#ifdef __cplusplus
}
#endif

#endif /* SHADING_FRESNEL_H */
