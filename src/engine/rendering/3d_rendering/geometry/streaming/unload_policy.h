/*
 * unload_policy.h
 * Mesh unloading strategy
 *
 * Part of the Geometry subsystem
 * Advanced 3D Rendering Engine
 */

#ifndef GEOMETRY_UNLOAD_POLICY_H
#define GEOMETRY_UNLOAD_POLICY_H

#include <stdint.h>
#include <stdbool.h>
#include <stddef.h>

#ifdef __cplusplus
extern "C" {
#endif

/* ============================================================================
 * TYPES
 * ============================================================================ */

typedef struct geometry_unload_policy_handle {
    uint32_t id;
} geometry_unload_policy_handle_t;

typedef struct geometry_unload_policy_desc {
    uint32_t flags;
    void* user_data;
} geometry_unload_policy_desc_t;

typedef struct geometry_unload_policy_info {
    uint32_t id;
    uint32_t flags;
    bool initialized;
} geometry_unload_policy_info_t;

/* ============================================================================
 * API
 * ============================================================================ */

/* Initialization */
int geometry_unload_policy_init(void);
void geometry_unload_policy_shutdown(void);

/* Lifecycle */
int geometry_unload_policy_create(geometry_unload_policy_handle_t* out_handle, const geometry_unload_policy_desc_t* desc);
void geometry_unload_policy_destroy(geometry_unload_policy_handle_t handle);

/* Operations */
int geometry_unload_policy_update(geometry_unload_policy_handle_t handle, const void* data, size_t size);
bool geometry_unload_policy_is_valid(geometry_unload_policy_handle_t handle);
int geometry_unload_policy_get_info(geometry_unload_policy_handle_t handle, geometry_unload_policy_info_t* out_info);
void geometry_unload_policy_mark_dirty(geometry_unload_policy_handle_t handle);
int geometry_unload_policy_process_pending(void);

/* Statistics */
uint32_t geometry_unload_policy_get_count(void);
size_t geometry_unload_policy_get_memory_usage(void);
void geometry_unload_policy_debug_print(void);

#ifdef __cplusplus
}
#endif

#endif /* GEOMETRY_UNLOAD_POLICY_H */
