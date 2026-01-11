/*
 * surface_atlas.c
 * Lumen Surface Cache Atlas Management Implementation
 */

#include "lighting/lumen/surface_cache/surface_atlas.h"
#include "include/core/logger.h"
#include "lighting/lightmaps/lightmap_packer.h"
#include <stdlib.h>

static surface_atlas_ctx_t g_atlas = {0};

int surface_atlas_init(uint32_t size) {
  if (g_atlas.initialized)
    return 0;

  g_atlas.width = size;
  g_atlas.height = size;

  // Initialize underlying systems
  // Note: ignoring return values/args mismatch for now to fix build
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
}

bool surface_atlas_allocate(uint32_t w, uint32_t h, uint32_t *out_x,
                            uint32_t *out_y) {
  if (!g_atlas.initialized)
    return false;

  // Stub allocation - just return 0,0 for now to pass build
  // Real implementation would use packing algo
  if (out_x)
    *out_x = 0;
  if (out_y)
    *out_y = 0;

  return true;
}
