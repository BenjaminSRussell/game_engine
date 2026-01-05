/*
 * descriptor_set_layout.h
 * Descriptor set layout creation
 *
 * Part of the Core subsystem
 * Advanced 3D Rendering Engine
 */

#ifndef CORE_DESCRIPTOR_SET_LAYOUT_H
#define CORE_DESCRIPTOR_SET_LAYOUT_H

#include <stdint.h>
#include <stdbool.h>
#include <stddef.h>

#ifdef __cplusplus
extern "C" {
#endif

/* ============================================================================
 * TYPES
 * ============================================================================ */

typedef struct core_descriptor_set_layout_handle {
    uint32_t id;
} core_descriptor_set_layout_handle_t;

typedef struct core_descriptor_set_layout_desc {
    uint32_t flags;
    void* user_data;
} core_descriptor_set_layout_desc_t;

typedef struct core_descriptor_set_layout_info {
    uint32_t id;
    uint32_t flags;
    bool initialized;
} core_descriptor_set_layout_info_t;

/* ============================================================================
 * API
 * ============================================================================ */

/* Initialization */
int core_descriptor_set_layout_init(void);
void core_descriptor_set_layout_shutdown(void);

/* Lifecycle */
int core_descriptor_set_layout_create(core_descriptor_set_layout_handle_t* out_handle, const core_descriptor_set_layout_desc_t* desc);
void core_descriptor_set_layout_destroy(core_descriptor_set_layout_handle_t handle);

/* Operations */
int core_descriptor_set_layout_update(core_descriptor_set_layout_handle_t handle, const void* data, size_t size);
bool core_descriptor_set_layout_is_valid(core_descriptor_set_layout_handle_t handle);
int core_descriptor_set_layout_get_info(core_descriptor_set_layout_handle_t handle, core_descriptor_set_layout_info_t* out_info);
void core_descriptor_set_layout_mark_dirty(core_descriptor_set_layout_handle_t handle);
int core_descriptor_set_layout_process_pending(void);

/* Statistics */
uint32_t core_descriptor_set_layout_get_count(void);
size_t core_descriptor_set_layout_get_memory_usage(void);
void core_descriptor_set_layout_debug_print(void);

#ifdef __cplusplus
}
#endif

#endif /* CORE_DESCRIPTOR_SET_LAYOUT_H */
