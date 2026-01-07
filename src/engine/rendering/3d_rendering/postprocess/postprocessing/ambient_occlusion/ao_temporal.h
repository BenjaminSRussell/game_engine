/*
 * ao_temporal.h
 * AO temporal filter
 *
 * Part of the Postprocessing subsystem
 * Advanced 3D Rendering Engine
 */

#ifndef POSTPROCESSING_AO_TEMPORAL_H
#define POSTPROCESSING_AO_TEMPORAL_H

#include <stdint.h>
#include <stdbool.h>
#include <stddef.h>

#ifdef __cplusplus
extern "C" {
#endif

/* ============================================================================
 * TYPES
 * ============================================================================ */

typedef struct postprocessing_ao_temporal_handle {
    uint32_t id;
} postprocessing_ao_temporal_handle_t;

typedef struct postprocessing_ao_temporal_desc {
    uint32_t flags;
    void* user_data;
} postprocessing_ao_temporal_desc_t;

typedef struct postprocessing_ao_temporal_info {
    uint32_t id;
    uint32_t flags;
    bool initialized;
} postprocessing_ao_temporal_info_t;

/* ============================================================================
 * API
 * ============================================================================ */

/* Initialization */
int postprocessing_ao_temporal_init(void);
void postprocessing_ao_temporal_shutdown(void);

/* Lifecycle */
int postprocessing_ao_temporal_create(postprocessing_ao_temporal_handle_t* out_handle, const postprocessing_ao_temporal_desc_t* desc);
void postprocessing_ao_temporal_destroy(postprocessing_ao_temporal_handle_t handle);

/* Operations */
int postprocessing_ao_temporal_update(postprocessing_ao_temporal_handle_t handle, const void* data, size_t size);
bool postprocessing_ao_temporal_is_valid(postprocessing_ao_temporal_handle_t handle);
int postprocessing_ao_temporal_get_info(postprocessing_ao_temporal_handle_t handle, postprocessing_ao_temporal_info_t* out_info);
void postprocessing_ao_temporal_mark_dirty(postprocessing_ao_temporal_handle_t handle);
int postprocessing_ao_temporal_process_pending(void);

/* Statistics */
uint32_t postprocessing_ao_temporal_get_count(void);
size_t postprocessing_ao_temporal_get_memory_usage(void);
void postprocessing_ao_temporal_debug_print(void);

#ifdef __cplusplus
}
#endif

#endif /* POSTPROCESSING_AO_TEMPORAL_H */
