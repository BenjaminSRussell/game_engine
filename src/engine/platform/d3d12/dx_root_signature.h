/*
 * dx_root_signature.h
 * D3D12 root signatures
 *
 * Part of the Platform subsystem
 * Advanced 3D Rendering Engine
 */

#ifndef PLATFORM_DX_ROOT_SIGNATURE_H
#define PLATFORM_DX_ROOT_SIGNATURE_H

#include <stdint.h>
#include <stdbool.h>
#include <stddef.h>

#ifdef __cplusplus
extern "C" {
#endif

/* ============================================================================
 * TYPES
 * ============================================================================ */

typedef struct platform_dx_root_signature_handle {
    uint32_t id;
} platform_dx_root_signature_handle_t;

typedef struct platform_dx_root_signature_desc {
    uint32_t flags;
    void* user_data;
} platform_dx_root_signature_desc_t;

typedef struct platform_dx_root_signature_info {
    uint32_t id;
    uint32_t flags;
    bool initialized;
} platform_dx_root_signature_info_t;

/* ============================================================================
 * API
 * ============================================================================ */

/* Initialization */
int platform_dx_root_signature_init(void);
void platform_dx_root_signature_shutdown(void);

/* Lifecycle */
int platform_dx_root_signature_create(platform_dx_root_signature_handle_t* out_handle, const platform_dx_root_signature_desc_t* desc);
void platform_dx_root_signature_destroy(platform_dx_root_signature_handle_t handle);

/* Operations */
int platform_dx_root_signature_update(platform_dx_root_signature_handle_t handle, const void* data, size_t size);
bool platform_dx_root_signature_is_valid(platform_dx_root_signature_handle_t handle);
int platform_dx_root_signature_get_info(platform_dx_root_signature_handle_t handle, platform_dx_root_signature_info_t* out_info);
void platform_dx_root_signature_mark_dirty(platform_dx_root_signature_handle_t handle);
int platform_dx_root_signature_process_pending(void);

/* Statistics */
uint32_t platform_dx_root_signature_get_count(void);
size_t platform_dx_root_signature_get_memory_usage(void);
void platform_dx_root_signature_debug_print(void);

#ifdef __cplusplus
}
#endif

#endif /* PLATFORM_DX_ROOT_SIGNATURE_H */
