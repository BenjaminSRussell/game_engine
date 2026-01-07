/*
 * occlusion_query.h
 * Occlusion query
 *
 * Part of the Occlusion subsystem
 * Advanced 3D Rendering Engine
 */

#ifndef OCCLUSION_OCCLUSION_QUERY_H
#define OCCLUSION_OCCLUSION_QUERY_H

#include <stdint.h>
#include <stdbool.h>
#include <stddef.h>

#ifdef __cplusplus
extern "C" {
#endif

/* ============================================================================
 * TYPES
 * ============================================================================ */

typedef struct occlusion_occlusion_query_handle {
    uint32_t id;
} occlusion_occlusion_query_handle_t;

typedef struct occlusion_occlusion_query_desc {
    uint32_t flags;
    void* user_data;
} occlusion_occlusion_query_desc_t;

typedef struct occlusion_occlusion_query_info {
    uint32_t id;
    uint32_t flags;
    bool initialized;
} occlusion_occlusion_query_info_t;

/* ============================================================================
 * API
 * ============================================================================ */

/* Initialization */
int occlusion_occlusion_query_init(void);
void occlusion_occlusion_query_shutdown(void);

/* Lifecycle */
int occlusion_occlusion_query_create(occlusion_occlusion_query_handle_t* out_handle, const occlusion_occlusion_query_desc_t* desc);
void occlusion_occlusion_query_destroy(occlusion_occlusion_query_handle_t handle);

/* Operations */
int occlusion_occlusion_query_update(occlusion_occlusion_query_handle_t handle, const void* data, size_t size);
bool occlusion_occlusion_query_is_valid(occlusion_occlusion_query_handle_t handle);
int occlusion_occlusion_query_get_info(occlusion_occlusion_query_handle_t handle, occlusion_occlusion_query_info_t* out_info);
void occlusion_occlusion_query_mark_dirty(occlusion_occlusion_query_handle_t handle);
int occlusion_occlusion_query_process_pending(void);

/* Statistics */
uint32_t occlusion_occlusion_query_get_count(void);
size_t occlusion_occlusion_query_get_memory_usage(void);
void occlusion_occlusion_query_debug_print(void);

#ifdef __cplusplus
}
#endif

#endif /* OCCLUSION_OCCLUSION_QUERY_H */
