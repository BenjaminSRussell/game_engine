#include <math/frustum.h>
#include <math/mat4.h>
#include <math/aabb.h>
#include <math/vec3.h>
#include <math/vec4.h>
#include <math.h>

static plane_t plane_from_vec4(vec4_t v) {
    plane_t plane;
    vec3_t normal = {v.x, v.y, v.z};
    float length = sqrtf(v.x * v.x + v.y * v.y + v.z * v.z);
    
    if (length > EPSILON) {
        plane.normal = vec3_scale(normal, 1.0f / length);
        plane.distance = v.w / length;
    } else {
        plane.normal = vec3_set(0, 0, 1);
        plane.distance = 0.0f;
    }
    
    return plane;
}

frustum_t frustum_from_mat4(mat4_t m) {
    frustum_t frustum;
    
    // Extract the 6 planes from the view-projection matrix
    // Left plane: column4 + column1
    vec4_t left = vec4_add(
        (vec4_t){m.m[3], m.m[7], m.m[11], m.m[12]},
        (vec4_t){m.m[0], m.m[4], m.m[8], m.m[12]}
    );
    frustum.planes[0] = plane_from_vec4(left);
    
    // Right plane: column4 - column1
    vec4_t right = vec4_sub(
        (vec4_t){m.m[3], m.m[7], m.m[11], m.m[12]},
        (vec4_t){m.m[0], m.m[4], m.m[8], m.m[12]}
    );
    frustum.planes[1] = plane_from_vec4(right);
    
    // Bottom plane: column4 + column2
    vec4_t bottom = vec4_add(
        (vec4_t){m.m[3], m.m[7], m.m[11], m.m[12]},
        (vec4_t){m.m[1], m.m[5], m.m[9], m.m[13]}
    );
    frustum.planes[2] = plane_from_vec4(bottom);
    
    // Top plane: column4 - column2
    vec4_t top = vec4_sub(
        (vec4_t){m.m[3], m.m[7], m.m[11], m.m[12]},
        (vec4_t){m.m[1], m.m[5], m.m[9], m.m[13]}
    );
    frustum.planes[3] = plane_from_vec4(top);
    
    // Near plane: column4 + column3
    vec4_t near = vec4_add(
        (vec4_t){m.m[3], m.m[7], m.m[11], m.m[12]},
        (vec4_t){m.m[2], m.m[6], m.m[10], m.m[14]}
    );
    frustum.planes[4] = plane_from_vec4(near);
    
    // Far plane: column4 - column3
    vec4_t far = vec4_sub(
        (vec4_t){m.m[3], m.m[7], m.m[11], m.m[12]},
        (vec4_t){m.m[2], m.m[6], m.m[10], m.m[14]}
    );
    frustum.planes[5] = plane_from_vec4(far);
    
    return frustum;
}

static float plane_distance_point(plane_t plane, vec3_t point) {
    return vec3_dot(plane.normal, point) + plane.distance;
}

bool frustum_intersects_aabb(frustum_t f, aabb_t a) {
    // Get the 8 corners of the AABB
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
    
    // Check each plane against the AABB
    for (int i = 0; i < 6; i++) {
        bool inside = false;
        
        // Check if any corner is inside the plane
        for (int j = 0; j < 8; j++) {
            if (plane_distance_point(f.planes[i], corners[j]) >= 0.0f) {
                inside = true;
                break;
            }
        }
        
        // If all corners are outside this plane, the AABB is outside the frustum
        if (!inside) {
            return false;
        }
    }
    
    return true;
}

bool frustum_intersects_point(frustum_t f, vec3_t point) {
    for (int i = 0; i < 6; i++) {
        if (plane_distance_point(f.planes[i], point) < 0.0f) {
            return false;
        }
    }
    return true;
}

bool frustum_intersects_sphere(frustum_t f, vec3_t center, float radius) {
    for (int i = 0; i < 6; i++) {
        float distance = plane_distance_point(f.planes[i], center);
        if (distance < -radius) {
            return false;
        }
    }
    return true;
}

bool frustum_contains_aabb(frustum_t f, aabb_t a) {
    // Get the 8 corners of the AABB
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
    
    // Check that all corners are inside all planes
    for (int i = 0; i < 6; i++) {
        for (int j = 0; j < 8; j++) {
            if (plane_distance_point(f.planes[i], corners[j]) < 0.0f) {
                return false;
            }
        }
    }
    
    return true;
}
