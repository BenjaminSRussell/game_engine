/*
 * quat_core_test.c
 * Axis-angle, euler, multiply, rotate, matrix, slerp, and nlerp.
 * Compiles quat_core.c directly and does not link Engine.
 */

#include <math/math_all.h>

#include <math.h>
#include <stdio.h>
#include <stdlib.h>

static void fail(const char *msg) {
    fprintf(stderr, "FAIL: %s\n", msg);
    exit(1);
}

static int near_f(float a, float b, float eps) {
    float d = a - b;
    if (d < 0.0f) {
        d = -d;
    }
    return d <= eps;
}

static void expect_quat(const char *name, Quat q, float w, float x, float y, float z,
                        float eps) {
    if (!near_f(q.w, w, eps) || !near_f(q.x, x, eps) || !near_f(q.y, y, eps) ||
        !near_f(q.z, z, eps)) {
        fprintf(stderr,
                "FAIL: %s got (w=%g x=%g y=%g z=%g) expected (w=%g x=%g y=%g z=%g)\n",
                name, q.w, q.x, q.y, q.z, w, x, y, z);
        exit(1);
    }
}

static void expect_vec3(const char *name, Vec3 v, float x, float y, float z, float eps) {
    if (!near_f(v.x, x, eps) || !near_f(v.y, y, eps) || !near_f(v.z, z, eps)) {
        fprintf(stderr, "FAIL: %s got (%g %g %g) expected (%g %g %g)\n", name, v.x, v.y,
                v.z, x, y, z);
        exit(1);
    }
}

/* Column vectors: named Mat4 columns are the images of the basis. */
static Vec3 mat_rotate(Mat4 m, Vec3 v) {
    Vec3 r;
    r.x = m.m00 * v.x + m.m01 * v.y + m.m02 * v.z;
    r.y = m.m10 * v.x + m.m11 * v.y + m.m12 * v.z;
    r.z = m.m20 * v.x + m.m21 * v.y + m.m22 * v.z;
    return r;
}

/* q and -q encode the same rotation. */
static int same_rotation(Quat a, Quat b, float eps) {
    int pos = near_f(a.w, b.w, eps) && near_f(a.x, b.x, eps) && near_f(a.y, b.y, eps) &&
              near_f(a.z, b.z, eps);
    int neg = near_f(a.w, -b.w, eps) && near_f(a.x, -b.x, eps) && near_f(a.y, -b.y, eps) &&
              near_f(a.z, -b.z, eps);
    return pos || neg;
}

