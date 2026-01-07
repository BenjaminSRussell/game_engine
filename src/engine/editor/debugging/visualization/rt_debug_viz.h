/*
 * rt_debug_viz.h
 * Ray Tracing Debug Visualization
 */

#ifndef RT_DEBUG_VIZ_H
#define RT_DEBUG_VIZ_H

#include <stdint.h>

void rt_debug_viz_draw_blas_bounds(const float* min, const float* max, uint32_t color);
void rt_debug_viz_draw_ray(const float* origin, const float* direction, float length, uint32_t color);
void rt_debug_viz_draw_probes(const float* positions, uint32_t count, float radius);

#endif // RT_DEBUG_VIZ_H
