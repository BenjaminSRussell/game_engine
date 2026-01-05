/*
 * exposure_adaptation.h
 * Auto exposure
 *
 * Part of the Postprocessing subsystem
 * Advanced 3D Rendering Engine
 */

#ifndef POSTPROCESSING_EXPOSURE_ADAPTATION_H
#define POSTPROCESSING_EXPOSURE_ADAPTATION_H

#include <stdint.h>
#include <stdbool.h>
#include <stddef.h>

#ifdef __cplusplus
extern "C" {
#endif

/* ============================================================================
 * TYPES
 * ============================================================================ */

typedef struct postprocessing_exposure_adaptation_handle {
    uint32_t id;
} postprocessing_exposure_adaptation_handle_t;

typedef struct postprocessing_exposure_adaptation_desc {
    uint32_t flags;
    void* user_data;
} postprocessing_exposure_adaptation_desc_t;

typedef struct postprocessing_exposure_adaptation_info {
    uint32_t id;
    uint32_t flags;
    bool initialized;
} postprocessing_exposure_adaptation_info_t;

/* ============================================================================
 * API
 * ============================================================================ */

/* Initialization */
int postprocessing_exposure_adaptation_init(void);
void postprocessing_exposure_adaptation_shutdown(void);

/* Lifecycle */
int postprocessing_exposure_adaptation_create(postprocessing_exposure_adaptation_handle_t* out_handle, const postprocessing_exposure_adaptation_desc_t* desc);
void postprocessing_exposure_adaptation_destroy(postprocessing_exposure_adaptation_handle_t handle);

/* Operations */
int postprocessing_exposure_adaptation_update(postprocessing_exposure_adaptation_handle_t handle, const void* data, size_t size);
bool postprocessing_exposure_adaptation_is_valid(postprocessing_exposure_adaptation_handle_t handle);
int postprocessing_exposure_adaptation_get_info(postprocessing_exposure_adaptation_handle_t handle, postprocessing_exposure_adaptation_info_t* out_info);
void postprocessing_exposure_adaptation_mark_dirty(postprocessing_exposure_adaptation_handle_t handle);
int postprocessing_exposure_adaptation_process_pending(void);

/* Statistics */
uint32_t postprocessing_exposure_adaptation_get_count(void);
size_t postprocessing_exposure_adaptation_get_memory_usage(void);
void postprocessing_exposure_adaptation_debug_print(void);

#ifdef __cplusplus
}
#endif

#endif /* POSTPROCESSING_EXPOSURE_ADAPTATION_H */
