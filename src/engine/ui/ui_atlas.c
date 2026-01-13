#include "ui_atlas.h"
#include "core/memory.h"
#include "core/logger.h"
#include "core/containers/hashmap.h"
#include <string.h>
#include <stdlib.h>

struct UIAtlas {
    uint32_t width;
    uint32_t height;
    uint32_t channels;
    uint8_t* pixels;
    uint32_t texture_id; // 0 means invalid/not uploaded

    // Simple Shelf Packer
    uint32_t cursor_x;
    uint32_t cursor_y;
    uint32_t current_row_height;

    HashMap* glyph_cache;
};

static u32 glyph_key_hash(const void* key) {
    const UIGlyphKey* k = (const UIGlyphKey*)key;
    u32 hash = 2166136261u;
    hash ^= k->codepoint;
    hash *= 16777619;
    hash ^= k->font_id;
    hash *= 16777619;
    hash ^= k->size;
    hash *= 16777619;
    return hash;
}

static bool glyph_key_equals(const void* a, const void* b) {
    const UIGlyphKey* ka = (const UIGlyphKey*)a;
    const UIGlyphKey* kb = (const UIGlyphKey*)b;
    return ka->codepoint == kb->codepoint &&
           ka->font_id == kb->font_id &&
           ka->size == kb->size;
}

UIAtlas* ui_atlas_create(uint32_t width, uint32_t height, uint32_t channels) {
    UIAtlas* atlas = (UIAtlas*)memory_alloc(sizeof(UIAtlas));
    if (!atlas) {
        LOG_ERROR(LOG_CAT_UI, "Failed to allocate UIAtlas structure");
        return NULL;
    }

    atlas->width = width;
    atlas->height = height;
    atlas->channels = channels;
    atlas->texture_id = 0;

    // Allocate pixels
    uint32_t data_size = width * height * channels;
    atlas->pixels = (uint8_t*)memory_alloc(data_size);
    if (!atlas->pixels) {
        LOG_ERROR(LOG_CAT_UI, "Failed to allocate UIAtlas pixels");
        memory_free(atlas);
        return NULL;
    }
    memset(atlas->pixels, 0, data_size); // Clear to transparent/black

    // Init packer
    atlas->cursor_x = 0;
    atlas->cursor_y = 0;
    atlas->current_row_height = 0;

    // Init cache
    atlas->glyph_cache = hashmap_create(
        256, // Initial capacity
        sizeof(UIGlyphKey),
        sizeof(UIGlyphInfo),
        glyph_key_hash,
        glyph_key_equals
    );

    if (!atlas->glyph_cache) {
        LOG_ERROR(LOG_CAT_UI, "Failed to create glyph cache map");
        memory_free(atlas->pixels);
        memory_free(atlas);
        return NULL;
    }

    LOG_INFO(LOG_CAT_UI, "Created UIAtlas %dx%d (%d channels)", width, height, channels);
    return atlas;
}

void ui_atlas_destroy(UIAtlas* atlas) {
    if (!atlas) return;

    if (atlas->glyph_cache) {
        hashmap_destroy(atlas->glyph_cache);
    }

    if (atlas->pixels) {
        memory_free(atlas->pixels);
    }

    memory_free(atlas);
    LOG_INFO(LOG_CAT_UI, "Destroyed UIAtlas");
}

void ui_atlas_clear(UIAtlas* atlas) {
    if (!atlas) return;

    uint32_t data_size = atlas->width * atlas->height * atlas->channels;
    memset(atlas->pixels, 0, data_size);

    atlas->cursor_x = 0;
    atlas->cursor_y = 0;
    atlas->current_row_height = 0;

    hashmap_clear(atlas->glyph_cache);
}

