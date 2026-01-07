/*
 * gpu_markers.h
 * GPU debug markers
 *
 * Part of the Profiling subsystem
 * Advanced 3D Rendering Engine
 */

#ifndef PROFILING_GPU_MARKERS_H
#define PROFILING_GPU_MARKERS_H

#include <stdint.h>
#include <stdbool.h>
#include <stddef.h>

#ifdef __cplusplus
extern "C" {
#endif

/* ============================================================================
 * TYPES
 * ============================================================================ */

typedef struct profiling_gpu_markers_handle {
    uint32_t id;
} profiling_gpu_markers_handle_t;

typedef struct profiling_gpu_markers_desc {
    uint32_t flags;
    void* user_data;
} profiling_gpu_markers_desc_t;

typedef struct profiling_gpu_markers_info {
    uint32_t id;
    uint32_t flags;
    bool initialized;
} profiling_gpu_markers_info_t;

/* ============================================================================
 * API
 * ============================================================================ */

/* Initialization */
int profiling_gpu_markers_init(void);
void profiling_gpu_markers_shutdown(void);

/* Lifecycle */
int profiling_gpu_markers_create(profiling_gpu_markers_handle_t* out_handle, const profiling_gpu_markers_desc_t* desc);
void profiling_gpu_markers_destroy(profiling_gpu_markers_handle_t handle);

/* Operations */
int profiling_gpu_markers_update(profiling_gpu_markers_handle_t handle, const void* data, size_t size);
bool profiling_gpu_markers_is_valid(profiling_gpu_markers_handle_t handle);
int profiling_gpu_markers_get_info(profiling_gpu_markers_handle_t handle, profiling_gpu_markers_info_t* out_info);
void profiling_gpu_markers_mark_dirty(profiling_gpu_markers_handle_t handle);
int profiling_gpu_markers_process_pending(void);

/* Statistics */
uint32_t profiling_gpu_markers_get_count(void);
size_t profiling_gpu_markers_get_memory_usage(void);
void profiling_gpu_markers_debug_print(void);

#ifdef __cplusplus
}
#endif

#endif /* PROFILING_GPU_MARKERS_H */
