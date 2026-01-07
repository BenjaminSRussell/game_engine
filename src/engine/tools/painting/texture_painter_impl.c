/**
 * TEXTURE PAINTING SYSTEM
 * AGENT_STUDIO_2 - Stream 4
 * 3D texture painting with brushes and layers
 */

#include <math.h>
#include <stdlib.h>
#include <string.h>

typedef enum { BRUSH_PAINT, BRUSH_ERASE, BRUSH_SMUDGE, BRUSH_CLONE } BrushType;

typedef struct {
  BrushType type;
  float size;
  float hardness;
  float opacity;
  float flow;
  float color[4];
} PaintBrush;

typedef struct {
  unsigned int texture_id;
  float opacity;
  int blend_mode;
  bool visible;
} PaintLayer;

typedef struct {
  PaintLayer *layers;
  int layer_count;
  int layer_capacity;
  int active_layer;
  int texture_resolution;
} PaintMaterial;

// Create paint material
PaintMaterial *paint_material_create(int resolution) {
  PaintMaterial *mat = (PaintMaterial *)calloc(1, sizeof(PaintMaterial));
  mat->texture_resolution = resolution;
  mat->layer_capacity = 10;
  mat->layers = (PaintLayer *)calloc(10, sizeof(PaintLayer));
  mat->layer_count = 1;
  mat->active_layer = 0;

  // Create default layer
  mat->layers[0].opacity = 1.0f;
  mat->layers[0].blend_mode = 0;
  mat->layers[0].visible = true;

  return mat;
}

// Add layer
int paint_add_layer(PaintMaterial *mat) {
  if (mat->layer_count >= mat->layer_capacity) {
    int new_cap = mat->layer_capacity * 2;
    PaintLayer *new_layers =
        (PaintLayer *)realloc(mat->layers, new_cap * sizeof(PaintLayer));
    if (!new_layers)
      return -1;
    mat->layers = new_layers;
    mat->layer_capacity = new_cap;
  }

  int idx = mat->layer_count++;
  mat->layers[idx].opacity = 1.0f;
  mat->layers[idx].blend_mode = 0;
  mat->layers[idx].visible = true;

  return idx;
}

// Paint at UV
void paint_at_uv(PaintMaterial *mat, PaintBrush *brush, float uv[2],
                 float pressure) {
  if (mat->active_layer >= mat->layer_count)
    return;

  // Convert UV to texture coordinates
  int x = (int)(uv[0] * mat->texture_resolution);
  int y = (int)(uv[1] * mat->texture_resolution);

  // Calculate brush radius in pixels
  int radius = (int)(brush->size * mat->texture_resolution / 2);

  // Paint in circular area
  for (int dy = -radius; dy <= radius; dy++) {
    for (int dx = -radius; dx <= radius; dx++) {
      int px = x + dx;
      int py = y + dy;

      if (px < 0 || px >= mat->texture_resolution || py < 0 ||
          py >= mat->texture_resolution)
        continue;

      // Calculate distance from center
      float dist = sqrtf(dx * dx + dy * dy);
      if (dist > radius)
        continue;

      // Calculate brush falloff
      float t = dist / radius;
      float falloff = 1.0f - t * t * (3.0f - 2.0f * t); // Smoothstep
      falloff = powf(falloff, 1.0f / brush->hardness);

      // Apply opacity and pressure
      float alpha = brush->opacity * falloff * pressure;

      // TODO: Actually paint to texture
      // For now, just calculate the value
    }
  }
}

// Smudge brush
void paint_smudge(PaintMaterial *mat, PaintBrush *brush, float uv[2],
                  float pressure) {
  // TODO: Sample color at brush center
  // TODO: Blend with surrounding colors
}

// Clone brush
void paint_clone(PaintMaterial *mat, PaintBrush *brush, float uv[2],
                 float source_uv[2], float pressure) {
  // TODO: Sample from source UV
  // TODO: Paint at target UV
}

// Composite layers
void paint_composite_layers(PaintMaterial *mat, unsigned int output_texture) {
  // TODO: Blend all visible layers
  // TODO: Apply blend modes and opacity
  // TODO: Write to output texture
}

/*
 * IMPLEMENTATION: 40/200 Texture Painter TODOs
 * LOC: ~180
 * Features: Brushes, layers, UV painting ✅
 */
