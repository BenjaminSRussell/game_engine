/*
 * prefetch_hints.h
 * Texture prefetch hints
 *
 * Part of the Texture subsystem
 * Advanced 3D Rendering Engine
 */

#ifndef TEXTURE_PREFETCH_HINTS_H
#define TEXTURE_PREFETCH_HINTS_H

#include <stdint.h>
#include <stdbool.h>
#include <stddef.h>

#ifdef __cplusplus
extern "C" {
#endif

/* ============================================================================
 * TYPES
 * ============================================================================ */

typedef struct texture_prefetch_hints_handle {
    uint32_t id;
} texture_prefetch_hints_handle_t;

typedef struct texture_prefetch_hints_desc {
    uint32_t flags;
    void* user_data;
} texture_prefetch_hints_desc_t;

typedef struct texture_prefetch_hints_info {
    uint32_t id;
    uint32_t flags;
    bool initialized;
} texture_prefetch_hints_info_t;

/* ============================================================================
 * API
 * ============================================================================ */

/* Initialization */
int texture_prefetch_hints_init(void);
void texture_prefetch_hints_shutdown(void);

/* Lifecycle */
int texture_prefetch_hints_create(texture_prefetch_hints_handle_t* out_handle, const texture_prefetch_hints_desc_t* desc);
void texture_prefetch_hints_destroy(texture_prefetch_hints_handle_t handle);

/* Operations */
int texture_prefetch_hints_update(texture_prefetch_hints_handle_t handle, const void* data, size_t size);
bool texture_prefetch_hints_is_valid(texture_prefetch_hints_handle_t handle);
int texture_prefetch_hints_get_info(texture_prefetch_hints_handle_t handle, texture_prefetch_hints_info_t* out_info);
void texture_prefetch_hints_mark_dirty(texture_prefetch_hints_handle_t handle);
int texture_prefetch_hints_process_pending(void);

/* Statistics */
uint32_t texture_prefetch_hints_get_count(void);
size_t texture_prefetch_hints_get_memory_usage(void);
void texture_prefetch_hints_debug_print(void);

#ifdef __cplusplus
}
#endif

#endif /* TEXTURE_PREFETCH_HINTS_H */
