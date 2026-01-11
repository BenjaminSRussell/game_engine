#include <rendering/texture_atlas.h>
#include <stdlib.h>
#include <stdio.h>

// Stubs for missing texture atlas implementation

void texture_atlas_init(TextureAtlas *atlas) {
    if (!atlas) return;
    atlas->texture_id = 0;
    atlas->width_tiles = ATLAS_WIDTH_TILES;
    atlas->height_tiles = ATLAS_HEIGHT_TILES;
    atlas->tile_size = TEXTURE_TILE_SIZE;
    atlas->uv_step_x = 1.0f / (float)atlas->width_tiles;
    atlas->uv_step_y = 1.0f / (float)atlas->height_tiles;
}

void texture_atlas_free(TextureAtlas *atlas) {
    // No-op
}

bool texture_atlas_load(TextureAtlas *atlas, const char *path) {
    return true; // Stub success
}

Vec2 texture_atlas_get_uv(TextureAtlas *atlas, u32 index, u8 corner) {
    return (Vec2){0.0f, 0.0f};
}

void texture_atlas_get_uvs(TextureAtlas *atlas, u32 index, Vec2 *out_uvs) {
    for(int i=0; i<4; i++) out_uvs[i] = (Vec2){0.0f, 0.0f};
}

// Missing link symbols
void texture_create_sampler(void) {}
void texture_load_atlas(void) {}
void texture_load_atlas_map(void) {}
void texture_setup_descriptors(void) {}
void texture_validate_atlas_map(void) {}
