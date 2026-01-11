#include <math/aabb.h>
#include <math/vec3.h>
#include <math.h>
#include <string.h>

aabb_t aabb_create(vec3_t min, vec3_t max) {
    aabb_t aabb;
    aabb.min = vec3_min(min, max);
    aabb.max = vec3_max(min, max);
    return aabb;
}

aabb_t aabb_from_points(const vec3_t* points, int count) {
    if (count <= 0) {
        return aabb_create(vec3_set(0, 0, 0), vec3_set(0, 0, 0));
    }
    
    aabb_t aabb;
    aabb.min = points[0];
    aabb.max = points[0];
    
    for (int i = 1; i < count; i++) {
        aabb_expand(&aabb, points[i]);
    }
    
    return aabb;
}

void aabb_expand(aabb_t* aabb, vec3_t point) {
    aabb->min = vec3_min(aabb->min, point);
    aabb->max = vec3_max(aabb->max, point);
}

bool aabb_intersects(aabb_t a, aabb_t b) {
    return a.min.x <= b.max.x && a.max.x >= b.min.x &&
           a.min.y <= b.max.y && a.max.y >= b.min.y &&
           a.min.z <= b.max.z && a.max.z >= b.min.z;
}

bool aabb_contains(aabb_t a, vec3_t point) {
    return point.x >= a.min.x && point.x <= a.max.x &&
           point.y >= a.min.y && point.y <= a.max.y &&
           point.z >= a.min.z && point.z <= a.max.z;
}

aabb_t aabb_union(aabb_t a, aabb_t b) {
    aabb_t result;
    result.min = vec3_min(a.min, b.min);
    result.max = vec3_max(a.max, b.max);
    return result;
}

aabb_t aabb_intersection(aabb_t a, aabb_t b) {
    aabb_t result;
    result.min = vec3_max(a.min, b.min);
    result.max = vec3_min(a.max, b.max);
    
    // Check if intersection is valid
    if (result.min.x > result.max.x || 
        result.min.y > result.max.y || 
        result.min.z > result.max.z) {
        return aabb_create(vec3_set(0, 0, 0), vec3_set(0, 0, 0));
    }
    
    return result;
}

vec3_t aabb_center(aabb_t a) {
    return vec3_scale(vec3_add(a.min, a.max), 0.5f);
}

vec3_t aabb_size(aabb_t a) {
    return vec3_sub(a.max, a.min);
}

float aabb_volume(aabb_t a) {
    vec3_t size = aabb_size(a);
    return size.x * size.y * size.z;
}

float aabb_surface_area(aabb_t a) {
    vec3_t size = aabb_size(a);
    return 2.0f * (size.x * size.y + size.y * size.z + size.z * size.x);
}

bool aabb_is_empty(aabb_t a) {
    return a.min.x > a.max.x || a.min.y > a.max.y || a.min.z > a.max.z;
}

aabb_t aabb_transform(aabb_t a, mat4_t transform) {
    vec3_t corners[8] = {
        {a.min.x, a.min.y, a.min.z},
        {a.max.x, a.min.y, a.min.z},
        {a.min.x, a.max.y, a.min.z},
        {a.max.x, a.max.y, a.min.z},
        {a.min.x, a.min.y, a.max.z},
        {a.max.x, a.min.y, a.max.z},
        {a.min.x, a.max.y, a.max.z},
        {a.max.x, a.max.y, a.max.z}
    };
    
    aabb_t result;
    vec3_t transformed_corner = mat4_mul_vec3(transform, corners[0], 1.0f);
    result.min = transformed_corner;
    result.max = transformed_corner;
    
    for (int i = 1; i < 8; i++) {
        transformed_corner = mat4_mul_vec3(transform, corners[i], 1.0f);
        aabb_expand(&result, transformed_corner);
    }
    
    return result;
}
