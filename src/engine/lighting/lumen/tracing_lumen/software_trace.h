/*
 * software_trace.h
 * Software ray tracing
 *
 * Part of the Lumen subsystem
 * Advanced 3D Rendering Engine
 */

#ifndef LUMEN_SOFTWARE_TRACE_H
#define LUMEN_SOFTWARE_TRACE_H

#include <stdint.h>
#include <stdbool.h>
#include <stddef.h>

#ifdef __cplusplus
extern "C" {
#endif

/* ============================================================================
 * TYPES
 * ============================================================================ */

typedef struct lumen_software_trace_handle {
    uint32_t id;
} lumen_software_trace_handle_t;

typedef struct lumen_software_trace_desc {
    uint32_t flags;
    void* user_data;
} lumen_software_trace_desc_t;

typedef struct lumen_software_trace_info {
    uint32_t id;
    uint32_t flags;
    bool initialized;
} lumen_software_trace_info_t;

/* ============================================================================
 * API
 * ============================================================================ */

/* Initialization */
int lumen_software_trace_init(void);
void lumen_software_trace_shutdown(void);

/* Lifecycle */
int lumen_software_trace_create(lumen_software_trace_handle_t* out_handle, const lumen_software_trace_desc_t* desc);
void lumen_software_trace_destroy(lumen_software_trace_handle_t handle);

/* Operations */
int lumen_software_trace_update(lumen_software_trace_handle_t handle, const void* data, size_t size);
bool lumen_software_trace_is_valid(lumen_software_trace_handle_t handle);
int lumen_software_trace_get_info(lumen_software_trace_handle_t handle, lumen_software_trace_info_t* out_info);
void lumen_software_trace_mark_dirty(lumen_software_trace_handle_t handle);
int lumen_software_trace_process_pending(void);

/* Statistics */
uint32_t lumen_software_trace_get_count(void);
size_t lumen_software_trace_get_memory_usage(void);
void lumen_software_trace_debug_print(void);

#ifdef __cplusplus
}
#endif

#endif /* LUMEN_SOFTWARE_TRACE_H */
