/*
 * frame_graph_viz.h
 * Frame graph visualization
 *
 * Part of the Profiling subsystem
 * Advanced 3D Rendering Engine
 */

#ifndef PROFILING_FRAME_GRAPH_VIZ_H
#define PROFILING_FRAME_GRAPH_VIZ_H

#include <stdint.h>
#include <stdbool.h>
#include <stddef.h>

#ifdef __cplusplus
extern "C" {
#endif

/* ============================================================================
 * TYPES
 * ============================================================================ */

typedef struct profiling_frame_graph_viz_handle {
    uint32_t id;
} profiling_frame_graph_viz_handle_t;

typedef struct profiling_frame_graph_viz_desc {
    uint32_t flags;
    void* user_data;
} profiling_frame_graph_viz_desc_t;

typedef struct profiling_frame_graph_viz_info {
    uint32_t id;
    uint32_t flags;
    bool initialized;
} profiling_frame_graph_viz_info_t;

/* ============================================================================
 * API
 * ============================================================================ */

/* Initialization */
int profiling_frame_graph_viz_init(void);
void profiling_frame_graph_viz_shutdown(void);

/* Lifecycle */
int profiling_frame_graph_viz_create(profiling_frame_graph_viz_handle_t* out_handle, const profiling_frame_graph_viz_desc_t* desc);
void profiling_frame_graph_viz_destroy(profiling_frame_graph_viz_handle_t handle);

/* Operations */
int profiling_frame_graph_viz_update(profiling_frame_graph_viz_handle_t handle, const void* data, size_t size);
bool profiling_frame_graph_viz_is_valid(profiling_frame_graph_viz_handle_t handle);
int profiling_frame_graph_viz_get_info(profiling_frame_graph_viz_handle_t handle, profiling_frame_graph_viz_info_t* out_info);
void profiling_frame_graph_viz_mark_dirty(profiling_frame_graph_viz_handle_t handle);
int profiling_frame_graph_viz_process_pending(void);

/* Statistics */
uint32_t profiling_frame_graph_viz_get_count(void);
size_t profiling_frame_graph_viz_get_memory_usage(void);
void profiling_frame_graph_viz_debug_print(void);

#ifdef __cplusplus
}
#endif

#endif /* PROFILING_FRAME_GRAPH_VIZ_H */
