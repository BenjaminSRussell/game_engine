/*
 * resident_mips.h
 * Resident mip tracking
 *
 * Part of the Texture subsystem
 * Advanced 3D Rendering Engine
 */

#ifndef TEXTURE_RESIDENT_MIPS_H
#define TEXTURE_RESIDENT_MIPS_H

#include <stdint.h>
#include <stdbool.h>
#include <stddef.h>

#ifdef __cplusplus
extern "C" {
#endif

/* ============================================================================
 * TYPES
 * ============================================================================ */

typedef struct texture_resident_mips_handle {
    uint32_t id;
} texture_resident_mips_handle_t;

typedef struct texture_resident_mips_desc {
    uint32_t flags;
    void* user_data;
} texture_resident_mips_desc_t;

typedef struct texture_resident_mips_info {
    uint32_t id;
    uint32_t flags;
    bool initialized;
} texture_resident_mips_info_t;

/* ============================================================================
 * API
 * ============================================================================ */

/* Initialization */
int texture_resident_mips_init(void);
void texture_resident_mips_shutdown(void);

/* Lifecycle */
int texture_resident_mips_create(texture_resident_mips_handle_t* out_handle, const texture_resident_mips_desc_t* desc);
void texture_resident_mips_destroy(texture_resident_mips_handle_t handle);

/* Operations */
int texture_resident_mips_update(texture_resident_mips_handle_t handle, const void* data, size_t size);
bool texture_resident_mips_is_valid(texture_resident_mips_handle_t handle);
int texture_resident_mips_get_info(texture_resident_mips_handle_t handle, texture_resident_mips_info_t* out_info);
void texture_resident_mips_mark_dirty(texture_resident_mips_handle_t handle);
int texture_resident_mips_process_pending(void);

/* Statistics */
uint32_t texture_resident_mips_get_count(void);
size_t texture_resident_mips_get_memory_usage(void);
void texture_resident_mips_debug_print(void);

#ifdef __cplusplus
}
#endif

#endif /* TEXTURE_RESIDENT_MIPS_H */
