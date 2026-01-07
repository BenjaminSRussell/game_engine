/*
 * depth_only.h
 * Depth-only pass
 *
 * Part of the Nanite subsystem
 * Advanced 3D Rendering Engine
 */

#ifndef NANITE_DEPTH_ONLY_H
#define NANITE_DEPTH_ONLY_H

#include <stdint.h>
#include <stdbool.h>
#include <stddef.h>

#ifdef __cplusplus
extern "C" {
#endif

/* ============================================================================
 * TYPES
 * ============================================================================ */

typedef struct nanite_depth_only_handle {
    uint32_t id;
} nanite_depth_only_handle_t;

typedef struct nanite_depth_only_desc {
    uint32_t flags;
    void* user_data;
} nanite_depth_only_desc_t;

typedef struct nanite_depth_only_info {
    uint32_t id;
    uint32_t flags;
    bool initialized;
} nanite_depth_only_info_t;

/* ============================================================================
 * API
 * ============================================================================ */

/* Initialization */
int nanite_depth_only_init(void);
void nanite_depth_only_shutdown(void);

/* Lifecycle */
int nanite_depth_only_create(nanite_depth_only_handle_t* out_handle, const nanite_depth_only_desc_t* desc);
void nanite_depth_only_destroy(nanite_depth_only_handle_t handle);

/* Operations */
int nanite_depth_only_update(nanite_depth_only_handle_t handle, const void* data, size_t size);
bool nanite_depth_only_is_valid(nanite_depth_only_handle_t handle);
int nanite_depth_only_get_info(nanite_depth_only_handle_t handle, nanite_depth_only_info_t* out_info);
void nanite_depth_only_mark_dirty(nanite_depth_only_handle_t handle);
int nanite_depth_only_process_pending(void);

/* Statistics */
uint32_t nanite_depth_only_get_count(void);
size_t nanite_depth_only_get_memory_usage(void);
void nanite_depth_only_debug_print(void);

#ifdef __cplusplus
}
#endif

#endif /* NANITE_DEPTH_ONLY_H */
