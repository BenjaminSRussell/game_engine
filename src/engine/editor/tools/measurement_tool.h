/*
 * measurement_tool.h
 * Distance measurement
 *
 * Part of the Editor subsystem
 * Advanced 3D Rendering Engine
 */

#ifndef EDITOR_MEASUREMENT_TOOL_H
#define EDITOR_MEASUREMENT_TOOL_H

#include <stdint.h>
#include <stdbool.h>
#include <stddef.h>

#ifdef __cplusplus
extern "C" {
#endif

/* ============================================================================
 * TYPES
 * ============================================================================ */

typedef struct editor_measurement_tool_handle {
    uint32_t id;
} editor_measurement_tool_handle_t;

typedef struct editor_measurement_tool_desc {
    uint32_t flags;
    void* user_data;
} editor_measurement_tool_desc_t;

typedef struct editor_measurement_tool_info {
    uint32_t id;
    uint32_t flags;
    bool initialized;
} editor_measurement_tool_info_t;

/* ============================================================================
 * API
 * ============================================================================ */

/* Initialization */
int editor_measurement_tool_init(void);
void editor_measurement_tool_shutdown(void);

/* Lifecycle */
int editor_measurement_tool_create(editor_measurement_tool_handle_t* out_handle, const editor_measurement_tool_desc_t* desc);
void editor_measurement_tool_destroy(editor_measurement_tool_handle_t handle);

/* Operations */
int editor_measurement_tool_update(editor_measurement_tool_handle_t handle, const void* data, size_t size);
bool editor_measurement_tool_is_valid(editor_measurement_tool_handle_t handle);
int editor_measurement_tool_get_info(editor_measurement_tool_handle_t handle, editor_measurement_tool_info_t* out_info);
void editor_measurement_tool_mark_dirty(editor_measurement_tool_handle_t handle);
int editor_measurement_tool_process_pending(void);

/* Statistics */
uint32_t editor_measurement_tool_get_count(void);
size_t editor_measurement_tool_get_memory_usage(void);
void editor_measurement_tool_debug_print(void);

#ifdef __cplusplus
}
#endif

#endif /* EDITOR_MEASUREMENT_TOOL_H */
