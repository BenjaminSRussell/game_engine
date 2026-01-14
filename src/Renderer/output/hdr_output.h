/*
 * hdr_output.h
 * HDR display output
 *
 * Part of the Rendering subsystem
 * Advanced 3D Rendering Engine
 */

#ifndef RENDERING_HDR_OUTPUT_H
#define RENDERING_HDR_OUTPUT_H

#include <stdint.h>
#include <stdbool.h>
#include <stddef.h>

#ifdef __cplusplus
extern "C" {
#endif

/* ============================================================================
 * TYPES
 * ============================================================================ */

typedef struct rendering_hdr_output_handle {
    uint32_t id;
} rendering_hdr_output_handle_t;

typedef struct rendering_hdr_output_desc {
    uint32_t flags;
    void* user_data;
} rendering_hdr_output_desc_t;

typedef struct rendering_hdr_output_info {
    uint32_t id;
    uint32_t flags;
    bool initialized;
} rendering_hdr_output_info_t;

/* ============================================================================
 * API
 * ============================================================================ */

/* Initialization */
int rendering_hdr_output_init(void);
void rendering_hdr_output_shutdown(void);

/* Lifecycle */
int rendering_hdr_output_create(rendering_hdr_output_handle_t* out_handle, const rendering_hdr_output_desc_t* desc);
void rendering_hdr_output_destroy(rendering_hdr_output_handle_t handle);

/* Operations */
int rendering_hdr_output_update(rendering_hdr_output_handle_t handle, const void* data, size_t size);
bool rendering_hdr_output_is_valid(rendering_hdr_output_handle_t handle);
int rendering_hdr_output_get_info(rendering_hdr_output_handle_t handle, rendering_hdr_output_info_t* out_info);
void rendering_hdr_output_mark_dirty(rendering_hdr_output_handle_t handle);
int rendering_hdr_output_process_pending(void);

/* Statistics */
uint32_t rendering_hdr_output_get_count(void);
size_t rendering_hdr_output_get_memory_usage(void);
void rendering_hdr_output_debug_print(void);

#ifdef __cplusplus
}
#endif

#endif /* RENDERING_HDR_OUTPUT_H */
