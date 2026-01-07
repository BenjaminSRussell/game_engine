/*
 * final_gather_rays.h
 * Final gather rays
 *
 * Part of the Lumen subsystem
 * Advanced 3D Rendering Engine
 */

#ifndef LUMEN_FINAL_GATHER_RAYS_H
#define LUMEN_FINAL_GATHER_RAYS_H

#include <stdint.h>
#include <stdbool.h>
#include <stddef.h>

#ifdef __cplusplus
extern "C" {
#endif

/* ============================================================================
 * TYPES
 * ============================================================================ */

typedef struct lumen_final_gather_rays_handle {
    uint32_t id;
} lumen_final_gather_rays_handle_t;

typedef struct lumen_final_gather_rays_desc {
    uint32_t flags;
    void* user_data;
} lumen_final_gather_rays_desc_t;

typedef struct lumen_final_gather_rays_info {
    uint32_t id;
    uint32_t flags;
    bool initialized;
} lumen_final_gather_rays_info_t;

/* ============================================================================
 * API
 * ============================================================================ */

/* Initialization */
int lumen_final_gather_rays_init(void);
void lumen_final_gather_rays_shutdown(void);

/* Lifecycle */
int lumen_final_gather_rays_create(lumen_final_gather_rays_handle_t* out_handle, const lumen_final_gather_rays_desc_t* desc);
void lumen_final_gather_rays_destroy(lumen_final_gather_rays_handle_t handle);

/* Operations */
int lumen_final_gather_rays_update(lumen_final_gather_rays_handle_t handle, const void* data, size_t size);
bool lumen_final_gather_rays_is_valid(lumen_final_gather_rays_handle_t handle);
int lumen_final_gather_rays_get_info(lumen_final_gather_rays_handle_t handle, lumen_final_gather_rays_info_t* out_info);
void lumen_final_gather_rays_mark_dirty(lumen_final_gather_rays_handle_t handle);
int lumen_final_gather_rays_process_pending(void);

/* Statistics */
uint32_t lumen_final_gather_rays_get_count(void);
size_t lumen_final_gather_rays_get_memory_usage(void);
void lumen_final_gather_rays_debug_print(void);

#ifdef __cplusplus
}
#endif

#endif /* LUMEN_FINAL_GATHER_RAYS_H */
