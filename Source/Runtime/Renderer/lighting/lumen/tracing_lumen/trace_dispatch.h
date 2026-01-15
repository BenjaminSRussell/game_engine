/*
 * trace_dispatch.h
 * Trace dispatch
 *
 * Part of the Lumen subsystem
 * Advanced 3D Rendering Engine
 */

#ifndef LUMEN_TRACE_DISPATCH_H
#define LUMEN_TRACE_DISPATCH_H

#include <stdint.h>
#include <stdbool.h>
#include <stddef.h>

#ifdef __cplusplus
extern "C" {
#endif

/* ============================================================================
 * TYPES
 * ============================================================================ */

typedef struct lumen_trace_dispatch_handle {
    uint32_t id;
} lumen_trace_dispatch_handle_t;

typedef struct lumen_trace_dispatch_desc {
    uint32_t flags;
    void* user_data;
} lumen_trace_dispatch_desc_t;

typedef struct lumen_trace_dispatch_info {
    uint32_t id;
    uint32_t flags;
    bool initialized;
} lumen_trace_dispatch_info_t;

/* ============================================================================
 * API
 * ============================================================================ */

/* Initialization */
int lumen_trace_dispatch_init(void);
void lumen_trace_dispatch_shutdown(void);

/* Lifecycle */
int lumen_trace_dispatch_create(lumen_trace_dispatch_handle_t* out_handle, const lumen_trace_dispatch_desc_t* desc);
void lumen_trace_dispatch_destroy(lumen_trace_dispatch_handle_t handle);

/* Operations */
int lumen_trace_dispatch_update(lumen_trace_dispatch_handle_t handle, const void* data, size_t size);
bool lumen_trace_dispatch_is_valid(lumen_trace_dispatch_handle_t handle);
int lumen_trace_dispatch_get_info(lumen_trace_dispatch_handle_t handle, lumen_trace_dispatch_info_t* out_info);
void lumen_trace_dispatch_mark_dirty(lumen_trace_dispatch_handle_t handle);
int lumen_trace_dispatch_process_pending(void);

/* Statistics */
uint32_t lumen_trace_dispatch_get_count(void);
size_t lumen_trace_dispatch_get_memory_usage(void);
void lumen_trace_dispatch_debug_print(void);

#ifdef __cplusplus
}
#endif

#endif /* LUMEN_TRACE_DISPATCH_H */
