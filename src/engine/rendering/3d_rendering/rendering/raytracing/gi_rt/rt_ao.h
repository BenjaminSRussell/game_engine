/*
 * rt_ao.h
 * Ray-Traced Ambient Occlusion (RTAO)
 *
 * Part of the Ray Tracing subsystem
 * Advanced 3D Rendering Engine
 */

#ifndef RT_AO_H
#define RT_AO_H

#include <stdint.h>
#include <stdbool.h>

#ifdef __cplusplus
extern "C" {
#endif

/* ============================================================================
 * API
 * ============================================================================ */

/* Lifecycle */
int rt_ao_init(void);
void rt_ao_shutdown(void);

/* Operations */
void rt_ao_dispatch(void* cmd_buffer, void* camera_data);
void rt_ao_set_radius(float radius);
void rt_ao_set_quality(int rays_per_pixel);

#ifdef __cplusplus
}
#endif

#endif /* RT_AO_H */
