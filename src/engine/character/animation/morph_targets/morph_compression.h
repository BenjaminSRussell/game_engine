/*
 * morph_compression.h
 * Morph data compression
 *
 * Part of the Animation subsystem
 * Advanced 3D Rendering Engine
 */

#ifndef ANIMATION_MORPH_COMPRESSION_H
#define ANIMATION_MORPH_COMPRESSION_H

#include <stdint.h>
#include <stdbool.h>
#include <stddef.h>

#ifdef __cplusplus
extern "C" {
#endif

/* ============================================================================
 * TYPES
 * ============================================================================ */

typedef struct animation_morph_compression_handle {
    uint32_t id;
} animation_morph_compression_handle_t;

typedef struct animation_morph_compression_desc {
    uint32_t flags;
    void* user_data;
} animation_morph_compression_desc_t;

typedef struct animation_morph_compression_info {
    uint32_t id;
    uint32_t flags;
    bool initialized;
} animation_morph_compression_info_t;

/* ============================================================================
 * API
 * ============================================================================ */

/* Initialization */
int animation_morph_compression_init(void);
void animation_morph_compression_shutdown(void);

/* Lifecycle */
int animation_morph_compression_create(animation_morph_compression_handle_t* out_handle, const animation_morph_compression_desc_t* desc);
void animation_morph_compression_destroy(animation_morph_compression_handle_t handle);

/* Operations */
int animation_morph_compression_update(animation_morph_compression_handle_t handle, const void* data, size_t size);
bool animation_morph_compression_is_valid(animation_morph_compression_handle_t handle);
int animation_morph_compression_get_info(animation_morph_compression_handle_t handle, animation_morph_compression_info_t* out_info);
void animation_morph_compression_mark_dirty(animation_morph_compression_handle_t handle);
int animation_morph_compression_process_pending(void);

/* Statistics */
uint32_t animation_morph_compression_get_count(void);
size_t animation_morph_compression_get_memory_usage(void);
void animation_morph_compression_debug_print(void);

#ifdef __cplusplus
}
#endif

#endif /* ANIMATION_MORPH_COMPRESSION_H */
