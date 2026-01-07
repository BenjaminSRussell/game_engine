#ifndef RENDERING_FRUSTUM_H
#define RENDERING_FRUSTUM_H

#include "mat4.h"
#include "aabb.h"

typedef struct plane {
    vec3_t normal;
    float distance;
} plane_t;

typedef struct frustum {
    plane_t planes[6]; // 0: Left, 1: Right, 2: Bottom, 3: Top, 4: Near, 5: Far
} frustum_t;

frustum_t frustum_from_mat4(mat4_t m);
bool frustum_intersects_aabb(frustum_t f, aabb_t a);

#endif // RENDERING_FRUSTUM_H
