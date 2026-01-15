/*
 * voronoi_fracture.h
 * Voronoi fracturing
 *
 * Part of the Destruction subsystem
 * Advanced 3D Rendering Engine
 */

#ifndef DESTRUCTION_VORONOI_FRACTURE_H
#define DESTRUCTION_VORONOI_FRACTURE_H

#include <stdint.h>
#include <stdbool.h>
#include <stddef.h>

#ifdef __cplusplus
extern "C" {
#endif

/* ============================================================================
 * TYPES
 * ============================================================================ */

typedef struct destruction_voronoi_fracture_handle {
    uint32_t id;
} destruction_voronoi_fracture_handle_t;

typedef struct destruction_voronoi_fracture_desc {
    uint32_t flags;
    void* user_data;
} destruction_voronoi_fracture_desc_t;

typedef struct destruction_voronoi_fracture_info {
    uint32_t id;
    uint32_t flags;
    bool initialized;
} destruction_voronoi_fracture_info_t;

/* ============================================================================
 * API
 * ============================================================================ */

/* Initialization */
int destruction_voronoi_fracture_init(void);
void destruction_voronoi_fracture_shutdown(void);

/* Lifecycle */
int destruction_voronoi_fracture_create(destruction_voronoi_fracture_handle_t* out_handle, const destruction_voronoi_fracture_desc_t* desc);
void destruction_voronoi_fracture_destroy(destruction_voronoi_fracture_handle_t handle);

/* Operations */
int destruction_voronoi_fracture_update(destruction_voronoi_fracture_handle_t handle, const void* data, size_t size);
bool destruction_voronoi_fracture_is_valid(destruction_voronoi_fracture_handle_t handle);
int destruction_voronoi_fracture_get_info(destruction_voronoi_fracture_handle_t handle, destruction_voronoi_fracture_info_t* out_info);
void destruction_voronoi_fracture_mark_dirty(destruction_voronoi_fracture_handle_t handle);
int destruction_voronoi_fracture_process_pending(void);

/* Statistics */
uint32_t destruction_voronoi_fracture_get_count(void);
size_t destruction_voronoi_fracture_get_memory_usage(void);
void destruction_voronoi_fracture_debug_print(void);

#ifdef __cplusplus
}
#endif

#endif /* DESTRUCTION_VORONOI_FRACTURE_H */
