/*
 * camera_math_test.c
 * Yaw/pitch basis, view, projection, and shake for rendering/core/camera.c.
 * Compiles that file directly and does not link Engine.
 */

#include "math/math_all.h"
#include "rendering/camera.h"

#include <stdio.h>
#include <stdlib.h>

static void fail(const char *msg) {
  fprintf(stderr, "FAIL: %s\n", msg);
  exit(1);
}

static int near_eq(f32 a, f32 b) {
  f32 d = a - b;
  if (d < 0.0f) {
    d = -d;
  }
  return d <= 1.0e-4f;
}

static void expect_vec(const char *msg, Vec3 v, f32 x, f32 y, f32 z) {
  if (!near_eq(v.x, x) || !near_eq(v.y, y) || !near_eq(v.z, z)) {
    fprintf(stderr, "FAIL: %s got (%.5f, %.5f, %.5f)\n", msg, v.x, v.y, v.z);
    exit(1);
  }
}

static void expect_mat(const char *msg, Mat4 a, Mat4 b) {
  int i;
  for (i = 0; i < 16; i++) {
    if (!near_eq(a.e[i], b.e[i])) {
      fprintf(stderr, "FAIL: %s element %d got %.6f want %.6f\n", msg, i, a.e[i],
              b.e[i]);
      exit(1);
    }
  }
}

static int is_identity(Mat4 m) {
  Mat4 id = mat4_identity();
  int i;
  for (i = 0; i < 16; i++) {
    if (!near_eq(m.e[i], id.e[i])) {
      return 0;
    }
  }
  return 1;
}

static void expect_orthonormal(const Camera *camera) {
  if (!near_eq(vec3_length(camera->front), 1.0f) ||
      !near_eq(vec3_length(camera->up), 1.0f) ||
      !near_eq(vec3_length(camera->right), 1.0f)) {
    fail("basis not unit length");
  }
  if (!near_eq(vec3_dot(camera->front, camera->up), 0.0f) ||
      !near_eq(vec3_dot(camera->front, camera->right), 0.0f) ||
      !near_eq(vec3_dot(camera->up, camera->right), 0.0f)) {
    fail("basis not orthogonal");
  }
}

