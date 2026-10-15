/*
 * mat4_core_test.c
 * Multiply, transpose, determinant, vector transforms, lerp, and NaN/Inf
 * checks for src/engine/math/linear_algebra/matrices/mat4/mat4_core.c.
 * Compiles that file directly and does not link Engine.
 */

#include <math/mat4.h>

#include <math.h>
#include <stdio.h>
#include <stdlib.h>

static void fail(const char *msg) {
  fprintf(stderr, "FAIL: %s\n", msg);
  exit(1);
}

static int near_f(float a, float b) { return fabsf(a - b) <= 0.0001f; }

static void expect_mat(Mat4 got, Mat4 want, const char *msg) {
  if (!mat4_is_equal(got, want, 0.0001f)) {
    fail(msg);
  }
}

int main(void) {
  Mat4 id = mat4_identity();
  Mat4 zero = mat4_zero();
  Mat4 left;
  Mat4 right;
  Mat4 product;
  Mat4 transposed;
  Mat4 lerped;
  Mat4 triangular;
  Mat4 singular;
  Vec4 v;
  Vec4 r4;
  Vec3 p;
  Vec3 r3;
  float det;

  if (!mat4_is_equal(id, id, 0.0f)) {
    fail("identity is not equal to itself");
  }
  if (mat4_is_equal(id, zero, 0.0f)) {
    fail("identity compared equal to zero");
  }
  expect_mat(mat4_mul(id, id), id, "I*I");

  left = mat4_zero();
  left.m[0][0] = 2.0f;
  left.m[1][0] = 4.0f;
  right = mat4_zero();
  right.m[0][0] = 3.0f;
  right.m[0][1] = 5.0f;
  product = mat4_mul(left, right);
  if (!near_f(product.m[0][0], 26.0f) || !near_f(product.m[0][1], 0.0f)) {
    fail("non-trivial multiply");
  }
  expect_mat(mat4_mul(left, id), left, "M*I");

  transposed = mat4_transpose(left);
  if (!near_f(transposed.m[0][1], left.m[1][0]) ||
      !near_f(transposed.m[1][0], left.m[0][1])) {
    fail("transpose swap");
  }
  expect_mat(mat4_transpose(transposed), left, "transpose twice");

  if (!near_f(mat4_determinant(id), 1.0f)) {
    fail("det identity");
  }
  if (!near_f(mat4_determinant(zero), 0.0f)) {
    fail("det zero");
  }

  /* Rows [[1, 7, 0, 0], [2, 3, 0, 0], [0, 0, 4, 0], [0, 0, 0, 5]]. */
  triangular = mat4_zero();
  triangular.m[0][0] = 1.0f;
  triangular.m[1][0] = 7.0f;
  triangular.m[0][1] = 2.0f;
  triangular.m[1][1] = 3.0f;
  triangular.m[2][2] = 4.0f;
  triangular.m[3][3] = 5.0f;
  det = mat4_determinant(triangular);
  if (!near_f(det, -220.0f)) {
    fprintf(stderr, "FAIL: det got %f\n", det);
    exit(1);
  }

  v.x = 1.0f;
  v.y = 2.0f;
  v.z = 3.0f;
  v.w = 4.0f;
  r4 = mat4_mul_vec4(id, v);
  if (!near_f(r4.x, 1.0f) || !near_f(r4.y, 2.0f) || !near_f(r4.z, 3.0f) ||
      !near_f(r4.w, 4.0f)) {
    fail("mul_vec4 identity");
  }

  right = mat4_zero();
  right.m[0][0] = 2.0f;
  right.m[1][1] = 3.0f;
  right.m[2][2] = 4.0f;
  right.m[3][3] = 5.0f;
  r4 = mat4_mul_vec4(right, v);
  if (!near_f(r4.x, 2.0f) || !near_f(r4.y, 6.0f) || !near_f(r4.z, 12.0f) ||
      !near_f(r4.w, 20.0f)) {
    fail("mul_vec4 diagonal");
  }

  p.x = 2.0f;
  p.y = 4.0f;
  p.z = 6.0f;
  r3 = mat4_mul_vec3(id, p, 1.0f);
  if (!near_f(r3.x, 2.0f) || !near_f(r3.y, 4.0f) || !near_f(r3.z, 6.0f)) {
    fail("mul_vec3 identity");
  }
  r3 = mat4_mul_vec3(id, p, 0.0f);
  if (!near_f(r3.x, 2.0f) || !near_f(r3.y, 4.0f) || !near_f(r3.z, 6.0f)) {
    fail("mul_vec3 w=0");
  }

  left = mat4_identity();
  left.m30 = 10.0f;
  left.m31 = 20.0f;
  left.m32 = 30.0f;
  r3 = mat4_mul_vec3(left, p, 1.0f);
  if (!near_f(r3.x, 12.0f) || !near_f(r3.y, 24.0f) || !near_f(r3.z, 36.0f)) {
    fail("mul_vec3 translation");
  }

  lerped = mat4_lerp(zero, id, 0.5f);
  if (!near_f(lerped.m[0][0], 0.5f) || !near_f(lerped.m[3][3], 0.5f) ||
      !near_f(lerped.m[0][1], 0.0f)) {
    fail("lerp");
  }
  if (mat4_is_equal(lerped, id, 0.0001f)) {
    fail("half lerp compared equal to identity");
  }

  if (mat4_has_nan(id) || mat4_has_inf(id)) {
    fail("identity flagged nan or inf");
  }
  left = id;
  left.m[2][1] = nanf("");
  if (!mat4_has_nan(left) || mat4_has_inf(left)) {
    fail("nan detection");
  }
  left = id;
  left.m[3][0] = INFINITY;
  if (!mat4_has_inf(left) || mat4_has_nan(left)) {
    fail("inf detection");
  }

  singular = mat4_zero();
  expect_mat(mat4_inverse(singular), id, "singular inverse");
  product = mat4_inverse(id);
  if (mat4_has_nan(product) || mat4_has_inf(product)) {
    fail("inverse of identity is not finite");
  }

  printf("ok\n");
  return 0;
}
