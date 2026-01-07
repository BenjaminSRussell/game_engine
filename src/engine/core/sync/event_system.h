/*
 * event_system.h
 * GPU event signaling
 *
 * Part of the Core subsystem
 * Advanced 3D Rendering Engine
 */

#ifndef CORE_EVENT_SYSTEM_H
#define CORE_EVENT_SYSTEM_H

#include <stdint.h>
#include <stdbool.h>
#include <stddef.h>

#ifdef __cplusplus
extern "C" {
#endif

/* ============================================================================
 * TYPES
 * ============================================================================ */

typedef struct core_event_system_handle {
    uint32_t id;
} core_event_system_handle_t;

typedef struct core_event_system_desc {
    uint32_t flags;
    void* user_data;
} core_event_system_desc_t;

typedef struct core_event_system_info {
    uint32_t id;
    uint32_t flags;
    bool initialized;
} core_event_system_info_t;

/* ============================================================================
 * API
 * ============================================================================ */

/* Initialization */
int core_event_system_init(void);
void core_event_system_shutdown(void);

/* Lifecycle */
int core_event_system_create(core_event_system_handle_t* out_handle, const core_event_system_desc_t* desc);
void core_event_system_destroy(core_event_system_handle_t handle);

/* Operations */
int core_event_system_update(core_event_system_handle_t handle, const void* data, size_t size);
bool core_event_system_is_valid(core_event_system_handle_t handle);
int core_event_system_get_info(core_event_system_handle_t handle, core_event_system_info_t* out_info);
void core_event_system_mark_dirty(core_event_system_handle_t handle);
int core_event_system_process_pending(void);

/* Statistics */
uint32_t core_event_system_get_count(void);
size_t core_event_system_get_memory_usage(void);
void core_event_system_debug_print(void);

#ifdef __cplusplus
}
#endif

#endif /* CORE_EVENT_SYSTEM_H */
