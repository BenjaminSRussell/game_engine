#ifndef LANDSCAPE_FOLIAGE_WIND_H
#define LANDSCAPE_FOLIAGE_WIND_H

#include <stdint.h>
#include <stdbool.h>
#include <stddef.h>
#include <math/vec3.h> 

#ifdef __cplusplus
extern "C" {
#endif

/* ============================================================================
 * TYPES
 * ============================================================================ */

typedef struct landscape_foliage_wind_handle {
    uint32_t id;
} landscape_foliage_wind_handle_t;

typedef struct landscape_foliage_wind_desc {
    uint32_t flags;
    void* user_data;
    Vec3 direction;
    float strength;
    float frequency;
    float turbulence_scale;
    float turbulence_speed;
} landscape_foliage_wind_desc_t;

typedef struct landscape_foliage_wind_info {
    uint32_t id;
    uint32_t flags;
    bool initialized;
} landscape_foliage_wind_info_t;

/* ============================================================================
 * API
 * ============================================================================ */

/* Initialization */
int landscape_foliage_wind_init(void);
void landscape_foliage_wind_shutdown(void);

/* Lifecycle */
int landscape_foliage_wind_create(landscape_foliage_wind_handle_t* out_handle, const landscape_foliage_wind_desc_t* desc);
void landscape_foliage_wind_destroy(landscape_foliage_wind_handle_t handle);

/* Operations */

// Update wind simulation state
int landscape_foliage_wind_update(landscape_foliage_wind_handle_t handle, float delta_time);

// Calculate wind displacement for a given position
// Used for CPU-side simulation or debug, GPU usually handles this
Vec3 landscape_foliage_wind_get_displacement(
    landscape_foliage_wind_handle_t handle,
    const Vec3* position,
    float stiffness
);

bool landscape_foliage_wind_is_valid(landscape_foliage_wind_handle_t handle);
int landscape_foliage_wind_get_info(landscape_foliage_wind_handle_t handle, landscape_foliage_wind_info_t* out_info);
void landscape_foliage_wind_mark_dirty(landscape_foliage_wind_handle_t handle);
int landscape_foliage_wind_process_pending(void);

/* Statistics */
uint32_t landscape_foliage_wind_get_count(void);
size_t landscape_foliage_wind_get_memory_usage(void);
void landscape_foliage_wind_debug_print(void);

#ifdef __cplusplus
}
#endif

#endif /* LANDSCAPE_FOLIAGE_WIND_H */
