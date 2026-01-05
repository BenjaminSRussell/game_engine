/*
 * push_constants.h
 * Push constant management
 *
 * Part of the Core subsystem
 * Advanced 3D Rendering Engine
 */

#ifndef CORE_PUSH_CONSTANTS_H
#define CORE_PUSH_CONSTANTS_H

#include <stdint.h>
#include <stdbool.h>
#include <stddef.h>

#ifdef __cplusplus
extern "C" {
#endif

/* ============================================================================
 * TYPES
 * ============================================================================ */

typedef struct core_push_constants_handle {
    uint32_t id;
} core_push_constants_handle_t;

typedef struct core_push_constants_desc {
    uint32_t flags;
    void* user_data;
} core_push_constants_desc_t;

typedef struct core_push_constants_info {
    uint32_t id;
    uint32_t flags;
    bool initialized;
} core_push_constants_info_t;

/* ============================================================================
 * API
 * ============================================================================ */

/* Initialization */
int core_push_constants_init(void);
void core_push_constants_shutdown(void);

/* Lifecycle */
int core_push_constants_create(core_push_constants_handle_t* out_handle, const core_push_constants_desc_t* desc);
void core_push_constants_destroy(core_push_constants_handle_t handle);

/* Operations */
int core_push_constants_update(core_push_constants_handle_t handle, const void* data, size_t size);
bool core_push_constants_is_valid(core_push_constants_handle_t handle);
int core_push_constants_get_info(core_push_constants_handle_t handle, core_push_constants_info_t* out_info);
void core_push_constants_mark_dirty(core_push_constants_handle_t handle);
int core_push_constants_process_pending(void);

/* Statistics */
uint32_t core_push_constants_get_count(void);
size_t core_push_constants_get_memory_usage(void);
void core_push_constants_debug_print(void);

#ifdef __cplusplus
}
#endif

#endif /* CORE_PUSH_CONSTANTS_H */
