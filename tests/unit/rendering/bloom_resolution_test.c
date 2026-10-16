/* Issue #110: bloom_add_to_graph must size its threshold, mip 0 and output
 * textures from the resolution given to bloom_create, not a hardcoded
 * 1920x1080. Compiles bloom.c directly with render-graph and logger stubs,
 * because the Engine library does not build on this host.
 */

#include "rendering/post_processing/bloom.h"
#include "core/logging/unified_logger.h"

#include <stdio.h>
#include <string.h>

/* Logger stub: bloom.c logs through the unified logger macros. */
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

/* Render-graph stubs: record every texture descriptor bloom.c creates. */
#define MAX_RECORDED_TEXTURES 32

typedef struct {
  char name[64];
  u32 width;
  u32 height;
} RecordedTexture;

static RecordedTexture s_textures[MAX_RECORDED_TEXTURES];
static u32 s_texture_count;
static u32 s_resource_count;
static u32 s_pass_count;
static char s_render_graph_token; /* bloom.c only passes this pointer through */

RGResourceHandle rg_create_texture(RenderGraph *rg, const RGTextureDesc *desc) {
  RGResourceHandle handle;
  (void)rg;
  handle.id = ++s_resource_count;
  if (s_texture_count < MAX_RECORDED_TEXTURES) {
    RecordedTexture *t = &s_textures[s_texture_count++];
    snprintf(t->name, sizeof(t->name), "%s", desc->name ? desc->name : "");
    t->width = desc->width;
    t->height = desc->height;
  }
  return handle;
}

RGPassHandle rg_add_pass(RenderGraph *rg, const RGPassDesc *desc) {
  RGPassHandle handle;
  (void)rg;
  (void)desc;
  handle.id = ++s_pass_count;
  return handle;
}

void rg_pass_read(RenderGraph *rg, RGPassHandle pass, RGResourceHandle resource) {
  (void)rg;
  (void)pass;
  (void)resource;
}

void rg_pass_write(RenderGraph *rg, RGPassHandle pass, RGResourceHandle resource) {
  (void)rg;
  (void)pass;
  (void)resource;
}

TextureID rg_ctx_get_texture(RGPassContext *ctx, RGResourceHandle handle) {
  TextureID id;
  (void)ctx;
  (void)handle;
  memset(&id, 0, sizeof(id));
  return id;
}

static int s_failures;

static const RecordedTexture *find_texture(const char *name) {
  for (u32 i = 0; i < s_texture_count; i++) {
    if (strcmp(s_textures[i].name, name) == 0) {
      return &s_textures[i];
    }
  }
  return NULL;
}

static void expect_size(const char *name, u32 width, u32 height) {
  const RecordedTexture *t = find_texture(name);
  if (!t) {
    printf("FAIL: texture '%s' was not created\n", name);
    s_failures++;
    return;
  }
  if (t->width != width || t->height != height) {
    printf("FAIL: %s is %ux%u, expected %ux%u\n", name, (unsigned)t->width,
           (unsigned)t->height, (unsigned)width, (unsigned)height);
    s_failures++;
  } else {
    printf("PASS: %s is %ux%u\n", name, (unsigned)width, (unsigned)height);
  }
}

static void check_resolution(u32 width, u32 height) {
  RenderGraph *rg = (RenderGraph *)&s_render_graph_token;
  RGResourceHandle scene_color;
  RGResourceHandle output;
  BloomContext *ctx = bloom_create(width, height);

  printf("-- bloom_create(%u, %u)\n", (unsigned)width, (unsigned)height);
  if (!ctx) {
    printf("FAIL: bloom_create returned NULL\n");
    s_failures++;
    return;
  }

  s_texture_count = 0;
  s_pass_count = 0;
  scene_color.id = 1;
  output = bloom_add_to_graph(rg, ctx, scene_color);
  if (output.id == RG_INVALID_RESOURCE.id) {
    printf("FAIL: bloom_add_to_graph returned an invalid handle\n");
    s_failures++;
  }

  expect_size("Bloom_Threshold", width, height);
  expect_size("Bloom_Mip0", width / 2, height / 2);
  expect_size("Bloom_Output", width, height);

  bloom_destroy(ctx);
}

int main(void) {
  check_resolution(2560, 1440);
  check_resolution(1280, 720);

  if (s_failures != 0) {
    printf("%d bloom resolution check(s) failed\n", s_failures);
    return 1;
  }
  printf("bloom resolution checks passed\n");
  return 0;
}
