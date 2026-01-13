#ifndef RENDERING_AABB_H
#define RENDERING_AABB_H

#include "math/vec3.h"

typedef struct aabb {
    vec3_t min;
    vec3_t max;
} aabb_t;

aabb_t aabb_create(vec3_t min, vec3_t max);
aabb_t aabb_from_points(const vec3_t* points, int count);
void aabb_expand(aabb_t* aabb, vec3_t point);
bool aabb_intersects(aabb_t a, aabb_t b);
bool aabb_contains(aabb_t a, vec3_t point);

#endif // RENDERING_AABB_H
