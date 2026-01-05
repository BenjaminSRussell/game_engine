/*
 * vegetation_placement.h
 * Procedural vegetation
 *
 * Part of the Landscape subsystem
 * Advanced 3D Rendering Engine
 */

#ifndef LANDSCAPE_VEGETATION_PLACEMENT_H
#define LANDSCAPE_VEGETATION_PLACEMENT_H

#include <stdint.h>
#include <stdbool.h>
#include <stddef.h>

#ifdef __cplusplus
extern "C" {
#endif

/* ============================================================================
 * TYPES
 * ============================================================================ */

typedef struct landscape_vegetation_placement_handle {
    uint32_t id;
} landscape_vegetation_placement_handle_t;

typedef struct landscape_vegetation_placement_desc {
    uint32_t flags;
    void* user_data;
} landscape_vegetation_placement_desc_t;

typedef struct landscape_vegetation_placement_info {
    uint32_t id;
    uint32_t flags;
    bool initialized;
} landscape_vegetation_placement_info_t;

/* ============================================================================
 * API
 * ============================================================================ */

/* Initialization */
int landscape_vegetation_placement_init(void);
void landscape_vegetation_placement_shutdown(void);

/* Lifecycle */
int landscape_vegetation_placement_create(landscape_vegetation_placement_handle_t* out_handle, const landscape_vegetation_placement_desc_t* desc);
void landscape_vegetation_placement_destroy(landscape_vegetation_placement_handle_t handle);

/* Operations */
int landscape_vegetation_placement_update(landscape_vegetation_placement_handle_t handle, const void* data, size_t size);
bool landscape_vegetation_placement_is_valid(landscape_vegetation_placement_handle_t handle);
int landscape_vegetation_placement_get_info(landscape_vegetation_placement_handle_t handle, landscape_vegetation_placement_info_t* out_info);
void landscape_vegetation_placement_mark_dirty(landscape_vegetation_placement_handle_t handle);
int landscape_vegetation_placement_process_pending(void);

/* Statistics */
uint32_t landscape_vegetation_placement_get_count(void);
size_t landscape_vegetation_placement_get_memory_usage(void);
void landscape_vegetation_placement_debug_print(void);

#ifdef __cplusplus
}
#endif

#endif /* LANDSCAPE_VEGETATION_PLACEMENT_H */
