/*
 * pipeline_stats.h
 * Pipeline statistics
 *
 * Part of the Profiling subsystem
 * Advanced 3D Rendering Engine
 */

#ifndef PROFILING_PIPELINE_STATS_H
#define PROFILING_PIPELINE_STATS_H

#include <stdint.h>
#include <stdbool.h>
#include <stddef.h>

#ifdef __cplusplus
extern "C" {
#endif

/* ============================================================================
 * TYPES
 * ============================================================================ */

typedef struct profiling_pipeline_stats_handle {
    uint32_t id;
} profiling_pipeline_stats_handle_t;

typedef struct profiling_pipeline_stats_desc {
    uint32_t flags;
    void* user_data;
} profiling_pipeline_stats_desc_t;

typedef struct profiling_pipeline_stats_info {
    uint32_t id;
    uint32_t flags;
    bool initialized;
} profiling_pipeline_stats_info_t;

/* ============================================================================
 * API
 * ============================================================================ */

/* Initialization */
int profiling_pipeline_stats_init(void);
void profiling_pipeline_stats_shutdown(void);

/* Lifecycle */
int profiling_pipeline_stats_create(profiling_pipeline_stats_handle_t* out_handle, const profiling_pipeline_stats_desc_t* desc);
void profiling_pipeline_stats_destroy(profiling_pipeline_stats_handle_t handle);

/* Operations */
int profiling_pipeline_stats_update(profiling_pipeline_stats_handle_t handle, const void* data, size_t size);
bool profiling_pipeline_stats_is_valid(profiling_pipeline_stats_handle_t handle);
int profiling_pipeline_stats_get_info(profiling_pipeline_stats_handle_t handle, profiling_pipeline_stats_info_t* out_info);
void profiling_pipeline_stats_mark_dirty(profiling_pipeline_stats_handle_t handle);
int profiling_pipeline_stats_process_pending(void);

/* Statistics */
uint32_t profiling_pipeline_stats_get_count(void);
size_t profiling_pipeline_stats_get_memory_usage(void);
void profiling_pipeline_stats_debug_print(void);

#ifdef __cplusplus
}
#endif

#endif /* PROFILING_PIPELINE_STATS_H */
