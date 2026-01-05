/*
 * dynamic_descriptors.h
 * Dynamic uniform/storage buffers
 *
 * Part of the Core subsystem
 * Advanced 3D Rendering Engine
 */

#ifndef CORE_DYNAMIC_DESCRIPTORS_H
#define CORE_DYNAMIC_DESCRIPTORS_H

#include <stdint.h>
#include <stdbool.h>
#include <stddef.h>

#ifdef __cplusplus
extern "C" {
#endif

/* ============================================================================
 * TYPES
 * ============================================================================ */

typedef struct core_dynamic_descriptors_handle {
    uint32_t id;
} core_dynamic_descriptors_handle_t;

typedef struct core_dynamic_descriptors_desc {
    uint32_t flags;
    void* user_data;
} core_dynamic_descriptors_desc_t;

typedef struct core_dynamic_descriptors_info {
    uint32_t id;
    uint32_t flags;
    bool initialized;
} core_dynamic_descriptors_info_t;

/* ============================================================================
 * API
 * ============================================================================ */

/* Initialization */
int core_dynamic_descriptors_init(void);
void core_dynamic_descriptors_shutdown(void);

/* Lifecycle */
int core_dynamic_descriptors_create(core_dynamic_descriptors_handle_t* out_handle, const core_dynamic_descriptors_desc_t* desc);
void core_dynamic_descriptors_destroy(core_dynamic_descriptors_handle_t handle);

/* Operations */
int core_dynamic_descriptors_update(core_dynamic_descriptors_handle_t handle, const void* data, size_t size);
bool core_dynamic_descriptors_is_valid(core_dynamic_descriptors_handle_t handle);
int core_dynamic_descriptors_get_info(core_dynamic_descriptors_handle_t handle, core_dynamic_descriptors_info_t* out_info);
void core_dynamic_descriptors_mark_dirty(core_dynamic_descriptors_handle_t handle);
int core_dynamic_descriptors_process_pending(void);

/* Statistics */
uint32_t core_dynamic_descriptors_get_count(void);
size_t core_dynamic_descriptors_get_memory_usage(void);
void core_dynamic_descriptors_debug_print(void);

#ifdef __cplusplus
}
#endif

#endif /* CORE_DYNAMIC_DESCRIPTORS_H */
