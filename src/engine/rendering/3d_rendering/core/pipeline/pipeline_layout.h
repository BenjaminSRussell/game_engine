/*
 * pipeline_layout.h
 * Pipeline layout management
 *
 * Part of the Core subsystem
 * Advanced 3D Rendering Engine
 */

#ifndef CORE_PIPELINE_LAYOUT_H
#define CORE_PIPELINE_LAYOUT_H

#include <stdint.h>
#include <stdbool.h>
#include <stddef.h>

#ifdef __cplusplus
extern "C" {
#endif

/* ============================================================================
 * TYPES
 * ============================================================================ */

typedef struct core_pipeline_layout_handle {
    uint32_t id;
} core_pipeline_layout_handle_t;

typedef struct core_pipeline_layout_desc {
    uint32_t flags;
    void* user_data;
} core_pipeline_layout_desc_t;

typedef struct core_pipeline_layout_info {
    uint32_t id;
    uint32_t flags;
    bool initialized;
} core_pipeline_layout_info_t;

/* ============================================================================
 * API
 * ============================================================================ */

/* Initialization */
int core_pipeline_layout_init(void);
void core_pipeline_layout_shutdown(void);

/* Lifecycle */
int core_pipeline_layout_create(core_pipeline_layout_handle_t* out_handle, const core_pipeline_layout_desc_t* desc);
void core_pipeline_layout_destroy(core_pipeline_layout_handle_t handle);

/* Operations */
int core_pipeline_layout_update(core_pipeline_layout_handle_t handle, const void* data, size_t size);
bool core_pipeline_layout_is_valid(core_pipeline_layout_handle_t handle);
int core_pipeline_layout_get_info(core_pipeline_layout_handle_t handle, core_pipeline_layout_info_t* out_info);
void core_pipeline_layout_mark_dirty(core_pipeline_layout_handle_t handle);
int core_pipeline_layout_process_pending(void);

/* Statistics */
uint32_t core_pipeline_layout_get_count(void);
size_t core_pipeline_layout_get_memory_usage(void);
void core_pipeline_layout_debug_print(void);

#ifdef __cplusplus
}
#endif

#endif /* CORE_PIPELINE_LAYOUT_H */
