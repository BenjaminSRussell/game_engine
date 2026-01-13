#include "include/math/quat.h"
#include <math.h>

// Helper
static f32 clampf(f32 v, f32 min, f32 max) {
    if (v < min) return min;
    if (v > max) return max;
    return v;
}

Quat quat_from_axis_angle(Vec3 axis, f32 angle) {
    f32 half_angle = angle * 0.5f;
    f32 s = sinf(half_angle);
    Quat q;
    q.w = cosf(half_angle);
    q.x = axis.x * s;
    q.y = axis.y * s;
    q.z = axis.z * s;
    return q;
}

Quat quat_from_euler(f32 pitch, f32 yaw, f32 roll) {
    f32 cy = cosf(yaw * 0.5f);
    f32 sy = sinf(yaw * 0.5f);
    f32 cp = cosf(pitch * 0.5f);
    f32 sp = sinf(pitch * 0.5f);
    f32 cr = cosf(roll * 0.5f);
    f32 sr = sinf(roll * 0.5f);

    Quat q;
    q.w = cr * cp * cy + sr * sp * sy;
    q.x = sr * cp * cy - cr * sp * sy;
    q.y = cr * sp * cy + sr * cp * sy;
    q.z = cr * cp * sy - sr * sp * cy;
    return q;
}

Quat quat_mul(Quat a, Quat b) {
    Quat q;
    q.w = a.w * b.w - a.x * b.x - a.y * b.y - a.z * b.z;
    q.x = a.w * b.x + a.x * b.w + a.y * b.z - a.z * b.y;
    q.y = a.w * b.y - a.x * b.z + a.y * b.w + a.z * b.x;
    q.z = a.w * b.z + a.x * b.y - a.y * b.x + a.z * b.w;
    return q;
}

Vec3 quat_rotate_vec3(Quat q, Vec3 v) {
    // Extract vector part of quaternion
    Vec3 u = {q.x, q.y, q.z};
    f32 s = q.w;

    // 2.0f * dot(u, v) * u
    f32 dot_uv = u.x * v.x + u.y * v.y + u.z * v.z;
    Vec3 term1 = {2.0f * dot_uv * u.x, 2.0f * dot_uv * u.y, 2.0f * dot_uv * u.z};

    // (s*s - dot(u, u)) * v
    f32 dot_uu = u.x * u.x + u.y * u.y + u.z * u.z;
    f32 s2_minus_dot_uu = s * s - dot_uu;
    Vec3 term2 = {s2_minus_dot_uu * v.x, s2_minus_dot_uu * v.y, s2_minus_dot_uu * v.z};

    // 2.0f * s * cross(u, v)
    Vec3 cross_uv = {
        u.y * v.z - u.z * v.y,
        u.z * v.x - u.x * v.z,
        u.x * v.y - u.y * v.x
    };
    Vec3 term3 = {2.0f * s * cross_uv.x, 2.0f * s * cross_uv.y, 2.0f * s * cross_uv.z};

    Vec3 result = {
        term1.x + term2.x + term3.x,
        term1.y + term2.y + term3.y,
        term1.z + term2.z + term3.z
    };
    return result;
}

Mat4 quat_to_mat4(Quat q) {
    Mat4 m = mat4_identity();
    f32 x = q.x, y = q.y, z = q.z, w = q.w;
    f32 x2 = x + x, y2 = y + y, z2 = z + z;
    f32 xx = x * x2, xy = x * y2, xz = x * z2;
    f32 yy = y * y2, yz = y * z2, zz = z * z2;
    f32 wx = w * x2, wy = w * y2, wz = w * z2;

    m.m00 = 1.0f - (yy + zz);
    m.m10 = xy + wz;
    m.m20 = xz - wy;

    m.m01 = xy - wz;
    m.m11 = 1.0f - (xx + zz);
    m.m21 = yz + wx;

    m.m02 = xz + wy;
    m.m12 = yz - wx;
    m.m22 = 1.0f - (xx + yy);

    return m;
}

Quat quat_slerp(Quat a, Quat b, f32 t) {
    f32 cosTheta = a.x * b.x + a.y * b.y + a.z * b.z + a.w * b.w;

    if (cosTheta < 0.0f) {
        b.x = -b.x; b.y = -b.y; b.z = -b.z; b.w = -b.w;
        cosTheta = -cosTheta;
    }

    if (cosTheta > 0.9995f) {
        // Linear interpolation
        Quat result = {
            a.w + t * (b.w - a.w),
            a.x + t * (b.x - a.x),
            a.y + t * (b.y - a.y),
            a.z + t * (b.z - a.z)
        };
        // Normalize
        f32 len = sqrtf(result.w*result.w + result.x*result.x + result.y*result.y + result.z*result.z);
        result.w /= len; result.x /= len; result.y /= len; result.z /= len;
        return result;
    }

    f32 angle = acosf(clampf(cosTheta, -1.0f, 1.0f));
    f32 sinAngle = sinf(angle);
    f32 t1 = sinf((1.0f - t) * angle) / sinAngle;
    f32 t2 = sinf(t * angle) / sinAngle;

    Quat result = {
        a.w * t1 + b.w * t2,
        a.x * t1 + b.x * t2,
        a.y * t1 + b.y * t2,
        a.z * t1 + b.z * t2
    };
    return result;
}

Quat quat_normalize(Quat q) {
    f32 len = sqrtf(q.w*q.w + q.x*q.x + q.y*q.y + q.z*q.z);
    if (len < EPSILON) return quat_identity();
    f32 inv_len = 1.0f / len;
    Quat r;
    r.w = q.w * inv_len;
    r.x = q.x * inv_len;
    r.y = q.y * inv_len;
    r.z = q.z * inv_len;
    return r;
}

Quat quat_nlerp(Quat a, Quat b, f32 t) {
    // Check dot product for shortest path
    f32 dot = a.w*b.w + a.x*b.x + a.y*b.y + a.z*b.z;
    if (dot < 0.0f) {
        b.w = -b.w;
        b.x = -b.x;
        b.y = -b.y;
        b.z = -b.z;
    }

    Quat r;
    r.w = a.w + (b.w - a.w) * t;
    r.x = a.x + (b.x - a.x) * t;
    r.y = a.y + (b.y - a.y) * t;
    r.z = a.z + (b.z - a.z) * t;
    return quat_normalize(r);
}
