// Unit tests for src/engine/rendering/animation/gpu_skinning.c.
//
// The unit is compiled directly, not linked against Engine. Its GPU backend
// calls resolve to the stand-ins defined below, and its headers resolve to
// tests/gpu_skinning/stubs (see tests/registry/test_gpu_skinning.cmake).

#include "rendering/animation/gpu_skinning.h"

#include <math.h>
#include <stdio.h>
#include <string.h>

static int g_checks = 0;
static int g_failures = 0;

#define CHECK(cond)                                                            \
  do {                                                                         \
    g_checks++;                                                                \
    if (!(cond)) {                                                             \
      g_failures++;                                                            \
      fprintf(stderr, "%s:%d: check failed: %s\n", __FILE__, __LINE__, #cond); \
    }                                                                          \
  } while (0)

#define CHECK_NEAR(actual, expected) CHECK(fabsf((actual) - (expected)) < 1e-4f)

// ---------------------------------------------------------------------------
// Stand-ins for the GPU backend that gpu_skinning.c calls.
// ---------------------------------------------------------------------------

static BufferID g_next_buffer_id = 1;
static TextureID g_next_texture_id = 1;
static u32 g_next_shader_id = 1;

BufferID buffer_create(const BufferDesc *desc) {
  (void)desc;
  return g_next_buffer_id++;
}

void buffer_destroy(BufferID buffer) { (void)buffer; }

void buffer_upload_data(BufferID buffer, const void *data, usize size) {
  (void)buffer;
  (void)data;
  (void)size;
}

void buffer_bind_compute(BufferID buffer, u32 binding) {
  (void)buffer;
  (void)binding;
}

void buffer_bind_image_compute(BufferID buffer, u32 binding) {
  (void)buffer;
  (void)binding;
}

TextureID texture_create(const TextureDesc *desc) {
  (void)desc;
  return g_next_texture_id++;
}

void texture_destroy(TextureID texture) { (void)texture; }

void texture_bind_compute(TextureID texture, u32 binding) {
  (void)texture;
  (void)binding;
}

u32 shader_load_compute(const char *path) {
  (void)path;
  return g_next_shader_id++;
}

void shader_destroy(u32 shader) { (void)shader; }

void shader_bind_compute(u32 shader) { (void)shader; }

void shader_set_uniform_compute(u32 shader, const char *name, const void *data,
                                usize size) {
  (void)shader;
  (void)name;
  (void)data;
  (void)size;
}

void shader_dispatch_compute(u32 shader, u32 groups_x, u32 groups_y,
                             u32 groups_z) {
  (void)shader;
  (void)groups_x;
  (void)groups_y;
  (void)groups_z;
}

void shader_memory_barrier_compute(void) {}

// ---------------------------------------------------------------------------
// Helpers
// ---------------------------------------------------------------------------

// Matrix whose element m[r][c] is base + 4*r + c, so every entry is distinct
// and a shifted or transposed copy cannot match by accident.
static mat4 make_distinct_matrix(f32 base) {
  mat4 m;
  for (int r = 0; r < 4; r++) {
    for (int c = 0; c < 4; c++) {
      m.m[r][c] = base + (f32)(4 * r + c);
    }
  }
  return m;
}

// ---------------------------------------------------------------------------
// Skinning matrix computation
// ---------------------------------------------------------------------------

static void test_matrix_to_skinning_matrix_copies_top_three_rows(void) {
  mat4 source = make_distinct_matrix(0.5f);
  mat3x4 skinning = gpu_skinning_matrix_to_skinning_matrix(&source);

  for (int r = 0; r < 3; r++) {
    for (int c = 0; c < 4; c++) {
      CHECK(skinning.m[r][c] == source.m[r][c]);
    }
  }
}

static void test_update_skeleton_refreshes_skinning_matrices(void) {
  GPUSkeleton skeleton;
  memset(&skeleton, 0, sizeof(skeleton));
  skeleton.bone_count = 3;
  skeleton.is_dirty = false;

  GPUBoneTransform bones[3];
  memset(bones, 0, sizeof(bones));
  for (u32 b = 0; b < 3; b++) {
    bones[b].transform = make_distinct_matrix(10.0f * (f32)(b + 1));
  }

  gpu_skinning_update_skeleton(&skeleton, bones);

  CHECK(skeleton.is_dirty);
  for (u32 b = 0; b < 3; b++) {
    CHECK(memcmp(&skeleton.bones[b].transform, &bones[b].transform,
                 sizeof(mat4)) == 0);
    for (int r = 0; r < 3; r++) {
      for (int c = 0; c < 4; c++) {
        CHECK(skeleton.bones[b].skinning_matrix.m[r][c] ==
              bones[b].transform.m[r][c]);
      }
    }
  }
  // Bones at or past bone_count are not written.
  CHECK(skeleton.bones[3].skinning_matrix.m[0][0] == 0.0f);
}

static void test_update_skeleton_ignores_null_arguments(void) {
  GPUSkeleton skeleton;
  memset(&skeleton, 0, sizeof(skeleton));
  skeleton.bone_count = 1;
  skeleton.is_dirty = false;

  GPUBoneTransform bone;
  memset(&bone, 0, sizeof(bone));

  gpu_skinning_update_skeleton(NULL, &bone);
  gpu_skinning_update_skeleton(&skeleton, NULL);
  CHECK(!skeleton.is_dirty);
}

// ---------------------------------------------------------------------------
// Tests that need the GPU skinning context (buffers and textures are stubbed).
// ---------------------------------------------------------------------------

static void test_create_skeleton_starts_at_identity(void) {
  CHECK(gpu_skinning_initialize(8, 8));

  GPUSkeleton *skeleton = gpu_skinning_create_skeleton("rig", 2);
  CHECK(skeleton != NULL);
  if (skeleton) {
    CHECK(skeleton->bone_count == 2);
    for (u32 b = 0; b < 2; b++) {
      for (int r = 0; r < 3; r++) {
        for (int c = 0; c < 4; c++) {
          CHECK(skeleton->bones[b].skinning_matrix.m[r][c] ==
                ((r == c) ? 1.0f : 0.0f));
        }
      }
    }
    gpu_skinning_destroy_skeleton(skeleton);
  }

  gpu_skinning_shutdown();
}

static void test_mesh_weights_are_normalized(void) {
  CHECK(gpu_skinning_initialize(8, 8));

  GPUSkinningVertex vertices[3];
  memset(vertices, 0, sizeof(vertices));
  vertices[0].bone_weights[0] = 2.0f; // normalizes to 0.5 / 0.5
  vertices[0].bone_weights[1] = 2.0f;
  vertices[1].bone_weights[0] = 1.0f; // normalizes to 0.25 / 0.75
  vertices[1].bone_weights[1] = 3.0f;
  // vertices[2] has no weight, so its weights must stay zero.

  u32 indices[3] = {0, 1, 2};
  GPUSkinningMesh *mesh = gpu_skinning_create_mesh(vertices, 3, indices, 3, 2);
  CHECK(mesh != NULL);
  if (mesh) {
    CHECK_NEAR(mesh->vertices[0].bone_weights[0], 0.5f);
    CHECK_NEAR(mesh->vertices[0].bone_weights[1], 0.5f);
    CHECK_NEAR(mesh->vertices[1].bone_weights[0], 0.25f);
    CHECK_NEAR(mesh->vertices[1].bone_weights[1], 0.75f);
    CHECK(mesh->vertices[2].bone_weights[0] == 0.0f);

    // Updating vertex data normalizes the new weights the same way.
    vertices[0].bone_weights[0] = 3.0f; // normalizes to 0.75 / 0.25
    vertices[0].bone_weights[1] = 1.0f;
    gpu_skinning_update_mesh_vertices(mesh, vertices);
    CHECK_NEAR(mesh->vertices[0].bone_weights[0], 0.75f);
    CHECK_NEAR(mesh->vertices[0].bone_weights[1], 0.25f);

    gpu_skinning_destroy_mesh(mesh);
  }

  gpu_skinning_shutdown();
}

static void test_instance_clamps_time_and_handles_playback(void) {
  CHECK(gpu_skinning_initialize(8, 8));

  GPUSkeleton *skeleton = gpu_skinning_create_skeleton("rig", 1);
  GPUSkinningVertex vertex;
  memset(&vertex, 0, sizeof(vertex));
  u32 index = 0;
  GPUSkinningMesh *mesh = gpu_skinning_create_mesh(&vertex, 1, &index, 1, 1);
  GPUAnimationInstance *instance =
      (skeleton && mesh) ? gpu_skinning_create_instance(skeleton, mesh) : NULL;
  CHECK(instance != NULL);

  if (instance) {
    gpu_skinning_set_animation_time(instance, -5.0f);
    CHECK_NEAR(instance->current_time, 0.0f);
    gpu_skinning_set_animation_time(instance, 1.0e6f);
    CHECK_NEAR(instance->current_time, GPU_SKINNING_MAX_TIME);
    gpu_skinning_set_animation_time(instance, 12.5f);

    // A paused instance does not advance.
    gpu_skinning_update_instance(instance, 1.0f);
    CHECK_NEAR(instance->current_time, 12.5f);

    gpu_skinning_play_animation(instance);
    CHECK(gpu_skinning_instance_needs_update(instance));
    instance->playback_speed = 2.0f;
    gpu_skinning_update_instance(instance, 0.25f); // +0.5 at speed 2
    CHECK_NEAR(instance->current_time, 13.0f);

    // A looping instance wraps past the maximum time.
    gpu_skinning_set_animation_time(instance, 3599.5f);
    gpu_skinning_update_instance(instance, 0.5f); // 3599.5 + 1.0 wraps to 0.5
    CHECK_NEAR(instance->current_time, 0.5f);
    CHECK(instance->is_playing);

    // A non-looping instance stops at the maximum time.
    instance->is_looping = false;
    gpu_skinning_set_animation_time(instance, 3599.5f);
    gpu_skinning_update_instance(instance, 0.5f);
    CHECK_NEAR(instance->current_time, GPU_SKINNING_MAX_TIME);
    CHECK(!instance->is_playing);

    gpu_skinning_destroy_instance(instance);
  }

  if (mesh) {
    gpu_skinning_destroy_mesh(mesh);
  }
  if (skeleton) {
    gpu_skinning_destroy_skeleton(skeleton);
  }

  gpu_skinning_shutdown();
}

int main(void) {
  test_matrix_to_skinning_matrix_copies_top_three_rows();
  test_update_skeleton_refreshes_skinning_matrices();
  test_update_skeleton_ignores_null_arguments();
  test_create_skeleton_starts_at_identity();
  test_mesh_weights_are_normalized();
  test_instance_clamps_time_and_handles_playback();

  printf("test_gpu_skinning: %d checks, %d failures\n", g_checks, g_failures);
  return g_failures == 0 ? 0 : 1;
}
