#include "ui/ui_atlas.h"
#include "ui/ui_renderer.h"

#include <stdio.h>

static int g_failed = 0;

#define EXPECT(cond, msg)                                                      \
  do {                                                                         \
    if (!(cond)) {                                                             \
      fprintf(stderr, "FAIL %s:%d: %s\n", __FILE__, __LINE__, msg);            \
      g_failed++;                                                              \
      return g_failed;                                                         \
    }                                                                          \
  } while (0)

int run_ui_atlas_renderer_tests(void) {
  UIAtlas *atlas = ui_atlas_create(1024, 1024, 4);
  EXPECT(atlas != NULL, "atlas create");

  UIAtlasRegion region;
  UIAtlasRegion second;
  EXPECT(ui_atlas_alloc(atlas, 32, 32, &region), "atlas alloc 32");
  EXPECT(region.width == 32, "region width");
  EXPECT(region.height == 32, "region height");
  EXPECT(ui_atlas_alloc(atlas, 16, 16, &second), "atlas alloc 16");
  EXPECT(second.x != region.x || second.y != region.y, "regions do not overlap origin");

  UIGlyphKey key = {.codepoint = 'A', .font_id = 1, .size = 16};
  UIGlyphInfo info;
  info.region = region;
  info.bearing_x = 1;
  info.bearing_y = 2;
  info.advance = 10;
  ui_atlas_add_glyph(atlas, key, &info);

  UIGlyphInfo out;
  EXPECT(ui_atlas_get_glyph(atlas, key, &out), "glyph cache hit");
  EXPECT(out.advance == 10, "glyph advance");
  EXPECT(!ui_atlas_get_glyph(atlas, (UIGlyphKey){.codepoint = 'B', .font_id = 1, .size = 16},
                             &out),
         "missing glyph");

  ui_atlas_destroy(atlas);

  UIRenderer renderer;
  EXPECT(ui_renderer_init(&renderer, 800, 600, UI_AA_NONE, UI_GPU_BACKEND_NONE,
                          UI_Z_SORT_NONE),
         "renderer init");

  UIVertex vertices[4] = {0};
  uint32_t indices[6] = {0, 1, 2, 0, 2, 3};
  ui_renderer_submit_element(&renderer, NULL, vertices, 4, indices, 6, 1);
  ui_renderer_submit_element(&renderer, NULL, vertices, 4, indices, 6, 2);

  Rect clip = {10, 10, 100, 100};
  ui_renderer_push_clip_rect(&renderer, clip);
  ui_renderer_push_clip_rect(&renderer, (Rect){50, 50, 20, 80});
  ui_renderer_pop_clip_rect(&renderer);
  ui_renderer_pop_clip_rect(&renderer);
  ui_renderer_pop_clip_rect(&renderer);

  ui_renderer_shutdown(&renderer);
  return g_failed;
}
