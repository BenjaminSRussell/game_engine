/*
 * pose_matching.h
 * Animation pose matching
 *
 * Part of the Animation subsystem
 * Advanced 3D Rendering Engine
 */

#ifndef ANIMATION_POSE_MATCHING_H
#define ANIMATION_POSE_MATCHING_H

#include <stdint.h>
#include <stdbool.h>
#include <stddef.h>

#ifdef __cplusplus
extern "C" {
#endif

/* ============================================================================
 * TYPES
 * ============================================================================ */

typedef struct animation_pose_matching_handle {
    uint32_t id;
} animation_pose_matching_handle_t;

typedef struct animation_pose_matching_desc {
    uint32_t flags;
    void* user_data;
} animation_pose_matching_desc_t;

typedef struct animation_pose_matching_info {
    uint32_t id;
    uint32_t flags;
    bool initialized;
} animation_pose_matching_info_t;

/* ============================================================================
 * API
 * ============================================================================ */

/* Initialization */
int animation_pose_matching_init(void);
void animation_pose_matching_shutdown(void);

/* Lifecycle */
int animation_pose_matching_create(animation_pose_matching_handle_t* out_handle, const animation_pose_matching_desc_t* desc);
void animation_pose_matching_destroy(animation_pose_matching_handle_t handle);

/* Operations */
int animation_pose_matching_update(animation_pose_matching_handle_t handle, const void* data, size_t size);
bool animation_pose_matching_is_valid(animation_pose_matching_handle_t handle);
int animation_pose_matching_get_info(animation_pose_matching_handle_t handle, animation_pose_matching_info_t* out_info);
void animation_pose_matching_mark_dirty(animation_pose_matching_handle_t handle);
int animation_pose_matching_process_pending(void);

/* Statistics */
uint32_t animation_pose_matching_get_count(void);
size_t animation_pose_matching_get_memory_usage(void);
void animation_pose_matching_debug_print(void);

#ifdef __cplusplus
}
#endif

#endif /* ANIMATION_POSE_MATCHING_H */
