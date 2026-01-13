#ifndef UI_ATLAS_H
#define UI_ATLAS_H

#include <stdint.h>
#include <stdbool.h>
#include "ui_types.h"

#ifdef __cplusplus
extern "C" {
#endif

// ============================================================================
// TYPES
// ============================================================================

typedef struct UIAtlas UIAtlas;

// Atlas region type
typedef enum {
    UI_ATLAS_REGION_TYPE_CUSTOM,
    UI_ATLAS_REGION_TYPE_GLYPH,
    UI_ATLAS_REGION_TYPE_ICON
} UIAtlasRegionType;

// Atlas region structure
typedef struct {
    uint32_t id;
    UIAtlasRegionType type;
    uint32_t x;
    uint32_t y;
    uint32_t width;
    uint32_t height;
    float u0, v0;
    float u1, v1;
    bool rotated;
} UIAtlasRegion;

// Glyph key for caching
typedef struct {
    uint32_t codepoint;
    uint32_t font_id;
    uint32_t size;
} UIGlyphKey;

// Glyph info
typedef struct {
    UIAtlasRegion region;
    int32_t bearing_x;
    int32_t bearing_y;
    int32_t advance;
} UIGlyphInfo;

// ============================================================================
// API
// ============================================================================

// Creation/Destruction
UIAtlas* ui_atlas_create(uint32_t width, uint32_t height, uint32_t channels);
void ui_atlas_destroy(UIAtlas* atlas);

// Atlas Management
void ui_atlas_clear(UIAtlas* atlas);
bool ui_atlas_resize(UIAtlas* atlas, uint32_t new_width, uint32_t new_height);
uint32_t ui_atlas_get_width(const UIAtlas* atlas);
uint32_t ui_atlas_get_height(const UIAtlas* atlas);
uint32_t ui_atlas_get_texture_id(const UIAtlas* atlas); // Returns GPU texture ID
const uint8_t* ui_atlas_get_data(const UIAtlas* atlas); // Returns raw pixel data

// Region Allocation
// Allocates a region in the atlas. Returns true on success, false if full.
bool ui_atlas_alloc(UIAtlas* atlas, uint32_t width, uint32_t height, UIAtlasRegion* out_region);

// Data Upload
// Uploads pixel data to a specific region
void ui_atlas_set_region_data(UIAtlas* atlas, const UIAtlasRegion* region, const uint8_t* data, uint32_t stride);

// Glyph Caching
bool ui_atlas_get_glyph(UIAtlas* atlas, UIGlyphKey key, UIGlyphInfo* out_glyph);
void ui_atlas_add_glyph(UIAtlas* atlas, UIGlyphKey key, const UIGlyphInfo* glyph);

// Debug
void ui_atlas_debug_save(const UIAtlas* atlas, const char* filename);

#ifdef __cplusplus
}
#endif

#endif // UI_ATLAS_H
