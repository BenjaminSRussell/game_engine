#include "debug_draw.h"
#include "engine/include/core/logger.h"
#include "engine/include/core/memory.h"
#include <core/time_system.h>
#include <rendering/renderer.h>
#include <stdlib.h>
#include <string.h>

#define MAX_DEBUG_LINES 4096
#define MAX_DEBUG_BOXES 1024
#define MAX_DEBUG_SPHERES 1024

typedef struct {
    DebugLine lines[MAX_DEBUG_LINES];
    u32 line_count;

    DebugBox boxes[MAX_DEBUG_BOXES];
    u32 box_count;

    DebugSphere spheres[MAX_DEBUG_SPHERES];
    u32 sphere_count;

    bool is_initialized;
} DebugDrawState;

static DebugDrawState g_debug_draw = {0};

void debug_draw_init(void) {
    memset(&g_debug_draw, 0, sizeof(DebugDrawState));
    g_debug_draw.is_initialized = true;
    LOG_INFO("Debug Draw system initialized");
}

void debug_draw_shutdown(void) {
    g_debug_draw.is_initialized = false;
}

void debug_draw_update(f32 delta_time) {
    if (!g_debug_draw.is_initialized) return;

    // Expire lines
    u32 active_lines = 0;
    for (u32 i = 0; i < g_debug_draw.line_count; i++) {
        g_debug_draw.lines[i].duration -= delta_time;
        if (g_debug_draw.lines[i].duration > 0.0f) {
            if (i != active_lines) {
                g_debug_draw.lines[active_lines] = g_debug_draw.lines[i];
            }
            active_lines++;
        }
    }
    g_debug_draw.line_count = active_lines;

    // Expire boxes
    u32 active_boxes = 0;
    for (u32 i = 0; i < g_debug_draw.box_count; i++) {
        g_debug_draw.boxes[i].duration -= delta_time;
        if (g_debug_draw.boxes[i].duration > 0.0f) {
            if (i != active_boxes) {
                g_debug_draw.boxes[active_boxes] = g_debug_draw.boxes[i];
            }
            active_boxes++;
        }
    }
    g_debug_draw.box_count = active_boxes;

    // Expire spheres
    u32 active_spheres = 0;
    for (u32 i = 0; i < g_debug_draw.sphere_count; i++) {
        g_debug_draw.spheres[i].duration -= delta_time;
        if (g_debug_draw.spheres[i].duration > 0.0f) {
            if (i != active_spheres) {
                g_debug_draw.spheres[active_spheres] = g_debug_draw.spheres[i];
            }
            active_spheres++;
        }
    }
    g_debug_draw.sphere_count = active_spheres;
}

void debug_draw_render(const Mat4* view, const Mat4* proj, IRenderer* renderer) {
    if (!g_debug_draw.is_initialized || !renderer) return;

    // Draw lines
    for (u32 i = 0; i < g_debug_draw.line_count; i++) {
        RENDERER_DRAW_LINE(renderer, g_debug_draw.lines[i].start, g_debug_draw.lines[i].end, g_debug_draw.lines[i].color);
    }

    // Draw boxes
    for (u32 i = 0; i < g_debug_draw.box_count; i++) {
        RENDERER_DRAW_BOX(renderer, g_debug_draw.boxes[i].center, g_debug_draw.boxes[i].size, g_debug_draw.boxes[i].rotation, g_debug_draw.boxes[i].color);
    }

    // Draw spheres
    for (u32 i = 0; i < g_debug_draw.sphere_count; i++) {
        RENDERER_DRAW_SPHERE(renderer, g_debug_draw.spheres[i].center, g_debug_draw.spheres[i].radius, g_debug_draw.spheres[i].color);
    }

    // Clear single-frame items (items added with duration <= 0)
    // Note: The update function handles decay of >0 durations.
    // We should clear items that were "one-shot" (duration <= 0) here, or ensure they are cleared before next frame accumulation.
    // Given the update logic decrements duration, a 0 duration item becomes negative immediately.
    // If we clear only negative duration items here, we cover one-shots.
    // However, the update loop handles compaction. We can simply reset counts if we assume immediate mode.
    // BUT we support persistence. So we rely on update() to cull.
    // EXCEPT for duration=0 items added THIS frame (since update ran before render or after?).
    // If update runs before render, 0 duration items might be culled before being seen!
    // Convention: debug_draw_update should be called at START of frame, debug_draw_render at END.
    // If we add items during frame, they will be rendered.
    // Then next frame update will decrement duration. 0 -> -dt. It will be removed.
    // So logic holds.
}

void debug_draw_line(Vec3 start, Vec3 end, Vec3 color, f32 duration) {
    if (g_debug_draw.line_count >= MAX_DEBUG_LINES) return;

    DebugLine* line = &g_debug_draw.lines[g_debug_draw.line_count++];
    line->start = start;
    line->end = end;
    line->color = color;
    line->duration = duration;
}

void debug_draw_box(Vec3 center, Vec3 size, Quat rotation, Vec3 color, f32 duration) {
    if (g_debug_draw.box_count >= MAX_DEBUG_BOXES) return;

    DebugBox* box = &g_debug_draw.boxes[g_debug_draw.box_count++];
    box->center = center;
    box->size = size;
    box->rotation = rotation;
    box->color = color;
    box->duration = duration;
}

void debug_draw_sphere(Vec3 center, f32 radius, Vec3 color, f32 duration) {
    if (g_debug_draw.sphere_count >= MAX_DEBUG_SPHERES) return;

    DebugSphere* sphere = &g_debug_draw.spheres[g_debug_draw.sphere_count++];
    sphere->center = center;
    sphere->radius = radius;
    sphere->color = color;
    sphere->duration = duration;
}

void debug_draw_axis_triad(Mat4 transform, f32 size, f32 duration) {
    Vec3 origin = {transform.m[3][0], transform.m[3][1], transform.m[3][2]};
    Vec3 x = {transform.m[0][0], transform.m[0][1], transform.m[0][2]};
    Vec3 y = {transform.m[1][0], transform.m[1][1], transform.m[1][2]};
    Vec3 z = {transform.m[2][0], transform.m[2][1], transform.m[2][2]};

    x = vec3_mul(vec3_normalize(x), size);
    y = vec3_mul(vec3_normalize(y), size);
    z = vec3_mul(vec3_normalize(z), size);

    debug_draw_line(origin, vec3_add(origin, x), (Vec3){1,0,0}, duration);
    debug_draw_line(origin, vec3_add(origin, y), (Vec3){0,1,0}, duration);
    debug_draw_line(origin, vec3_add(origin, z), (Vec3){0,0,1}, duration);
}
