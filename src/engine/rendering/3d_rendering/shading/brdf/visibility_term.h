/*
 * visibility_term.h
 * Visibility/geometry term
 *
 * Part of the Shading subsystem
 * Advanced 3D Rendering Engine
 */

#ifndef SHADING_VISIBILITY_TERM_H
#define SHADING_VISIBILITY_TERM_H

#include <stdint.h>
#include <stdbool.h>
#include <stddef.h>

#ifdef __cplusplus
extern "C" {
#endif

/* ============================================================================
 * TYPES
 * ============================================================================ */

typedef struct shading_visibility_term_handle {
    uint32_t id;
} shading_visibility_term_handle_t;

typedef struct shading_visibility_term_desc {
    uint32_t flags;
    void* user_data;
} shading_visibility_term_desc_t;

typedef struct shading_visibility_term_info {
    uint32_t id;
    uint32_t flags;
    bool initialized;
} shading_visibility_term_info_t;

/* ============================================================================
 * API
 * ============================================================================ */

/* Initialization */
int shading_visibility_term_init(void);
void shading_visibility_term_shutdown(void);

/* Lifecycle */
int shading_visibility_term_create(shading_visibility_term_handle_t* out_handle, const shading_visibility_term_desc_t* desc);
void shading_visibility_term_destroy(shading_visibility_term_handle_t handle);

/* Operations */
int shading_visibility_term_update(shading_visibility_term_handle_t handle, const void* data, size_t size);
bool shading_visibility_term_is_valid(shading_visibility_term_handle_t handle);
int shading_visibility_term_get_info(shading_visibility_term_handle_t handle, shading_visibility_term_info_t* out_info);
void shading_visibility_term_mark_dirty(shading_visibility_term_handle_t handle);
int shading_visibility_term_process_pending(void);

/* Statistics */
uint32_t shading_visibility_term_get_count(void);
size_t shading_visibility_term_get_memory_usage(void);
void shading_visibility_term_debug_print(void);

#ifdef __cplusplus
}
#endif

#endif /* SHADING_VISIBILITY_TERM_H */
