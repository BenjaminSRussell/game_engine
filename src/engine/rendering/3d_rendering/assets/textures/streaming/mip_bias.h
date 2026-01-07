/*
 * mip_bias.h
 * Streaming mip bias
 *
 * Part of the Texture subsystem
 * Advanced 3D Rendering Engine
 */

#ifndef TEXTURE_MIP_BIAS_H
#define TEXTURE_MIP_BIAS_H

#include <stdint.h>
#include <stdbool.h>
#include <stddef.h>

#ifdef __cplusplus
extern "C" {
#endif

/* ============================================================================
 * TYPES
 * ============================================================================ */

typedef struct texture_mip_bias_handle {
    uint32_t id;
} texture_mip_bias_handle_t;

typedef struct texture_mip_bias_desc {
    uint32_t flags;
    void* user_data;
} texture_mip_bias_desc_t;

typedef struct texture_mip_bias_info {
    uint32_t id;
    uint32_t flags;
    bool initialized;
} texture_mip_bias_info_t;

/* ============================================================================
 * API
 * ============================================================================ */

/* Initialization */
int texture_mip_bias_init(void);
void texture_mip_bias_shutdown(void);

/* Lifecycle */
int texture_mip_bias_create(texture_mip_bias_handle_t* out_handle, const texture_mip_bias_desc_t* desc);
void texture_mip_bias_destroy(texture_mip_bias_handle_t handle);

/* Operations */
int texture_mip_bias_update(texture_mip_bias_handle_t handle, const void* data, size_t size);
bool texture_mip_bias_is_valid(texture_mip_bias_handle_t handle);
int texture_mip_bias_get_info(texture_mip_bias_handle_t handle, texture_mip_bias_info_t* out_info);
void texture_mip_bias_mark_dirty(texture_mip_bias_handle_t handle);
int texture_mip_bias_process_pending(void);

/* Statistics */
uint32_t texture_mip_bias_get_count(void);
size_t texture_mip_bias_get_memory_usage(void);
void texture_mip_bias_debug_print(void);

#ifdef __cplusplus
}
#endif

#endif /* TEXTURE_MIP_BIAS_H */
