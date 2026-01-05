/*
 * pcf_filter.h
 * PCF shadow filtering
 *
 * Part of the Lighting subsystem
 * Advanced 3D Rendering Engine
 */

#ifndef LIGHTING_PCF_FILTER_H
#define LIGHTING_PCF_FILTER_H

#include <stdint.h>
#include <stdbool.h>
#include <stddef.h>

#ifdef __cplusplus
extern "C" {
#endif

/* ============================================================================
 * TYPES
 * ============================================================================ */

typedef struct lighting_pcf_filter_handle {
    uint32_t id;
} lighting_pcf_filter_handle_t;

typedef struct lighting_pcf_filter_desc {
    uint32_t flags;
    void* user_data;
} lighting_pcf_filter_desc_t;

typedef struct lighting_pcf_filter_info {
    uint32_t id;
    uint32_t flags;
    bool initialized;
} lighting_pcf_filter_info_t;

/* ============================================================================
 * API
 * ============================================================================ */

/* Initialization */
int lighting_pcf_filter_init(void);
void lighting_pcf_filter_shutdown(void);

/* Lifecycle */
int lighting_pcf_filter_create(lighting_pcf_filter_handle_t* out_handle, const lighting_pcf_filter_desc_t* desc);
void lighting_pcf_filter_destroy(lighting_pcf_filter_handle_t handle);

/* Operations */
int lighting_pcf_filter_update(lighting_pcf_filter_handle_t handle, const void* data, size_t size);
bool lighting_pcf_filter_is_valid(lighting_pcf_filter_handle_t handle);
int lighting_pcf_filter_get_info(lighting_pcf_filter_handle_t handle, lighting_pcf_filter_info_t* out_info);
void lighting_pcf_filter_mark_dirty(lighting_pcf_filter_handle_t handle);
int lighting_pcf_filter_process_pending(void);

/* Statistics */
uint32_t lighting_pcf_filter_get_count(void);
size_t lighting_pcf_filter_get_memory_usage(void);
void lighting_pcf_filter_debug_print(void);

/* PCF Sampling */
typedef struct { struct { uint32_t handle; } id; } texture_handle_t;
float lighting_pcf_sample_shadow(texture_handle_t shadow_map, const float* shadow_coord, float bias);
float lighting_pcf_sample_shadow_3x3(texture_handle_t shadow_map, const float* shadow_coord, float bias);
float lighting_pcf_sample_shadow_5x5(texture_handle_t shadow_map, const float* shadow_coord, float bias);
float lighting_pcf_sample_shadow_poisson(texture_handle_t shadow_map, const float* shadow_coord, float bias, uint32_t sample_count);
void lighting_pcf_set_kernel_size(uint32_t size);
void lighting_pcf_set_filter_radius(float radius);


#ifdef __cplusplus
}
#endif

#endif /* LIGHTING_PCF_FILTER_H */
