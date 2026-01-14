// src/engine/ai/style_templates.c
//
// Purpose: 25 different 3D asset style templates for AI generation.
// Each style defines parameters for generating assets in that style.
//
#include <ai/ai_asset_generator.h>
#include "engine/include/core/logger.h"
#include <include/math/math_all.h>
#include <stdlib.h>
#include <string.h>

// Style template parameters
typedef struct {
  AssetStyle style;
  const char *name;
  const char *description;

  // Generation parameters
  f32 complexity_range[2]; // Min/max complexity
  f32 detail_level;        // Detail level (0.0-1.0)
  f32 smoothness;          // Surface smoothness (0.0-1.0)
  f32 color_saturation;    // Color saturation (0.0-1.0)
  f32 edge_sharpness;      // Edge sharpness (0.0-1.0)

  // Snap point configuration
  u32 snap_point_count; // Typical snap point count
  f32 snap_point_size;  // Snap point size
  bool grid_aligned;    // Whether snap points are grid-aligned

  // Geometry parameters
  f32 polygon_count_factor; // Polygon count multiplier
  bool use_smoothing;       // Use smoothing groups
  bool use_subdivision;     // Use subdivision surfaces

  // Texture parameters
  u32 texture_resolution;  // Texture resolution
  bool use_normal_maps;    // Use normal maps
  bool use_roughness_maps; // Use roughness maps

  // AI generation hints
  const char *keywords[16]; // Style keywords
  u32 keyword_count;
} StyleTemplate;

