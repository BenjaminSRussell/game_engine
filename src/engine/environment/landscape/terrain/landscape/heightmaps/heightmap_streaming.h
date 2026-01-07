#ifndef LANDSCAPE_HEIGHTMAP_STREAMING_H
#define LANDSCAPE_HEIGHTMAP_STREAMING_H

#include <stdint.h>
#include <stdbool.h>
#include <stddef.h>
#include <math/vec3.h> // For coordinate mapping

#ifdef __cplusplus
extern "C" {
#endif

/* ============================================================================
 * TYPES
 * ============================================================================ */

typedef struct landscape_heightmap_streaming_handle {
    uint32_t id;
} landscape_heightmap_streaming_handle_t;

typedef struct heightmap_tile {
    uint32_t x;
    uint32_t y;
    uint32_t lod_level;
    uint16_t* height_data;  // 16-bit height data (0-65535)
    uint32_t width;
    uint32_t height;
    bool loaded;
    bool dirty;
    uint64_t last_accessed;
} heightmap_tile_t;

typedef struct landscape_heightmap_streaming_desc {
    uint32_t flags;
    void* user_data;
    uint32_t tile_size;           // e.g., 512
    float world_scale;            // Units per tile
    float height_scale;           // Vertical scale
    const char* data_source;      // Path or identifier
} landscape_heightmap_streaming_desc_t;

typedef struct landscape_heightmap_streaming_info {
    uint32_t id;
    uint32_t flags;
    bool initialized;
    uint32_t tiles_loaded;
    uint32_t memory_used;
} landscape_heightmap_streaming_info_t;

/* ============================================================================
 * API
 * ============================================================================ */

/* Initialization */
int landscape_heightmap_streaming_init(void);
void landscape_heightmap_streaming_shutdown(void);

/* Lifecycle */
int landscape_heightmap_streaming_create(landscape_heightmap_streaming_handle_t* out_handle, const landscape_heightmap_streaming_desc_t* desc);
void landscape_heightmap_streaming_destroy(landscape_heightmap_streaming_handle_t handle);

/* Operations */
int landscape_heightmap_streaming_update(landscape_heightmap_streaming_handle_t handle, Vec3 camera_pos);
bool landscape_heightmap_streaming_is_valid(landscape_heightmap_streaming_handle_t handle);
int landscape_heightmap_streaming_get_info(landscape_heightmap_streaming_handle_t handle, landscape_heightmap_streaming_info_t* out_info);
void landscape_heightmap_streaming_mark_dirty(landscape_heightmap_streaming_handle_t handle);
int landscape_heightmap_streaming_process_pending(void);

/* Data Access */
float landscape_heightmap_sample(landscape_heightmap_streaming_handle_t handle, float x, float z);
heightmap_tile_t* landscape_heightmap_get_tile(landscape_heightmap_streaming_handle_t handle, uint32_t max_x, uint32_t max_y);
void landscape_heightmap_prefetch(landscape_heightmap_streaming_handle_t handle, float x, float z, float radius);

/* Statistics */
uint32_t landscape_heightmap_streaming_get_count(void);
size_t landscape_heightmap_streaming_get_memory_usage(void);
void landscape_heightmap_streaming_debug_print(void);

#ifdef __cplusplus
}
#endif

#endif /* LANDSCAPE_HEIGHTMAP_STREAMING_H */
