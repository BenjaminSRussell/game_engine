#ifndef LANDSCAPE_TERRAIN_GENERATION_H
#define LANDSCAPE_TERRAIN_GENERATION_H

#include <stdint.h>
#include <stdbool.h>
#include <stddef.h>

#ifdef __cplusplus
extern "C" {
#endif

/* ============================================================================
 * TYPES
 * ============================================================================ */

typedef struct landscape_terrain_generation_handle {
    uint32_t id;
} landscape_terrain_generation_handle_t;

typedef struct landscape_terrain_generation_desc {
    uint32_t flags;
    void* user_data;
    uint32_t seed;
    float scale;
    float persistence;
    float lacunarity;
    int octaves;
    float height_scale;
} landscape_terrain_generation_desc_t;

typedef struct landscape_terrain_generation_info {
    uint32_t id;
    uint32_t flags;
    bool initialized;
} landscape_terrain_generation_info_t;

/* ============================================================================
 * API
 * ============================================================================ */

/* Initialization */
int landscape_terrain_generation_init(void);
void landscape_terrain_generation_shutdown(void);

/* Lifecycle */
int landscape_terrain_generation_create(landscape_terrain_generation_handle_t* out_handle, const landscape_terrain_generation_desc_t* desc);
void landscape_terrain_generation_destroy(landscape_terrain_generation_handle_t handle);

/* Generation Operations */

/*
 * Generate heightmap data for a specific region.
 * region_x, region_z: Coordinates of the region (e.g., chunk coordinates)
 * width, height: Dimensions of the output buffer
 * out_heightmap: float buffer to fill (must be width * height)
 */
int landscape_terrain_generation_fill_heightmap(
    landscape_terrain_generation_handle_t handle,
    int region_x,
    int region_z,
    int width,
    int height,
    float* out_heightmap
);

/* Operations */
int landscape_terrain_generation_update(landscape_terrain_generation_handle_t handle, const void* data, size_t size);
bool landscape_terrain_generation_is_valid(landscape_terrain_generation_handle_t handle);
int landscape_terrain_generation_get_info(landscape_terrain_generation_handle_t handle, landscape_terrain_generation_info_t* out_info);
void landscape_terrain_generation_mark_dirty(landscape_terrain_generation_handle_t handle);
int landscape_terrain_generation_process_pending(void);

/* Statistics */
uint32_t landscape_terrain_generation_get_count(void);
size_t landscape_terrain_generation_get_memory_usage(void);
void landscape_terrain_generation_debug_print(void);

#ifdef __cplusplus
}
#endif

#endif /* LANDSCAPE_TERRAIN_GENERATION_H */
