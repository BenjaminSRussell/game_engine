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

