#ifndef TEXTURE_ATLAS_H
#define TEXTURE_ATLAS_H

#include "engine/include/common.h"
#include <math/vec2.h>

// Standard texture atlas configuration
#define ATLAS_WIDTH_TILES 16
#define ATLAS_HEIGHT_TILES 16
#define TEXTURE_TILE_SIZE 16

typedef struct {
  u32 texture_id;   // OpenGL texture ID
  u16 tile_size;    // Size of one tile in pixels (e.g., 16)
  u16 width_tiles;  // Width in tiles (e.g., 16)
  u16 height_tiles; // Height in tiles (e.g., 16)
  f32 uv_step_x;    // 1.0 / width_tiles
  f32 uv_step_y;    // 1.0 / height_tiles
} TextureAtlas;

// Initialize global texture atlas
void texture_atlas_init(TextureAtlas *atlas);
void texture_atlas_free(TextureAtlas *atlas);

// Load atlas from file
bool texture_atlas_load(TextureAtlas *atlas, const char *path);

// Get UV coordinates for a specific texture index
// corners: 0=top-left, 1=top-right, 2=bottom-right, 3=bottom-left
Vec2 texture_atlas_get_uv(TextureAtlas *atlas, u32 index, u8 corner);

// Get UV quad for a specific texture index (returns array of 4 Vec2s)
void texture_atlas_get_uvs(TextureAtlas *atlas, u32 index, Vec2 *out_uvs);

#endif // TEXTURE_ATLAS_H
