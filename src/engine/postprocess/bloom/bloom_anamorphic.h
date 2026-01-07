/*
 * bloom_anamorphic.h
 * Anamorphic bloom
 *
 * Part of the Postprocessing subsystem
 * Advanced 3D Rendering Engine
 */

#ifndef POSTPROCESSING_BLOOM_ANAMORPHIC_H
#define POSTPROCESSING_BLOOM_ANAMORPHIC_H

#include <stdint.h>
#include <stdbool.h>
#include <stddef.h>

#ifdef __cplusplus
extern "C" {
#endif

/* ============================================================================
 * TYPES
 * ============================================================================ */

typedef struct postprocessing_bloom_anamorphic_handle {
    uint32_t id;
} postprocessing_bloom_anamorphic_handle_t;

typedef struct postprocessing_bloom_anamorphic_desc {
    uint32_t flags;
    void* user_data;
} postprocessing_bloom_anamorphic_desc_t;

typedef struct postprocessing_bloom_anamorphic_info {
    uint32_t id;
    uint32_t flags;
    bool initialized;
} postprocessing_bloom_anamorphic_info_t;

/* ============================================================================
 * API
 * ============================================================================ */

/* Initialization */
int postprocessing_bloom_anamorphic_init(void);
void postprocessing_bloom_anamorphic_shutdown(void);

/* Lifecycle */
int postprocessing_bloom_anamorphic_create(postprocessing_bloom_anamorphic_handle_t* out_handle, const postprocessing_bloom_anamorphic_desc_t* desc);
void postprocessing_bloom_anamorphic_destroy(postprocessing_bloom_anamorphic_handle_t handle);

/* Operations */
int postprocessing_bloom_anamorphic_update(postprocessing_bloom_anamorphic_handle_t handle, const void* data, size_t size);
bool postprocessing_bloom_anamorphic_is_valid(postprocessing_bloom_anamorphic_handle_t handle);
int postprocessing_bloom_anamorphic_get_info(postprocessing_bloom_anamorphic_handle_t handle, postprocessing_bloom_anamorphic_info_t* out_info);
void postprocessing_bloom_anamorphic_mark_dirty(postprocessing_bloom_anamorphic_handle_t handle);
int postprocessing_bloom_anamorphic_process_pending(void);

/* Statistics */
uint32_t postprocessing_bloom_anamorphic_get_count(void);
size_t postprocessing_bloom_anamorphic_get_memory_usage(void);
void postprocessing_bloom_anamorphic_debug_print(void);

#ifdef __cplusplus
}
#endif

#endif /* POSTPROCESSING_BLOOM_ANAMORPHIC_H */
