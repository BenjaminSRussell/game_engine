/*
 * gemstone.h
 * Gemstone material
 *
 * Part of the Shading subsystem
 * Advanced 3D Rendering Engine
 */

#ifndef SHADING_GEMSTONE_H
#define SHADING_GEMSTONE_H

#include <stdint.h>
#include <stdbool.h>
#include <stddef.h>

#ifdef __cplusplus
extern "C" {
#endif

/* ============================================================================
 * TYPES
 * ============================================================================ */

typedef struct shading_gemstone_handle {
    uint32_t id;
} shading_gemstone_handle_t;

typedef struct shading_gemstone_desc {
    uint32_t flags;
    void* user_data;
} shading_gemstone_desc_t;

typedef struct shading_gemstone_info {
    uint32_t id;
    uint32_t flags;
    bool initialized;
} shading_gemstone_info_t;

/* ============================================================================
 * API
 * ============================================================================ */

/* Initialization */
int shading_gemstone_init(void);
void shading_gemstone_shutdown(void);

/* Lifecycle */
int shading_gemstone_create(shading_gemstone_handle_t* out_handle, const shading_gemstone_desc_t* desc);
void shading_gemstone_destroy(shading_gemstone_handle_t handle);

/* Operations */
int shading_gemstone_update(shading_gemstone_handle_t handle, const void* data, size_t size);
bool shading_gemstone_is_valid(shading_gemstone_handle_t handle);
int shading_gemstone_get_info(shading_gemstone_handle_t handle, shading_gemstone_info_t* out_info);
void shading_gemstone_mark_dirty(shading_gemstone_handle_t handle);
int shading_gemstone_process_pending(void);

/* Statistics */
uint32_t shading_gemstone_get_count(void);
size_t shading_gemstone_get_memory_usage(void);
void shading_gemstone_debug_print(void);

#ifdef __cplusplus
}
#endif

#endif /* SHADING_GEMSTONE_H */
