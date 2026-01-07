/*
 * mtl_hdr.m
 * Metal HDR utilities implementation
 *
 * Part of the Platform subsystem
 * Advanced 3D Rendering Engine
 */

#include "backend/metal/mtl_hdr.h"
#import <CoreGraphics/CoreGraphics.h>
#import <Metal/Metal.h>
#include <include/math/math.h>
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
