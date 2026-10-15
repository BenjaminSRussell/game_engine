// Test-only stand-in for rendering/core/texture.h (see buffer.h). TextureID is
// the u32 typedef from core/math/types.h.
#ifndef GPU_SKINNING_TEST_TEXTURE_H
#define GPU_SKINNING_TEST_TEXTURE_H

#include "core/math/types.h"

typedef struct TextureDesc {
  u32 width;
  u32 height;
  u32 format;
  u32 usage;
} TextureDesc;

enum {
  TEXTURE_FORMAT_RGBA32F = 1,
};

enum {
  TEXTURE_USAGE_SAMPLED = BIT(0),
  TEXTURE_USAGE_STORAGE = BIT(1),
};

TextureID texture_create(const TextureDesc *desc);
void texture_destroy(TextureID texture);
void texture_bind_compute(TextureID texture, u32 binding);

#endif
