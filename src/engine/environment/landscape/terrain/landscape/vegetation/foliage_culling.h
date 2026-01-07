#ifndef LANDSCAPE_FOLIAGE_CULLING_H
#define LANDSCAPE_FOLIAGE_CULLING_H

#include <stdint.h>
#include <stdbool.h>
#include <stddef.h>
#include <math/vec3.h> // For bounding box

#ifdef __cplusplus
extern "C" {
#endif

/* ============================================================================
 * TYPES
 * ============================================================================ */

typedef struct landscape_foliage_culling_handle {
    uint32_t id;
} landscape_foliage_culling_handle_t;

typedef struct landscape_foliage_culling_desc {
    uint32_t flags;
    void* user_data;
    float max_distance;
    float fov_y;
} landscape_foliage_culling_desc_t;

typedef struct landscape_foliage_culling_info {
    uint32_t id;
    uint32_t flags;
    bool initialized;
    uint32_t last_culled_count;
} landscape_foliage_culling_info_t;

/* ============================================================================
 * API
 * ============================================================================ */

/* Initialization */
int landscape_foliage_culling_init(void);
void landscape_foliage_culling_shutdown(void);

/* Lifecycle */
int landscape_foliage_culling_create(landscape_foliage_culling_handle_t* out_handle, const landscape_foliage_culling_desc_t* desc);
void landscape_foliage_culling_destroy(landscape_foliage_culling_handle_t handle);

/* Culling Operations */
// Culls instances against a camera frustum and distance
// Returns number of visible instances
uint32_t landscape_foliage_cull_instances(
    landscape_foliage_culling_handle_t handle,
    const Vec3* camera_pos,
    const Vec3* camera_dir,
    const void* instances,
    uint32_t instance_count,
    size_t stride,
    uint8_t* out_visibility_mask // Optional: array of bytes 0=invisible, 1=visible
);

/* Operations */
int landscape_foliage_culling_update(landscape_foliage_culling_handle_t handle, const void* data, size_t size);
bool landscape_foliage_culling_is_valid(landscape_foliage_culling_handle_t handle);
int landscape_foliage_culling_get_info(landscape_foliage_culling_handle_t handle, landscape_foliage_culling_info_t* out_info);
void landscape_foliage_culling_mark_dirty(landscape_foliage_culling_handle_t handle);
int landscape_foliage_culling_process_pending(void);

/* Statistics */
uint32_t landscape_foliage_culling_get_count(void);
size_t landscape_foliage_culling_get_memory_usage(void);
void landscape_foliage_culling_debug_print(void);

#ifdef __cplusplus
}
#endif

#endif /* LANDSCAPE_FOLIAGE_CULLING_H */
