/*
 * present.h
 * Frame presentation
 *
 * Part of the Rendering subsystem
 * Advanced 3D Rendering Engine
 */

#ifndef RENDERING_PRESENT_H
#define RENDERING_PRESENT_H

#include <stdint.h>
#include <stdbool.h>
#include <stddef.h>

#ifdef __cplusplus
extern "C" {
#endif

/* ============================================================================
 * TYPES
 * ============================================================================ */

typedef struct rendering_present_handle {
    uint32_t id;
} rendering_present_handle_t;

typedef struct rendering_present_desc {
    uint32_t flags;
    void* user_data;
} rendering_present_desc_t;

typedef struct rendering_present_info {
    uint32_t id;
    uint32_t flags;
    bool initialized;
} rendering_present_info_t;

/* ============================================================================
 * API
 * ============================================================================ */

/* Initialization */
int rendering_present_init(void);
void rendering_present_shutdown(void);

/* Lifecycle */
int rendering_present_create(rendering_present_handle_t* out_handle, const rendering_present_desc_t* desc);
void rendering_present_destroy(rendering_present_handle_t handle);

/* Operations */
int rendering_present_update(rendering_present_handle_t handle, const void* data, size_t size);
bool rendering_present_is_valid(rendering_present_handle_t handle);
int rendering_present_get_info(rendering_present_handle_t handle, rendering_present_info_t* out_info);
void rendering_present_mark_dirty(rendering_present_handle_t handle);
int rendering_present_process_pending(void);

/* Statistics */
uint32_t rendering_present_get_count(void);
size_t rendering_present_get_memory_usage(void);
void rendering_present_debug_print(void);

#ifdef __cplusplus
}
#endif

#endif /* RENDERING_PRESENT_H */
