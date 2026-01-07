/*
 * iridescence.h
 * Thin-film iridescence
 *
 * Part of the Shading subsystem
 * Advanced 3D Rendering Engine
 */

#ifndef SHADING_IRIDESCENCE_H
#define SHADING_IRIDESCENCE_H

#include <stdint.h>
#include <stdbool.h>
#include <stddef.h>

#ifdef __cplusplus
extern "C" {
#endif

/* ============================================================================
 * TYPES
 * ============================================================================ */

typedef struct shading_iridescence_handle {
    uint32_t id;
} shading_iridescence_handle_t;

typedef struct shading_iridescence_desc {
    uint32_t flags;
    void* user_data;
} shading_iridescence_desc_t;

typedef struct shading_iridescence_info {
    uint32_t id;
    uint32_t flags;
    bool initialized;
} shading_iridescence_info_t;

/* ============================================================================
 * API
 * ============================================================================ */

/* Initialization */
int shading_iridescence_init(void);
void shading_iridescence_shutdown(void);

/* Lifecycle */
int shading_iridescence_create(shading_iridescence_handle_t* out_handle, const shading_iridescence_desc_t* desc);
void shading_iridescence_destroy(shading_iridescence_handle_t handle);

/* Operations */
int shading_iridescence_update(shading_iridescence_handle_t handle, const void* data, size_t size);
bool shading_iridescence_is_valid(shading_iridescence_handle_t handle);
int shading_iridescence_get_info(shading_iridescence_handle_t handle, shading_iridescence_info_t* out_info);
void shading_iridescence_mark_dirty(shading_iridescence_handle_t handle);
int shading_iridescence_process_pending(void);

/* Statistics */
uint32_t shading_iridescence_get_count(void);
size_t shading_iridescence_get_memory_usage(void);
void shading_iridescence_debug_print(void);

#ifdef __cplusplus
}
#endif

#endif /* SHADING_IRIDESCENCE_H */