int main(void) {
    const float eps = 1e-4f;
    const float pi = 3.14159265f;
    const float h = 0.70710678f;
    Quat id;
    Quat q;
    Quat qx;
    Quat qy;
    Quat prod;
    Quat mid;
    Vec3 v;
    Vec3 rotated;
    Mat4 m;
    Vec3 basis[3];
    int i;

    id = quat_identity();
    expect_quat("identity", id, 1.0f, 0.0f, 0.0f, 0.0f, eps);

    q = quat_create(2.0f, 3.0f, 4.0f, 5.0f);
    expect_quat("create", q, 2.0f, 3.0f, 4.0f, 5.0f, eps);

    q = quat_from_axis_angle(vec3_create(0.0f, 1.0f, 0.0f), pi * 0.5f);
    expect_quat("axis-angle y 90", q, h, 0.0f, h, 0.0f, eps);

    rotated = quat_rotate_vec3(q, vec3_create(0.0f, 0.0f, 1.0f));
    expect_vec3("y90 * +Z", rotated, 1.0f, 0.0f, 0.0f, eps);
    rotated = quat_rotate_vec3(q, vec3_create(1.0f, 0.0f, 0.0f));
    expect_vec3("y90 * +X", rotated, 0.0f, 0.0f, -1.0f, eps);
    rotated = quat_rotate_vec3(q, vec3_create(0.0f, 1.0f, 0.0f));
    expect_vec3("y90 * +Y", rotated, 0.0f, 1.0f, 0.0f, eps);

    rotated = quat_rotate_vec3(quat_create(-q.w, -q.x, -q.y, -q.z),
                               vec3_create(0.0f, 0.0f, 1.0f));
    expect_vec3("-q * +Z", rotated, 1.0f, 0.0f, 0.0f, eps);

    expect_quat("mul id left", quat_mul(id, q), q.w, q.x, q.y, q.z, eps);
    expect_quat("mul id right", quat_mul(q, id), q.w, q.x, q.y, q.z, eps);

    qx = quat_from_axis_angle(vec3_create(1.0f, 0.0f, 0.0f), pi * 0.5f);
    qy = quat_from_axis_angle(vec3_create(0.0f, 1.0f, 0.0f), pi * 0.5f);
    /* Hamilton product applies the right-hand rotation first. */
    rotated = quat_rotate_vec3(quat_mul(qx, qy), vec3_create(0.0f, 0.0f, 1.0f));
    expect_vec3("Y then X * +Z", rotated, 1.0f, 0.0f, 0.0f, eps);

    expect_quat("normalize", quat_normalize(quat_create(1.0f, 1.0f, 1.0f, 1.0f)), 0.5f, 0.5f,
                0.5f, 0.5f, eps);
    expect_quat("normalize zero", quat_normalize(quat_create(0.0f, 0.0f, 0.0f, 0.0f)), 1.0f,
                0.0f, 0.0f, 0.0f, eps);

    expect_quat("conjugate", quat_conjugate(q), q.w, -q.x, -q.y, -q.z, eps);

    expect_quat("inverse scaled w", quat_inverse(quat_create(2.0f, 0.0f, 0.0f, 0.0f)), 0.5f,
                0.0f, 0.0f, 0.0f, eps);
    expect_quat("inverse tiny", quat_inverse(quat_create(1e-8f, 0.0f, 0.0f, 0.0f)), 1.0f, 0.0f,
                0.0f, 0.0f, eps);

    prod = quat_mul(q, quat_inverse(q));
    expect_quat("q * inv(q)", prod, 1.0f, 0.0f, 0.0f, 0.0f, eps);
    prod = quat_mul(q, quat_conjugate(q));
    expect_quat("q * conj(q)", prod, 1.0f, 0.0f, 0.0f, 0.0f, eps);

    /* quat_from_euler(pitch, yaw, roll) stores roll in x, pitch in y, yaw in z. */
    q = quat_from_euler(0.0f, 0.0f, pi * 0.5f);
    expect_quat("euler roll 90", q, h, h, 0.0f, 0.0f, eps);
    v = quat_to_euler(q);
    expect_vec3("euler roll back", v, pi * 0.5f, 0.0f, 0.0f, eps);

    q = quat_from_euler(0.3f, -0.4f, 0.5f);
    v = quat_to_euler(q);
    expect_vec3("euler roundtrip", v, 0.5f, 0.3f, -0.4f, eps);

    mid = quat_nlerp(quat_identity(), quat_create(0.0f, 0.0f, 0.0f, 1.0f), 0.5f);
    expect_quat("nlerp mid", mid, h, 0.0f, 0.0f, h, eps);
    expect_quat("nlerp t0", quat_nlerp(qy, qx, 0.0f), qy.w, qy.x, qy.y, qy.z, eps);
    expect_quat("nlerp t1", quat_nlerp(qy, qx, 1.0f), qx.w, qx.x, qx.y, qx.z, eps);

    mid = quat_nlerp(quat_identity(), quat_create(-h, 0.0f, -h, 0.0f), 0.5f);
    if (!same_rotation(mid, quat_from_axis_angle(vec3_create(0.0f, 1.0f, 0.0f), pi * 0.25f),
                       eps)) {
        fail("nlerp shortest path");
    }

    expect_quat("slerp t0", quat_slerp(id, qy, 0.0f), id.w, id.x, id.y, id.z, eps);
    expect_quat("slerp t1", quat_slerp(id, qy, 1.0f), qy.w, qy.x, qy.y, qy.z, eps);
    expect_quat("slerp 45", quat_slerp(id, qy, 0.5f), 0.9238795f, 0.0f, 0.3826834f, 0.0f, eps);
    if (!same_rotation(quat_slerp(id, quat_create(-qy.w, -qy.x, -qy.y, -qy.z), 0.5f),
                       quat_slerp(id, qy, 0.5f), eps)) {
        fail("slerp shortest path");
    }

    {
        Quat tiny = quat_from_axis_angle(vec3_create(0.0f, 1.0f, 0.0f), 0.01f);
        Quat got = quat_slerp(quat_identity(), tiny, 0.5f);
        Quat exp = quat_from_axis_angle(vec3_create(0.0f, 1.0f, 0.0f), 0.005f);
        if (!same_rotation(got, exp, eps)) {
            fail("slerp near parallel");
        }
    }

    m = quat_to_mat4(qy);
    if (!near_f(m.m00, 0.0f, eps) || !near_f(m.m11, 1.0f, eps) || !near_f(m.m22, 0.0f, eps) ||
        !near_f(m.m02, 1.0f, eps) || !near_f(m.m20, -1.0f, eps) || !near_f(m.m33, 1.0f, eps)) {
        fprintf(stderr, "FAIL: mat4 y90 m00=%g m11=%g m22=%g m02=%g m20=%g m33=%g\n", m.m00,
                m.m11, m.m22, m.m02, m.m20, m.m33);
        exit(1);
    }

    basis[0] = vec3_create(1.0f, 0.0f, 0.0f);
    basis[1] = vec3_create(0.0f, 1.0f, 0.0f);
    basis[2] = vec3_create(0.0f, 0.0f, 1.0f);
    q = quat_normalize(quat_from_euler(0.2f, -0.7f, 1.1f));
    m = quat_to_mat4(q);
    for (i = 0; i < 3; i++) {
        Vec3 by_q = quat_rotate_vec3(q, basis[i]);
        Vec3 by_m = mat_rotate(m, basis[i]);
        if (!near_f(by_q.x, by_m.x, eps) || !near_f(by_q.y, by_m.y, eps) ||
            !near_f(by_q.z, by_m.z, eps)) {
            fprintf(stderr, "FAIL: mat vs quat basis %d q=(%g %g %g) m=(%g %g %g)\n", i, by_q.x,
                    by_q.y, by_q.z, by_m.x, by_m.y, by_m.z);
            exit(1);
        }
    }

    q = quat_from_euler(pi * 0.5f, 0.2f, 0.0f);
    v = quat_to_euler(q);
    if (!near_f(v.y, pi * 0.5f, 1e-3f)) {
        fprintf(stderr, "FAIL: gimbal pitch y=%g\n", v.y);
        exit(1);
    }

    printf("quat_core tests passed\n");
    return 0;
}
