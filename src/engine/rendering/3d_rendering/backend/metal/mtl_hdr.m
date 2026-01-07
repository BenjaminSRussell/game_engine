/*
 * mtl_hdr.m
 * Metal HDR utilities implementation
 *
 * Part of the Platform subsystem
 * Advanced 3D Rendering Engine
 */

#include "mtl_hdr.h"
#import <CoreGraphics/CoreGraphics.h>
#import <Metal/Metal.h>
#include <math.h>
#include <stdio.h>

/* ============================================================================
 * COLOR SPACE MANAGEMENT
 * ============================================================================
 */

CGColorSpaceRef metal_hdr_create_color_space(bool extended_range) {
  if (extended_range) {
    // Extended Linear Display P3 for HDR content
    if (@available(macOS 10.11, *)) {
      CGColorSpaceRef space =
          CGColorSpaceCreateWithName(kCGColorSpaceExtendedLinearDisplayP3);
      if (space)
        return space;
    }
    // Fallback to standard linear Display P3
    return CGColorSpaceCreateWithName(kCGColorSpaceLinearDisplayP3);
  } else {
    // Standard Display P3 for wide color gamut
    return CGColorSpaceCreateWithName(kCGColorSpaceDisplayP3);
  }
}

bool metal_hdr_validate_format(MTLPixelFormat format) {
  // HDR-suitable formats
  switch (format) {
  case MTLPixelFormatRGBA16Float:
  case MTLPixelFormatRGBA32Float:
  case MTLPixelFormatRGB10A2Unorm:
  case MTLPixelFormatBGR10A2Unorm:
    return true;
  default:
    return false;
  }
}

MTLPixelFormat metal_hdr_recommend_format(bool hdr_capable,
                                          bool prefer_performance) {
  if (!hdr_capable) {
    // Standard SDR format
    return MTLPixelFormatBGRA8Unorm;
  }

  if (prefer_performance) {
    // RGB10A2 is more efficient than Float16 on Apple Silicon
    return MTLPixelFormatRGB10A2Unorm;
  } else {
    // Float16 offers best quality for HDR
    return MTLPixelFormatRGBA16Float;
  }
}

const char *metal_hdr_get_format_name(MTLPixelFormat format) {
  switch (format) {
  case MTLPixelFormatBGRA8Unorm:
    return "BGRA8Unorm";
  case MTLPixelFormatRGBA8Unorm:
    return "RGBA8Unorm";
  case MTLPixelFormatRGBA16Float:
    return "RGBA16Float";
  case MTLPixelFormatRGBA32Float:
    return "RGBA32Float";
  case MTLPixelFormatRGB10A2Unorm:
    return "RGB10A2Unorm";
  case MTLPixelFormatBGR10A2Unorm:
    return "BGR10A2Unorm";
  default:
    return "Unknown";
  }
}

/* ============================================================================
 * COLOR CONVERSION
 * ============================================================================
 */

// SRGB gamma correction constants
#define SRGB_ALPHA 0.055f
#define SRGB_THRESHOLD 0.04045f
#define SRGB_LINEAR_THRESHOLD 0.0031308f
#define SRGB_GAMMA 2.4f

static inline float srgb_to_linear_component(float c) {
  if (c <= SRGB_THRESHOLD) {
    return c / 12.92f;
  } else {
    return powf((c + SRGB_ALPHA) / (1.0f + SRGB_ALPHA), SRGB_GAMMA);
  }
}

static inline float linear_to_srgb_component(float c) {
  if (c <= SRGB_LINEAR_THRESHOLD) {
    return c * 12.92f;
  } else {
    return (1.0f + SRGB_ALPHA) * powf(c, 1.0f / SRGB_GAMMA) - SRGB_ALPHA;
  }
}

void metal_hdr_convert_srgb_to_linear(float *rgb) {
  if (!rgb)
    return;

  rgb[0] = srgb_to_linear_component(rgb[0]);
  rgb[1] = srgb_to_linear_component(rgb[1]);
  rgb[2] = srgb_to_linear_component(rgb[2]);
}

void metal_hdr_convert_linear_to_srgb(float *rgb) {
  if (!rgb)
    return;

  rgb[0] = linear_to_srgb_component(rgb[0]);
  rgb[1] = linear_to_srgb_component(rgb[1]);
  rgb[2] = linear_to_srgb_component(rgb[2]);
}

float metal_hdr_calculate_whitepoint(float edr_headroom) {
  // Reference white in HDR is typically 80 nits (SDR)
  // EDR headroom extends this (e.g., 2.0 = 160 nits, 4.0 = 320 nits)
  // Normalize to [0, 1] range where 1.0 = SDR white
  return 1.0f / edr_headroom;
}

/* ============================================================================
 * TONE MAPPING
 * ============================================================================
 */

