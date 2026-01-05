/*
 * command_buffer.h
 * Command buffer recording and management
 *
 * Part of the Core subsystem
 * Advanced 3D Rendering Engine
 */

#ifndef CORE_COMMAND_BUFFER_H
#define CORE_COMMAND_BUFFER_H

#include <stdint.h>
#include <stdbool.h>
#include <stddef.h>

#ifdef __cplusplus
extern "C" {
#endif

/* ============================================================================
 * TYPES
 * ============================================================================ */

typedef struct core_command_buffer_handle {
    uint32_t id;
} core_command_buffer_handle_t;

typedef struct core_command_buffer_desc {
    uint32_t flags;
    void* user_data;
} core_command_buffer_desc_t;

typedef struct core_command_buffer_info {
    uint32_t id;
    uint32_t flags;
    bool initialized;
} core_command_buffer_info_t;

/* ============================================================================
 * API
 * ============================================================================ */

/* Initialization */
int core_command_buffer_init(void);
void core_command_buffer_shutdown(void);

/* Lifecycle */
int core_command_buffer_create(core_command_buffer_handle_t* out_handle, const core_command_buffer_desc_t* desc);
void core_command_buffer_destroy(core_command_buffer_handle_t handle);

/* Operations */
int core_command_buffer_update(core_command_buffer_handle_t handle, const void* data, size_t size);
bool core_command_buffer_is_valid(core_command_buffer_handle_t handle);
int core_command_buffer_get_info(core_command_buffer_handle_t handle, core_command_buffer_info_t* out_info);
void core_command_buffer_mark_dirty(core_command_buffer_handle_t handle);
int core_command_buffer_process_pending(void);

/* Statistics */
uint32_t core_command_buffer_get_count(void);
size_t core_command_buffer_get_memory_usage(void);
void core_command_buffer_debug_print(void);

#ifdef __cplusplus
}
#endif

#endif /* CORE_COMMAND_BUFFER_H */
