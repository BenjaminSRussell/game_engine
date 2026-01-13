/*
 * bent_normals.h
 * Bent normal generation
 *
 * Part of the Postprocessing subsystem
 * Advanced 3D Rendering Engine
 */

#ifndef POSTPROCESSING_BENT_NORMALS_H
#define POSTPROCESSING_BENT_NORMALS_H

#include <stdint.h>
#include <stdbool.h>
#include <stddef.h>

#ifdef __cplusplus
extern "C" {
#endif

/* ============================================================================
 * TYPES
 * ============================================================================ */

typedef struct postprocessing_bent_normals_handle {
    uint32_t id;
} postprocessing_bent_normals_handle_t;

typedef struct postprocessing_bent_normals_desc {
    uint32_t flags;
    void* user_data;
} postprocessing_bent_normals_desc_t;

typedef struct postprocessing_bent_normals_info {
    uint32_t id;
    uint32_t flags;
    bool initialized;
} postprocessing_bent_normals_info_t;

/* ============================================================================
 * API
 * ============================================================================ */

/* Initialization */
int postprocessing_bent_normals_init(void);
void postprocessing_bent_normals_shutdown(void);

/* Lifecycle */
int postprocessing_bent_normals_create(postprocessing_bent_normals_handle_t* out_handle, const postprocessing_bent_normals_desc_t* desc);
void postprocessing_bent_normals_destroy(postprocessing_bent_normals_handle_t handle);

/* Operations */
int postprocessing_bent_normals_update(postprocessing_bent_normals_handle_t handle, const void* data, size_t size);
bool postprocessing_bent_normals_is_valid(postprocessing_bent_normals_handle_t handle);
int postprocessing_bent_normals_get_info(postprocessing_bent_normals_handle_t handle, postprocessing_bent_normals_info_t* out_info);
void postprocessing_bent_normals_mark_dirty(postprocessing_bent_normals_handle_t handle);
int postprocessing_bent_normals_process_pending(void);

/* Statistics */
uint32_t postprocessing_bent_normals_get_count(void);
size_t postprocessing_bent_normals_get_memory_usage(void);
void postprocessing_bent_normals_debug_print(void);

#ifdef __cplusplus
}
#endif

#endif /* POSTPROCESSING_BENT_NORMALS_H */
