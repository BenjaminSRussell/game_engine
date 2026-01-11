/*
 * surface_atlas.h
 * Lumen Surface Cache Atlas Management
 */

#ifndef SURFACE_ATLAS_H
#define SURFACE_ATLAS_H

#include <stdbool.h>
#include <stdint.h>

typedef struct surface_atlas_ctx {
  uint32_t width;
  uint32_t height;
  void *albedo_atlas;
  void *emission_atlas;
  void *depth_atlas;
  bool initialized;
} surface_atlas_ctx_t;

typedef struct {
  uint32_t x;
  uint32_t y;
  uint32_t width;
  uint32_t height;
  float uv_scale_x;
  float uv_scale_y;
  float uv_bias_x;
  float uv_bias_y;
} lumen_surface_atlas_region_t;

int surface_atlas_init(uint32_t size);
void surface_atlas_shutdown(void);
bool surface_atlas_allocate(uint32_t w, uint32_t h, uint32_t *out_x,
                            uint32_t *out_y);

#endif // SURFACE_ATLAS_H
