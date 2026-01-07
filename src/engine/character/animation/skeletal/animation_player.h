/*
 * animation_player.h
 * Animation playback
 *
 * Part of the Animation subsystem
 * Advanced 3D Rendering Engine
 */

#ifndef ANIMATION_ANIMATION_PLAYER_H
#define ANIMATION_ANIMATION_PLAYER_H

#include <stdint.h>
#include <stdbool.h>
#include <stddef.h>

#ifdef __cplusplus
extern "C" {
#endif

/* ============================================================================
 * TYPES
 * ============================================================================ */

typedef struct animation_animation_player_handle {
    uint32_t id;
} animation_animation_player_handle_t;

typedef struct animation_animation_player_desc {
    uint32_t flags;
    void* user_data;
} animation_animation_player_desc_t;

typedef struct animation_animation_player_info {
    uint32_t id;
    uint32_t flags;
    bool initialized;
} animation_animation_player_info_t;

/* ============================================================================
 * API
 * ============================================================================ */

/* Initialization */
int animation_animation_player_init(void);
void animation_animation_player_shutdown(void);

/* Lifecycle */
int animation_animation_player_create(animation_animation_player_handle_t* out_handle, const animation_animation_player_desc_t* desc);
void animation_animation_player_destroy(animation_animation_player_handle_t handle);

/* Operations */
int animation_animation_player_update(animation_animation_player_handle_t handle, const void* data, size_t size);
bool animation_animation_player_is_valid(animation_animation_player_handle_t handle);
int animation_animation_player_get_info(animation_animation_player_handle_t handle, animation_animation_player_info_t* out_info);
void animation_animation_player_mark_dirty(animation_animation_player_handle_t handle);
int animation_animation_player_process_pending(void);

/* Statistics */
uint32_t animation_animation_player_get_count(void);
size_t animation_animation_player_get_memory_usage(void);
void animation_animation_player_debug_print(void);

#ifdef __cplusplus
}
#endif

#endif /* ANIMATION_ANIMATION_PLAYER_H */
