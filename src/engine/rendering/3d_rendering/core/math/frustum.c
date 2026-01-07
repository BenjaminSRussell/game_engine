#include "frustum.h"
#include <math.h>

frustum_t frustum_from_mat4(mat4_t m) {
    frustum_t f;
    float *e = m.m;

    // Left
    f.planes[0].normal = vec3_set(e[3] + e[0], e[7] + e[4], e[11] + e[8]);
    f.planes[0].distance = e[15] + e[12];

    // Right
    f.planes[1].normal = vec3_set(e[3] - e[0], e[7] - e[4], e[11] - e[8]);
    f.planes[1].distance = e[15] - e[12];

    // Bottom
    f.planes[2].normal = vec3_set(e[3] + e[1], e[7] + e[5], e[11] + e[9]);
    f.planes[2].distance = e[15] + e[13];

    // Top
    f.planes[3].normal = vec3_set(e[3] - e[1], e[7] - e[5], e[11] - e[9]);
    f.planes[3].distance = e[15] - e[13];

    // Near
    f.planes[4].normal = vec3_set(e[3] + e[2], e[7] + e[6], e[11] + e[10]);
    f.planes[4].distance = e[15] + e[14];

    // Far
    f.planes[5].normal = vec3_set(e[3] - e[2], e[7] - e[6], e[11] - e[10]);
    f.planes[5].distance = e[15] - e[14];

    // Normalize planes
    for (int i = 0; i < 6; i++) {
        float len = vec3_length(f.planes[i].normal);
        f.planes[i].normal = vec3_scale(f.planes[i].normal, 1.0f / len);
        f.planes[i].distance /= len;
    }

    return f;
}

bool frustum_intersects_aabb(frustum_t f, aabb_t a) {
    for (int i = 0; i < 6; i++) {
        vec3_t p;
        p.x = f.planes[i].normal.x > 0 ? a.max.x : a.min.x;
        p.y = f.planes[i].normal.y > 0 ? a.max.y : a.min.y;
        p.z = f.planes[i].normal.z > 0 ? a.max.z : a.min.z;

        if (vec3_dot(f.planes[i].normal, p) + f.planes[i].distance < 0) {
            return false;
        }
    }
    return true;
}
