/*
 * surface_atlas.c
 * Lumen Surface Cache Atlas Management Implementation
 */

#include "lighting/lumen/surface_cache/surface_atlas.h"
#include "include/core/logger.h"
#include "lighting/lightmaps/lightmap_packer.h"

static surface_atlas_ctx_t g_atlas = {0};
static uint32_t g_cursor_x = 0;
static uint32_t g_cursor_y = 0;
static uint32_t g_row_h = 0;

static void surface_atlas_reset_cursor(void) {
  g_cursor_x = 0;
  g_cursor_y = 0;
  g_row_h = 0;
}

int surface_atlas_init(uint32_t size) {
  if (g_atlas.initialized)
    return 0;

  g_atlas.width = size;
  g_atlas.height = size;
  surface_atlas_reset_cursor();

  /* Packer init takes no size; the atlas owns placement. */
  lighting_lightmap_packer_init();

  g_atlas.initialized = true;
  LOG_INFO("Surface Atlas Initialized: %dx%d", size, size);
  return 0;
}

void surface_atlas_shutdown(void) {
  if (!g_atlas.initialized)
    return;

  lighting_lightmap_packer_shutdown();
  g_atlas.initialized = false;
  g_atlas.width = 0;
  g_atlas.height = 0;
  surface_atlas_reset_cursor();
}

bool surface_atlas_allocate(uint32_t w, uint32_t h, uint32_t *out_x,
                            uint32_t *out_y) {
  uint32_t x;
  uint32_t y;

  if (!g_atlas.initialized || w == 0 || h == 0)
    return false;
  if (w > g_atlas.width || h > g_atlas.height)
    return false;

  x = g_cursor_x;
  y = g_cursor_y;
  if (w > g_atlas.width - x) {
    if (g_row_h > g_atlas.height - y)
      return false;
    y = g_cursor_y + g_row_h;
    x = 0;
    if (h > g_atlas.height - y)
      return false;
    g_cursor_y = y;
    g_row_h = h;
    g_cursor_x = w;
  } else {
    if (h > g_atlas.height - y)
      return false;
    if (h > g_row_h)
      g_row_h = h;
    g_cursor_x = x + w;
  }

  if (out_x)
    *out_x = x;
  if (out_y)
    *out_y = y;
  return true;
}
