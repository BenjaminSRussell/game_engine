#ifndef LANDSCAPE_FOLIAGE_SYSTEM_H
#define LANDSCAPE_FOLIAGE_SYSTEM_H

#include <stdint.h>
#include <stdbool.h>
#include <stddef.h>
#include <math/vec3.h>
#include "../vegetation/grass_renderer.h"

#ifdef __cplusplus
extern "C" {
#endif

/* ============================================================================
 * TYPES
 * ============================================================================ */

typedef struct landscape_foliage_system_handle {
    uint32_t id;
} landscape_foliage_system_handle_t;

typedef struct landscape_foliage_system_desc {
    uint32_t flags;
    void* user_data;
    uint32_t seed;
    float global_density_multiplier;
} landscape_foliage_system_desc_t;

typedef struct landscape_foliage_system_info {
    uint32_t id;
    uint32_t flags;
    bool initialized;
    uint32_t active_generators;
} landscape_foliage_system_info_t;

/* ============================================================================
 * API
 * ============================================================================ */

/* Initialization */
int landscape_foliage_system_init(void);
void landscape_foliage_system_shutdown(void);

/* Lifecycle */
int landscape_foliage_system_create(landscape_foliage_system_handle_t* out_handle, const landscape_foliage_system_desc_t* desc);
void landscape_foliage_system_destroy(landscape_foliage_system_handle_t handle);

/* Generation */
// Generates grass instances in a given volume/area
// Returns number of instances generated
uint32_t landscape_foliage_generate_grass_instances(
    landscape_foliage_system_handle_t handle,
    const Vec3* bounds_min,
    const Vec3* bounds_max,
    float density,
    void* out_instances, // Stride assumed from renderer
    uint32_t max_instances
);

/* Operations */
int landscape_foliage_system_update(landscape_foliage_system_handle_t handle, const void* data, size_t size);
bool landscape_foliage_system_is_valid(landscape_foliage_system_handle_t handle);
int landscape_foliage_system_get_info(landscape_foliage_system_handle_t handle, landscape_foliage_system_info_t* out_info);
void landscape_foliage_system_mark_dirty(landscape_foliage_system_handle_t handle);
int landscape_foliage_system_process_pending(void);

/* Statistics */
uint32_t landscape_foliage_system_get_count(void);
size_t landscape_foliage_system_get_memory_usage(void);
void landscape_foliage_system_debug_print(void);

#ifdef __cplusplus
}
#endif

#endif /* LANDSCAPE_FOLIAGE_SYSTEM_H */
