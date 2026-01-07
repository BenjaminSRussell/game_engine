#define STB_IMAGE_IMPLEMENTATION
#include "../../../../include/core/logger.h"
#include "../../../../include/engine/asset_importers.h"
#include "../../../../include/vendor/stb_image.h"
#include <stdlib.h>

ImportedTexture *asset_importer_load_texture_from_memory(const void *data,
                                                         size_t size,
                                                         bool flip_y) {
  if (!data || size == 0) {
    LOG_ERROR("Invalid texture data");
    return NULL;
  }

  stbi_set_flip_vertically_on_load(flip_y);

  int width, height, channels;
  // Force 4 channels (RGBA) for consistency in the engine
  unsigned char *pixels = stbi_load_from_memory(
      (const unsigned char *)data, (int)size, &width, &height, &channels, 4);

  if (!pixels) {
    LOG_ERROR("Failed to load texture from memory: %s", stbi_failure_reason());
    return NULL;
  }

  ImportedTexture *texture = (ImportedTexture *)malloc(sizeof(ImportedTexture));
  if (!texture) {
    LOG_ERROR("Failed to allocate ImportedTexture");
    stbi_image_free(pixels);
    return NULL;
  }

  texture->pixels = pixels;
  texture->width = (u32)width;
  texture->height = (u32)height;
  texture->channels = 4; // We forced 4 channels
  texture->size_bytes = (u32)(width * height * 4);
  texture->is_hdr = false;

  // Initialize mipmap fields (Phase 9)
  texture->mipmap_data = NULL;
  texture->mipmap_sizes = NULL;
  texture->mipmap_count = 1; // Base level only for now

  LOG_INFO("Loaded texture from memory: %ux%u (4 channels)", width, height);
  return texture;
}

ImportedTexture *asset_importer_load_texture(const char *path, bool flip_y) {
  if (!path) {
    LOG_ERROR("Invalid texture path");
    return NULL;
  }

  stbi_set_flip_vertically_on_load(flip_y);

  int width, height, channels;
  void *pixels = NULL;
  bool is_hdr = false;
  bool is_16bit = false;
  size_t pixel_size = 1; // Default 8-bit per channel

  // Check for HDR
  if (stbi_is_hdr(path)) {
    is_hdr = true;
    pixel_size = sizeof(float);
    // Force 4 channels (RGBA32F)
    pixels = stbi_loadf(path, &width, &height, &channels, 4);
  }
  // Check for 16-bit
  else if (stbi_is_16_bit(path)) {
    is_16bit = true;
    pixel_size = sizeof(unsigned short);
    // Force 4 channels (RGBA16)
    pixels = stbi_load_16(path, &width, &height, &channels, 4);
  }
  // Default 8-bit
  else {
    // Force 4 channels (RGBA8)
    pixels = stbi_load(path, &width, &height, &channels, 4);
  }

  if (!pixels) {
    LOG_ERROR("Failed to load texture '%s': %s", path, stbi_failure_reason());
    return NULL;
  }

  ImportedTexture *texture = (ImportedTexture *)malloc(sizeof(ImportedTexture));
  if (!texture) {
    LOG_ERROR("Failed to allocate ImportedTexture");
    stbi_image_free(pixels);
    return NULL;
  }

  texture->pixels = pixels;
  texture->width = (u32)width;
  texture->height = (u32)height;
  texture->channels = 4; // Always forced to 4
  texture->size_bytes = (u32)(width * height * 4 * pixel_size);
  texture->is_hdr = is_hdr;

  const char *type_str =
      is_hdr ? "HDR (32-bit float)" : (is_16bit ? "16-bit" : "8-bit");
  LOG_INFO("Loaded texture '%s': %ux%u %s (4 channels)", path, width, height,
           type_str);

  // Initialize mipmap fields (Phase 9)
  texture->mipmap_data = NULL;
  texture->mipmap_sizes = NULL;
  texture->mipmap_count = 1; // Base level only

  // TODO: Generate mipmaps using stb_image_resize
  // For now, we only store the base level
  // A full implementation would:
  // 1. Calculate mipmap count: floor(log2(max(width, height))) + 1
  // 2. Allocate mipmap arrays
  // 3. Generate each level by downsampling 2x

  return texture;
}

void asset_importer_free_texture(ImportedTexture *texture) {
  if (!texture)
    return;

  if (texture->pixels) {
    stbi_image_free(texture->pixels);
  }

  // Free mipmaps (Phase 9)
  if (texture->mipmap_data) {
    for (u32 i = 0; i < texture->mipmap_count; i++) {
      if (texture->mipmap_data[i]) {
        free(texture->mipmap_data[i]);
      }
    }
    free(texture->mipmap_data);
  }
  if (texture->mipmap_sizes) {
    free(texture->mipmap_sizes);
  }

  free(texture);
}
