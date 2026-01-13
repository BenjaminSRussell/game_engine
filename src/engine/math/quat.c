#include <math/quat.h>
#include <math.h>

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

Vec3 quat_rotate_vec3(Quat q, Vec3 v) {
    // Extract vector part of quaternion
    Vec3 u = {q.x, q.y, q.z};

    // Extract scalar part of quaternion
    f32 s = q.w;

    // Do the math
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

    // Result
    Vec3 result = {
        term1.x + term2.x + term3.x,
        term1.y + term2.y + term3.y,
        term1.z + term2.z + term3.z
    };
    return result;
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

Mat4 quat_to_mat4(Quat q) {
    Mat4 m;
    f32 xx = q.x * q.x;
    f32 yy = q.y * q.y;
    f32 zz = q.z * q.z;
    f32 xy = q.x * q.y;
    f32 xz = q.x * q.z;
    f32 yz = q.y * q.z;
    f32 wx = q.w * q.x;
    f32 wy = q.w * q.y;
    f32 wz = q.w * q.z;

    m.m00 = 1.0f - 2.0f * (yy + zz);
    m.m01 = 2.0f * (xy - wz);
    m.m02 = 2.0f * (xz + wy);
    m.m03 = 0.0f;

    m.m10 = 2.0f * (xy + wz);
    m.m11 = 1.0f - 2.0f * (xx + zz);
    m.m12 = 2.0f * (yz - wx);
    m.m13 = 0.0f;

    m.m20 = 2.0f * (xz - wy);
    m.m21 = 2.0f * (yz + wx);
    m.m22 = 1.0f - 2.0f * (xx + yy);
    m.m23 = 0.0f;

    m.m30 = 0.0f;
    m.m31 = 0.0f;
    m.m32 = 0.0f;
    m.m33 = 1.0f;

    return m;
}

Quat quat_slerp(Quat a, Quat b, f32 t) {
    // Slerp implementation
    f32 dot = a.w * b.w + a.x * b.x + a.y * b.y + a.z * b.z;

    if (dot < 0.0f) {
        b.w = -b.w;
        b.x = -b.x;
        b.y = -b.y;
        b.z = -b.z;
        dot = -dot;
    }

    const f32 DOT_THRESHOLD = 0.9995f;
    if (dot > DOT_THRESHOLD) {
        // Linear interpolation for small angles
        Quat result;
        result.w = a.w + (b.w - a.w) * t;
        result.x = a.x + (b.x - a.x) * t;
        result.y = a.y + (b.y - a.y) * t;
        result.z = a.z + (b.z - a.z) * t;
        // Normalize
        f32 len = sqrtf(result.w * result.w + result.x * result.x + result.y * result.y + result.z * result.z);
        result.w /= len;
        result.x /= len;
        result.y /= len;
        result.z /= len;
        return result;
    }

    f32 theta_0 = acosf(dot);
    f32 theta = theta_0 * t;
    f32 sin_theta = sinf(theta);
    f32 sin_theta_0 = sinf(theta_0);

    f32 s0 = cosf(theta) - dot * sin_theta / sin_theta_0;
    f32 s1 = sin_theta / sin_theta_0;

    Quat result;
    result.w = s0 * a.w + s1 * b.w;
    result.x = s0 * a.x + s1 * b.x;
    result.y = s0 * a.y + s1 * b.y;
    result.z = s0 * a.z + s1 * b.z;
    return result;
}

Quat quat_mul(Quat a, Quat b) {
    Quat res;
    res.w = a.w * b.w - a.x * b.x - a.y * b.y - a.z * b.z;
    res.x = a.w * b.x + a.x * b.w + a.y * b.z - a.z * b.y;
    res.y = a.w * b.y - a.x * b.z + a.y * b.w + a.z * b.x;
    res.z = a.w * b.z + a.x * b.y - a.y * b.x + a.z * b.w;
    return res;
}

Quat quat_normalize(Quat q) {
    f32 len_sq = q.w * q.w + q.x * q.x + q.y * q.y + q.z * q.z;
    if (len_sq < 0.0001f) return (Quat){.w=1, .x=0, .y=0, .z=0};
    f32 len = sqrtf(len_sq);
    Quat res;
    res.w = q.w / len;
    res.x = q.x / len;
    res.y = q.y / len;
    res.z = q.z / len;
    return res;
}
