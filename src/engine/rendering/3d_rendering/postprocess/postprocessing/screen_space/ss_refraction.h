/*
 * ss_refraction.h
 * Screen-space refraction
 *
 * Part of the Postprocessing subsystem
 * Advanced 3D Rendering Engine
 */

#ifndef POSTPROCESSING_SS_REFRACTION_H
#define POSTPROCESSING_SS_REFRACTION_H

#include <stdint.h>
#include <stdbool.h>
#include <stddef.h>

#ifdef __cplusplus
extern "C" {
#endif

/* ============================================================================
 * TYPES
 * ============================================================================ */

typedef struct postprocessing_ss_refraction_handle {
    uint32_t id;
} postprocessing_ss_refraction_handle_t;

typedef struct postprocessing_ss_refraction_desc {
    uint32_t flags;
    void* user_data;
} postprocessing_ss_refraction_desc_t;

typedef struct postprocessing_ss_refraction_info {
    uint32_t id;
    uint32_t flags;
    bool initialized;
} postprocessing_ss_refraction_info_t;

/* ============================================================================
 * API
 * ============================================================================ */

/* Initialization */
int postprocessing_ss_refraction_init(void);
void postprocessing_ss_refraction_shutdown(void);

/* Lifecycle */
int postprocessing_ss_refraction_create(postprocessing_ss_refraction_handle_t* out_handle, const postprocessing_ss_refraction_desc_t* desc);
void postprocessing_ss_refraction_destroy(postprocessing_ss_refraction_handle_t handle);

/* Operations */
int postprocessing_ss_refraction_update(postprocessing_ss_refraction_handle_t handle, const void* data, size_t size);
bool postprocessing_ss_refraction_is_valid(postprocessing_ss_refraction_handle_t handle);
int postprocessing_ss_refraction_get_info(postprocessing_ss_refraction_handle_t handle, postprocessing_ss_refraction_info_t* out_info);
void postprocessing_ss_refraction_mark_dirty(postprocessing_ss_refraction_handle_t handle);
int postprocessing_ss_refraction_process_pending(void);

/* Statistics */
uint32_t postprocessing_ss_refraction_get_count(void);
size_t postprocessing_ss_refraction_get_memory_usage(void);
void postprocessing_ss_refraction_debug_print(void);

#ifdef __cplusplus
}
#endif

#endif /* POSTPROCESSING_SS_REFRACTION_H */
