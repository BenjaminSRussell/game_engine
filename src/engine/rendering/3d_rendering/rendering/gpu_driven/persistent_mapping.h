/*
 * persistent_mapping.h
 * Persistent buffer mapping
 *
 * Part of the Rendering subsystem
 * Advanced 3D Rendering Engine
 */

#ifndef RENDERING_PERSISTENT_MAPPING_H
#define RENDERING_PERSISTENT_MAPPING_H

#include <stdint.h>
#include <stdbool.h>
#include <stddef.h>

#ifdef __cplusplus
extern "C" {
#endif

/* ============================================================================
 * TYPES
 * ============================================================================ */

typedef struct rendering_persistent_mapping_handle {
    uint32_t id;
} rendering_persistent_mapping_handle_t;

typedef struct rendering_persistent_mapping_desc {
    uint32_t flags;
    void* user_data;
} rendering_persistent_mapping_desc_t;

typedef struct rendering_persistent_mapping_info {
    uint32_t id;
    uint32_t flags;
    bool initialized;
} rendering_persistent_mapping_info_t;

/* ============================================================================
 * API
 * ============================================================================ */

/* Initialization */
int rendering_persistent_mapping_init(void);
void rendering_persistent_mapping_shutdown(void);

/* Lifecycle */
int rendering_persistent_mapping_create(rendering_persistent_mapping_handle_t* out_handle, const rendering_persistent_mapping_desc_t* desc);
void rendering_persistent_mapping_destroy(rendering_persistent_mapping_handle_t handle);

/* Operations */
int rendering_persistent_mapping_update(rendering_persistent_mapping_handle_t handle, const void* data, size_t size);
bool rendering_persistent_mapping_is_valid(rendering_persistent_mapping_handle_t handle);
int rendering_persistent_mapping_get_info(rendering_persistent_mapping_handle_t handle, rendering_persistent_mapping_info_t* out_info);
void rendering_persistent_mapping_mark_dirty(rendering_persistent_mapping_handle_t handle);
int rendering_persistent_mapping_process_pending(void);

/* Statistics */
uint32_t rendering_persistent_mapping_get_count(void);
size_t rendering_persistent_mapping_get_memory_usage(void);
void rendering_persistent_mapping_debug_print(void);

#ifdef __cplusplus
}
#endif

#endif /* RENDERING_PERSISTENT_MAPPING_H */
