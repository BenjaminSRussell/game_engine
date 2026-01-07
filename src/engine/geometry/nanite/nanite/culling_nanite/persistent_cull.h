/*
 * persistent_cull.h
 * Persistent culling
 *
 * Part of the Nanite subsystem
 * Advanced 3D Rendering Engine
 */

#ifndef NANITE_PERSISTENT_CULL_H
#define NANITE_PERSISTENT_CULL_H

#include <stdint.h>
#include <stdbool.h>
#include <stddef.h>

#ifdef __cplusplus
extern "C" {
#endif

/* ============================================================================
 * TYPES
 * ============================================================================ */

typedef struct nanite_persistent_cull_handle {
    uint32_t id;
} nanite_persistent_cull_handle_t;

typedef struct nanite_persistent_cull_desc {
    uint32_t flags;
    void* user_data;
} nanite_persistent_cull_desc_t;

typedef struct nanite_persistent_cull_info {
    uint32_t id;
    uint32_t flags;
    bool initialized;
} nanite_persistent_cull_info_t;

/* ============================================================================
 * API
 * ============================================================================ */

/* Initialization */
int nanite_persistent_cull_init(void);
void nanite_persistent_cull_shutdown(void);

/* Lifecycle */
int nanite_persistent_cull_create(nanite_persistent_cull_handle_t* out_handle, const nanite_persistent_cull_desc_t* desc);
void nanite_persistent_cull_destroy(nanite_persistent_cull_handle_t handle);

/* Operations */
int nanite_persistent_cull_update(nanite_persistent_cull_handle_t handle, const void* data, size_t size);
bool nanite_persistent_cull_is_valid(nanite_persistent_cull_handle_t handle);
int nanite_persistent_cull_get_info(nanite_persistent_cull_handle_t handle, nanite_persistent_cull_info_t* out_info);
void nanite_persistent_cull_mark_dirty(nanite_persistent_cull_handle_t handle);
int nanite_persistent_cull_process_pending(void);

/* Statistics */
uint32_t nanite_persistent_cull_get_count(void);
size_t nanite_persistent_cull_get_memory_usage(void);
void nanite_persistent_cull_debug_print(void);

#ifdef __cplusplus
}
#endif

#endif /* NANITE_PERSISTENT_CULL_H */
