/*
 * screen_trace.h
 * Screen-space tracing
 *
 * Part of the Lumen subsystem
 * Advanced 3D Rendering Engine
 */

#ifndef LUMEN_SCREEN_TRACE_H
#define LUMEN_SCREEN_TRACE_H

#include <stdint.h>
#include <stdbool.h>
#include <stddef.h>

#ifdef __cplusplus
extern "C" {
#endif

/* ============================================================================
 * TYPES
 * ============================================================================ */

typedef struct lumen_screen_trace_handle {
    uint32_t id;
} lumen_screen_trace_handle_t;

typedef struct lumen_screen_trace_desc {
    uint32_t flags;
    void* user_data;
} lumen_screen_trace_desc_t;

typedef struct lumen_screen_trace_info {
    uint32_t id;
    uint32_t flags;
    bool initialized;
} lumen_screen_trace_info_t;

/* ============================================================================
 * API
 * ============================================================================ */

/* Initialization */
int lumen_screen_trace_init(void);
void lumen_screen_trace_shutdown(void);

/* Lifecycle */
int lumen_screen_trace_create(lumen_screen_trace_handle_t* out_handle, const lumen_screen_trace_desc_t* desc);
void lumen_screen_trace_destroy(lumen_screen_trace_handle_t handle);

/* Operations */
int lumen_screen_trace_update(lumen_screen_trace_handle_t handle, const void* data, size_t size);
bool lumen_screen_trace_is_valid(lumen_screen_trace_handle_t handle);
int lumen_screen_trace_get_info(lumen_screen_trace_handle_t handle, lumen_screen_trace_info_t* out_info);
void lumen_screen_trace_mark_dirty(lumen_screen_trace_handle_t handle);
int lumen_screen_trace_process_pending(void);

/* Statistics */
uint32_t lumen_screen_trace_get_count(void);
size_t lumen_screen_trace_get_memory_usage(void);
void lumen_screen_trace_debug_print(void);

#ifdef __cplusplus
}
#endif

#endif /* LUMEN_SCREEN_TRACE_H */
