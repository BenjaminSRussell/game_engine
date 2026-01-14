#ifndef DEBUG_DRAW_H
#define DEBUG_DRAW_H

#include <core/types.h>
#include <math/vec3.h>
#include <math/quat.h>
#include <math/mat4.h>

typedef struct IRenderer IRenderer;

typedef struct {
    Vec3 start;
    Vec3 end;
    Vec3 color;
    f32 duration; // <= 0 for single frame
} DebugLine;

typedef struct {
    Vec3 center;
    Vec3 size;
    Quat rotation;
    Vec3 color;
    f32 duration;
} DebugBox;

typedef struct {
    Vec3 center;
    f32 radius;
    Vec3 color;
    f32 duration;
} DebugSphere;

void debug_draw_init(void);
void debug_draw_shutdown(void);
void debug_draw_update(f32 delta_time);
void debug_draw_render(const Mat4* view, const Mat4* proj, IRenderer* renderer);

// Drawing API
void debug_draw_line(Vec3 start, Vec3 end, Vec3 color, f32 duration);
void debug_draw_box(Vec3 center, Vec3 size, Quat rotation, Vec3 color, f32 duration);
void debug_draw_sphere(Vec3 center, f32 radius, Vec3 color, f32 duration);
void debug_draw_axis_triad(Mat4 transform, f32 size, f32 duration);

#endif // DEBUG_DRAW_H
