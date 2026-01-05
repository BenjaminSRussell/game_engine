/*
 * batch_statistics.c
 * Rendering Statistics and Metrics
 *
 * Part of the Scene Management subsystem
 * Advanced 3D Rendering Engine
 */

#include "batch_statistics.h"
#include <stdint.h>
#include <string.h>

typedef struct render_stats {
    uint32_t draw_calls;
    uint32_t instances_drawn;
    uint32_t triangles_drawn;
    uint32_t vertices_processed;
    uint32_t batches_created;
    uint32_t culled_instances;
} render_stats_t;

static render_stats_t g_stats = {0};
static render_stats_t g_last_frame_stats = {0};

void batch_statistics_reset(void) {
    g_last_frame_stats = g_stats;
    memset(&g_stats, 0, sizeof(render_stats_t));
}

void batch_statistics_add_draw(uint32_t count, uint32_t tris) {
    g_stats.draw_calls++;
    g_stats.instances_drawn += count;
    g_stats.triangles_drawn += tris;
}

void batch_statistics_add_cull(uint32_t count) {
    g_stats.culled_instances += count;
}

void batch_statistics_get(uint32_t* draws, uint32_t* instances, uint32_t* tris) {
    if (draws) *draws = g_last_frame_stats.draw_calls;
    if (instances) *instances = g_last_frame_stats.instances_drawn;
    if (tris) *tris = g_last_frame_stats.triangles_drawn;
}
