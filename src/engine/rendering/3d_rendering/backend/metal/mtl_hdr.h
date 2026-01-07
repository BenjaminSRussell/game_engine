/*
 * mtl_hdr.h
 * Metal HDR color space conversion and tone mapping utilities
 *
 * Part of the Platform subsystem
 * Advanced 3D Rendering Engine
 */

#ifndef PLATFORM_MTL_HDR_H
#define PLATFORM_MTL_HDR_H

#include <stdint.h>
#include <stdbool.h>

#ifdef __OBJC__
#import <Metal/Metal.h>
#import <CoreGraphics/CoreGraphics.h>
#else
typedef void* CGColorSpaceRef;
typedef uint32_t MTLPixelFormat;
#endif

#ifdef __cplusplus
extern "C" {
#endif

/* ============================================================================
 * COLOR SPACE MANAGEMENT
 * ============================================================================ */

/**
 * Create an HDR-capable color space.
 * @param extended_range If true, creates Extended Linear Display P3. If false, creates standard Display P3.
 * @return CGColorSpaceRef (must be released by caller with CGColorSpaceRelease).
 */
CGColorSpaceRef metal_hdr_create_color_space(bool extended_range);

/**
 * Validate if a pixel format is suitable for HDR output.
 * @param format MTLPixelFormat to validate.
 * @return true if format supports HDR, false otherwise.
 */
bool metal_hdr_validate_format(MTLPixelFormat format);

/**
 * Recommend optimal pixel format based on capabilities.
 * @param hdr_capable Whether the display supports HDR.
 * @param prefer_performance If true, prefer performance over quality when applicable.
 * @return Recommended MTLPixelFormat.
 */
MTLPixelFormat metal_hdr_recommend_format(bool hdr_capable, bool prefer_performance);

/**
 * Get human-readable name for a pixel format (debug helper).
 * @param format MTLPixelFormat to query.
 * @return String name of format (do not free).
 */
const char* metal_hdr_get_format_name(MTLPixelFormat format);

/* ============================================================================
 * COLOR CONVERSION
 * ============================================================================ */

/**
 * Convert SRGB color values to linear space.
 * @param rgb Array of 3 floats [r, g, b] in range [0, 1]. Modified in-place.
 */
void metal_hdr_convert_srgb_to_linear(float* rgb);

/**
 * Convert linear color values to SRGB space.
 * @param rgb Array of 3 floats [r, g, b] in range [0, 1]. Modified in-place.
 */
void metal_hdr_convert_linear_to_srgb(float* rgb);

/**
 * Calculate reference white point value for HDR rendering.
 * @param edr_headroom EDR headroom from display (e.g., 2.0, 4.0).
 * @return Reference white level in linear space.
 */
float metal_hdr_calculate_whitepoint(float edr_headroom);

/* ============================================================================
 * TONE MAPPING (CPU-SIDE HELPERS)
 * ============================================================================ */

/**
 * Apply Reinhard tone mapping to an RGB value.
 * @param rgb Array of 3 floats [r, g, b] in linear space. Modified in-place.
 * @param max_white Maximum white luminance for tone mapping.
 */
void metal_hdr_tonemap_reinhard(float* rgb, float max_white);

/**
 * Apply simple linear tone mapping (clamp).
 * @param rgb Array of 3 floats [r, g, b] in linear space. Modified in-place.
 * @param max_value Maximum color value before clamping.
 */
void metal_hdr_tonemap_linear(float* rgb, float max_value);

/**
 * Calculate relative luminance from RGB (Rec. 709).
 * @param rgb Array of 3 floats [r, g, b] in linear space.
 * @return Luminance value.
 */
float metal_hdr_calculate_luminance(const float* rgb);

#ifdef __cplusplus
}
#endif

#endif /* PLATFORM_MTL_HDR_H */
