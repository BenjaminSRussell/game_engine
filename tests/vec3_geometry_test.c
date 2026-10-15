/*
 * vec3_geometry_test.c
 * Angle, slerp, reflect, project, and related Vec3 helpers.
 * Compiles vec3_geometry.c directly and does not link Engine.
 */

#include <math/vec3.h>

#include <math.h>
#include <stdio.h>
#include <stdlib.h>

static int g_fails;

static void expect_f(const char *name, f32 got, f32 want, f32 eps) {
    if (!(fabsf(got - want) <= eps)) {
        fprintf(stderr, "FAIL %s: got %.8f want %.8f\n", name, (double)got, (double)want);
        g_fails++;
    }
}

static void expect_v(const char *name, Vec3 got, Vec3 want, f32 eps) {
    if (!vec3_is_equal(got, want, eps)) {
        fprintf(stderr,
                "FAIL %s: got (%.8f, %.8f, %.8f) want (%.8f, %.8f, %.8f)\n",
                name,
                (double)got.x, (double)got.y, (double)got.z,
                (double)want.x, (double)want.y, (double)want.z);
        g_fails++;
    }
}

int main(void) {
    const f32 eps = 1e-4f;
    Vec3 x = vec3_create(1.0f, 0.0f, 0.0f);
    Vec3 y = vec3_create(0.0f, 1.0f, 0.0f);
    Vec3 zero = vec3_zero();
    Vec3 diagonal;
    Vec3 mid;
    Vec3 onto;

    expect_f("angle perpendicular", vec3_angle_between(x, y), PI * 0.5f, eps);
    expect_f("angle parallel", vec3_angle_between(x, vec3_mul(x, 2.0f)), 0.0f, eps);
    expect_f("angle opposite", vec3_angle_between(x, vec3_mul(x, -1.0f)), PI, eps);
    expect_f("angle zero", vec3_angle_between(zero, x), 0.0f, eps);

    expect_v("slerp identical", vec3_slerp(x, x, 0.5f), x, eps);
    mid = vec3_slerp(x, y, 0.5f);
    expect_v("slerp midpoint", mid, vec3_create(0.70710678f, 0.70710678f, 0.0f), eps);
    expect_v("slerp start", vec3_slerp(vec3_mul(x, 2.0f), vec3_mul(y, 2.0f), 0.0f),
             vec3_mul(x, 2.0f), eps);
    expect_v("slerp end", vec3_slerp(vec3_mul(x, 2.0f), vec3_mul(y, 2.0f), 1.0f),
             vec3_mul(y, 2.0f), eps);

    expect_v("reflect", vec3_reflect(vec3_create(1.0f, -1.0f, 0.0f), y),
             vec3_create(1.0f, 1.0f, 0.0f), eps);

    expect_v("project axis", vec3_project(vec3_create(2.0f, 3.0f, 4.0f), x),
             vec3_create(2.0f, 0.0f, 0.0f), eps);
    onto = vec3_create(0.0f, 2.0f, 0.0f);
    expect_v("project scaled", vec3_project(vec3_create(2.0f, 3.0f, 4.0f), onto),
             vec3_create(0.0f, 3.0f, 0.0f), eps);
    expect_v("project zero", vec3_project(x, zero), zero, eps);

    expect_v("project plane", vec3_project_plane(vec3_create(2.0f, 3.0f, 4.0f), onto),
             vec3_create(2.0f, 0.0f, 4.0f), eps);

    if (!vec3_is_equal(x, vec3_create(1.0001f, 0.0f, 0.0f), 0.001f)) {
        fprintf(stderr, "FAIL equal within tolerance\n");
        g_fails++;
    }
    if (vec3_is_equal(x, y, 0.001f)) {
        fprintf(stderr, "FAIL unequal vectors compared equal\n");
        g_fails++;
    }

    expect_v("face forward", vec3_face_forward(x, vec3_mul(y, -1.0f), y), x, eps);
    expect_v("face backward", vec3_face_forward(x, y, y), vec3_mul(x, -1.0f), eps);
    expect_v("face perpendicular", vec3_face_forward(y, x, vec3_forward()),
             vec3_mul(y, -1.0f), eps);

    diagonal = vec3_create(1.0f, 1.0f, 0.0f);
    expect_v("orthogonal", vec3_orthogonal(diagonal, x), y, eps);
    expect_v("orthogonal zero ref", vec3_orthogonal(diagonal, zero), diagonal, eps);

    if (g_fails != 0) {
        fprintf(stderr, "%d vec3 geometry check(s) failed\n", g_fails);
        return 1;
    }
    printf("vec3 geometry checks passed\n");
    return 0;
}
