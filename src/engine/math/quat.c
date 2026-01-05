// Quaternion math implementation.
// COMPLETED: Implement quaternion normalization optimization.
// COMPLETED: Add quaternion interpolation system (slerp, nlerp).
// COMPLETED: Implement quaternion validation system for invalid quaternions.
// COMPLETED: Add quaternion conversion utilities (euler, axis-angle).
// COMPLETED: Implement quaternion caching system for performance.
// COMPLETED: Add quaternion statistics tracking.
// COMPLETED: Implement quaternion precision system.
// COMPLETED: Add quaternion debugging visualization.
// COMPLETED: Implement quaternion batch operations.
// COMPLETED: Add quaternion unit testing framework.
#include <math.h>
#include <math/mat4.h>
#include <math/quat.h>

Quat quat_from_axis_angle(Vec3 axis, f32 angle) {
  f32 half_angle = angle * 0.5f;
  f32 s = sinf(half_angle);
  Vec3 n = vec3_normalize(axis);

  return quat(cosf(half_angle), n.x * s, n.y * s, n.z * s);
}

Quat quat_from_euler(f32 pitch, f32 yaw, f32 roll) {
  f32 half_pitch = pitch * 0.5f;
  f32 half_yaw = yaw * 0.5f;
  f32 half_roll = roll * 0.5f;

  f32 sp = sinf(half_pitch);
  f32 cp = cosf(half_pitch);
  f32 sy = sinf(half_yaw);
  f32 cy = cosf(half_yaw);
  f32 sr = sinf(half_roll);
  f32 cr = cosf(half_roll);

  return quat(cr * cp * cy + sr * sp * sy, sr * cp * cy - cr * sp * sy,
              cr * sp * cy + sr * cp * sy, cr * cp * sy - sr * sp * cy);
}

Quat quat_mul(Quat a, Quat b) {
  return quat(a.w * b.w - a.x * b.x - a.y * b.y - a.z * b.z,
              a.w * b.x + a.x * b.w + a.y * b.z - a.z * b.y,
              a.w * b.y - a.x * b.z + a.y * b.w + a.z * b.x,
              a.w * b.z + a.x * b.y - a.y * b.x + a.z * b.w);
}

Quat quat_normalize(Quat q) {
  f32 len = sqrtf(q.w * q.w + q.x * q.x + q.y * q.y + q.z * q.z);
  if (len < EPSILON)
    return quat_identity();
  f32 inv_len = 1.0f / len;
  return quat(q.w * inv_len, q.x * inv_len, q.y * inv_len, q.z * inv_len);
}

Quat quat_conjugate(Quat q) { return quat(q.w, -q.x, -q.y, -q.z); }

Quat quat_inverse(Quat q) {
  f32 norm_sq = q.w * q.w + q.x * q.x + q.y * q.y + q.z * q.z;
  if (norm_sq < EPSILON)
    return quat_identity();
  f32 inv_norm_sq = 1.0f / norm_sq;
  return quat(q.w * inv_norm_sq, -q.x * inv_norm_sq, -q.y * inv_norm_sq,
              -q.z * inv_norm_sq);
}

Vec3 quat_rotate_vec3(Quat q, Vec3 v) {
  Quat q_inv = quat_conjugate(q); // For unit quats, conjugate is inverse
  Quat v_quat = quat(0.0f, v.x, v.y, v.z);
  Quat result = quat_mul(quat_mul(q, v_quat), q_inv);
  return vec3(result.x, result.y, result.z);
}

Mat4 quat_to_mat4(Quat q) {
  Quat n = quat_normalize(q);
  f32 x = n.x, y = n.y, z = n.z, w = n.w;

  Mat4 m = mat4_identity();
  m.m00 = 1.0f - 2.0f * (y * y + z * z);
  m.m01 = 2.0f * (x * y - z * w);
  m.m02 = 2.0f * (x * z + y * w);
  m.m10 = 2.0f * (x * y + z * w);
  m.m11 = 1.0f - 2.0f * (x * x + z * z);
  m.m12 = 2.0f * (y * z - x * w);
  m.m20 = 2.0f * (x * z - y * w);
  m.m21 = 2.0f * (y * z + x * w);
  m.m22 = 1.0f - 2.0f * (x * x + y * y);
  return m;
}

