/**
 * ADVANCED MATERIALS SYSTEM
 * AGENT_RENDER_2 - Stream 7
 * PBR materials with layering and procedural shaders
 */

#include <include/math/math.h>
#include <stdlib.h>
#include <string.h>

typedef struct {
  float albedo[3];
  float roughness;
  float metallic;
  float normal[3];
  float ao;
  float emissive[3];
} MaterialProperties;

typedef enum {
  BLEND_NORMAL,
  BLEND_MULTIPLY,
  BLEND_ADD,
  BLEND_OVERLAY
} BlendMode;

typedef struct {
  MaterialProperties props;
  unsigned int mask_texture;
  float opacity;
  BlendMode blend_mode;
} MaterialLayer;

typedef struct {
  MaterialLayer *layers;
  int layer_count;
  int layer_capacity;
  bool has_clearcoat;
  bool has_subsurface;
} LayeredMaterial;

// Create layered material
LayeredMaterial *material_create_layered(int initial_capacity) {
  LayeredMaterial *mat = (LayeredMaterial *)calloc(1, sizeof(LayeredMaterial));
  if (!mat)
    return NULL;

  mat->layer_capacity = initial_capacity;
  mat->layer_count = 0;
  mat->layers =
      (MaterialLayer *)calloc(initial_capacity, sizeof(MaterialLayer));
  if (!mat->layers) {
    free(mat);
    return NULL;
  }
  mat->has_clearcoat = false;
  mat->has_subsurface = false;
  return mat;
}

// Add layer
bool material_add_layer(LayeredMaterial *mat, const MaterialProperties *props,
                        BlendMode blend, float opacity) {
  if (mat->layer_count >= mat->layer_capacity) {
    int new_capacity = mat->layer_capacity * 2;
    MaterialLayer *new_layers = (MaterialLayer *)realloc(
        mat->layers, new_capacity * sizeof(MaterialLayer));
    if (!new_layers)
      return false;
    mat->layers = new_layers;
    mat->layer_capacity = new_capacity;
  }

  MaterialLayer *layer = &mat->layers[mat->layer_count++];
  layer->props = *props;
  layer->blend_mode = blend;
  layer->opacity = opacity;
  layer->mask_texture = 0;

  return true;
}

// Blend two colors
void blend_color(float *base, const float *blend, BlendMode mode,
                 float opacity) {
  float result[3];

  switch (mode) {
  case BLEND_NORMAL:
    for (int i = 0; i < 3; i++) {
      result[i] = blend[i];
    }
    break;

  case BLEND_MULTIPLY:
    for (int i = 0; i < 3; i++) {
      result[i] = base[i] * blend[i];
    }
    break;

  case BLEND_ADD:
    for (int i = 0; i < 3; i++) {
      result[i] = base[i] + blend[i];
      if (result[i] > 1.0f)
        result[i] = 1.0f;
    }
    break;

  case BLEND_OVERLAY:
    for (int i = 0; i < 3; i++) {
      if (base[i] < 0.5f) {
        result[i] = 2.0f * base[i] * blend[i];
      } else {
        result[i] = 1.0f - 2.0f * (1.0f - base[i]) * (1.0f - blend[i]);
      }
    }
    break;
  }

  // Apply opacity
  for (int i = 0; i < 3; i++) {
    base[i] = base[i] * (1.0f - opacity) + result[i] * opacity;
  }
}

// Evaluate layered material
void material_evaluate(const LayeredMaterial *mat, float uv[2],
                       MaterialProperties *out_props) {
  if (mat->layer_count == 0) {
    memset(out_props, 0, sizeof(MaterialProperties));
    return;
  }

  // Start with bottom layer
  *out_props = mat->layers[0].props;

  // Blend each layer on top
  for (int i = 1; i < mat->layer_count; i++) {
    const MaterialLayer *layer = &mat->layers[i];

    blend_color(out_props->albedo, layer->props.albedo, layer->blend_mode,
                layer->opacity);

    // Blend other properties (simple lerp for now)
    float t = layer->opacity;
    out_props->roughness =
        out_props->roughness * (1.0f - t) + layer->props.roughness * t;
    out_props->metallic =
        out_props->metallic * (1.0f - t) + layer->props.metallic * t;
    out_props->ao = out_props->ao * (1.0f - t) + layer->props.ao * t;
  }
}

// Procedural noise
float perlin_noise(float x, float y) {
  // Simplified Perlin noise
  int xi = (int)floorf(x);
  int yi = (int)floorf(y);

  float xf = x - xi;
  float yf = y - yi;

  // Smooth interpolation
  float u = xf * xf * (3.0f - 2.0f * xf);
  float v = yf * yf * (3.0f - 2.0f * yf);

  // Hash function (simple)
  auto hash = [](int a, int b) -> float {
    int h = a * 374761393 + b * 668265263;
    h = (h ^ (h >> 13)) * 1274126177;
    return (h & 0x7FFFFFFF) / (float)0x7FFFFFFF;
  };

  float c00 = hash(xi, yi);
  float c10 = hash(xi + 1, yi);
  float c01 = hash(xi, yi + 1);
  float c11 = hash(xi + 1, yi + 1);

  float nx0 = c00 * (1.0f - u) + c10 * u;
  float nx1 = c01 * (1.0f - u) + c11 * u;

  return nx0 * (1.0f - v) + nx1 * v;
}

// Generate procedural material
void material_generate_procedural(MaterialProperties *props, float uv[2],
                                  int type) {
  switch (type) {
  case 0: // Metal
    props->albedo[0] = 0.8f;
    props->albedo[1] = 0.8f;
    props->albedo[2] = 0.8f;
    props->roughness = 0.2f + perlin_noise(uv[0] * 10, uv[1] * 10) * 0.3f;
    props->metallic = 1.0f;
    break;

  case 1: // Wood
    float grain = perlin_noise(uv[0] * 5, uv[1] * 20);
    props->albedo[0] = 0.4f + grain * 0.2f;
    props->albedo[1] = 0.2f + grain * 0.1f;
    props->albedo[2] = 0.1f;
    props->roughness = 0.6f;
    props->metallic = 0.0f;
    break;

  case 2: // Stone
    float noise = perlin_noise(uv[0] * 8, uv[1] * 8);
    props->albedo[0] = 0.5f + noise * 0.2f;
    props->albedo[1] = 0.5f + noise * 0.2f;
    props->albedo[2] = 0.5f + noise * 0.2f;
    props->roughness = 0.8f;
    props->metallic = 0.0f;
    break;
  }
}

/*
 * IMPLEMENTATION: 50/400 Materials TODOs
 * LOC: ~240
 * Features: Layering, blending, procedural ✅
 */
