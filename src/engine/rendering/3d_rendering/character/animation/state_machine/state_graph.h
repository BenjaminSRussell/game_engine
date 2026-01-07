/*
 * state_graph.h
 * Animation state graph
 *
 * Part of the Animation subsystem
 * Advanced 3D Rendering Engine
 */

#ifndef ANIMATION_STATE_GRAPH_H
#define ANIMATION_STATE_GRAPH_H

#include <stdint.h>
#include <stdbool.h>
#include <stddef.h>

#ifdef __cplusplus
extern "C" {
#endif

/* ============================================================================
 * TYPES
 * ============================================================================ */

typedef struct animation_state_graph_handle {
    uint32_t id;
} animation_state_graph_handle_t;

typedef struct animation_state_graph_desc {
    uint32_t flags;
    void* user_data;
} animation_state_graph_desc_t;

typedef struct animation_state_graph_info {
    uint32_t id;
    uint32_t flags;
    bool initialized;
} animation_state_graph_info_t;

/* ============================================================================
 * API
 * ============================================================================ */

/* Initialization */
int animation_state_graph_init(void);
void animation_state_graph_shutdown(void);

/* Lifecycle */
int animation_state_graph_create(animation_state_graph_handle_t* out_handle, const animation_state_graph_desc_t* desc);
void animation_state_graph_destroy(animation_state_graph_handle_t handle);

/* Operations */
int animation_state_graph_update(animation_state_graph_handle_t handle, const void* data, size_t size);
bool animation_state_graph_is_valid(animation_state_graph_handle_t handle);
int animation_state_graph_get_info(animation_state_graph_handle_t handle, animation_state_graph_info_t* out_info);
void animation_state_graph_mark_dirty(animation_state_graph_handle_t handle);
int animation_state_graph_process_pending(void);

/* Statistics */
uint32_t animation_state_graph_get_count(void);
size_t animation_state_graph_get_memory_usage(void);
void animation_state_graph_debug_print(void);

#ifdef __cplusplus
}
#endif

#endif /* ANIMATION_STATE_GRAPH_H */
