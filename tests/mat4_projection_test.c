/*
 * mat4_projection_test.c
 * Orthographic, perspective, and look-at from mat4_projection.c.
 * Compiles that file directly and does not link Engine.
 */

#include <math/math_all.h>

#include <math.h>
#include <stdio.h>
#include <stdlib.h>

static int g_fails;

static void expect_near(const char *name, float got, float want) {
  if (fabsf(got - want) > 1.0e-4f) {
    fprintf(stderr, "FAIL %s: got %g want %g\n", name, got, want);
    g_fails++;
  }
}

/* Same product as mat4_mul_vec4: x' = m00*x + m10*y + m20*z + m30. */
static Vec4 mul_point(Mat4 m, float x, float y, float z) {
  Vec4 r;
  r.x = m.m00 * x + m.m10 * y + m.m20 * z + m.m30;
  r.y = m.m01 * x + m.m11 * y + m.m21 * z + m.m31;
  r.z = m.m02 * x + m.m12 * y + m.m22 * z + m.m32;
  r.w = m.m03 * x + m.m13 * y + m.m23 * z + m.m33;
  return r;
}

static void expect_view(const char *name, Mat4 m, Vec3 p, float x, float y,
                        float z) {
  Vec4 c = mul_point(m, p.x, p.y, p.z);
  expect_near(name, c.w, 1.0f);
  expect_near(name, c.x, x);
  expect_near(name, c.y, y);
  expect_near(name, c.z, z);
}

static void expect_ndc(const char *name, Mat4 m, float x, float y, float z,
                       float nx, float ny, float nz) {
  Vec4 c = mul_point(m, x, y, z);
  if (fabsf(c.w) < 1.0e-5f) {
    fprintf(stderr, "FAIL %s: clip w=%g\n", name, c.w);
    g_fails++;
    return;
  }
  expect_near(name, c.x / c.w, nx);
  expect_near(name, c.y / c.w, ny);
  expect_near(name, c.z / c.w, nz);
}

static void test_ortho(void) {
  Mat4 ortho;
  Mat4 sym;

  ortho = mat4_ortho(0.0f, 2.0f, -1.0f, 3.0f, 1.0f, 5.0f);
  expect_ndc("ortho left bottom near", ortho, 0.0f, -1.0f, -1.0f, -1.0f, -1.0f,
             -1.0f);
  expect_ndc("ortho right top far", ortho, 2.0f, 3.0f, -5.0f, 1.0f, 1.0f, 1.0f);
  expect_ndc("ortho center", ortho, 1.0f, 1.0f, -3.0f, 0.0f, 0.0f, 0.0f);

  sym = mat4_ortho(-10.0f, 10.0f, -10.0f, 10.0f, -1.0f, 1.0f);
  expect_view("ortho symmetric origin", sym, vec3_create(0.0f, 0.0f, 0.0f),
              0.0f, 0.0f, 0.0f);
}

static void test_perspective(void) {
  const float fov = 3.14159265359f * 0.5f;
  const float near = 1.0f;
  const float far = 100.0f;
  Mat4 proj = mat4_perspective(fov, 1.0f, near, far);

  expect_ndc("persp near center", proj, 0.0f, 0.0f, -near, 0.0f, 0.0f, -1.0f);
  expect_ndc("persp far center", proj, 0.0f, 0.0f, -far, 0.0f, 0.0f, 1.0f);
  expect_ndc("persp near right", proj, near, 0.0f, -near, 1.0f, 0.0f, -1.0f);
  expect_ndc("persp near top", proj, 0.0f, near, -near, 0.0f, 1.0f, -1.0f);
  expect_near("persp w term", proj.m23, -1.0f);
  expect_near("persp m33", proj.m33, 0.0f);
}

static void test_look_at(void) {
  Vec3 eye = vec3_create(10.0f, 0.0f, 0.0f);
  Vec3 target = vec3_create(0.0f, 0.0f, 0.0f);
  Vec3 up = vec3_create(0.0f, 1.0f, 0.0f);
  Mat4 view = mat4_look_at(eye, target, up);
  Mat4 axis;
  Vec3 axis_eye;
  Vec3 axis_target;

  expect_view("look eye", view, eye, 0.0f, 0.0f, 0.0f);
  expect_view("look target", view, target, 0.0f, 0.0f, -10.0f);
  expect_view("look ahead", view, vec3_create(9.0f, 0.0f, 0.0f), 0.0f, 0.0f,
              -1.0f);
  expect_view("look right", view, vec3_create(9.0f, 0.0f, -1.0f), 1.0f, 0.0f,
              -1.0f);
  expect_view("look up", view, vec3_create(10.0f, 1.0f, 0.0f), 0.0f, 1.0f,
              0.0f);

  axis_eye = vec3_create(0.0f, 0.0f, 10.0f);
  axis_target = vec3_create(0.0f, 0.0f, 0.0f);
  axis = mat4_look_at(axis_eye, axis_target, up);
  expect_view("look axis target", axis, axis_target, 0.0f, 0.0f, -10.0f);
  expect_view("look axis eye", axis, axis_eye, 0.0f, 0.0f, 0.0f);
}

int main(void) {
  test_ortho();
  test_perspective();
  test_look_at();
  if (g_fails != 0) {
    fprintf(stderr, "%d projection checks failed\n", g_fails);
    return 1;
  }
  printf("mat4 projection tests passed\n");
  return 0;
}
