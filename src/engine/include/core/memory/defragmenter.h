/*
 * defragmenter.h
 * Memory defragmentation
 *
 * Part of the Core subsystem
 * Advanced 3D Rendering Engine
 */

#ifndef CORE_DEFRAGMENTER_H
#define CORE_DEFRAGMENTER_H

#include <stdint.h>
#include <stdbool.h>
#include <stddef.h>

#ifdef __cplusplus
extern "C" {
#endif

/* ============================================================================
 * TYPES
 * ============================================================================ */

typedef struct core_defragmenter_handle {
    uint32_t id;
} core_defragmenter_handle_t;

typedef struct core_defragmenter_desc {
    uint32_t flags;
    void* user_data;
} core_defragmenter_desc_t;

typedef struct core_defragmenter_info {
    uint32_t id;
    uint32_t flags;
    bool initialized;
} core_defragmenter_info_t;

/* ============================================================================
 * API
 * ============================================================================ */

/* Initialization */
int core_defragmenter_init(void);
void core_defragmenter_shutdown(void);

/* Lifecycle */
int core_defragmenter_create(core_defragmenter_handle_t* out_handle, const core_defragmenter_desc_t* desc);
void core_defragmenter_destroy(core_defragmenter_handle_t handle);

/* Operations */
int core_defragmenter_update(core_defragmenter_handle_t handle, const void* data, size_t size);
bool core_defragmenter_is_valid(core_defragmenter_handle_t handle);
int core_defragmenter_get_info(core_defragmenter_handle_t handle, core_defragmenter_info_t* out_info);
void core_defragmenter_mark_dirty(core_defragmenter_handle_t handle);
int core_defragmenter_process_pending(void);

typedef void (*core_defragmenter_render_node_callback_t)(void);
void core_defragmenter_register_render_node_callback(core_defragmenter_render_node_callback_t callback);

/* Statistics */
uint32_t core_defragmenter_get_count(void);
size_t core_defragmenter_get_memory_usage(void);
void core_defragmenter_debug_print(void);

#ifdef __cplusplus
}
#endif

#endif /* CORE_DEFRAGMENTER_H */
