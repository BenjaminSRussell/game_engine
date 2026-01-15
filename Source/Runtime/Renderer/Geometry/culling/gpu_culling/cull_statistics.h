/*
 * cull_statistics.h
 * Culling statistics
 *
 * Part of the Culling subsystem
 * Advanced 3D Rendering Engine
 */

#ifndef CULLING_CULL_STATISTICS_H
#define CULLING_CULL_STATISTICS_H

#include <stdint.h>
#include <stdbool.h>
#include <stddef.h>

#ifdef __cplusplus
extern "C" {
#endif

/* ============================================================================
 * TYPES
 * ============================================================================ */

typedef struct culling_cull_statistics_handle {
    uint32_t id;
} culling_cull_statistics_handle_t;

typedef struct culling_cull_statistics_desc {
    uint32_t flags;
    void* user_data;
} culling_cull_statistics_desc_t;

typedef struct culling_cull_statistics_info {
    uint32_t id;
    uint32_t flags;
    bool initialized;
} culling_cull_statistics_info_t;

/* ============================================================================
 * API
 * ============================================================================ */

/* Initialization */
int culling_cull_statistics_init(void);
void culling_cull_statistics_shutdown(void);

/* Lifecycle */
int culling_cull_statistics_create(culling_cull_statistics_handle_t* out_handle, const culling_cull_statistics_desc_t* desc);
void culling_cull_statistics_destroy(culling_cull_statistics_handle_t handle);

/* Operations */
int culling_cull_statistics_update(culling_cull_statistics_handle_t handle, const void* data, size_t size);
bool culling_cull_statistics_is_valid(culling_cull_statistics_handle_t handle);
int culling_cull_statistics_get_info(culling_cull_statistics_handle_t handle, culling_cull_statistics_info_t* out_info);
void culling_cull_statistics_mark_dirty(culling_cull_statistics_handle_t handle);
int culling_cull_statistics_process_pending(void);

/* Statistics */
uint32_t culling_cull_statistics_get_count(void);
size_t culling_cull_statistics_get_memory_usage(void);
void culling_cull_statistics_debug_print(void);

#ifdef __cplusplus
}
#endif

#endif /* CULLING_CULL_STATISTICS_H */
