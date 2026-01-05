/*
 * bc_compression.h
 * BC format compression
 *
 * Part of the Texture subsystem
 * Advanced 3D Rendering Engine
 */

#ifndef TEXTURE_BC_COMPRESSION_H
#define TEXTURE_BC_COMPRESSION_H

#include <stdint.h>
#include <stdbool.h>
#include <stddef.h>

#ifdef __cplusplus
extern "C" {
#endif

/* ============================================================================
 * TYPES
 * ============================================================================ */

typedef struct texture_bc_compression_handle {
    uint32_t id;
} texture_bc_compression_handle_t;

typedef struct texture_bc_compression_desc {
    uint32_t flags;
    void* user_data;
} texture_bc_compression_desc_t;

typedef struct texture_bc_compression_info {
    uint32_t id;
    uint32_t flags;
    bool initialized;
} texture_bc_compression_info_t;

/* ============================================================================
 * API
 * ============================================================================ */

/* Initialization */
int texture_bc_compression_init(void);
void texture_bc_compression_shutdown(void);

/* Lifecycle */
int texture_bc_compression_create(texture_bc_compression_handle_t* out_handle, const texture_bc_compression_desc_t* desc);
void texture_bc_compression_destroy(texture_bc_compression_handle_t handle);

/* Operations */
int texture_bc_compression_update(texture_bc_compression_handle_t handle, const void* data, size_t size);
bool texture_bc_compression_is_valid(texture_bc_compression_handle_t handle);
int texture_bc_compression_get_info(texture_bc_compression_handle_t handle, texture_bc_compression_info_t* out_info);
void texture_bc_compression_mark_dirty(texture_bc_compression_handle_t handle);
int texture_bc_compression_process_pending(void);

/* Statistics */
uint32_t texture_bc_compression_get_count(void);
size_t texture_bc_compression_get_memory_usage(void);
void texture_bc_compression_debug_print(void);

#ifdef __cplusplus
}
#endif

#endif /* TEXTURE_BC_COMPRESSION_H */