float metal_hdr_calculate_luminance(const float *rgb) {
  if (!rgb)
    return 0.0f;

  // Rec. 709 luminance weights
  return 0.2126f * rgb[0] + 0.7152f * rgb[1] + 0.0722f * rgb[2];
}

void metal_hdr_tonemap_reinhard(float *rgb, float max_white) {
  if (!rgb)
    return;

  float luma = metal_hdr_calculate_luminance(rgb);
  if (luma <= 0.0f)
    return;

  // Reinhard tone mapping
  float numerator = luma * (1.0f + luma / (max_white * max_white));
  float new_luma = numerator / (1.0f + luma);

  // Apply to RGB
  float scale = new_luma / luma;
  rgb[0] *= scale;
  rgb[1] *= scale;
  rgb[2] *= scale;
}

void metal_hdr_tonemap_linear(float *rgb, float max_value) {
  if (!rgb)
    return;

  // Simple clamp to max value
  if (rgb[0] > max_value)
    rgb[0] = max_value;
  if (rgb[1] > max_value)
    rgb[1] = max_value;
  if (rgb[2] > max_value)
    rgb[2] = max_value;
}

/* ============================================================================
 * ADVANCED COLOR MANAGEMENT
 * ============================================================================
 */

// Simplified P3 to sRGB conversion matrix (assuming both in linear space)
// This is an approximation - full conversion requires XYZ intermediate
void metal_hdr_convert_p3_to_srgb(float *rgb) {
  if (!rgb)
    return;

  // P3 to sRGB matrix (simplified)
  float r = rgb[0] * 1.2249f - rgb[1] * 0.2247f + rgb[2] * 0.0f;
  float g = rgb[0] * -0.0420f + rgb[1] * 1.0419f + rgb[2] * 0.0002f;
  float b = rgb[0] * -0.0196f - rgb[1] * 0.0786f + rgb[2] * 1.0979f;

  rgb[0] = r;
  rgb[1] = g;
  rgb[2] = b;
}

void metal_hdr_convert_srgb_to_p3(float *rgb) {
  if (!rgb)
    return;

  // sRGB to P3 matrix (inverse of above, simplified)
  float r = rgb[0] * 0.8225f + rgb[1] * 0.1774f + rgb[2] * 0.0f;
  float g = rgb[0] * 0.0332f + rgb[1] * 0.9669f + rgb[2] * 0.0f;
  float b = rgb[0] * 0.0171f + rgb[1] * 0.0724f + rgb[2] * 0.9108f;

  rgb[0] = r;
  rgb[1] = g;
  rgb[2] = b;
}

bool metal_hdr_is_in_srgb_gamut(const float *rgb) {
  if (!rgb)
    return false;

  // Simple check: all components in [0, 1]
  return (rgb[0] >= 0.0f && rgb[0] <= 1.0f && rgb[1] >= 0.0f &&
          rgb[1] <= 1.0f && rgb[2] >= 0.0f && rgb[2] <= 1.0f);
}

void metal_hdr_clamp_to_gamut(float *rgb, float max_value) {
  if (!rgb)
    return;

  for (int i = 0; i < 3; i++) {
    if (rgb[i] < 0.0f)
      rgb[i] = 0.0f;
    if (rgb[i] > max_value)
      rgb[i] = max_value;
  }
}

float metal_hdr_calculate_color_difference(const float *rgb1,
                                           const float *rgb2) {
  if (!rgb1 || !rgb2)
    return 0.0f;

  // Simple Euclidean distance in RGB space (Delta E approximation)
  float dr = rgb1[0] - rgb2[0];
  float dg = rgb1[1] - rgb2[1];
  float db = rgb1[2] - rgb2[2];

  return sqrtf(dr * dr + dg * dg + db * db);
}

/* ============================================================================
 * ADVANCED TONE MAPPING
 * ============================================================================
 */

void metal_hdr_tonemap_aces(float *rgb) {
  if (!rgb)
    return;

  // ACES Filmic Tone Mapping (approximation)
  // https://knarkowicz.wordpress.com/2016/01/06/aces-filmic-tone-mapping-curve/

  const float a = 2.51f;
  const float b = 0.03f;
  const float c = 2.43f;
  const float d = 0.59f;
  const float e = 0.14f;

  for (int i = 0; i < 3; i++) {
    float x = rgb[i];
    rgb[i] = (x * (a * x + b)) / (x * (c * x + d) + e);
    // Clamp to [0, 1]
    if (rgb[i] < 0.0f)
      rgb[i] = 0.0f;
    if (rgb[i] > 1.0f)
      rgb[i] = 1.0f;
  }
}

static float uncharted2_tonemap_partial(float x) {
  const float A = 0.15f;
  const float B = 0.50f;
  const float C = 0.10f;
  const float D = 0.20f;
  const float E = 0.02f;
  const float F = 0.30f;
  return ((x * (A * x + C * B) + D * E) / (x * (A * x + B) + D * F)) - E / F;
}

