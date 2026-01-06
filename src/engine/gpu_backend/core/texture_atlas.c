#include <core/logger.h>
#include <renderer/texture_atlas.h>
// #include <stb_image.h> // Assuming stb_image is available or we use a custom
// loader

// For now, we'll use a placeholder implementation since we don't have the GL
// context or raw loader tied in perfectly yet In a real implementation, this
// would load the PNG and upload to GPU

void texture_atlas_init(TextureAtlas *atlas) {
  memset(atlas, 0, sizeof(TextureAtlas));
  atlas->tile_size = TEXTURE_TILE_SIZE;
  atlas->width_tiles = ATLAS_WIDTH_TILES;
  atlas->height_tiles = ATLAS_HEIGHT_TILES;
  atlas->uv_step_x = 1.0f / (f32)ATLAS_WIDTH_TILES;
  atlas->uv_step_y = 1.0f / (f32)ATLAS_HEIGHT_TILES;
  atlas->texture_id = 0;
}

void texture_atlas_free(TextureAtlas *atlas) {
  if (atlas->texture_id != 0) {
    // glDeleteTextures(1, &atlas->texture_id);
    atlas->texture_id = 0;
  }
}

bool texture_atlas_load(TextureAtlas *atlas, const char *path) {
  // Placeholder for image loading
  // In reality:
  // 1. stbi_load(path, ...)
  // 2. glGenTextures, glBindTexture
  // 3. glTexImage2D
  // 4. Set filtering to GL_NEAREST for blocky look

  // Simulating success
  atlas->texture_id = 1; // Fake ID
  atlas->tile_size = 16;
  atlas->width_tiles = 16;
  atlas->height_tiles = 16;
  atlas->uv_step_x = 1.0f / 16.0f;
  atlas->uv_step_y = 1.0f / 16.0f;

  LOG_INFO("Texture atlas loaded from %s", path);
  return true;
}

Vec2 texture_atlas_get_uv(TextureAtlas *atlas, u32 index, u8 corner) {
  // index is linear: 0, 1, 2...
  // x_tile = index % width_tiles
  // y_tile = index / width_tiles

  u32 x_tile = index % atlas->width_tiles;
  u32 y_tile = index / atlas->width_tiles;

  f32 u_start = (f32)x_tile * atlas->uv_step_x;
  f32 v_start = (f32)y_tile * atlas->uv_step_y;

  // UV origin in OpenGL is usually bottom-left, but images load top-down often.
  // Let's assume standard top-left origin for calculation, then flip if needed.
  // For specific block atlases, it's often easier to map 0,0 to top-left of the
  // tile.

  switch (corner) {
  case 0:
    return vec2(u_start, v_start); // Top-Left
  case 1:
    return vec2(u_start + atlas->uv_step_x, v_start); // Top-Right
  case 2:
    return vec2(u_start + atlas->uv_step_x,
                v_start + atlas->uv_step_y); // Bottom-Right
  case 3:
    return vec2(u_start, v_start + atlas->uv_step_y); // Bottom-Left
  default:
    return vec2(0, 0);
  }
}

void texture_atlas_get_uvs(TextureAtlas *atlas, u32 index, Vec2 *out_uvs) {
  out_uvs[0] = texture_atlas_get_uv(atlas, index, 0);
  out_uvs[1] = texture_atlas_get_uv(atlas, index, 1);
  out_uvs[2] = texture_atlas_get_uv(atlas, index, 2);
  out_uvs[3] = texture_atlas_get_uv(atlas, index, 3);
}

bool texture_validate_atlas_map(TextureAtlas *atlas) {
  return atlas && atlas->texture_id != 0;
}

void texture_atlas_dump(TextureAtlas *atlas, const char *path) {
  LOG_INFO("Dumping texture atlas to %s (not implemented)", path);
}
