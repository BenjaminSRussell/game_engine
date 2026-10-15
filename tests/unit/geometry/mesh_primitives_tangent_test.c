// Issue #192: mesh_create_cube must compute real tangents instead of the
// {0,0,0,1} placeholder. Expected directions are derived from the cube layout
// in mesh_create_cube, and each vertex is also checked against the mesh's own
// positions and texcoords. Plain counters are used instead of assert() so the
// checks run in Release builds too.
#include "geometry/mesh_primitives.h"

#include <math.h>
#include <stdio.h>
#include <stdlib.h>

static int g_failures = 0;

#define CHECK(cond, msg)                                                  \
  do {                                                                    \
    if (!(cond)) {                                                        \
      g_failures++;                                                       \
      fprintf(stderr, "FAIL %s:%d: %s\n", __FILE__, __LINE__, (msg));     \
    }                                                                     \
  } while (0)

static Vec3 v3(f32 x, f32 y, f32 z) { return vec3_create(x, y, z); }

static f32 dot3(Vec3 a, Vec3 b) { return a.x * b.x + a.y * b.y + a.z * b.z; }

static Vec3 cross3(Vec3 a, Vec3 b) {
  return v3(a.y * b.z - a.z * b.y, a.z * b.x - a.x * b.z, a.x * b.y - a.y * b.x);
}

static Vec3 sub3(Vec3 a, Vec3 b) { return v3(a.x - b.x, a.y - b.y, a.z - b.z); }

static Vec3 scale3(Vec3 a, f32 s) { return v3(a.x * s, a.y * s, a.z * s); }

static f32 len3(Vec3 a) { return sqrtf(dot3(a, a)); }

static int near3(Vec3 a, Vec3 b, f32 eps) {
  return fabsf(a.x - b.x) < eps && fabsf(a.y - b.y) < eps &&
         fabsf(a.z - b.z) < eps;
}

// Expected +U direction per face, in mesh_create_cube's face order: Front,
// Back, Top, Bottom, Right, Left. U runs from the BL corner to the BR corner.
static const f32 kFaceTangent[6][3] = {
    {1, 0, 0},  // Front:  +X
    {-1, 0, 0}, // Back:   -X
    {1, 0, 0},  // Top:    +X
    {1, 0, 0},  // Bottom: +X
    {0, 0, -1}, // Right:  -Z
    {0, 0, 1},  // Left:   +Z
};

int main(void) {
  mesh_t *cube = mesh_create_cube(2.0f);
  if (!cube) {
    fprintf(stderr, "FAIL: mesh_create_cube returned NULL\n");
    return EXIT_FAILURE;
  }
  CHECK(cube->vertex_count == 24, "cube must have 24 vertices");
  CHECK(cube->index_count == 36, "cube must have 36 indices");

  for (u32 f = 0; f < 6; f++) {
    // Corners are stored BL, BR, TR, TL. The BL -> TL edge is +V (v 0 -> 1).
    const geometry_vertex_t *bl = &cube->vertices[4 * f + 0];
    const geometry_vertex_t *tl = &cube->vertices[4 * f + 3];
    CHECK(fabsf((tl->texcoord.y - bl->texcoord.y) - 1.0f) < 1e-6f,
          "face +V edge must span v 0 -> 1");
    Vec3 v_dir = sub3(tl->position, bl->position);
    v_dir = scale3(v_dir, 1.0f / len3(v_dir));

    for (u32 k = 0; k < 4; k++) {
      const geometry_vertex_t *vtx = &cube->vertices[4 * f + k];
      Vec3 n = vtx->normal;
      Vec3 t = v3(vtx->tangent.x, vtx->tangent.y, vtx->tangent.z);
      f32 w = vtx->tangent.w;

      CHECK(fabsf(len3(t) - 1.0f) < 1e-5f, "tangent must have unit length");
      CHECK(fabsf(dot3(t, n)) < 1e-5f,
            "tangent must be perpendicular to the normal");
      Vec3 expect_t =
          v3(kFaceTangent[f][0], kFaceTangent[f][1], kFaceTangent[f][2]);
      CHECK(near3(t, expect_t, 1e-5f),
            "tangent must point along +U of its face");
      CHECK(w == 1.0f, "handedness must be +1 for this UV layout");
      // The bitangent implied by normal, tangent and handedness must match +V
      Vec3 bitangent = scale3(cross3(n, t), w);
      CHECK(dot3(bitangent, v_dir) > 1.0f - 1e-5f,
            "cross(normal, tangent) * w must point along +V");
    }
  }

  free(cube->vertices);
  free(cube->indices);
  free(cube->submeshes);
  free(cube);

  if (g_failures) {
    fprintf(stderr, "%d tangent check(s) failed\n", g_failures);
    return EXIT_FAILURE;
  }
  printf("mesh_create_cube tangents: all checks passed\n");
  return EXIT_SUCCESS;
}