// Style templates for all 25 styles
static StyleTemplate g_style_templates[ASSET_STYLE_COUNT] = {
    // 1. Realistic
    {ASSET_STYLE_REALISTIC,
     "Realistic",
     "Photorealistic assets",
     {0.7f, 1.0f},
     0.9f,
     0.95f,
     0.8f,
     0.3f,
     6,
     0.1f,
     false,
     1.0f,
     true,
     true,
     2048,
     true,
     true,
     {"realistic", "photoreal", "detailed", "textured", NULL},
     4},

    // 2. Cartoon
    {ASSET_STYLE_CARTOON,
     "Cartoon",
     "Cartoon-style assets",
     {0.3f, 0.6f},
     0.5f,
     0.7f,
     1.0f,
     0.8f,
     8,
     0.15f,
     true,
     0.5f,
     false,
     false,
     1024,
     false,
     false,
     {"cartoon", "stylized", "colorful", "simple", NULL},
     4},

    // 3. Anime
    {ASSET_STYLE_ANIME,
     "Anime",
     "Anime-style assets",
     {0.4f, 0.7f},
     0.6f,
     0.8f,
     0.9f,
     0.6f,
     6,
     0.12f,
     false,
     0.6f,
     true,
     false,
     1024,
     true,
     false,
     {"anime", "manga", "cel-shaded", "bright", NULL},
     4},

    // 4. Pixel
    {ASSET_STYLE_PIXEL,
     "Pixel",
     "Pixel art style",
     {0.1f, 0.3f},
     0.2f,
     0.1f,
     0.9f,
     1.0f,
     4,
     0.2f,
     true,
     0.1f,
     false,
     false,
     256,
     false,
     false,
     {"pixel", "8bit", "retro", "low-res", NULL},
     4},

    // 5. Low Poly
    {ASSET_STYLE_LOW_POLY,
     "Low Poly",
     "Low polygon count style",
     {0.2f, 0.4f},
     0.3f,
     0.5f,
     0.7f,
     0.9f,
     6,
     0.1f,
     true,
     0.3f,
     false,
     false,
     512,
     false,
     false,
     {"low-poly", "minimal", "geometric", "simple", NULL},
     4},

    // 6. High Poly
    {ASSET_STYLE_HIGH_POLY,
     "High Poly",
     "High polygon count style",
     {0.8f, 1.0f},
     0.95f,
     0.98f,
     0.85f,
     0.2f,
     8,
     0.08f,
     false,
     2.0f,
     true,
     true,
     4096,
     true,
     true,
     {"high-poly", "detailed", "smooth", "realistic", NULL},
     4},

    // 7. Stylized
    {ASSET_STYLE_STYLIZED,
     "Stylized",
     "Artistic stylized assets",
     {0.4f, 0.7f},
     0.6f,
     0.75f,
     0.85f,
     0.5f,
     6,
     0.12f,
     false,
     0.7f,
     true,
     false,
     1024,
     true,
     false,
     {"stylized", "artistic", "unique", "creative", NULL},
     4},

    // 8. Cel-Shaded
    {ASSET_STYLE_CEL_SHADED,
     "Cel-Shaded",
     "Cel-shaded style",
     {0.3f, 0.6f},
     0.5f,
     0.6f,
     0.9f,
     0.7f,
     6,
     0.12f,
     false,
     0.5f,
     false,
     false,
     1024,
     false,
     false,
     {"cel-shaded", "toon", "flat", "comic", NULL},
     4},

    // 9. Toon
    {ASSET_STYLE_TOON,
     "Toon",
     "Toon style assets",
     {0.3f, 0.5f},
     0.4f,
     0.65f,
     0.95f,
     0.75f,
     6,
     0.13f,
     false,
     0.4f,
     false,
     false,
     512,
     false,
     false,
     {"toon", "cartoon", "simple", "bright", NULL},
     4},

    // 10. Organic
    {ASSET_STYLE_ORGANIC,
     "Organic",
     "Organic flowing shapes",
     {0.5f, 0.8f},
     0.7f,
     0.9f,
     0.75f,
     0.1f,
     4,
     0.1f,
     false,
     0.8f,
     true,
     true,
     1024,
     true,
     false,
     {"organic", "flowing", "smooth", "natural", NULL},
     4},

    // 11. Technical
    {ASSET_STYLE_TECHNICAL,
     "Technical",
     "Technical/mechanical style",
     {0.6f, 0.9f},
     0.8f,
     0.4f,
     0.6f,
     0.95f,
     12,
     0.08f,
     true,
     1.2f,
     false,
     false,
     2048,
     true,
     true,
     {"technical", "mechanical", "precise", "industrial", NULL},
     4},

    // 12. Medieval
    {ASSET_STYLE_MEDIEVAL,
     "Medieval",
     "Medieval fantasy style",
     {0.5f, 0.8f},
     0.7f,
     0.6f,
     0.7f,
     0.6f,
     8,
     0.12f,
     false,
     0.9f,
     true,
     false,
     1024,
     true,
     true,
     {"medieval", "fantasy", "stone", "wood", NULL},
     4},

    // 13. Sci-fi
    {ASSET_STYLE_SCIFI,
     "Sci-Fi",
     "Science fiction style",
     {0.6f, 0.9f},
     0.8f,
     0.7f,
     0.65f,
     0.7f,
     10,
     0.1f,
     true,
     1.1f,
     true,
     false,
     2048,
     true,
     true,
     {"sci-fi", "futuristic", "tech", "space", NULL},
     4},

    // 14. Steampunk
    {ASSET_STYLE_STEAMPUNK,
     "Steampunk",
     "Steampunk style",
     {0.6f, 0.85f},
     0.75f,
     0.5f,
     0.7f,
     0.8f,
     10,
     0.11f,
     false,
     1.0f,
     true,
     false,
     1024,
     true,
     true,
     {"steampunk", "brass", "gears", "victorian", NULL},
     4},

    // 15. Cyberpunk
    {ASSET_STYLE_CYBERPUNK,
     "Cyberpunk",
     "Cyberpunk style",
     {0.6f, 0.9f},
     0.8f,
     0.75f,
     0.9f,
     0.65f,
     8,
     0.1f,
     true,
     1.2f,
     true,
     false,
     2048,
     true,
     true,
     {"cyberpunk", "neon", "futuristic", "dark", NULL},
     4},

    // 16. Fantasy
    {ASSET_STYLE_FANTASY,
     "Fantasy",
     "Fantasy style",
     {0.5f, 0.8f},
     0.7f,
     0.7f,
     0.8f,
     0.5f,
     8,
     0.12f,
     false,
     0.9f,
     true,
     false,
     1024,
     true,
     true,
     {"fantasy", "magical", "ornate", "detailed", NULL},
     4},

    // 17. Horror
    {ASSET_STYLE_HORROR,
     "Horror",
     "Horror style",
     {0.6f, 0.9f},
     0.8f,
     0.5f,
     0.4f,
     0.7f,
     6,
     0.1f,
     false,
     1.0f,
     true,
     false,
     1024,
     true,
     true,
     {"horror", "dark", "scary", "gritty", NULL},
     4},

    // 18. Cute
    {ASSET_STYLE_CUTE,
     "Cute",
     "Cute/kawaii style",
     {0.2f, 0.5f},
     0.4f,
     0.8f,
     1.0f,
     0.6f,
     6,
     0.15f,
     false,
     0.4f,
     true,
     false,
     512,
     false,
     false,
     {"cute", "kawaii", "adorable", "soft", NULL},
     4},

    // 19. Gritty
    {ASSET_STYLE_GRITTY,
     "Gritty",
     "Gritty realistic style",
     {0.7f, 0.95f},
     0.85f,
     0.4f,
     0.5f,
     0.8f,
     6,
     0.1f,
     false,
     1.1f,
     true,
     false,
     2048,
     true,
     true,
     {"gritty", "realistic", "worn", "detailed", NULL},
     4},

    // 20. Minimalist
    {ASSET_STYLE_MINIMALIST,
     "Minimalist",
     "Minimalist style",
     {0.1f, 0.3f},
     0.2f,
     0.9f,
     0.5f,
     0.9f,
     4,
     0.2f,
     true,
     0.2f,
     false,
     false,
     256,
     false,
     false,
     {"minimalist", "simple", "clean", "geometric", NULL},
     4},

    // 21. Detailed
    {ASSET_STYLE_DETAILED,
     "Detailed",
     "Highly detailed style",
     {0.8f, 1.0f},
     0.95f,
     0.85f,
     0.8f,
     0.3f,
     10,
     0.08f,
     false,
     1.5f,
     true,
     true,
     4096,
     true,
     true,
     {"detailed", "intricate", "complex", "realistic", NULL},
     4},

    // 22. Blocky (Minecraft-like)
    {ASSET_STYLE_BLOCKY,
     "Blocky",
     "Blocky voxel style",
     {0.2f, 0.4f},
     0.3f,
     0.1f,
     0.8f,
     1.0f,
     12,
     0.2f,
     true,
     0.2f,
     false,
     false,
     256,
     false,
     false,
     {"blocky", "voxel", "minecraft", "cubic", NULL},
     4},

    // 23. Smooth
    {ASSET_STYLE_SMOOTH,
     "Smooth",
     "Smooth flowing style",
     {0.4f, 0.7f},
     0.6f,
     0.95f,
     0.75f,
     0.2f,
     6,
     0.1f,
     false,
     0.7f,
     true,
     true,
     1024,
     true,
     false,
     {"smooth", "flowing", "organic", "soft", NULL},
     4},

    // 24. Geometric
    {ASSET_STYLE_GEOMETRIC,
     "Geometric",
     "Geometric shapes",
     {0.3f, 0.6f},
     0.5f,
     0.6f,
     0.7f,
     1.0f,
     8,
     0.12f,
     true,
     0.5f,
     false,
     false,
     512,
     false,
     false,
     {"geometric", "angular", "precise", "mathematical", NULL},
     4},

    // 25. Organic Flow
    {ASSET_STYLE_ORGANIC_FLOW,
     "Organic Flow",
     "Flowing organic style",
     {0.5f, 0.8f},
     0.7f,
     0.92f,
     0.8f,
     0.15f,
     4,
     0.1f,
     false,
     0.85f,
     true,
     true,
     1024,
     true,
     false,
     {"organic", "flowing", "natural", "smooth", NULL},
     4}};

