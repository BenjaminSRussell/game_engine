/*
 * fuzz_lighting.h
 * Fuzz lighting
 *
 * Part of the Shading subsystem
 * Advanced 3D Rendering Engine
 */

#ifndef SHADING_FUZZ_LIGHTING_H
#define SHADING_FUZZ_LIGHTING_H

#include <stdint.h>
#include <stdbool.h>
#include <stddef.h>

#ifdef __cplusplus
extern "C" {
#endif

/* ============================================================================
 * TYPES
 * ============================================================================ */

typedef struct shading_fuzz_lighting_handle {
    uint32_t id;
} shading_fuzz_lighting_handle_t;

typedef struct shading_fuzz_lighting_desc {
    uint32_t flags;
    void* user_data;
} shading_fuzz_lighting_desc_t;

typedef struct shading_fuzz_lighting_info {
    uint32_t id;
    uint32_t flags;
    bool initialized;
} shading_fuzz_lighting_info_t;

/* ============================================================================
 * API
 * ============================================================================ */

/* Initialization */
int shading_fuzz_lighting_init(void);
void shading_fuzz_lighting_shutdown(void);

/* Lifecycle */
int shading_fuzz_lighting_create(shading_fuzz_lighting_handle_t* out_handle, const shading_fuzz_lighting_desc_t* desc);
void shading_fuzz_lighting_destroy(shading_fuzz_lighting_handle_t handle);

/* Operations */
int shading_fuzz_lighting_update(shading_fuzz_lighting_handle_t handle, const void* data, size_t size);
bool shading_fuzz_lighting_is_valid(shading_fuzz_lighting_handle_t handle);
int shading_fuzz_lighting_get_info(shading_fuzz_lighting_handle_t handle, shading_fuzz_lighting_info_t* out_info);
void shading_fuzz_lighting_mark_dirty(shading_fuzz_lighting_handle_t handle);
int shading_fuzz_lighting_process_pending(void);

/* Statistics */
uint32_t shading_fuzz_lighting_get_count(void);
size_t shading_fuzz_lighting_get_memory_usage(void);
void shading_fuzz_lighting_debug_print(void);

#ifdef __cplusplus
}
#endif

#endif /* SHADING_FUZZ_LIGHTING_H */
