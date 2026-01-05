/*
 * basis_transcoder.h
 * Basis Universal transcoding
 *
 * Part of the Texture subsystem
 * Advanced 3D Rendering Engine
 */

#ifndef TEXTURE_BASIS_TRANSCODER_H
#define TEXTURE_BASIS_TRANSCODER_H

#include <stdint.h>
#include <stdbool.h>
#include <stddef.h>

#ifdef __cplusplus
extern "C" {
#endif

/* ============================================================================
 * TYPES
 * ============================================================================ */

typedef struct texture_basis_transcoder_handle {
    uint32_t id;
} texture_basis_transcoder_handle_t;

typedef struct texture_basis_transcoder_desc {
    uint32_t flags;
    void* user_data;
} texture_basis_transcoder_desc_t;

typedef struct texture_basis_transcoder_info {
    uint32_t id;
    uint32_t flags;
    bool initialized;
} texture_basis_transcoder_info_t;

/* ============================================================================
 * API
 * ============================================================================ */

/* Initialization */
int texture_basis_transcoder_init(void);
void texture_basis_transcoder_shutdown(void);

/* Lifecycle */
int texture_basis_transcoder_create(texture_basis_transcoder_handle_t* out_handle, const texture_basis_transcoder_desc_t* desc);
void texture_basis_transcoder_destroy(texture_basis_transcoder_handle_t handle);

/* Operations */
int texture_basis_transcoder_update(texture_basis_transcoder_handle_t handle, const void* data, size_t size);
bool texture_basis_transcoder_is_valid(texture_basis_transcoder_handle_t handle);
int texture_basis_transcoder_get_info(texture_basis_transcoder_handle_t handle, texture_basis_transcoder_info_t* out_info);
void texture_basis_transcoder_mark_dirty(texture_basis_transcoder_handle_t handle);
int texture_basis_transcoder_process_pending(void);

/* Statistics */
uint32_t texture_basis_transcoder_get_count(void);
size_t texture_basis_transcoder_get_memory_usage(void);
void texture_basis_transcoder_debug_print(void);

#ifdef __cplusplus
}
#endif

#endif /* TEXTURE_BASIS_TRANSCODER_H */
