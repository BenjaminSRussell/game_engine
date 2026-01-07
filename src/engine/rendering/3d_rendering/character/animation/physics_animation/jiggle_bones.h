/*
 * jiggle_bones.h
 * Secondary motion bones
 *
 * Part of the Animation subsystem
 * Advanced 3D Rendering Engine
 */

#ifndef ANIMATION_JIGGLE_BONES_H
#define ANIMATION_JIGGLE_BONES_H

#include <stdint.h>
#include <stdbool.h>
#include <stddef.h>

#ifdef __cplusplus
extern "C" {
#endif

/* ============================================================================
 * TYPES
 * ============================================================================ */

typedef struct animation_jiggle_bones_handle {
    uint32_t id;
} animation_jiggle_bones_handle_t;

typedef struct animation_jiggle_bones_desc {
    uint32_t flags;
    void* user_data;
} animation_jiggle_bones_desc_t;

typedef struct animation_jiggle_bones_info {
    uint32_t id;
    uint32_t flags;
    bool initialized;
} animation_jiggle_bones_info_t;

/* ============================================================================
 * API
 * ============================================================================ */

/* Initialization */
int animation_jiggle_bones_init(void);
void animation_jiggle_bones_shutdown(void);

/* Lifecycle */
int animation_jiggle_bones_create(animation_jiggle_bones_handle_t* out_handle, const animation_jiggle_bones_desc_t* desc);
void animation_jiggle_bones_destroy(animation_jiggle_bones_handle_t handle);

/* Operations */
int animation_jiggle_bones_update(animation_jiggle_bones_handle_t handle, const void* data, size_t size);
bool animation_jiggle_bones_is_valid(animation_jiggle_bones_handle_t handle);
int animation_jiggle_bones_get_info(animation_jiggle_bones_handle_t handle, animation_jiggle_bones_info_t* out_info);
void animation_jiggle_bones_mark_dirty(animation_jiggle_bones_handle_t handle);
int animation_jiggle_bones_process_pending(void);

/* Statistics */
uint32_t animation_jiggle_bones_get_count(void);
size_t animation_jiggle_bones_get_memory_usage(void);
void animation_jiggle_bones_debug_print(void);

#ifdef __cplusplus
}
#endif

#endif /* ANIMATION_JIGGLE_BONES_H */
