/*
 * morph_weights.h
 * Morph weight blending
 *
 * Part of the Animation subsystem
 * Advanced 3D Rendering Engine
 */

#ifndef ANIMATION_MORPH_WEIGHTS_H
#define ANIMATION_MORPH_WEIGHTS_H

#include <stdint.h>
#include <stdbool.h>
#include <stddef.h>

#ifdef __cplusplus
extern "C" {
#endif

/* ============================================================================
 * TYPES
 * ============================================================================ */

typedef struct animation_morph_weights_handle {
    uint32_t id;
} animation_morph_weights_handle_t;

typedef struct animation_morph_weights_desc {
    uint32_t flags;
    void* user_data;
} animation_morph_weights_desc_t;

typedef struct animation_morph_weights_info {
    uint32_t id;
    uint32_t flags;
    bool initialized;
} animation_morph_weights_info_t;

/* ============================================================================
 * API
 * ============================================================================ */

/* Initialization */
int animation_morph_weights_init(void);
void animation_morph_weights_shutdown(void);

/* Lifecycle */
int animation_morph_weights_create(animation_morph_weights_handle_t* out_handle, const animation_morph_weights_desc_t* desc);
void animation_morph_weights_destroy(animation_morph_weights_handle_t handle);

/* Operations */
int animation_morph_weights_update(animation_morph_weights_handle_t handle, const void* data, size_t size);
bool animation_morph_weights_is_valid(animation_morph_weights_handle_t handle);
int animation_morph_weights_get_info(animation_morph_weights_handle_t handle, animation_morph_weights_info_t* out_info);
void animation_morph_weights_mark_dirty(animation_morph_weights_handle_t handle);
int animation_morph_weights_process_pending(void);

/* Statistics */
uint32_t animation_morph_weights_get_count(void);
size_t animation_morph_weights_get_memory_usage(void);
void animation_morph_weights_debug_print(void);

#ifdef __cplusplus
}
#endif

#endif /* ANIMATION_MORPH_WEIGHTS_H */
