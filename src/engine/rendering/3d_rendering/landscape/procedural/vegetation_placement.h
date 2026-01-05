#ifndef LANDSCAPE_VEGETATION_PLACEMENT_H
#define LANDSCAPE_VEGETATION_PLACEMENT_H

#include <stdint.h>
#include <stdbool.h>
#include <stddef.h>
#include "../vegetation/tree_renderer.h"

#ifdef __cplusplus
extern "C" {
#endif

/* ============================================================================
 * TYPES
 * ============================================================================ */

typedef struct landscape_vegetation_placement_handle {
    uint32_t id;
} landscape_vegetation_placement_handle_t;

typedef struct vegetation_rule {
    float min_height;
    float max_height;
    float min_slope;
    float max_slope;
    float density; // Instances per square unit (approx)
    uint32_t seed_offset;
    float scale_min;
    float scale_max;
} vegetation_rule_t;

typedef struct landscape_vegetation_placement_desc {
    uint32_t flags;
    void* user_data;
    vegetation_rule_t rule;
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

/* Main function */

/*
 * Generate instances based on heightmap and rules.
 * heightmap: Array of height values
 * width, height: Dimensions of heightmap
 * spacing: World space distance between heightmap pixels
 * out_positions: Array of Vec3 to fill (caller must allocate enough)
 * out_scales: Array of float to fill
 * out_rotations: Array of float to fill
 * max_instances: Capacity of output arrays
 * Returns actual number of instances generated.
 */
uint32_t landscape_vegetation_placement_generate(
    landscape_vegetation_placement_handle_t handle,
    const float* heightmap,
    int width,
    int height,
    float spacing,
    Vec3* out_positions,
    float* out_scales,
    float* out_rotations,
    uint32_t max_instances
);

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