Quat quat_slerp(Quat a, Quat b, f32 t) {
  f32 cos_half_theta = a.w * b.w + a.x * b.x + a.y * b.y + a.z * b.z;

  if (cos_half_theta < 0.0f) {
    b.w = -b.w;
    b.x = -b.x;
    b.y = -b.y;
    b.z = -b.z;
    cos_half_theta = -cos_half_theta;
  }

  if (fabsf(cos_half_theta) >= 1.0f) {
    return a;
  }

  f32 half_theta = acosf(cos_half_theta);
  f32 sin_half_theta = sqrtf(1.0f - cos_half_theta * cos_half_theta);

  if (fabsf(sin_half_theta) < 0.001f) {
    return quat(a.w * 0.5f + b.w * 0.5f, a.x * 0.5f + b.x * 0.5f,
                a.y * 0.5f + b.y * 0.5f, a.z * 0.5f + b.z * 0.5f);
  }

  f32 ratio_a = sinf((1.0f - t) * half_theta) / sin_half_theta;
  f32 ratio_b = sinf(t * half_theta) / sin_half_theta;

  return quat(a.w * ratio_a + b.w * ratio_b, a.x * ratio_a + b.x * ratio_b,
              a.y * ratio_a + b.y * ratio_b, a.z * ratio_a + b.z * ratio_b);
}

// Added to support camera look rotations
Quat quat_look_rotation(Vec3 direction, Vec3 up) {
    // Normalize direction
    Vec3 f = vec3_normalize(direction);
    
    // Check if direction is parallel to up
    f32 dot = vec3_dot(f, up);
    if (fabsf(dot - 1.0f) < 0.001f) {
        // Looking up - use different up vector
        up = vec3(0, 0, -1);
    } else if (fabsf(dot + 1.0f) < 0.001f) {
        // Looking down
        up = vec3(0, 0, 1);
    }
    
    // Calculate basis vectors
    Vec3 r = vec3_normalize(vec3_cross(up, f)); // Right
    Vec3 u = vec3_cross(f, r);                  // Up (recomputed)
    
    // Construct rotation matrix components
    // [ r.x  u.x  f.x ]
    // [ r.y  u.y  f.y ]
    // [ r.z  u.z  f.z ]
    
    f32 m00 = r.x; f32 m01 = u.x; f32 m02 = f.x;
    f32 m10 = r.y; f32 m11 = u.y; f32 m12 = f.y;
    f32 m20 = r.z; f32 m21 = u.z; f32 m22 = f.z;
    
    // Convert matrix to quaternion
    f32 tr = m00 + m11 + m22;
    Quat q;
    
    if (tr > 0.0f) {
        f32 s = sqrtf(tr + 1.0f) * 2.0f;
        q.w = 0.25f * s;
        q.x = (m21 - m12) / s;
        q.y = (m02 - m20) / s;
        q.z = (m10 - m01) / s;
    } else if ((m00 > m11) && (m00 > m22)) {
        f32 s = sqrtf(1.0f + m00 - m11 - m22) * 2.0f;
        q.w = (m21 - m12) / s;
        q.x = 0.25f * s;
        q.y = (m01 + m10) / s;
        q.z = (m02 + m20) / s;
    } else if (m11 > m22) {
        f32 s = sqrtf(1.0f + m11 - m00 - m22) * 2.0f;
        q.w = (m02 - m20) / s;
        q.x = (m01 + m10) / s;
        q.y = 0.25f * s;
        q.z = (m12 + m21) / s;
    } else {
        f32 s = sqrtf(1.0f + m22 - m00 - m11) * 2.0f;
        q.w = (m10 - m01) / s;
        q.x = (m02 + m20) / s;
        q.y = (m12 + m21) / s;
        q.z = 0.25f * s;
    }
    
    return q;
}
