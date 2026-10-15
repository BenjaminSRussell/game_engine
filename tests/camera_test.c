// Unit test for src/engine/rendering/core/camera.c (issue #263).
// Built together with camera.c. Engine is not linked because it does not
// build on origin/main.
#include "rendering/camera.h"

#include <math.h>
#include <stddef.h>
#include <stdio.h>

static int g_checks = 0;
static int g_failures = 0;

#define CHECK(cond)                                                    \
  do {                                                                 \
    g_checks++;                                                        \
    if (!(cond)) {                                                     \
      g_failures++;                                                    \
      fprintf(stderr, "FAIL %s:%d: %s\n", __FILE__, __LINE__, #cond); \
    }                                                                  \
  } while (0)

#define NEAR(a, b) (fabsf((float)(a) - (float)(b)) < 1e-5f)

static int vec3_near(Vec3 a, Vec3 b) {
  return NEAR(a.x, b.x) && NEAR(a.y, b.y) && NEAR(a.z, b.z);
}

static int vec3_finite(Vec3 v) {
  return isfinite(v.x) && isfinite(v.y) && isfinite(v.z);
}

static f32 dot3(Vec3 a, Vec3 b) { return a.x * b.x + a.y * b.y + a.z * b.z; }

// The pointer types below follow the declarations in camera.h. A definition in
// camera.c that does not match its header declaration fails to compile.
static void test_signatures(void) {
  void (*init_fn)(Camera *, Vec3, f32, f32) = camera_init;
  Vec3 (*forward_fn)(const Camera *) = camera_get_forward;
  Mat4 (*projection_fn)(const Camera *, f32) = camera_get_projection_matrix;
  CHECK(init_fn != NULL);
  CHECK(forward_fn != NULL);
  CHECK(projection_fn != NULL);
}

static void test_init_stores_state(void) {
  Camera cam;
  Vec3 pos = vec3(1.0f, 2.0f, 3.0f);
  camera_init(&cam, pos, 30.0f, 20.0f);
  CHECK(vec3_near(cam.position, pos));
  CHECK(NEAR(cam.yaw, 30.0f));
  CHECK(NEAR(cam.pitch, 20.0f));
  CHECK(NEAR(cam.fov, 70.0f));
  CHECK(NEAR(cam.near_plane, 0.1f));
  CHECK(NEAR(cam.far_plane, 1000.0f));
  CHECK(NEAR(cam.shake_duration, 0.0f));
  CHECK(NEAR(cam.shake_intensity, 0.0f));
  CHECK(NEAR(cam.shake_time, 0.0f));
  CHECK(vec3_near(cam.shake_offset, vec3(0.0f, 0.0f, 0.0f)));
}

static void test_yaw_zero_faces_plus_x(void) {
  Camera cam;
  camera_init(&cam, vec3(0.0f, 0.0f, 0.0f), 0.0f, 0.0f);
  CHECK(vec3_near(cam.front, vec3(1.0f, 0.0f, 0.0f)));
  CHECK(vec3_near(cam.right, vec3(0.0f, 0.0f, 1.0f)));
  CHECK(vec3_near(cam.up, vec3(0.0f, 1.0f, 0.0f)));
}

static void test_yaw_minus_90_faces_minus_z(void) {
  Camera cam;
  camera_init(&cam, vec3(0.0f, 0.0f, 0.0f), -90.0f, 0.0f);
  CHECK(vec3_near(cam.front, vec3(0.0f, 0.0f, -1.0f)));
  CHECK(vec3_near(cam.right, vec3(1.0f, 0.0f, 0.0f)));
  CHECK(vec3_near(cam.up, vec3(0.0f, 1.0f, 0.0f)));
}

static void test_pitch_90_looks_straight_up(void) {
  Camera cam;
  camera_init(&cam, vec3(0.0f, 0.0f, 0.0f), 0.0f, 90.0f);
  CHECK(vec3_near(cam.front, vec3(0.0f, 1.0f, 0.0f)));
  CHECK(vec3_near(cam.right, vec3(0.0f, 0.0f, 1.0f)));
  CHECK(vec3_near(cam.up, vec3(-1.0f, 0.0f, 0.0f)));
}

static void test_basis_is_orthonormal(void) {
  static const f32 angles[][2] = {
      {0.0f, 0.0f},    {30.0f, 20.0f},   {-135.0f, -45.0f}, {270.0f, 89.0f},
      {45.0f, -89.0f}, {0.0f, 90.0f},    {0.0f, -90.0f},    {720.0f, 0.5f},
  };
  size_t i;
  for (i = 0; i < sizeof(angles) / sizeof(angles[0]); i++) {
    Camera cam;
    camera_init(&cam, vec3(0.0f, 0.0f, 0.0f), angles[i][0], angles[i][1]);
    CHECK(vec3_finite(cam.front));
    CHECK(vec3_finite(cam.right));
    CHECK(vec3_finite(cam.up));
    CHECK(NEAR(dot3(cam.front, cam.front), 1.0f));
    CHECK(NEAR(dot3(cam.right, cam.right), 1.0f));
    CHECK(NEAR(dot3(cam.up, cam.up), 1.0f));
    CHECK(NEAR(dot3(cam.front, cam.right), 0.0f));
    CHECK(NEAR(dot3(cam.front, cam.up), 0.0f));
    CHECK(NEAR(dot3(cam.right, cam.up), 0.0f));
  }
}

static void test_forward_and_null_safety(void) {
  Camera cam;
  const Camera *const_cam;
  Vec3 fwd;

  camera_init(&cam, vec3(0.0f, 0.0f, 0.0f), 90.0f, 0.0f);
  const_cam = &cam;
  fwd = camera_get_forward(const_cam);
  CHECK(vec3_near(fwd, cam.front));
  CHECK(vec3_near(fwd, vec3(0.0f, 0.0f, 1.0f)));
  CHECK(vec3_near(camera_get_forward(NULL), vec3(0.0f, 0.0f, -1.0f)));

  camera_init(NULL, vec3(0.0f, 0.0f, 0.0f), 0.0f, 0.0f);
  CHECK(vec3_near(cam.front, vec3(0.0f, 0.0f, 1.0f)));
}

int main(void) {
  test_signatures();
  test_init_stores_state();
  test_yaw_zero_faces_plus_x();
  test_yaw_minus_90_faces_minus_z();
  test_pitch_90_looks_straight_up();
  test_basis_is_orthonormal();
  test_forward_and_null_safety();

  if (g_failures != 0) {
    fprintf(stderr, "test_camera: %d of %d checks failed\n", g_failures,
            g_checks);
    return 1;
  }
  printf("test_camera: all %d checks passed\n", g_checks);
  return 0;
}
