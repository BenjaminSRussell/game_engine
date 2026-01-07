/*
 * ggx_brdf.h
 * GGX microfacet BRDF
 *
 * Part of the Shading subsystem
 * Advanced 3D Rendering Engine
 */

#ifndef SHADING_GGX_BRDF_H
#define SHADING_GGX_BRDF_H

#include <stdint.h>
#include <stdbool.h>
#include <stddef.h>

#ifdef __cplusplus
extern "C" {
#endif

/* ============================================================================
 * TYPES
 * ============================================================================ */

typedef struct shading_ggx_brdf_handle {
    uint32_t id;
} shading_ggx_brdf_handle_t;

typedef struct shading_ggx_brdf_desc {
    uint32_t flags;
    void* user_data;
} shading_ggx_brdf_desc_t;

typedef struct shading_ggx_brdf_info {
    uint32_t id;
    uint32_t flags;
    bool initialized;
} shading_ggx_brdf_info_t;

/* ============================================================================
 * API
 * ============================================================================ */

/* Initialization */
int shading_ggx_brdf_init(void);
void shading_ggx_brdf_shutdown(void);

/* Lifecycle */
int shading_ggx_brdf_create(shading_ggx_brdf_handle_t* out_handle, const shading_ggx_brdf_desc_t* desc);
void shading_ggx_brdf_destroy(shading_ggx_brdf_handle_t handle);

/* Operations */
int shading_ggx_brdf_update(shading_ggx_brdf_handle_t handle, const void* data, size_t size);
bool shading_ggx_brdf_is_valid(shading_ggx_brdf_handle_t handle);
int shading_ggx_brdf_get_info(shading_ggx_brdf_handle_t handle, shading_ggx_brdf_info_t* out_info);
void shading_ggx_brdf_mark_dirty(shading_ggx_brdf_handle_t handle);
int shading_ggx_brdf_process_pending(void);

/* Statistics */
uint32_t shading_ggx_brdf_get_count(void);
size_t shading_ggx_brdf_get_memory_usage(void);
void shading_ggx_brdf_debug_print(void);

#ifdef __cplusplus
}
#endif

#endif /* SHADING_GGX_BRDF_H */