int main(void) {
  Camera camera;
  Camera shaken;
  Vec3 forward;
  Vec3 position;
  Mat4 view;
  Mat4 expected_view;
  Mat4 proj;
  Mat4 expected_proj;
  Vec3 eye;

  camera_init(NULL, vec3(0.0f, 0.0f, 0.0f), 0.0f, 0.0f);
  camera_set_position(NULL, vec3(1.0f, 0.0f, 0.0f));
  camera_add_shake(NULL, 1.0f, 1.0f);
  camera_update_shake(NULL, 0.1f);
  forward = camera_get_forward(NULL);
  expect_vec("null forward", forward, 0.0f, 0.0f, -1.0f);
  if (!is_identity(camera_get_view_matrix(NULL)) ||
      !is_identity(camera_get_projection_matrix(NULL, 1.0f))) {
    fail("null matrices");
  }

  position = vec3(1.0f, 2.0f, 3.0f);
  camera_init(&camera, position, 0.0f, 0.0f);
  if (!near_eq(camera.position.x, 1.0f) || !near_eq(camera.position.y, 2.0f) ||
      !near_eq(camera.position.z, 3.0f)) {
    fail("position");
  }
  if (!near_eq(camera.yaw, 0.0f) || !near_eq(camera.pitch, 0.0f) ||
      !near_eq(camera.fov, 70.0f) || !near_eq(camera.near_plane, 0.1f) ||
      !near_eq(camera.far_plane, 1000.0f)) {
    fail("init parameters");
  }
  forward = camera_get_forward(&camera);
  expect_vec("yaw 0 forward", forward, 1.0f, 0.0f, 0.0f);
  expect_vec("yaw 0 up", camera.up, 0.0f, 1.0f, 0.0f);
  expect_vec("yaw 0 right", camera.right, 0.0f, 0.0f, 1.0f);
  expect_orthonormal(&camera);

  camera_init(&camera, vec3(0.0f, 0.0f, 5.0f), -90.0f, 0.0f);
  forward = camera_get_forward(&camera);
  expect_vec("yaw -90 forward", forward, 0.0f, 0.0f, -1.0f);
  expect_orthonormal(&camera);

  camera_init(&camera, position, 0.0f, 30.0f);
  forward = camera_get_forward(&camera);
  expect_vec("pitch 30 forward", forward, 0.8660254f, 0.5f, 0.0f);
  expect_orthonormal(&camera);

  camera_init(&camera, position, 10.0f, 90.0f);
  if (!near_eq(camera.pitch, 89.0f)) {
    fail("pitch clamp");
  }
  if (!(camera_get_forward(&camera).y > 0.99f)) {
    fail("steep pitch forward");
  }
  expect_orthonormal(&camera);

  camera_init(&camera, position, -90.0f, 0.0f);
  camera_set_position(&camera, vec3(4.0f, 5.0f, 6.0f));
  expect_vec("moved forward", camera_get_forward(&camera), 0.0f, 0.0f, -1.0f);
  if (!near_eq(camera.position.x, 4.0f) || !near_eq(camera.position.y, 5.0f) ||
      !near_eq(camera.position.z, 6.0f)) {
    fail("set position");
  }

  camera_init(&camera, vec3(0.0f, 0.0f, 5.0f), -90.0f, 0.0f);
  view = camera_get_view_matrix(&camera);
  eye = vec3_add(camera.position, camera.shake_offset);
  expected_view =
      mat4_look_at(eye, vec3_add(eye, camera.front), camera.up);
  expect_mat("view", view, expected_view);
  if (is_identity(view) || !near_eq(view.m32, -5.0f)) {
    fail("view is not a real look-at");
  }

  camera_set_position(&camera, vec3(0.0f, 2.0f, 5.0f));
  view = camera_get_view_matrix(&camera);
  eye = camera.position;
  expected_view = mat4_look_at(eye, vec3_add(eye, camera.front), camera.up);
  expect_mat("moved view", view, expected_view);
  if (near_eq(view.m32, -5.0f) && near_eq(view.m31, 0.0f)) {
    fail("set_position did not change the view");
  }

  proj = camera_get_projection_matrix(&camera, 2.0f);
  expected_proj = mat4_perspective(camera.fov * DEG_TO_RAD, 2.0f,
                                   camera.near_plane, camera.far_plane);
  expect_mat("projection", proj, expected_proj);
  if (is_identity(proj) || !near_eq(proj.m23, -1.0f)) {
    fail("projection is not perspective");
  }
  if (!is_identity(camera_get_projection_matrix(&camera, 0.0f))) {
    fail("bad aspect");
  }

  camera_init(&shaken, vec3(0.0f, 0.0f, 5.0f), -90.0f, 0.0f);
  camera_add_shake(&shaken, 0.2f, 1.0f);
  camera_add_shake(&shaken, 0.5f, 1.0f);
  if (!near_eq(shaken.shake_intensity, 0.5f)) {
    fail("stronger shake");
  }
  camera_add_shake(&shaken, 0.1f, 0.2f);
  if (!near_eq(shaken.shake_intensity, 0.5f) ||
      !near_eq(shaken.shake_duration, 1.0f)) {
    fail("weaker shake replaced a stronger one");
  }
  if (!near_eq(vec3_length(shaken.shake_offset), 0.0f)) {
    fail("shake offset before update");
  }
  camera_update_shake(&shaken, 0.25f);
  if (!(vec3_length(shaken.shake_offset) > 0.01f)) {
    fail("shake offset");
  }
  view = camera_get_view_matrix(&shaken);
  expected_view = mat4_look_at(vec3_add(shaken.position, shaken.shake_offset),
                               vec3_add(vec3_add(shaken.position, shaken.shake_offset),
                                        shaken.front),
                               shaken.up);
  expect_mat("shaken view", view, expected_view);
  camera_init(&camera, vec3(0.0f, 0.0f, 5.0f), -90.0f, 0.0f);
  if (near_eq(view.m30, camera_get_view_matrix(&camera).m30) &&
      near_eq(view.m31, camera_get_view_matrix(&camera).m31) &&
      near_eq(view.m32, camera_get_view_matrix(&camera).m32)) {
    fail("shake did not move the view");
  }
  camera_update_shake(&shaken, 1.0f);
  if (!near_eq(shaken.shake_duration, 0.0f) ||
      !near_eq(vec3_length(shaken.shake_offset), 0.0f)) {
    fail("shake did not finish");
  }
  expect_mat("settled view", camera_get_view_matrix(&shaken),
             camera_get_view_matrix(&camera));

  printf("ok\n");
  return 0;
}
