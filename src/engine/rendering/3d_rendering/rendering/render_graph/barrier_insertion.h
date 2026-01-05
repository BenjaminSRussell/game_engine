/*
 * barrier_insertion.h
 * Automatic barriers
 *
 * Part of the Rendering subsystem
 * Advanced 3D Rendering Engine
 */

#ifndef RENDERING_BARRIER_INSERTION_H
#define RENDERING_BARRIER_INSERTION_H

#include <stdint.h>
#include <stdbool.h>
#include <stddef.h>

#ifdef __cplusplus
extern "C" {
#endif

/* ============================================================================
 * TYPES
 * ============================================================================ */

typedef struct rendering_barrier_insertion_handle {
    uint32_t id;
} rendering_barrier_insertion_handle_t;

typedef struct rendering_barrier_insertion_desc {
    uint32_t flags;
    void* user_data;
} rendering_barrier_insertion_desc_t;

typedef struct rendering_barrier_insertion_info {
    uint32_t id;
    uint32_t flags;
    bool initialized;
} rendering_barrier_insertion_info_t;

/* ============================================================================
 * API
 * ============================================================================ */

/* Initialization */
int rendering_barrier_insertion_init(void);
void rendering_barrier_insertion_shutdown(void);

/* Lifecycle */
int rendering_barrier_insertion_create(rendering_barrier_insertion_handle_t* out_handle, const rendering_barrier_insertion_desc_t* desc);
void rendering_barrier_insertion_destroy(rendering_barrier_insertion_handle_t handle);

/* Operations */
int rendering_barrier_insertion_update(rendering_barrier_insertion_handle_t handle, const void* data, size_t size);
bool rendering_barrier_insertion_is_valid(rendering_barrier_insertion_handle_t handle);
int rendering_barrier_insertion_get_info(rendering_barrier_insertion_handle_t handle, rendering_barrier_insertion_info_t* out_info);
void rendering_barrier_insertion_mark_dirty(rendering_barrier_insertion_handle_t handle);
int rendering_barrier_insertion_process_pending(void);

/* Statistics */
uint32_t rendering_barrier_insertion_get_count(void);
size_t rendering_barrier_insertion_get_memory_usage(void);
void rendering_barrier_insertion_debug_print(void);

#ifdef __cplusplus
}
#endif

#endif /* RENDERING_BARRIER_INSERTION_H */
