/*
 * sdsm.h
 * Sample distribution shadows
 *
 * Part of the Shadows Advanced subsystem
 * Advanced 3D Rendering Engine
 */

#ifndef SHADOWS_ADVANCED_SDSM_H
#define SHADOWS_ADVANCED_SDSM_H

#include <stdint.h>
#include <stdbool.h>
#include <stddef.h>

#ifdef __cplusplus
extern "C" {
#endif

/* ============================================================================
 * TYPES
 * ============================================================================ */

typedef struct shadows_advanced_sdsm_handle {
    uint32_t id;
} shadows_advanced_sdsm_handle_t;

typedef struct shadows_advanced_sdsm_desc {
    uint32_t flags;
    void* user_data;
} shadows_advanced_sdsm_desc_t;

typedef struct shadows_advanced_sdsm_info {
    uint32_t id;
    uint32_t flags;
    bool initialized;
} shadows_advanced_sdsm_info_t;

/* ============================================================================
 * API
 * ============================================================================ */

/* Initialization */
int shadows_advanced_sdsm_init(void);
void shadows_advanced_sdsm_shutdown(void);

/* Lifecycle */
int shadows_advanced_sdsm_create(shadows_advanced_sdsm_handle_t* out_handle, const shadows_advanced_sdsm_desc_t* desc);
void shadows_advanced_sdsm_destroy(shadows_advanced_sdsm_handle_t handle);

/* Operations */
int shadows_advanced_sdsm_update(shadows_advanced_sdsm_handle_t handle, const void* data, size_t size);
bool shadows_advanced_sdsm_is_valid(shadows_advanced_sdsm_handle_t handle);
int shadows_advanced_sdsm_get_info(shadows_advanced_sdsm_handle_t handle, shadows_advanced_sdsm_info_t* out_info);
void shadows_advanced_sdsm_mark_dirty(shadows_advanced_sdsm_handle_t handle);
int shadows_advanced_sdsm_process_pending(void);

/* Statistics */
uint32_t shadows_advanced_sdsm_get_count(void);
size_t shadows_advanced_sdsm_get_memory_usage(void);
void shadows_advanced_sdsm_debug_print(void);

#ifdef __cplusplus
}
#endif

#endif /* SHADOWS_ADVANCED_SDSM_H */
