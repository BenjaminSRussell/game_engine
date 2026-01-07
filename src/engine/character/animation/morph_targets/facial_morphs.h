/*
 * facial_morphs.h
 * Facial expression morphs
 *
 * Part of the Animation subsystem
 * Advanced 3D Rendering Engine
 */

#ifndef ANIMATION_FACIAL_MORPHS_H
#define ANIMATION_FACIAL_MORPHS_H

#include <stdint.h>
#include <stdbool.h>
#include <stddef.h>

#ifdef __cplusplus
extern "C" {
#endif

/* ============================================================================
 * TYPES
 * ============================================================================ */

typedef struct animation_facial_morphs_handle {
    uint32_t id;
} animation_facial_morphs_handle_t;

typedef struct animation_facial_morphs_desc {
    uint32_t flags;
    void* user_data;
} animation_facial_morphs_desc_t;

typedef struct animation_facial_morphs_info {
    uint32_t id;
    uint32_t flags;
    bool initialized;
} animation_facial_morphs_info_t;

/* ============================================================================
 * API
 * ============================================================================ */

/* Initialization */
int animation_facial_morphs_init(void);
void animation_facial_morphs_shutdown(void);

/* Lifecycle */
int animation_facial_morphs_create(animation_facial_morphs_handle_t* out_handle, const animation_facial_morphs_desc_t* desc);
void animation_facial_morphs_destroy(animation_facial_morphs_handle_t handle);

/* Operations */
int animation_facial_morphs_update(animation_facial_morphs_handle_t handle, const void* data, size_t size);
bool animation_facial_morphs_is_valid(animation_facial_morphs_handle_t handle);
int animation_facial_morphs_get_info(animation_facial_morphs_handle_t handle, animation_facial_morphs_info_t* out_info);
void animation_facial_morphs_mark_dirty(animation_facial_morphs_handle_t handle);
int animation_facial_morphs_process_pending(void);

/* Statistics */
uint32_t animation_facial_morphs_get_count(void);
size_t animation_facial_morphs_get_memory_usage(void);
void animation_facial_morphs_debug_print(void);

#ifdef __cplusplus
}
#endif

#endif /* ANIMATION_FACIAL_MORPHS_H */
