/*
 * pre_integrated_sss.h
 * Pre-integrated skin
 *
 * Part of the Shading subsystem
 * Advanced 3D Rendering Engine
 */

#ifndef SHADING_PRE_INTEGRATED_SSS_H
#define SHADING_PRE_INTEGRATED_SSS_H

#include <stdint.h>
#include <stdbool.h>
#include <stddef.h>

#ifdef __cplusplus
extern "C" {
#endif

/* ============================================================================
 * TYPES
 * ============================================================================ */

typedef struct shading_pre_integrated_sss_handle {
    uint32_t id;
} shading_pre_integrated_sss_handle_t;

typedef struct shading_pre_integrated_sss_desc {
    uint32_t flags;
    void* user_data;
} shading_pre_integrated_sss_desc_t;

typedef struct shading_pre_integrated_sss_info {
    uint32_t id;
    uint32_t flags;
    bool initialized;
} shading_pre_integrated_sss_info_t;

/* ============================================================================
 * API
 * ============================================================================ */

/* Initialization */
int shading_pre_integrated_sss_init(void);
void shading_pre_integrated_sss_shutdown(void);

/* Lifecycle */
int shading_pre_integrated_sss_create(shading_pre_integrated_sss_handle_t* out_handle, const shading_pre_integrated_sss_desc_t* desc);
void shading_pre_integrated_sss_destroy(shading_pre_integrated_sss_handle_t handle);

/* Operations */
int shading_pre_integrated_sss_update(shading_pre_integrated_sss_handle_t handle, const void* data, size_t size);
bool shading_pre_integrated_sss_is_valid(shading_pre_integrated_sss_handle_t handle);
int shading_pre_integrated_sss_get_info(shading_pre_integrated_sss_handle_t handle, shading_pre_integrated_sss_info_t* out_info);
void shading_pre_integrated_sss_mark_dirty(shading_pre_integrated_sss_handle_t handle);
int shading_pre_integrated_sss_process_pending(void);

/* Statistics */
uint32_t shading_pre_integrated_sss_get_count(void);
size_t shading_pre_integrated_sss_get_memory_usage(void);
void shading_pre_integrated_sss_debug_print(void);

#ifdef __cplusplus
}
#endif

#endif /* SHADING_PRE_INTEGRATED_SSS_H */
