/*
 * sampler_cache.h
 * Sampler state caching
 *
 * Part of the Texture subsystem
 * Advanced 3D Rendering Engine
 */

#ifndef TEXTURE_SAMPLER_CACHE_H
#define TEXTURE_SAMPLER_CACHE_H

#include <stdint.h>
#include <stdbool.h>
#include <stddef.h>

#ifdef __cplusplus
extern "C" {
#endif

/* ============================================================================
 * TYPES
 * ============================================================================ */

typedef struct texture_sampler_cache_handle {
    uint32_t id;
} texture_sampler_cache_handle_t;

typedef struct texture_sampler_cache_desc {
    uint32_t flags;
    void* user_data;
} texture_sampler_cache_desc_t;

typedef struct texture_sampler_cache_info {
    uint32_t id;
    uint32_t flags;
    bool initialized;
} texture_sampler_cache_info_t;

/* ============================================================================
 * API
 * ============================================================================ */

/* Initialization */
int texture_sampler_cache_init(void);
void texture_sampler_cache_shutdown(void);

/* Lifecycle */
int texture_sampler_cache_create(texture_sampler_cache_handle_t* out_handle, const texture_sampler_cache_desc_t* desc);
void texture_sampler_cache_destroy(texture_sampler_cache_handle_t handle);

/* Operations */
int texture_sampler_cache_update(texture_sampler_cache_handle_t handle, const void* data, size_t size);
bool texture_sampler_cache_is_valid(texture_sampler_cache_handle_t handle);
int texture_sampler_cache_get_info(texture_sampler_cache_handle_t handle, texture_sampler_cache_info_t* out_info);
void texture_sampler_cache_mark_dirty(texture_sampler_cache_handle_t handle);
int texture_sampler_cache_process_pending(void);

/* Statistics */
uint32_t texture_sampler_cache_get_count(void);
size_t texture_sampler_cache_get_memory_usage(void);
void texture_sampler_cache_debug_print(void);

#ifdef __cplusplus
}
#endif

#endif /* TEXTURE_SAMPLER_CACHE_H */