// Get style template
const StyleTemplate *ai_get_style_template(AssetStyle style) {
  if (style >= ASSET_STYLE_COUNT) {
    return NULL;
  }
  return &g_style_templates[style];
}

// Get style name
const char *ai_get_style_name(AssetStyle style) {
  const StyleTemplate *template = ai_get_style_template(style);
  return template ? template->name : "Unknown";
}

// Get style description
const char *ai_get_style_description(AssetStyle style) {
  const StyleTemplate *template = ai_get_style_template(style);
  return template ? template->description : "Unknown style";
}

// Apply style to config
void ai_apply_style_to_config(AssetConfig *config, AssetStyle style) {
  if (!config) {
    return;
  }

  const StyleTemplate *template = ai_get_style_template(style);
  if (!template) {
    return;
  }

  config->style = style;
  config->complexity_target =
      (template->complexity_range[0] + template->complexity_range[1]) * 0.5f;
  config->detail_level = template->detail_level;
}

// Get style parameters
f32 ai_get_style_smoothness(AssetStyle style) {
  const StyleTemplate *template = ai_get_style_template(style);
  return template ? template->smoothness : 0.5f;
}

f32 ai_get_style_color_saturation(AssetStyle style) {
  const StyleTemplate *template = ai_get_style_template(style);
  return template ? template->color_saturation : 0.7f;
}

u32 ai_get_style_snap_point_count(AssetStyle style) {
  const StyleTemplate *template = ai_get_style_template(style);
  return template ? template->snap_point_count : 6;
}

bool ai_get_style_grid_aligned(AssetStyle style) {
  const StyleTemplate *template = ai_get_style_template(style);
  return template ? template->grid_aligned : false;
}
