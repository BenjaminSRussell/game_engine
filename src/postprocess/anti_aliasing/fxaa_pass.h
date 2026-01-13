/*
 * fxaa_pass.h
 * FXAA post-process
 *
 * Part of the Postprocessing subsystem
 * Advanced 3D Rendering Engine
 */

#ifndef POSTPROCESSING_FXAA_PASS_H
#define POSTPROCESSING_FXAA_PASS_H

#include <stdint.h>
#include <stdbool.h>
#include <stddef.h>

#ifdef __cplusplus
extern "C" {
#endif

/* ============================================================================
 * TYPES
 * ============================================================================ */

typedef struct postprocessing_fxaa_pass_handle {
    uint32_t id;
} postprocessing_fxaa_pass_handle_t;

typedef struct postprocessing_fxaa_pass_desc {
    uint32_t flags;
    void* user_data;
} postprocessing_fxaa_pass_desc_t;

typedef struct postprocessing_fxaa_pass_info {
    uint32_t id;
    uint32_t flags;
    bool initialized;
} postprocessing_fxaa_pass_info_t;

/* ============================================================================
 * API
 * ============================================================================ */

/* Initialization */
int postprocessing_fxaa_pass_init(void);
void postprocessing_fxaa_pass_shutdown(void);

/* Lifecycle */
int postprocessing_fxaa_pass_create(postprocessing_fxaa_pass_handle_t* out_handle, const postprocessing_fxaa_pass_desc_t* desc);
void postprocessing_fxaa_pass_destroy(postprocessing_fxaa_pass_handle_t handle);

/* Operations */
int postprocessing_fxaa_pass_update(postprocessing_fxaa_pass_handle_t handle, const void* data, size_t size);
bool postprocessing_fxaa_pass_is_valid(postprocessing_fxaa_pass_handle_t handle);
int postprocessing_fxaa_pass_get_info(postprocessing_fxaa_pass_handle_t handle, postprocessing_fxaa_pass_info_t* out_info);
void postprocessing_fxaa_pass_mark_dirty(postprocessing_fxaa_pass_handle_t handle);
int postprocessing_fxaa_pass_process_pending(void);

/* Statistics */
uint32_t postprocessing_fxaa_pass_get_count(void);
size_t postprocessing_fxaa_pass_get_memory_usage(void);
void postprocessing_fxaa_pass_debug_print(void);

#ifdef __cplusplus
}
#endif

#endif /* POSTPROCESSING_FXAA_PASS_H */
