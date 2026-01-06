/*
 * hdr_encoding.h
 * HDR output encoding
 *
 * Part of the Postprocessing subsystem
 * Advanced 3D Rendering Engine
 */

#ifndef POSTPROCESSING_HDR_ENCODING_H
#define POSTPROCESSING_HDR_ENCODING_H

#include <stdint.h>
#include <stdbool.h>
#include <stddef.h>

#ifdef __cplusplus
extern "C" {
#endif

/* ============================================================================
 * TYPES
 * ============================================================================ */

typedef struct postprocessing_hdr_encoding_handle {
    uint32_t id;
} postprocessing_hdr_encoding_handle_t;

typedef struct postprocessing_hdr_encoding_desc {
    uint32_t flags;
    void* user_data;
} postprocessing_hdr_encoding_desc_t;

typedef struct postprocessing_hdr_encoding_info {
    uint32_t id;
    uint32_t flags;
    bool initialized;
} postprocessing_hdr_encoding_info_t;

/* ============================================================================
 * API
 * ============================================================================ */

/* Initialization */
int postprocessing_hdr_encoding_init(void);
void postprocessing_hdr_encoding_shutdown(void);

/* Lifecycle */
int postprocessing_hdr_encoding_create(postprocessing_hdr_encoding_handle_t* out_handle, const postprocessing_hdr_encoding_desc_t* desc);
void postprocessing_hdr_encoding_destroy(postprocessing_hdr_encoding_handle_t handle);

/* Operations */
int postprocessing_hdr_encoding_update(postprocessing_hdr_encoding_handle_t handle, const void* data, size_t size);
bool postprocessing_hdr_encoding_is_valid(postprocessing_hdr_encoding_handle_t handle);
int postprocessing_hdr_encoding_get_info(postprocessing_hdr_encoding_handle_t handle, postprocessing_hdr_encoding_info_t* out_info);
void postprocessing_hdr_encoding_mark_dirty(postprocessing_hdr_encoding_handle_t handle);
int postprocessing_hdr_encoding_process_pending(void);

/* Statistics */
uint32_t postprocessing_hdr_encoding_get_count(void);
size_t postprocessing_hdr_encoding_get_memory_usage(void);
void postprocessing_hdr_encoding_debug_print(void);

#ifdef __cplusplus
}
#endif

#endif /* POSTPROCESSING_HDR_ENCODING_H */
