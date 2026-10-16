/* Known-value checks for the tone curves in tonemapping.c, and for the operator
 * and exposure handling in tonemap_add_to_graph. Issue #230.
 *
 * The unit is included directly so its static curve helpers are reachable.
 * Engine does not build on origin/main, so this target cannot link it. The
 * render graph, shader and logger calls the unit makes are stubbed below.
 */

#include "../../../src/engine/rendering/post_processing/tonemapping.c"

#include <math.h>
#include <stdio.h>
#include <stdlib.h>

static RGResourceHandle g_created_texture;
static const RGPassDesc *g_last_pass_desc;

void unified_logger_log(LogLevel level, LogCategory category, const char *file,
                        int line, const char *function, const char *format,
                        ...) {
  (void)level;
  (void)category;
  (void)file;
  (void)line;
  (void)function;
  (void)format;
}

uint32_t shader_load_from_file(const char *filepath, ShaderType type) {
  (void)filepath;
  (void)type;
  return 1;
}

bool shader_compile(uint32_t module_id) {
  (void)module_id;
  return true;
}

RGResourceHandle rg_create_texture(RenderGraph *rg, const RGTextureDesc *desc) {
  (void)rg;
  (void)desc;
  return g_created_texture;
}

RGPassHandle rg_add_pass(RenderGraph *rg, const RGPassDesc *desc) {
  (void)rg;
  g_last_pass_desc = desc;
  return (RGPassHandle){1};
}

void rg_pass_read(RenderGraph *rg, RGPassHandle pass,
                  RGResourceHandle resource) {
  (void)rg;
  (void)pass;
  (void)resource;
}

void rg_pass_write(RenderGraph *rg, RGPassHandle pass,
                   RGResourceHandle resource) {
  (void)rg;
  (void)pass;
  (void)resource;
}

TextureID rg_ctx_get_texture(RGPassContext *ctx, RGResourceHandle handle) {
  (void)ctx;
  (void)handle;
  return 0;
}

static int g_failed = 0;

static bool tm_near(float actual, float expected) {
  return fabsf(actual - expected) < 1e-5f;
}

#define EXPECT(cond, msg)                                                      \
  do {                                                                         \
    if (!(cond)) {                                                             \
      fprintf(stderr, "FAIL %s:%d: %s\n", __FILE__, __LINE__, msg);            \
      g_failed++;                                                              \
      return;                                                                  \
    }                                                                          \
  } while (0)

#define EXPECT_NEAR(actual, expected, msg)                                     \
  do {                                                                         \
    float got_ = (actual);                                                     \
    float want_ = (expected);                                                  \
    if (!tm_near(got_, want_)) {                                               \
      fprintf(stderr, "FAIL %s:%d: %s (got %.7f, want %.7f)\n", __FILE__,      \
              __LINE__, msg, got_, want_);                                     \
      g_failed++;                                                              \
      return;                                                                  \
    }                                                                          \
  } while (0)

/* Expected values are the closed-form curves evaluated at each input. */

static void test_reinhard_known_values(void) {
  EXPECT_NEAR(reinhard_tonemap(0.0f), 0.0f, "reinhard(0)");
  EXPECT_NEAR(reinhard_tonemap(0.18f), 0.1525424f, "reinhard(0.18)");
  EXPECT_NEAR(reinhard_tonemap(0.5f), 1.0f / 3.0f, "reinhard(0.5)");
  EXPECT_NEAR(reinhard_tonemap(1.0f), 0.5f, "reinhard(1)");
  EXPECT_NEAR(reinhard_tonemap(4.0f), 0.8f, "reinhard(4)");
}

static void test_aces_known_values(void) {
  EXPECT_NEAR(aces_tonemap(0.0f), 0.0f, "aces(0)");
  EXPECT_NEAR(aces_tonemap(0.18f), 0.2668989f, "aces(0.18)");
  EXPECT_NEAR(aces_tonemap(0.5f), 0.6163070f, "aces(0.5)");
  EXPECT_NEAR(aces_tonemap(1.0f), 0.8037975f, "aces(1)");
  EXPECT_NEAR(aces_tonemap(4.0f), 0.9734171f, "aces(4)");
}

static void test_filmic_known_values(void) {
  EXPECT_NEAR(uncharted2_tonemap(0.0f), 0.0f, "filmic(0)");
  EXPECT_NEAR(uncharted2_tonemap(0.18f), 0.0486633f, "filmic(0.18)");
  EXPECT_NEAR(uncharted2_tonemap(0.5f), 0.1247002f, "filmic(0.5)");
  EXPECT_NEAR(uncharted2_tonemap(1.0f), 0.2206573f, "filmic(1)");
  EXPECT_NEAR(uncharted2_tonemap(4.0f), 0.5171898f, "filmic(4)");
}

static void test_apply_tonemap_selects_operator(void) {
  EXPECT_NEAR(apply_tonemap(1.0f, TONEMAP_ACES), 0.8037975f, "ACES selected");
  EXPECT_NEAR(apply_tonemap(1.0f, TONEMAP_FILMIC), 0.2206573f,
              "FILMIC selected");
  EXPECT_NEAR(apply_tonemap(1.0f, TONEMAP_REINHARD), 0.5f, "REINHARD selected");
  EXPECT_NEAR(apply_tonemap(0.37f, TONEMAP_COUNT), 0.37f,
              "unknown operator passes the value through");
}

static void test_graph_rejects_bad_inputs_and_clamps(void) {
  static int graph_storage; /* the stubs never dereference the graph */
  RenderGraph *graph = (RenderGraph *)&graph_storage;
  RGResourceHandle hdr = {3};
  RGResourceHandle out;
  TonemapPassData *pass;

  g_created_texture = (RGResourceHandle){42};

  out = tonemap_add_to_graph(NULL, hdr, TONEMAP_ACES, 1.0f);
  EXPECT(out.id == RG_INVALID_RESOURCE.id, "NULL graph should be rejected");

  out = tonemap_add_to_graph(graph, RG_INVALID_RESOURCE, TONEMAP_ACES, 1.0f);
  EXPECT(out.id == RG_INVALID_RESOURCE.id,
         "invalid HDR input should be rejected");

  out = tonemap_add_to_graph(graph, hdr, TONEMAP_COUNT, 10.0f);
  EXPECT(out.id == 42, "valid call should return the LDR output");
  pass = (TonemapPassData *)g_last_pass_desc->user_data;
  EXPECT(pass->operator == TONEMAP_ACES,
         "out-of-range operator should fall back to ACES");
  EXPECT_NEAR(pass->exposure, 4.0f, "exposure should clamp to 4.0");
  free(pass);

  out = tonemap_add_to_graph(graph, hdr, TONEMAP_REINHARD, 0.0f);
  EXPECT(out.id == 42, "valid call should return the LDR output");
  pass = (TonemapPassData *)g_last_pass_desc->user_data;
  EXPECT(pass->operator == TONEMAP_REINHARD, "valid operator should be kept");
  EXPECT_NEAR(pass->exposure, 0.1f, "exposure should clamp to 0.1");
  free(pass);
}

int main(void) {
  test_reinhard_known_values();
  test_aces_known_values();
  test_filmic_known_values();
  test_apply_tonemap_selects_operator();
  test_graph_rejects_bad_inputs_and_clamps();

  if (g_failed) {
    fprintf(stderr, "%d tonemapping test(s) failed\n", g_failed);
    return 1;
  }
  printf("tonemapping tests passed\n");
  return 0;
}
