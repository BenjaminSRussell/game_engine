/*
 * taa_velocity.h
 * TAA velocity buffer
 *
 * Part of the Postprocessing subsystem
 * Advanced 3D Rendering Engine
 */

#ifndef POSTPROCESSING_TAA_VELOCITY_H
#define POSTPROCESSING_TAA_VELOCITY_H

#include <stdint.h>
#include <stdbool.h>
#include <stddef.h>

#ifdef __cplusplus
extern "C" {
#endif

/* ============================================================================
 * TYPES
 * ============================================================================ */

typedef struct postprocessing_taa_velocity_handle {
    uint32_t id;
} postprocessing_taa_velocity_handle_t;

typedef struct postprocessing_taa_velocity_desc {
    uint32_t flags;
    void* user_data;
} postprocessing_taa_velocity_desc_t;

typedef struct postprocessing_taa_velocity_info {
    uint32_t id;
    uint32_t flags;
    bool initialized;
} postprocessing_taa_velocity_info_t;

/* ============================================================================
 * API
 * ============================================================================ */

/* Initialization */
int postprocessing_taa_velocity_init(void);
void postprocessing_taa_velocity_shutdown(void);

/* Lifecycle */
int postprocessing_taa_velocity_create(postprocessing_taa_velocity_handle_t* out_handle, const postprocessing_taa_velocity_desc_t* desc);
void postprocessing_taa_velocity_destroy(postprocessing_taa_velocity_handle_t handle);

/* Operations */
int postprocessing_taa_velocity_update(postprocessing_taa_velocity_handle_t handle, const void* data, size_t size);
bool postprocessing_taa_velocity_is_valid(postprocessing_taa_velocity_handle_t handle);
int postprocessing_taa_velocity_get_info(postprocessing_taa_velocity_handle_t handle, postprocessing_taa_velocity_info_t* out_info);
void postprocessing_taa_velocity_mark_dirty(postprocessing_taa_velocity_handle_t handle);
int postprocessing_taa_velocity_process_pending(void);

/* Statistics */
uint32_t postprocessing_taa_velocity_get_count(void);
size_t postprocessing_taa_velocity_get_memory_usage(void);
void postprocessing_taa_velocity_debug_print(void);

#ifdef __cplusplus
}
#endif

#endif /* POSTPROCESSING_TAA_VELOCITY_H */
