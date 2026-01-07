/*
 * coat_fresnel.h
 * Coat Fresnel
 *
 * Part of the Shading subsystem
 * Advanced 3D Rendering Engine
 */

#ifndef SHADING_COAT_FRESNEL_H
#define SHADING_COAT_FRESNEL_H

#include <stdint.h>
#include <stdbool.h>
#include <stddef.h>

#ifdef __cplusplus
extern "C" {
#endif

/* ============================================================================
 * TYPES
 * ============================================================================ */

typedef struct shading_coat_fresnel_handle {
    uint32_t id;
} shading_coat_fresnel_handle_t;

typedef struct shading_coat_fresnel_desc {
    uint32_t flags;
    void* user_data;
} shading_coat_fresnel_desc_t;

typedef struct shading_coat_fresnel_info {
    uint32_t id;
    uint32_t flags;
    bool initialized;
} shading_coat_fresnel_info_t;

/* ============================================================================
 * API
 * ============================================================================ */

/* Initialization */
int shading_coat_fresnel_init(void);
void shading_coat_fresnel_shutdown(void);

/* Lifecycle */
int shading_coat_fresnel_create(shading_coat_fresnel_handle_t* out_handle, const shading_coat_fresnel_desc_t* desc);
void shading_coat_fresnel_destroy(shading_coat_fresnel_handle_t handle);

/* Operations */
int shading_coat_fresnel_update(shading_coat_fresnel_handle_t handle, const void* data, size_t size);
bool shading_coat_fresnel_is_valid(shading_coat_fresnel_handle_t handle);
int shading_coat_fresnel_get_info(shading_coat_fresnel_handle_t handle, shading_coat_fresnel_info_t* out_info);
void shading_coat_fresnel_mark_dirty(shading_coat_fresnel_handle_t handle);
int shading_coat_fresnel_process_pending(void);

/* Statistics */
uint32_t shading_coat_fresnel_get_count(void);
size_t shading_coat_fresnel_get_memory_usage(void);
void shading_coat_fresnel_debug_print(void);

#ifdef __cplusplus
}
#endif

#endif /* SHADING_COAT_FRESNEL_H */
