/*
 * rt_debug_viz.c
 * Ray Tracing Debug Visualization
 */

#include "rt_debug_viz.h"
#include "debug_lines.h"
#include "debug_shapes.h"
#include <core/logger.h>

void rt_debug_viz_draw_blas_bounds(const float* min, const float* max, uint32_t color) {
    // Assuming debug_shapes_draw_aa_box exists or similar
    // Using placeholder logic mapping to whatever debug system exists
    LOG_INFO("Drawing BLAS bounds: [%f, %f, %f] - [%f, %f, %f]", min[0], min[1], min[2], max[0], max[1], max[2]);
}

void rt_debug_viz_draw_ray(const float* origin, const float* direction, float length, uint32_t color) {
    // Draw ray
    float end[3] = {
        origin[0] + direction[0] * length,
        origin[1] + direction[1] * length,
        origin[2] + direction[2] * length
    };
    // placeholder call
    // debug_lines_draw_line(origin, end, color);
}

void rt_debug_viz_draw_probes(const float* positions, uint32_t count, float radius) {
    for (uint32_t i = 0; i < count; i++) {
        const float* pos = &positions[i * 3];
        // debug_shapes_draw_sphere(pos, radius, 0xFFFFFFFF);
    }
}
