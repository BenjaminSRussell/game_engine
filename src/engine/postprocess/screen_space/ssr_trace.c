/*
 * ssr_trace.c
 * Legacy stub - SSR implementation moved to postprocess/ssr/
 *
 * NOTE: The main SSR implementation is now located at:
 * src/engine/rendering/3d_rendering/postprocess/ssr/ssr_trace.m
 * src/engine/rendering/3d_rendering/postprocess/ssr/ssr.metal
 *
 * This file is kept for API compatibility with the postprocessing subsystem.
 * For new code, use the implementation in postprocess/ssr/ directly.
 */

#include "postprocess/screen_space/ssr_trace.h"
#include <stdio.h>

// Forward to main implementation
// In your build system, link to: ../../../postprocess/ssr/ssr_trace.m

#define SSR_TRACE_DEPRECATED \
    printf("Warning: Using deprecated SSR stub. Use postprocess/ssr/ssr_trace.m instead\\n");

/* Stub implementations - these maintain API compatibility but should not be used */

int postprocessing_ssr_trace_init(void) {
    SSR_TRACE_DEPRECATED
    return 0;
}

void postprocessing_ssr_trace_shutdown(void) {
    SSR_TRACE_DEPRECATED
}

int postprocessing_ssr_trace_create(postprocessing_ssr_trace_handle_t* out_handle, const postprocessing_ssr_trace_desc_t* desc) {
    SSR_TRACE_DEPRECATED
    if (out_handle) out_handle->id = 0;
    return 0;
}

void postprocessing_ssr_trace_destroy(postprocessing_ssr_trace_handle_t handle) {
    SSR_TRACE_DEPRECATED
}

int postprocessing_ssr_trace_update(postprocessing_ssr_trace_handle_t handle, const void* data, size_t size) {
    SSR_TRACE_DEPRECATED
    return 0;
}

bool postprocessing_ssr_trace_is_valid(postprocessing_ssr_trace_handle_t handle) {
    SSR_TRACE_DEPRECATED
    return false;
}

int postprocessing_ssr_trace_get_info(postprocessing_ssr_trace_handle_t handle, postprocessing_ssr_trace_info_t* out_info) {
    SSR_TRACE_DEPRECATED
    return -1;
}

void postprocessing_ssr_trace_mark_dirty(postprocessing_ssr_trace_handle_t handle) {
    SSR_TRACE_DEPRECATED
}

int postprocessing_ssr_trace_process_pending(void) {
    SSR_TRACE_DEPRECATED
    return 0;
}

uint32_t postprocessing_ssr_trace_get_count(void) {
    SSR_TRACE_DEPRECATED
    return 0;
}

size_t postprocessing_ssr_trace_get_memory_usage(void) {
    SSR_TRACE_DEPRECATED
    return 0;
}

void postprocessing_ssr_trace_debug_print(void) {
    SSR_TRACE_DEPRECATED
}

/* End of ssr_trace.c */
