#include "../../include/core/logger.h"
#include "../../include/engine/asset_importers.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

// Minimal 1x1 Red Pixel uncompressed TGA (24-bit RGB)
// Header: 18 bytes
// Data: 3 bytes (BGR)
const unsigned char ONE_PIXEL_TGA[] = {
    0x00,       // ID length
    0x00,       // Color map type
    0x02,       // Image type (2 = uncompressed RGB)
    0x00, 0x00, // Color map spec
    0x00, 0x00,
    0x00,       // Color map depth
    0x00, 0x00, // X origin
    0x00, 0x00, // Y origin
    0x01, 0x00, // Width (1)
    0x01, 0x00, // Height (1)
    0x18,       // Bits per pixel (24)
    0x00,       // Image descriptor
    // Data (BGR) - Red
    0x00, 0x00, 0xFF};

int main() {
  logger_init(LOG_LEVEL_INFO, LOG_TARGET_CONSOLE, NULL);
  LOG_INFO("Starting Texture Importer Test...");

  // Test: Load minimal TGA from memory
  LOG_INFO("\n=== Test: Load 1x1 TGA ===");
  ImportedTexture *texture = asset_importer_load_texture_from_memory(
      ONE_PIXEL_TGA, sizeof(ONE_PIXEL_TGA), true);

  if (texture) {
    LOG_INFO("✓ Successfully loaded texture!");
    LOG_INFO("  Width: %u", texture->width);
    LOG_INFO("  Height: %u", texture->height);
    LOG_INFO("  Channels: %u", texture->channels);
    LOG_INFO("  Size: %u bytes", texture->size_bytes);

    // Use cast to unsigned char* to inspect pixels
    unsigned char *px = (unsigned char *)texture->pixels;
    if (texture->width == 1 && texture->height == 1 && texture->channels == 4) {
      LOG_INFO("  Pixel[0]: R=%d G=%d B=%d A=%d", px[0], px[1], px[2], px[3]);
      // Expect Red (255, 0, 0, 255) approx
      // Note: TGA BGR should be converted to RGBA by STB with req_comp=4
      if (px[0] == 255 && px[1] == 0 && px[2] == 0 && px[3] == 255) {
        LOG_INFO("✓ Pixel color correct (Red)!");
      } else {
        LOG_WARN("? Pixel color unexpected.");
      }
    }

    asset_importer_free_texture(texture);
    LOG_INFO("✓ Freed texture memory.");
  } else {
    LOG_ERROR("✗ Failed to load texture!");
    return 1;
  }

  LOG_INFO("\n=== Test Complete ===");
  return 0;
}
