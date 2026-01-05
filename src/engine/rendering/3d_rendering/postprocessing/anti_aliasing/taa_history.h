/*
 * taa_history.h
 * TAA history management
 *
 * Part of the Postprocessing subsystem
 * Advanced 3D Rendering Engine
 */

#ifndef POSTPROCESSING_TAA_HISTORY_H
#define POSTPROCESSING_TAA_HISTORY_H

#include <stdint.h>
#include <stdbool.h>
#include <stddef.h>

#ifdef __cplusplus
extern "C" {
#endif

/* ============================================================================
 * TYPES
 * ============================================================================ */

typedef struct postprocessing_taa_history_handle {
    uint32_t id;
} postprocessing_taa_history_handle_t;

typedef struct postprocessing_taa_history_desc {
    uint32_t flags;
    void* user_data;
} postprocessing_taa_history_desc_t;

typedef struct postprocessing_taa_history_info {
    uint32_t id;
    uint32_t flags;
    bool initialized;
} postprocessing_taa_history_info_t;

/* ============================================================================
 * API
 * ============================================================================ */

/* Initialization */
int postprocessing_taa_history_init(void);
void postprocessing_taa_history_shutdown(void);

/* Lifecycle */
int postprocessing_taa_history_create(postprocessing_taa_history_handle_t* out_handle, const postprocessing_taa_history_desc_t* desc);
void postprocessing_taa_history_destroy(postprocessing_taa_history_handle_t handle);

/* Operations */
int postprocessing_taa_history_update(postprocessing_taa_history_handle_t handle, const void* data, size_t size);
bool postprocessing_taa_history_is_valid(postprocessing_taa_history_handle_t handle);
int postprocessing_taa_history_get_info(postprocessing_taa_history_handle_t handle, postprocessing_taa_history_info_t* out_info);
void postprocessing_taa_history_mark_dirty(postprocessing_taa_history_handle_t handle);
int postprocessing_taa_history_process_pending(void);

/* Statistics */
uint32_t postprocessing_taa_history_get_count(void);
size_t postprocessing_taa_history_get_memory_usage(void);
void postprocessing_taa_history_debug_print(void);

#ifdef __cplusplus
}
#endif

#endif /* POSTPROCESSING_TAA_HISTORY_H */
