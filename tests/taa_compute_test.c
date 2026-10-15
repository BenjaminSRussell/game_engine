// tests/taa_compute_test.c
//
// Unit test for src/engine/rendering/post_processing/taa_compute.c (issue #227).
// The unit is compiled directly because the Engine library does not build yet.
// The compute backend, texture and allocator calls are fakes that record what
// the unit asks for, so the TAA logic is checked without a GPU.

#include "core/logging/unified_logger.h"
#include "core/math/types.h"
#include "core/memory/unified_allocator.h"
#include "rendering/core/texture.h"
#include "rendering/post_processing/taa_compute.h"
#include "rendering/post_processing/taa_compute_backend.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

// ============================================================================
// CHECK HELPERS
// ============================================================================

static int g_checks = 0;
static int g_failures = 0;

#define CHECK(cond)                                                            \
  do {                                                                         \
    g_checks++;                                                                \
    if (!(cond)) {                                                             \
      g_failures++;                                                            \
      fprintf(stderr, "%s:%d: CHECK failed: %s\n", __FILE__, __LINE__, #cond); \
    }                                                                          \
  } while (0)

static bool approx(f32 actual, f32 expected, f32 tolerance) {
  f32 diff = actual - expected;
  return diff <= tolerance && diff >= -tolerance;
}

static bool is_zero(vec4 v) {
  return v.x == 0.0f && v.y == 0.0f && v.z == 0.0f && v.w == 0.0f;
}

// ============================================================================
// FAKES
// ============================================================================

// Same members and order as the uniform block taa_compute.c passes as "params".
typedef struct TaaParams {
  vec2 jitter_offset;
  f32 blend_factor;
  f32 variance_clamp;
  vec2 texel_size;
  u32 frame_index;
  u32 padding[3];
} TaaParams;

#define MAX_BINDS 16

typedef struct FakeBind {
  bool image;
  TextureID texture;
  u32 slot;
} FakeBind;

typedef struct FakeState {
  // Textures
  u32 create_calls;
  u32 fail_create_at; // 1-based create call that returns NULL; 0 never fails
  u32 textures_created;
  u32 textures_destroyed;
  TextureID next_texture_id;
  TextureCreateInfo last_info;
  u32 clear_calls;
  TextureID clear_texture;
  vec4 clear_color;

  // Compute programs
  u32 load_calls;
  bool fail_load;
  u32 next_program_id;
  u32 loaded_program;
  char loaded_path[128];
  u32 programs_destroyed;
  u32 last_destroyed_program;
  u32 bound_program;
  u32 dispatch_calls;
  u32 dispatch_program;
  u32 dispatch_x;
  u32 dispatch_y;
  u32 dispatch_z;
  u32 barrier_calls;
  u32 uniform_calls;
  u32 params_program;
  size_t params_size;
  TaaParams params;

  // Texture bindings made by the most recent process call
  FakeBind binds[MAX_BINDS];
  u32 bind_count;

  // Allocations and logging
  u32 live_allocations;
  u32 log_errors;
} FakeState;

static FakeState g_fake;

static void reset_fakes(void) {
  memset(&g_fake, 0, sizeof g_fake);
  g_fake.next_texture_id = 1;
  g_fake.next_program_id = 7;
}

// Texture functions declared in rendering/core/texture.h.
Texture *texture_create(const TextureCreateInfo *info) {
  g_fake.create_calls++;
  if (g_fake.fail_create_at != 0 &&
      g_fake.create_calls == g_fake.fail_create_at)
    return NULL;

  Texture *texture = calloc(1, sizeof(*texture));
  if (!texture)
    return NULL;
  texture->texture_id = g_fake.next_texture_id++;
  texture->width = info->width;
  texture->height = info->height;
  g_fake.textures_created++;
  g_fake.last_info = *info;
  return texture;
}

TextureID texture_get_id(Texture *texture) {
  return texture ? texture->texture_id : 0;
}

void texture_destroy(Texture *texture) {
  if (!texture)
    return;
  g_fake.textures_destroyed++;
  free(texture);
}

// Backend functions declared in taa_compute_backend.h.
void texture_clear(TextureID texture, vec4 color) {
  g_fake.clear_calls++;
  g_fake.clear_texture = texture;
  g_fake.clear_color = color;
}

static void record_bind(bool image, TextureID texture, u32 slot) {
  if (g_fake.bind_count >= MAX_BINDS)
    return;
  g_fake.binds[g_fake.bind_count].image = image;
  g_fake.binds[g_fake.bind_count].texture = texture;
  g_fake.binds[g_fake.bind_count].slot = slot;
  g_fake.bind_count++;
}

void texture_bind_compute(TextureID texture, u32 slot) {
  record_bind(false, texture, slot);
}

void texture_bind_image_compute(TextureID texture, u32 slot) {
  record_bind(true, texture, slot);
}

u32 shader_load_compute(const char *path) {
  g_fake.load_calls++;
  if (g_fake.fail_load)
    return 0;
  snprintf(g_fake.loaded_path, sizeof g_fake.loaded_path, "%s", path);
  g_fake.loaded_program = g_fake.next_program_id++;
  return g_fake.loaded_program;
}

void shader_destroy_compute(u32 program) {
  g_fake.programs_destroyed++;
  g_fake.last_destroyed_program = program;
}

void shader_bind_compute(u32 program) { g_fake.bound_program = program; }

void shader_set_uniform_compute(u32 program, const char *name, const void *data,
                                size_t size) {
  if (strcmp(name, "params") != 0)
    return;
  g_fake.uniform_calls++;
  g_fake.params_program = program;
  g_fake.params_size = size;
  memcpy(&g_fake.params, data,
         size < sizeof g_fake.params ? size : sizeof g_fake.params);
}

void shader_dispatch_compute(u32 program, u32 x, u32 y, u32 z) {
  g_fake.dispatch_calls++;
  g_fake.dispatch_program = program;
  g_fake.dispatch_x = x;
  g_fake.dispatch_y = y;
  g_fake.dispatch_z = z;
}

void shader_memory_barrier_compute(void) { g_fake.barrier_calls++; }

// Logger entry point used by the LOG_*_CAT macros.
void unified_logger_log(LogLevel level, LogCategory category,
                        const char *file, int line, const char *function,
                        const char *format, ...) {
  (void)category;
  (void)file;
  (void)line;
  (void)function;
  (void)format;
  if (level == LOG_LEVEL_ERROR)
    g_fake.log_errors++;
}

// Allocator entry points used by MALLOC_PERSISTENT and FREE.
Allocator *g_default_allocator = NULL;
Allocator *g_persistent_allocator = NULL;

void *allocator_alloc(Allocator *allocator, size_t size) {
  (void)allocator;
  void *memory = malloc(size);
  if (memory)
    g_fake.live_allocations++;
  return memory;
}

void allocator_free(Allocator *allocator, void *ptr) {
  (void)allocator;
  if (!ptr)
    return;
  g_fake.live_allocations--;
  free(ptr);
}

// ============================================================================
// HELPERS
// ============================================================================

// Stand-in texture IDs for the caller-owned inputs. The fakes never read them.
enum { CURRENT_FRAME = 100, VELOCITY_BUFFER = 101, DEPTH_BUFFER = 102 };

static void run_process(TAAComputeContext *ctx) {
  g_fake.bind_count = 0;
  taa_compute_process(ctx, CURRENT_FRAME, VELOCITY_BUFFER, DEPTH_BUFFER);
}

static bool has_bind(bool image, TextureID texture, u32 slot) {
  for (u32 i = 0; i < g_fake.bind_count; i++) {
    const FakeBind *bind = &g_fake.binds[i];
    if (bind->image == image && bind->texture == texture && bind->slot == slot)
      return true;
  }
  return false;
}

static TextureID bound_texture(bool image, u32 slot) {
  for (u32 i = 0; i < g_fake.bind_count; i++) {
    const FakeBind *bind = &g_fake.binds[i];
    if (bind->image == image && bind->slot == slot)
      return bind->texture;
  }
  return 0;
}

// ============================================================================
// TESTS
// ============================================================================

static void test_create_and_destroy(void) {
  reset_fakes();
  TAAComputeContext *ctx = taa_compute_create(1920, 1080);
  CHECK(ctx != NULL);
  CHECK(g_fake.textures_created == 2); // history and output
  CHECK(g_fake.last_info.width == 1920);
  CHECK(g_fake.last_info.height == 1080);
  CHECK(g_fake.last_info.format == TEXFMT_RGBA16F);
  CHECK((g_fake.last_info.usage & TEXTURE_USAGE_STORAGE) != 0);
  CHECK((g_fake.last_info.usage & TEXTURE_USAGE_SAMPLED) != 0);
  CHECK(g_fake.load_calls == 1);
  CHECK(strcmp(g_fake.loaded_path,
               "shaders/post_processing/taa_compute.comp") == 0);
  CHECK(taa_compute_get_history(ctx) != 0);
  CHECK(taa_compute_get_output(ctx) == 0); // no frame processed yet

  taa_compute_destroy(ctx);
  CHECK(g_fake.textures_destroyed == 2);
  CHECK(g_fake.programs_destroyed == 1);
  CHECK(g_fake.last_destroyed_program == g_fake.loaded_program);
  CHECK(g_fake.live_allocations == 0);

  taa_compute_destroy(NULL); // must be a no-op
  CHECK(g_fake.textures_destroyed == 2);
}

static void test_create_failures_release_partial_state(void) {
  reset_fakes();
  g_fake.fail_create_at = 2; // the output texture fails
  CHECK(taa_compute_create(64, 64) == NULL);
  CHECK(g_fake.textures_destroyed == 1); // the history texture is released
  CHECK(g_fake.load_calls == 0);
  CHECK(g_fake.live_allocations == 0);

  reset_fakes();
  g_fake.fail_load = true;
  CHECK(taa_compute_create(64, 64) == NULL);
  CHECK(g_fake.textures_destroyed == 2);
  CHECK(g_fake.programs_destroyed == 0);
  CHECK(g_fake.live_allocations == 0);
}

static void test_jitter_sequence(void) {
  // Halton(2,3) for frames 0..7, centred on zero. The unit stores base-3 values
  // to three decimals, so the tolerance is 1e-3.
  static const f32 expected[8][2] = {
      {-0.5f, -0.5f},       {0.0f, -0.16667f},    {-0.25f, 0.16667f},
      {0.25f, -0.38889f},   {-0.375f, -0.05556f}, {0.125f, 0.27778f},
      {-0.125f, -0.27778f}, {0.375f, 0.05556f}};

  reset_fakes();
  TAAComputeContext *ctx = taa_compute_create(64, 64);
  CHECK(ctx != NULL);

  // Ten frames cover the 8-frame wrap.
  for (u32 frame = 0; frame < 10; frame++) {
    const u32 phase = frame % 8;
    f32 x = 0.0f;
    f32 y = 0.0f;
    taa_compute_get_jitter_offset(ctx, &x, &y);
    CHECK(approx(x, expected[phase][0], 1e-3f));
    CHECK(approx(y, expected[phase][1], 1e-3f));

    run_process(ctx);
    // The uniform block carries the same jitter as the getter.
    CHECK(approx(g_fake.params.jitter_offset.x, expected[phase][0], 1e-3f));
    CHECK(approx(g_fake.params.jitter_offset.y, expected[phase][1], 1e-3f));
    CHECK(g_fake.params.frame_index == frame);
  }

  // jitter_scale multiplies the offset. Reset so the phase is 0 again.
  taa_compute_reset(ctx);
  TAAComputeSettings settings = {
      .blend_factor = 0.05f,
      .sharpness = 0.5f,
      .enable_sharpening = true,
      .enable_jitter = true,
      .sample_pattern = 0,
      .jitter_scale = 2.0f,
  };
  taa_compute_update_settings(ctx, &settings);
  f32 x = 0.0f;
  f32 y = 0.0f;
  taa_compute_get_jitter_offset(ctx, &x, &y);
  CHECK(approx(x, -1.0f, 1e-5f));
  CHECK(approx(y, -1.0f, 1e-5f));

  // Jitter disabled gives a zero offset, in the getter and in the uniforms.
  settings.enable_jitter = false;
  taa_compute_update_settings(ctx, &settings);
  taa_compute_get_jitter_offset(ctx, &x, &y);
  CHECK(x == 0.0f && y == 0.0f);
  run_process(ctx);
  CHECK(g_fake.params.jitter_offset.x == 0.0f);
  CHECK(g_fake.params.jitter_offset.y == 0.0f);

  taa_compute_destroy(ctx);
  CHECK(g_fake.live_allocations == 0);
}

static void test_process_binds_dispatch_and_uniforms(void) {
  reset_fakes();
  TAAComputeContext *ctx = taa_compute_create(1000, 500);
  CHECK(ctx != NULL);
  const TextureID history = taa_compute_get_history(ctx);

  run_process(ctx);
  const TextureID output = taa_compute_get_output(ctx);

  // Four sampled inputs and one storage image.
  CHECK(g_fake.bind_count == 5);
  CHECK(has_bind(false, CURRENT_FRAME, 0));
  CHECK(has_bind(false, history, 1));
  CHECK(has_bind(false, VELOCITY_BUFFER, 2));
  CHECK(has_bind(false, DEPTH_BUFFER, 3));
  CHECK(has_bind(true, output, 0)); // the image written by the dispatch

  CHECK(g_fake.bound_program == g_fake.loaded_program);
  CHECK(g_fake.dispatch_calls == 1);
  CHECK(g_fake.dispatch_program == g_fake.loaded_program);
  // 16x16 groups, rounded up: 1000x500 -> 63x32 groups.
  CHECK(g_fake.dispatch_x == 63);
  CHECK(g_fake.dispatch_y == 32);
  CHECK(g_fake.dispatch_z == 1);
  CHECK(g_fake.barrier_calls == 1);

  // The uniform block has the layout the unit declares and holds this frame.
  CHECK(g_fake.uniform_calls == 1);
  CHECK(g_fake.params_size == sizeof(TaaParams));
  CHECK(g_fake.params_program == g_fake.loaded_program);
  CHECK(approx(g_fake.params.jitter_offset.x, -0.5f, 1e-5f));
  CHECK(approx(g_fake.params.jitter_offset.y, -0.5f, 1e-5f));
  CHECK(approx(g_fake.params.blend_factor, 0.05f, 1e-6f));
  CHECK(approx(g_fake.params.variance_clamp, 0.1f, 1e-6f));
  CHECK(approx(g_fake.params.texel_size.x, 1.0f / 1000.0f, 1e-7f));
  CHECK(approx(g_fake.params.texel_size.y, 1.0f / 500.0f, 1e-7f));
  CHECK(g_fake.params.frame_index == 0);

  taa_compute_destroy(ctx);
  CHECK(g_fake.textures_destroyed == g_fake.textures_created);
  CHECK(g_fake.live_allocations == 0);
}

static void test_ping_pong_and_output(void) {
  reset_fakes();
  TAAComputeContext *ctx = taa_compute_create(64, 64);
  CHECK(ctx != NULL);
  const TextureID initial_history = taa_compute_get_history(ctx);

  // Frame 0 reads the initial history and writes the other buffer.
  run_process(ctx);
  const TextureID read0 = bound_texture(false, 1);
  const TextureID write0 = bound_texture(true, 0);
  CHECK(read0 == initial_history);
  CHECK(write0 != 0);
  CHECK(write0 != read0);
  // The output is the texture the dispatch wrote. It must not be the buffer the
  // swap left behind.
  CHECK(taa_compute_get_output(ctx) == write0);
  CHECK(taa_compute_get_history(ctx) == write0);

  // Frame 1 reads frame 0's result and writes into the buffer frame 0 read.
  run_process(ctx);
  const TextureID read1 = bound_texture(false, 1);
  const TextureID write1 = bound_texture(true, 0);
  CHECK(read1 == write0);
  CHECK(write1 == read0);
  CHECK(write1 != read1);
  CHECK(taa_compute_get_output(ctx) == write1);

  taa_compute_destroy(ctx);
  CHECK(g_fake.live_allocations == 0);
}

static void test_reset_restarts_sequence_and_clears_history(void) {
  reset_fakes();
  TAAComputeContext *ctx = taa_compute_create(64, 64);
  CHECK(ctx != NULL);
  run_process(ctx);
  run_process(ctx);
  run_process(ctx);
  const TextureID history = taa_compute_get_history(ctx);

  taa_compute_reset(ctx);
  CHECK(g_fake.clear_calls == 1);
  CHECK(g_fake.clear_texture == history);
  CHECK(is_zero(g_fake.clear_color));

  run_process(ctx);
  CHECK(g_fake.params.frame_index == 0);
  CHECK(approx(g_fake.params.jitter_offset.x, -0.5f, 1e-5f));
  CHECK(approx(g_fake.params.jitter_offset.y, -0.5f, 1e-5f));

  taa_compute_destroy(ctx);
  CHECK(g_fake.live_allocations == 0);
}

static void test_update_settings(void) {
  reset_fakes();
  TAAComputeContext *ctx = taa_compute_create(64, 64);
  CHECK(ctx != NULL);

  TAAComputeSettings settings = {
      .blend_factor = 0.1f,
      .sharpness = 0.8f,
      .enable_sharpening = true,
      .enable_jitter = false,
      .sample_pattern = 1,
      .jitter_scale = 1.5f,
  };
  taa_compute_update_settings(ctx, &settings);
  run_process(ctx);
  CHECK(approx(g_fake.params.blend_factor, 0.1f, 1e-6f));
  CHECK(g_fake.params.jitter_offset.x == 0.0f);
  CHECK(g_fake.params.jitter_offset.y == 0.0f);

  settings.enable_jitter = true;
  settings.jitter_scale = 2.0f;
  taa_compute_update_settings(ctx, &settings);
  taa_compute_reset(ctx);
  f32 x = 0.0f;
  f32 y = 0.0f;
  taa_compute_get_jitter_offset(ctx, &x, &y);
  CHECK(approx(x, -1.0f, 1e-5f));
  CHECK(approx(y, -1.0f, 1e-5f));

  // A NULL settings pointer is ignored and the settings stay as they were.
  taa_compute_update_settings(ctx, NULL);
  taa_compute_get_jitter_offset(ctx, &x, &y);
  CHECK(approx(x, -1.0f, 1e-5f));
  CHECK(approx(y, -1.0f, 1e-5f));

  taa_compute_destroy(ctx);
  CHECK(g_fake.live_allocations == 0);
}

static void test_null_arguments_are_ignored(void) {
  reset_fakes();
  run_process(NULL);
  CHECK(g_fake.bind_count == 0);
  CHECK(g_fake.dispatch_calls == 0);
  CHECK(taa_compute_get_output(NULL) == 0);
  CHECK(taa_compute_get_history(NULL) == 0);

  taa_compute_reset(NULL);
  taa_compute_update_settings(NULL, NULL);
  CHECK(g_fake.clear_calls == 0);

  f32 x = 9.0f;
  f32 y = 9.0f;
  taa_compute_get_jitter_offset(NULL, &x, &y);
  CHECK(x == 9.0f);
  CHECK(y == 9.0f);
}

int main(void) {
  test_create_and_destroy();
  test_create_failures_release_partial_state();
  test_jitter_sequence();
  test_process_binds_dispatch_and_uniforms();
  test_ping_pong_and_output();
  test_reset_restarts_sequence_and_clears_history();
  test_update_settings();
  test_null_arguments_are_ignored();

  printf("taa_compute: %d checks, %d failures\n", g_checks, g_failures);
  return g_failures == 0 ? 0 : 1;
}
