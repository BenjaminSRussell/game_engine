/*
 * multiscatter_ggx.h
 * Multi-scatter GGX
 *
 * Part of the Shading subsystem
 * Advanced 3D Rendering Engine
 */

#ifndef SHADING_MULTISCATTER_GGX_H
#define SHADING_MULTISCATTER_GGX_H

#include <stdint.h>
#include <stdbool.h>
#include <stddef.h>

#ifdef __cplusplus
extern "C" {
#endif

/* ============================================================================
 * TYPES
 * ============================================================================ */

typedef struct shading_multiscatter_ggx_handle {
    uint32_t id;
} shading_multiscatter_ggx_handle_t;

typedef struct shading_multiscatter_ggx_desc {
    uint32_t flags;
    void* user_data;
} shading_multiscatter_ggx_desc_t;

typedef struct shading_multiscatter_ggx_info {
    uint32_t id;
    uint32_t flags;
    bool initialized;
} shading_multiscatter_ggx_info_t;

/* ============================================================================
 * API
 * ============================================================================ */

/* Initialization */
int shading_multiscatter_ggx_init(void);
void shading_multiscatter_ggx_shutdown(void);

/* Lifecycle */
int shading_multiscatter_ggx_create(shading_multiscatter_ggx_handle_t* out_handle, const shading_multiscatter_ggx_desc_t* desc);
void shading_multiscatter_ggx_destroy(shading_multiscatter_ggx_handle_t handle);

/* Operations */
int shading_multiscatter_ggx_update(shading_multiscatter_ggx_handle_t handle, const void* data, size_t size);
bool shading_multiscatter_ggx_is_valid(shading_multiscatter_ggx_handle_t handle);
int shading_multiscatter_ggx_get_info(shading_multiscatter_ggx_handle_t handle, shading_multiscatter_ggx_info_t* out_info);
void shading_multiscatter_ggx_mark_dirty(shading_multiscatter_ggx_handle_t handle);
int shading_multiscatter_ggx_process_pending(void);

/* Statistics */
uint32_t shading_multiscatter_ggx_get_count(void);
size_t shading_multiscatter_ggx_get_memory_usage(void);
void shading_multiscatter_ggx_debug_print(void);

#ifdef __cplusplus
}
#endif

#endif /* SHADING_MULTISCATTER_GGX_H */
