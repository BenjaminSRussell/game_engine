/*
 * indirect_commands.h
 * Indirect draw/dispatch command building
 *
 * Part of the Core subsystem
 * Advanced 3D Rendering Engine
 */

#ifndef CORE_INDIRECT_COMMANDS_H
#define CORE_INDIRECT_COMMANDS_H

#include <stdint.h>
#include <stdbool.h>
#include <stddef.h>

#ifdef __cplusplus
extern "C" {
#endif

/* ============================================================================
 * ERROR CODES
 * ============================================================================ */
#define CORE_INDIRECT_ERROR_NONE 0
#define CORE_INDIRECT_ERROR_INVALID_ARGS -1
#define CORE_INDIRECT_ERROR_NOT_INITIALIZED -2
#define CORE_INDIRECT_ERROR_OUT_OF_MEMORY -3
#define CORE_INDIRECT_ERROR_INVALID_HANDLE -4
#define CORE_INDIRECT_ERROR_INVALID_STATE -5

/* ============================================================================
 * TYPES
 * ============================================================================ */

typedef struct core_indirect_commands_handle {
    uint32_t id;
} core_indirect_commands_handle_t;

typedef struct core_indirect_commands_desc {
    uint32_t flags;
    void* user_data;
} core_indirect_commands_desc_t;

typedef struct core_indirect_commands_info {
    uint32_t id;
    uint32_t flags;
    bool initialized;
} core_indirect_commands_info_t;

/* ============================================================================
 * API
 * ============================================================================ */

/* Initialization */
int core_indirect_commands_init(void);
void core_indirect_commands_shutdown(void);

/* Lifecycle */
int core_indirect_commands_create(core_indirect_commands_handle_t* out_handle, const core_indirect_commands_desc_t* desc);
void core_indirect_commands_destroy(core_indirect_commands_handle_t handle);

/* Operations */
int core_indirect_commands_update(core_indirect_commands_handle_t handle, const void* data, size_t size);
bool core_indirect_commands_is_valid(core_indirect_commands_handle_t handle);
int core_indirect_commands_get_info(core_indirect_commands_handle_t handle, core_indirect_commands_info_t* out_info);
void core_indirect_commands_mark_dirty(core_indirect_commands_handle_t handle);
int core_indirect_commands_process_pending(void);
int core_indirect_commands_reload(core_indirect_commands_handle_t handle);

/* Statistics */
uint32_t core_indirect_commands_get_count(void);
size_t core_indirect_commands_get_memory_usage(void);
void core_indirect_commands_debug_print(void);

#ifdef __cplusplus
}
#endif

#endif /* CORE_INDIRECT_COMMANDS_H */