bool ui_atlas_resize(UIAtlas* atlas, uint32_t new_width, uint32_t new_height) {
    // Basic implementation: destroy and recreate (losing data)
    // A better implementation would try to repack.
    // For now, we return false to indicate "not supported" or "failed".
    // TODO: Implement resizing with repacking.
    LOG_WARN(LOG_CAT_UI, "UIAtlas resize not fully implemented, data will be lost");

    uint8_t* new_pixels = (uint8_t*)memory_realloc(atlas->pixels, new_width * new_height * atlas->channels);
    if (!new_pixels) return false;

    atlas->pixels = new_pixels;
    atlas->width = new_width;
    atlas->height = new_height;

    // Reset (since we lost data coherence usually, or we need to keep old data in corner)
    // Here we just clear for simplicity as this is a cache.
    ui_atlas_clear(atlas);

    return true;
}

uint32_t ui_atlas_get_width(const UIAtlas* atlas) {
    return atlas ? atlas->width : 0;
}

uint32_t ui_atlas_get_height(const UIAtlas* atlas) {
    return atlas ? atlas->height : 0;
}

uint32_t ui_atlas_get_texture_id(const UIAtlas* atlas) {
    return atlas ? atlas->texture_id : 0;
}

const uint8_t* ui_atlas_get_data(const UIAtlas* atlas) {
    return atlas ? atlas->pixels : NULL;
}

bool ui_atlas_alloc(UIAtlas* atlas, uint32_t width, uint32_t height, UIAtlasRegion* out_region) {
    if (!atlas || !out_region) return false;

    // Add 1 pixel padding to avoid bleeding
    uint32_t pad = 1;
    uint32_t padded_width = width + pad;
    uint32_t padded_height = height + pad;

    if (atlas->cursor_x + padded_width > atlas->width) {
        // Move to next row
        atlas->cursor_x = 0;
        atlas->cursor_y += atlas->current_row_height;
        atlas->current_row_height = 0;
    }

    if (atlas->cursor_y + padded_height > atlas->height) {
        // Full
        LOG_WARN(LOG_CAT_UI, "UIAtlas is full");
        return false;
    }

    // Allocation successful
    out_region->x = atlas->cursor_x;
    out_region->y = atlas->cursor_y;
    out_region->width = width;
    out_region->height = height;
    out_region->type = UI_ATLAS_REGION_TYPE_CUSTOM;
    out_region->rotated = false;

    // UV calculation
    out_region->u0 = (float)out_region->x / atlas->width;
    out_region->v0 = (float)out_region->y / atlas->height;
    out_region->u1 = (float)(out_region->x + width) / atlas->width;
    out_region->v1 = (float)(out_region->y + height) / atlas->height;

    // Update cursor
    atlas->cursor_x += padded_width;
    if (padded_height > atlas->current_row_height) {
        atlas->current_row_height = padded_height;
    }

    return true;
}

void ui_atlas_set_region_data(UIAtlas* atlas, const UIAtlasRegion* region, const uint8_t* data, uint32_t stride) {
    if (!atlas || !region || !data || !atlas->pixels) return;

    // Copy row by row
    for (uint32_t y = 0; y < region->height; y++) {
        uint32_t src_idx = y * stride;
        uint32_t dst_idx = ((region->y + y) * atlas->width + region->x) * atlas->channels;

        // Ensure bounds (clamping)
        if (region->y + y >= atlas->height) break;
        uint32_t copy_width = region->width;
        if (region->x + copy_width > atlas->width) copy_width = atlas->width - region->x;

        memcpy(atlas->pixels + dst_idx, data + src_idx, copy_width * atlas->channels);
    }
}

bool ui_atlas_get_glyph(UIAtlas* atlas, UIGlyphKey key, UIGlyphInfo* out_glyph) {
    if (!atlas) return false;

    void* result = hashmap_get(atlas->glyph_cache, &key);
    if (result) {
        if (out_glyph) {
            *out_glyph = *(UIGlyphInfo*)result;
        }
        return true;
    }
    return false;
}

void ui_atlas_add_glyph(UIAtlas* atlas, UIGlyphKey key, const UIGlyphInfo* glyph) {
    if (!atlas) return;

    hashmap_insert(atlas->glyph_cache, &key, glyph);
}

void ui_atlas_debug_save(const UIAtlas* atlas, const char* filename) {
    // TODO: Implement saving to TGA/PNG for debugging
    LOG_INFO(LOG_CAT_UI, "Saved atlas debug image to %s (stub)", filename);
}
