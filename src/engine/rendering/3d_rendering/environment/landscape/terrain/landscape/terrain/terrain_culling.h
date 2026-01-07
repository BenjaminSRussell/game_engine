#ifndef LANDSCAPE_TERRAIN_CULLING_H
#define LANDSCAPE_TERRAIN_CULLING_H

#include <stdint.h>
#include <stdbool.h>
#include <stddef.h>

#ifdef __cplusplus
extern "C" {
#endif

/* ============================================================================
 * TYPES
 * ============================================================================ */

typedef struct landscape_terrain_culling_handle {
    uint32_t id;
} landscape_terrain_culling_handle_t;

typedef struct landscape_terrain_culling_desc {
    uint32_t flags;
    void* user_data;
    float max_distance;
} landscape_terrain_culling_desc_t;

typedef struct landscape_terrain_culling_info {
    uint32_t id;
    uint32_t flags;
    bool initialized;
    uint32_t last_visible_count;
} landscape_terrain_culling_info_t;

typedef struct terrain_chunk_aabb {
    float min[3];
    float max[3];
} terrain_chunk_aabb_t;

/* ============================================================================
 * API
 * ============================================================================ */

/* Initialization */
int landscape_terrain_culling_init(void);
void landscape_terrain_culling_shutdown(void);

/* Lifecycle */
int landscape_terrain_culling_create(landscape_terrain_culling_handle_t* out_handle, const landscape_terrain_culling_desc_t* desc);
void landscape_terrain_culling_destroy(landscape_terrain_culling_handle_t handle);

/* Culling Operations */

/*
 * Cull terrain chunks against a frustum.
 * camera_pos: float[3]
 * view_proj_matrix: float[16] (column major)
 * chunks: Array of AABBs
 * chunk_count: Number of chunks
 * out_visibility: Byte array to be filled (0=invisible, 1=visible). Must be at least chunk_count bytes.
 * Returns: Number of visible chunks.
 */
uint32_t landscape_terrain_cull_chunks(
    landscape_terrain_culling_handle_t handle,
    const float* camera_pos,
    const float* view_proj_matrix,
    const terrain_chunk_aabb_t* chunks,
    uint32_t chunk_count,
    uint8_t* out_visibility
);

/* Operations */
int landscape_terrain_culling_update(landscape_terrain_culling_handle_t handle, const void* data, size_t size);
bool landscape_terrain_culling_is_valid(landscape_terrain_culling_handle_t handle);
int landscape_terrain_culling_get_info(landscape_terrain_culling_handle_t handle, landscape_terrain_culling_info_t* out_info);
void landscape_terrain_culling_mark_dirty(landscape_terrain_culling_handle_t handle);
int landscape_terrain_culling_process_pending(void);

/* Statistics */
uint32_t landscape_terrain_culling_get_count(void);
size_t landscape_terrain_culling_get_memory_usage(void);
void landscape_terrain_culling_debug_print(void);

#ifdef __cplusplus
}
#endif

#endif /* LANDSCAPE_TERRAIN_CULLING_H */
