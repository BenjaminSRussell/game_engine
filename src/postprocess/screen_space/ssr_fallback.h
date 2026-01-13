/*
 * ssr_fallback.h
 * SSR probe fallback
 *
 * Part of the Postprocessing subsystem
 * Advanced 3D Rendering Engine
 */

#ifndef POSTPROCESSING_SSR_FALLBACK_H
#define POSTPROCESSING_SSR_FALLBACK_H

#include <stdint.h>
#include <stdbool.h>
#include <stddef.h>

#ifdef __cplusplus
extern "C" {
#endif

/* ============================================================================
 * TYPES
 * ============================================================================ */

typedef struct postprocessing_ssr_fallback_handle {
    uint32_t id;
} postprocessing_ssr_fallback_handle_t;

typedef struct postprocessing_ssr_fallback_desc {
    uint32_t flags;
    void* user_data;
} postprocessing_ssr_fallback_desc_t;

typedef struct postprocessing_ssr_fallback_info {
    uint32_t id;
    uint32_t flags;
    bool initialized;
} postprocessing_ssr_fallback_info_t;

/* ============================================================================
 * API
 * ============================================================================ */

/* Initialization */
int postprocessing_ssr_fallback_init(void);
void postprocessing_ssr_fallback_shutdown(void);

/* Lifecycle */
int postprocessing_ssr_fallback_create(postprocessing_ssr_fallback_handle_t* out_handle, const postprocessing_ssr_fallback_desc_t* desc);
void postprocessing_ssr_fallback_destroy(postprocessing_ssr_fallback_handle_t handle);

/* Operations */
int postprocessing_ssr_fallback_update(postprocessing_ssr_fallback_handle_t handle, const void* data, size_t size);
bool postprocessing_ssr_fallback_is_valid(postprocessing_ssr_fallback_handle_t handle);
int postprocessing_ssr_fallback_get_info(postprocessing_ssr_fallback_handle_t handle, postprocessing_ssr_fallback_info_t* out_info);
void postprocessing_ssr_fallback_mark_dirty(postprocessing_ssr_fallback_handle_t handle);
int postprocessing_ssr_fallback_process_pending(void);

/* Statistics */
uint32_t postprocessing_ssr_fallback_get_count(void);
size_t postprocessing_ssr_fallback_get_memory_usage(void);
void postprocessing_ssr_fallback_debug_print(void);

#ifdef __cplusplus
}
#endif

#endif /* POSTPROCESSING_SSR_FALLBACK_H */
