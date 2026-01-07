/*
 * hbao_pass.h
 * Horizon-based AO
 *
 * Part of the Postprocessing subsystem
 * Advanced 3D Rendering Engine
 */

#ifndef POSTPROCESSING_HBAO_PASS_H
#define POSTPROCESSING_HBAO_PASS_H

#include <stdint.h>
#include <stdbool.h>
#include <stddef.h>

#ifdef __cplusplus
extern "C" {
#endif

/* ============================================================================
 * TYPES
 * ============================================================================ */

typedef struct postprocessing_hbao_pass_handle {
    uint32_t id;
} postprocessing_hbao_pass_handle_t;

typedef struct postprocessing_hbao_pass_desc {
    uint32_t flags;
    void* user_data;
} postprocessing_hbao_pass_desc_t;

typedef struct postprocessing_hbao_pass_info {
    uint32_t id;
    uint32_t flags;
    bool initialized;
} postprocessing_hbao_pass_info_t;

/* ============================================================================
 * API
 * ============================================================================ */

/* Initialization */
int postprocessing_hbao_pass_init(void);
void postprocessing_hbao_pass_shutdown(void);

/* Lifecycle */
int postprocessing_hbao_pass_create(postprocessing_hbao_pass_handle_t* out_handle, const postprocessing_hbao_pass_desc_t* desc);
void postprocessing_hbao_pass_destroy(postprocessing_hbao_pass_handle_t handle);

/* Operations */
int postprocessing_hbao_pass_update(postprocessing_hbao_pass_handle_t handle, const void* data, size_t size);
bool postprocessing_hbao_pass_is_valid(postprocessing_hbao_pass_handle_t handle);
int postprocessing_hbao_pass_get_info(postprocessing_hbao_pass_handle_t handle, postprocessing_hbao_pass_info_t* out_info);
void postprocessing_hbao_pass_mark_dirty(postprocessing_hbao_pass_handle_t handle);
int postprocessing_hbao_pass_process_pending(void);

/* Statistics */
uint32_t postprocessing_hbao_pass_get_count(void);
size_t postprocessing_hbao_pass_get_memory_usage(void);
void postprocessing_hbao_pass_debug_print(void);

#ifdef __cplusplus
}
#endif

#endif /* POSTPROCESSING_HBAO_PASS_H */
