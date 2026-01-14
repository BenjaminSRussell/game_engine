/*
 * ssr_trace.h
 * Screen-space reflection trace
 *
 * Part of the Postprocessing subsystem
 * Advanced 3D Rendering Engine
 */

#ifndef POSTPROCESSING_SSR_TRACE_H
#define POSTPROCESSING_SSR_TRACE_H

#include <stdint.h>
#include <stdbool.h>
#include <stddef.h>

#ifdef __cplusplus
extern "C" {
#endif

/* ============================================================================
 * TYPES
 * ============================================================================ */

typedef struct postprocessing_ssr_trace_handle {
    uint32_t id;
} postprocessing_ssr_trace_handle_t;

typedef struct postprocessing_ssr_trace_desc {
    uint32_t flags;
    void* user_data;
} postprocessing_ssr_trace_desc_t;

typedef struct postprocessing_ssr_trace_info {
    uint32_t id;
    uint32_t flags;
    bool initialized;
} postprocessing_ssr_trace_info_t;

/* ============================================================================
 * API
 * ============================================================================ */

/* Initialization */
int postprocessing_ssr_trace_init(void);
void postprocessing_ssr_trace_shutdown(void);

/* Lifecycle */
int postprocessing_ssr_trace_create(postprocessing_ssr_trace_handle_t* out_handle, const postprocessing_ssr_trace_desc_t* desc);
void postprocessing_ssr_trace_destroy(postprocessing_ssr_trace_handle_t handle);

/* Operations */
int postprocessing_ssr_trace_update(postprocessing_ssr_trace_handle_t handle, const void* data, size_t size);
bool postprocessing_ssr_trace_is_valid(postprocessing_ssr_trace_handle_t handle);
int postprocessing_ssr_trace_get_info(postprocessing_ssr_trace_handle_t handle, postprocessing_ssr_trace_info_t* out_info);
void postprocessing_ssr_trace_mark_dirty(postprocessing_ssr_trace_handle_t handle);
int postprocessing_ssr_trace_process_pending(void);

/* Statistics */
uint32_t postprocessing_ssr_trace_get_count(void);
size_t postprocessing_ssr_trace_get_memory_usage(void);
void postprocessing_ssr_trace_debug_print(void);

#ifdef __cplusplus
}
#endif

#endif /* POSTPROCESSING_SSR_TRACE_H */
