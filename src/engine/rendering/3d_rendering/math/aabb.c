#include "aabb.h"

aabb_t aabb_create(vec3_t min, vec3_t max) {
    return (aabb_t){ min, max };
}

aabb_t aabb_from_points(const vec3_t* points, int count) {
    if (count == 0) return (aabb_t){ vec3_zero(), vec3_zero() };
    
    vec3_t min = points[0];
    vec3_t max = points[0];
    
    for (int i = 1; i < count; i++) {
        if (points[i].x < min.x) min.x = points[i].x;
        if (points[i].y < min.y) min.y = points[i].y;
        if (points[i].z < min.z) min.z = points[i].z;
        
        if (points[i].x > max.x) max.x = points[i].x;
        if (points[i].y > max.y) max.y = points[i].y;
        if (points[i].z > max.z) max.z = points[i].z;
    }
    
    return (aabb_t){ min, max };
}

void aabb_expand(aabb_t* aabb, vec3_t point) {
    if (point.x < aabb->min.x) aabb->min.x = point.x;
    if (point.y < aabb->min.y) aabb->min.y = point.y;
    if (point.z < aabb->min.z) aabb->min.z = point.z;
    
    if (point.x > aabb->max.x) aabb->max.x = point.x;
    if (point.y > aabb->max.y) aabb->max.y = point.y;
    if (point.z > aabb->max.z) aabb->max.z = point.z;
}

bool aabb_intersects(aabb_t a, aabb_t b) {
    return (a.min.x <= b.max.x && a.max.x >= b.min.x) &&
           (a.min.y <= b.max.y && a.max.y >= b.min.y) &&
           (a.min.z <= b.max.z && a.max.z >= b.min.z);
}

bool aabb_contains(aabb_t a, vec3_t point) {
    return (point.x >= a.min.x && point.x <= a.max.x) &&
           (point.y >= a.min.y && point.y <= a.max.y) &&
           (point.z >= a.min.z && point.z <= a.max.z);
}
