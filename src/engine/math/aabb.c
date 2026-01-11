#include <math/aabb.h>
#include <math/vec3.h>
#include <math.h>
#include <string.h>

AABB aabb_create(Vec3 min, Vec3 max) {
    AABB aabb;
    aabb.min = vec3_min(min, max);
    aabb.max = vec3_max(min, max);
    return aabb;
}

AABB aabb_from_points(const Vec3* points, int count) {
    if (count <= 0) {
        return aabb_create(vec3(0, 0, 0), vec3(0, 0, 0));
    }
    
    AABB aabb;
    aabb.min = points[0];
    aabb.max = points[0];
    
    for (int i = 1; i < count; i++) {
        aabb_expand(&aabb, points[i]);
    }
    
    return aabb;
}

void aabb_expand(AABB* aabb, Vec3 point) {
    aabb->min = vec3_min(aabb->min, point);
    aabb->max = vec3_max(aabb->max, point);
}

bool aabb_intersects(AABB a, AABB b) {
    return a.min.x <= b.max.x && a.max.x >= b.min.x &&
           a.min.y <= b.max.y && a.max.y >= b.min.y &&
           a.min.z <= b.max.z && a.max.z >= b.min.z;
}

bool aabb_contains(AABB a, Vec3 point) {
    return point.x >= a.min.x && point.x <= a.max.x &&
           point.y >= a.min.y && point.y <= a.max.y &&
           point.z >= a.min.z && point.z <= a.max.z;
}

AABB aabb_union(AABB a, AABB b) {
    AABB result;
    result.min = vec3_min(a.min, b.min);
    result.max = vec3_max(a.max, b.max);
    return result;
}

AABB aabb_intersection(AABB a, AABB b) {
    AABB result;
    result.min = vec3_max(a.min, b.min);
    result.max = vec3_min(a.max, b.max);
    
    // Check if intersection is valid
    if (result.min.x > result.max.x || 
        result.min.y > result.max.y || 
        result.min.z > result.max.z) {
        return aabb_create(vec3(0, 0, 0), vec3(0, 0, 0));
    }
    
    return result;
}

Vec3 aabb_center(AABB a) {
    return vec3_scale(vec3_add(a.min, a.max), vec3(0.5f, 0.5f, 0.5f));
}

Vec3 aabb_size(AABB a) {
    return vec3_sub(a.max, a.min);
}

float aabb_volume(AABB a) {
    Vec3 size = aabb_size(a);
    return size.x * size.y * size.z;
}

float aabb_surface_area(AABB a) {
    Vec3 size = aabb_size(a);
    return 2.0f * (size.x * size.y + size.y * size.z + size.z * size.x);
}

bool aabb_is_empty(AABB a) {
    return a.min.x > a.max.x || a.min.y > a.max.y || a.min.z > a.max.z;
}

AABB aabb_transform(AABB a, Mat4 transform) {
    Vec3 corners[8] = {
        {a.min.x, a.min.y, a.min.z},
        {a.max.x, a.min.y, a.min.z},
        {a.min.x, a.max.y, a.min.z},
        {a.max.x, a.max.y, a.min.z},
        {a.min.x, a.min.y, a.max.z},
        {a.max.x, a.min.y, a.max.z},
        {a.min.x, a.max.y, a.max.z},
        {a.max.x, a.max.y, a.max.z}
    };
    
    AABB result;
    Vec3 transformed_corner = mat4_transform_point(transform, corners[0]);
    result.min = transformed_corner;
    result.max = transformed_corner;
    
    for (int i = 1; i < 8; i++) {
        transformed_corner = mat4_transform_point(transform, corners[i]);
        aabb_expand(&result, transformed_corner);
    }
    
    return result;
}
