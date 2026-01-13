/*
 * indirect_commands.h
 * Indirect draw/dispatch command building
 *
 * Part of the Core subsystem
 * Advanced 3D Rendering Engine
 */

#ifndef CORE_INDIRECT_COMMANDS_H
#define CORE_INDIRECT_COMMANDS_H

#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>

#ifdef __cplusplus
extern "C" {
#endif

/* ============================================================================
 * CONSTANTS & ERROR CODES
 * ============================================================================
 */

typedef enum {
  CORE_INDIRECT_SUCCESS = 0,
  CORE_INDIRECT_ERROR_INVALID_ARGS = -1,
  CORE_INDIRECT_ERROR_NOT_INITIALIZED = -2,
  CORE_INDIRECT_ERROR_OUT_OF_MEMORY = -3,
  CORE_INDIRECT_ERROR_INVALID_HANDLE = -4,
  CORE_INDIRECT_ERROR_BUFFER_TOO_SMALL = -5,
  CORE_INDIRECT_ERROR_ALREADY_INITIALIZED = -6,
  CORE_INDIRECT_ERROR_INTERNAL = -7
} core_indirect_error_t;

/* ============================================================================
 * TYPES
 * ============================================================================
 */

typedef enum CoreIndirectCommandsResult {
  CORE_INDIRECT_COMMANDS_SUCCESS = 0,
  CORE_INDIRECT_COMMANDS_ERROR_UNKNOWN = -1,
  CORE_INDIRECT_COMMANDS_ERROR_INVALID_ARGS = -2,
  CORE_INDIRECT_COMMANDS_ERROR_NOT_INITIALIZED = -3,
  CORE_INDIRECT_COMMANDS_ERROR_OUT_OF_MEMORY = -4,
  CORE_INDIRECT_COMMANDS_ERROR_BUFFER_TOO_SMALL = -5,
  CORE_INDIRECT_COMMANDS_ERROR_INVALID_HANDLE = -6,
  CORE_INDIRECT_COMMANDS_ERROR_ALREADY_INITIALIZED = -7,
  CORE_INDIRECT_COMMANDS_ERROR_LIMIT_REACHED = -8,
  CORE_INDIRECT_COMMANDS_ERROR_LOCK_FAILED = -9
} CoreIndirectCommandsResult;

typedef struct core_indirect_commands_handle {
  uint32_t id;
} core_indirect_commands_handle_t;

typedef struct core_indirect_commands_desc {
  uint32_t flags;
  void *user_data;
  uint32_t lod_level;
} core_indirect_commands_desc_t;

typedef struct core_indirect_commands_info {
  uint32_t id;
  uint32_t flags;
  bool initialized;
  uint32_t lod_level;
} core_indirect_commands_info_t;

typedef struct core_indirect_commands_stats {
  uint32_t active_commands;
  uint32_t total_created;
  uint32_t total_updates;
  uint32_t validation_failures;
  size_t memory_usage;
} core_indirect_commands_stats_t;

/* ============================================================================
 * API
 * ============================================================================
 */

/* Initialization */
CoreIndirectCommandsResult core_indirect_commands_init(void);
void core_indirect_commands_shutdown(void);

/* Lifecycle */
CoreIndirectCommandsResult
core_indirect_commands_create(core_indirect_commands_handle_t *out_handle,
                              const core_indirect_commands_desc_t *desc);
void core_indirect_commands_destroy(core_indirect_commands_handle_t handle);

/* Operations */
CoreIndirectCommandsResult
core_indirect_commands_update(core_indirect_commands_handle_t handle,
                              const void *data, size_t size);
CoreIndirectCommandsResult
core_indirect_commands_update_partial(core_indirect_commands_handle_t handle,
                                      uint32_t offset, const void *data,
                                      size_t size);
bool core_indirect_commands_is_valid(core_indirect_commands_handle_t handle);
CoreIndirectCommandsResult
core_indirect_commands_get_info(core_indirect_commands_handle_t handle,
                                core_indirect_commands_info_t *out_info);
void core_indirect_commands_mark_dirty(core_indirect_commands_handle_t handle);
int core_indirect_commands_process_pending(void);
int core_indirect_commands_reload(core_indirect_commands_handle_t handle);

/* Serialization */
int core_indirect_commands_serialize(core_indirect_commands_handle_t handle,
                                     void *buffer, size_t buffer_size,
                                     size_t *out_size);
int core_indirect_commands_deserialize(
    const void *buffer, size_t buffer_size,
    core_indirect_commands_handle_t *out_handle);

/* Statistics */
uint32_t core_indirect_commands_get_count(void);
size_t core_indirect_commands_get_memory_usage(void);
void core_indirect_commands_get_stats(
    core_indirect_commands_stats_t *out_stats);
void core_indirect_commands_debug_print(void);

#ifdef __cplusplus
}
#endif

#endif /* CORE_INDIRECT_COMMANDS_H */
