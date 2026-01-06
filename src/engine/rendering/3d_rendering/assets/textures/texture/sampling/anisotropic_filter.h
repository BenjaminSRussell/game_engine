/*
 * anisotropic_filter.h
 * Anisotropic filtering
 *
 * Part of the Texture subsystem
 * Advanced 3D Rendering Engine
 */

#ifndef TEXTURE_ANISOTROPIC_FILTER_H
#define TEXTURE_ANISOTROPIC_FILTER_H

#include <stdint.h>
#include <stdbool.h>
#include <stddef.h>

#ifdef __cplusplus
extern "C" {
#endif

/* ============================================================================
 * TYPES
 * ============================================================================ */

typedef struct texture_anisotropic_filter_handle {
    uint32_t id;
} texture_anisotropic_filter_handle_t;

typedef struct texture_anisotropic_filter_desc {
    uint32_t flags;
    void* user_data;
} texture_anisotropic_filter_desc_t;

typedef struct texture_anisotropic_filter_info {
    uint32_t id;
    uint32_t flags;
    bool initialized;
} texture_anisotropic_filter_info_t;

/* ============================================================================
 * API
 * ============================================================================ */

/* Initialization */
int texture_anisotropic_filter_init(void);
void texture_anisotropic_filter_shutdown(void);

/* Lifecycle */
int texture_anisotropic_filter_create(texture_anisotropic_filter_handle_t* out_handle, const texture_anisotropic_filter_desc_t* desc);
void texture_anisotropic_filter_destroy(texture_anisotropic_filter_handle_t handle);

/* Operations */
int texture_anisotropic_filter_update(texture_anisotropic_filter_handle_t handle, const void* data, size_t size);
bool texture_anisotropic_filter_is_valid(texture_anisotropic_filter_handle_t handle);
int texture_anisotropic_filter_get_info(texture_anisotropic_filter_handle_t handle, texture_anisotropic_filter_info_t* out_info);
void texture_anisotropic_filter_mark_dirty(texture_anisotropic_filter_handle_t handle);
int texture_anisotropic_filter_process_pending(void);

/* Statistics */
uint32_t texture_anisotropic_filter_get_count(void);
size_t texture_anisotropic_filter_get_memory_usage(void);
void texture_anisotropic_filter_debug_print(void);

#ifdef __cplusplus
}
#endif

#endif /* TEXTURE_ANISOTROPIC_FILTER_H */
