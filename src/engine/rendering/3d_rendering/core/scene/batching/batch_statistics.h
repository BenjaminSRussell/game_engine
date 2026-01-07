/*
 * batch_statistics.h
 * Batch statistics
 *
 * Part of the Scene Management subsystem
 * Advanced 3D Rendering Engine
 */

#ifndef SCENE_MANAGEMENT_BATCH_STATISTICS_H
#define SCENE_MANAGEMENT_BATCH_STATISTICS_H

#include <stdint.h>
#include <stdbool.h>
#include <stddef.h>

#ifdef __cplusplus
extern "C" {
#endif

/* ============================================================================
 * TYPES
 * ============================================================================ */

typedef struct scene_management_batch_statistics_handle {
    uint32_t id;
} scene_management_batch_statistics_handle_t;

typedef struct scene_management_batch_statistics_desc {
    uint32_t flags;
    void* user_data;
} scene_management_batch_statistics_desc_t;

typedef struct scene_management_batch_statistics_info {
    uint32_t id;
    uint32_t flags;
    bool initialized;
} scene_management_batch_statistics_info_t;

/* ============================================================================
 * API
 * ============================================================================ */

/* Initialization */
int scene_management_batch_statistics_init(void);
void scene_management_batch_statistics_shutdown(void);

/* Lifecycle */
int scene_management_batch_statistics_create(scene_management_batch_statistics_handle_t* out_handle, const scene_management_batch_statistics_desc_t* desc);
void scene_management_batch_statistics_destroy(scene_management_batch_statistics_handle_t handle);

/* Operations */
int scene_management_batch_statistics_update(scene_management_batch_statistics_handle_t handle, const void* data, size_t size);
bool scene_management_batch_statistics_is_valid(scene_management_batch_statistics_handle_t handle);
int scene_management_batch_statistics_get_info(scene_management_batch_statistics_handle_t handle, scene_management_batch_statistics_info_t* out_info);
void scene_management_batch_statistics_mark_dirty(scene_management_batch_statistics_handle_t handle);
int scene_management_batch_statistics_process_pending(void);

/* Statistics */
uint32_t scene_management_batch_statistics_get_count(void);
size_t scene_management_batch_statistics_get_memory_usage(void);
void scene_management_batch_statistics_debug_print(void);

#ifdef __cplusplus
}
#endif

#endif /* SCENE_MANAGEMENT_BATCH_STATISTICS_H */
