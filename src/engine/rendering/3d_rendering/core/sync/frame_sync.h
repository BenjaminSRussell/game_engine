/*
 * frame_sync.h
 * Frame-to-frame synchronization
 *
 * Part of the Core subsystem
 * Advanced 3D Rendering Engine
 */

#ifndef CORE_FRAME_SYNC_H
#define CORE_FRAME_SYNC_H

#include <stdint.h>
#include <stdbool.h>
#include <stddef.h>

#ifdef __cplusplus
extern "C" {
#endif

/* ============================================================================
 * TYPES
 * ============================================================================ */

typedef struct core_frame_sync_handle {
    uint32_t id;
} core_frame_sync_handle_t;

typedef struct core_frame_sync_desc {
    uint32_t flags;
    void* user_data;
} core_frame_sync_desc_t;

typedef struct core_frame_sync_info {
    uint32_t id;
    uint32_t flags;
    bool initialized;
} core_frame_sync_info_t;

/* ============================================================================
 * API
 * ============================================================================ */

/* Initialization */
int core_frame_sync_init(void);
void core_frame_sync_shutdown(void);

/* Lifecycle */
int core_frame_sync_create(core_frame_sync_handle_t* out_handle, const core_frame_sync_desc_t* desc);
void core_frame_sync_destroy(core_frame_sync_handle_t handle);

/* Operations */
int core_frame_sync_update(core_frame_sync_handle_t handle, const void* data, size_t size);
bool core_frame_sync_is_valid(core_frame_sync_handle_t handle);
int core_frame_sync_get_info(core_frame_sync_handle_t handle, core_frame_sync_info_t* out_info);
void core_frame_sync_mark_dirty(core_frame_sync_handle_t handle);
int core_frame_sync_process_pending(void);

/* Statistics */
uint32_t core_frame_sync_get_count(void);
size_t core_frame_sync_get_memory_usage(void);
void core_frame_sync_debug_print(void);

#ifdef __cplusplus
}
#endif

#endif /* CORE_FRAME_SYNC_H */
