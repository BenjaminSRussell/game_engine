/*
 * camera_path.h
 * Camera path visualization
 *
 * Part of the Editor subsystem
 * Advanced 3D Rendering Engine
 */

#ifndef EDITOR_CAMERA_PATH_H
#define EDITOR_CAMERA_PATH_H

#include <stdint.h>
#include <stdbool.h>
#include <stddef.h>

#ifdef __cplusplus
extern "C" {
#endif

/* ============================================================================
 * TYPES
 * ============================================================================ */

typedef struct editor_camera_path_handle {
    uint32_t id;
} editor_camera_path_handle_t;

typedef struct editor_camera_path_desc {
    uint32_t flags;
    void* user_data;
} editor_camera_path_desc_t;

typedef struct editor_camera_path_info {
    uint32_t id;
    uint32_t flags;
    bool initialized;
} editor_camera_path_info_t;

/* ============================================================================
 * API
 * ============================================================================ */

/* Initialization */
int editor_camera_path_init(void);
void editor_camera_path_shutdown(void);

/* Lifecycle */
int editor_camera_path_create(editor_camera_path_handle_t* out_handle, const editor_camera_path_desc_t* desc);
void editor_camera_path_destroy(editor_camera_path_handle_t handle);

/* Operations */
int editor_camera_path_update(editor_camera_path_handle_t handle, const void* data, size_t size);
bool editor_camera_path_is_valid(editor_camera_path_handle_t handle);
int editor_camera_path_get_info(editor_camera_path_handle_t handle, editor_camera_path_info_t* out_info);
void editor_camera_path_mark_dirty(editor_camera_path_handle_t handle);
int editor_camera_path_process_pending(void);

/* Statistics */
uint32_t editor_camera_path_get_count(void);
size_t editor_camera_path_get_memory_usage(void);
void editor_camera_path_debug_print(void);

#ifdef __cplusplus
}
#endif

#endif /* EDITOR_CAMERA_PATH_H */