void metal_hdr_tonemap_uncharted2(float *rgb, float exposure) {
  if (!rgb)
    return;

  const float W = 11.2f; // White point

  for (int i = 0; i < 3; i++) {
    float curr = uncharted2_tonemap_partial(rgb[i] * exposure);
    float white_scale = 1.0f / uncharted2_tonemap_partial(W);
    rgb[i] = curr * white_scale;
  }
}

void metal_hdr_tonemap_custom(float *rgb, float toe, float shoulder) {
  if (!rgb)
    return;

  // Custom S-curve tone mapping
  for (int i = 0; i < 3; i++) {
    float x = rgb[i];

    // Apply toe (shadow) compression
    if (x < 0.5f) {
      x = x * (1.0f - toe * (1.0f - x * 2.0f));
    }
    // Apply shoulder (highlight) compression
    else {
      float t = (x - 0.5f) * 2.0f;
      x = 0.5f + t * (1.0f - shoulder * t) / 2.0f;
    }

    rgb[i] = x;
  }
}

float metal_hdr_calculate_auto_exposure(const uint32_t *histogram,
                                        uint32_t bin_count) {
  if (!histogram || bin_count == 0)
    return 1.0f;

  // Calculate average luminance from histogram
  uint64_t total_pixels = 0;
  uint64_t weighted_sum = 0;

  for (uint32_t i = 0; i < bin_count; i++) {
    total_pixels += histogram[i];
    weighted_sum += histogram[i] * i;
  }

  if (total_pixels == 0)
    return 1.0f;

  float avg_bin = (float)weighted_sum / (float)total_pixels;
  float avg_luminance = avg_bin / (float)bin_count;

  // Target middle gray at 0.18 (photographic standard)
  float target = 0.18f;
  float exposure = target / (avg_luminance + 0.001f); // Avoid division by zero

  // Clamp to reasonable range
  if (exposure < 0.1f)
    exposure = 0.1f;
  if (exposure > 10.0f)
    exposure = 10.0f;

  return exposure;
}

/* ============================================================================
 * METAL SHADER GENERATION
 * ============================================================================
 */

uint32_t metal_hdr_generate_tonemap_shader(uint32_t operator_type,
                                           char *out_shader_code,
                                           uint32_t max_size) {
  if (!out_shader_code || max_size == 0)
    return 0;

  const char *shader_template_reinhard =
      "kernel void tonemap_reinhard(texture2d<float, access::read> inTexture "
      "[[texture(0)]],\n"
      "                             texture2d<float, access::write> outTexture "
      "[[texture(1)]],\n"
      "                             uint2 gid [[thread_position_in_grid]]) {\n"
      "    float4 color = inTexture.read(gid);\n"
      "    float luma = dot(color.rgb, float3(0.2126, 0.7152, 0.0722));\n"
      "    float scale = luma / (1.0 + luma);\n"
      "    color.rgb *= (scale / (luma + 0.001));\n"
      "    outTexture.write(color, gid);\n"
      "}\n";

  const char *shader_template_aces =
      "kernel void tonemap_aces(texture2d<float, access::read> inTexture "
      "[[texture(0)]],\n"
      "                         texture2d<float, access::write> outTexture "
      "[[texture(1)]],\n"
      "                         uint2 gid [[thread_position_in_grid]]) {\n"
      "    float4 color = inTexture.read(gid);\n"
      "    float3 x = color.rgb;\n"
      "    float3 result = (x * (2.51 * x + 0.03)) / (x * (2.43 * x + 0.59) + "
      "0.14);\n"
      "    outTexture.write(float4(clamp(result, 0.0, 1.0), color.a), gid);\n"
      "}\n";

  const char *shader_template =
      (operator_type == 1) ? shader_template_aces : shader_template_reinhard;

  uint32_t len = (uint32_t)strlen(shader_template);
  if (len >= max_size)
    return 0;

  strcpy(out_shader_code, shader_template);
  return len;
}

uint32_t metal_hdr_generate_colorspace_shader(uint32_t from_space,
                                              uint32_t to_space,
                                              char *out_shader_code,
                                              uint32_t max_size) {
  if (!out_shader_code || max_size == 0)
    return 0;

  const char *shader_template =
      "kernel void colorspace_convert(texture2d<float, access::read> inTexture "
      "[[texture(0)]],\n"
      "                               texture2d<float, access::write> "
      "outTexture [[texture(1)]],\n"
      "                               uint2 gid [[thread_position_in_grid]]) "
      "{\n"
      "    float4 color = inTexture.read(gid);\n"
      "    // Color space conversion would go here\n"
      "    outTexture.write(color, gid);\n"
      "}\n";

  uint32_t len = (uint32_t)strlen(shader_template);
  if (len >= max_size)
    return 0;

  strcpy(out_shader_code, shader_template);
  return len;
}
