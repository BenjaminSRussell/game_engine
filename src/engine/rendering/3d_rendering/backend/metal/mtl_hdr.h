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

/* ============================================================================
 * ADVANCED COLOR MANAGEMENT
 * ============================================================================ */

/**
 * Convert Display P3 to sRGB color space.
 * @param rgb Array of 3 floats [r, g, b] in Display P3. Modified in-place to sRGB.
 */
void metal_hdr_convert_p3_to_srgb(float* rgb);

/**
 * Convert sRGB to Display P3 color space.
 * @param rgb Array of 3 floats [r, g, b] in sRGB. Modified in-place to Display P3.
 */
void metal_hdr_convert_srgb_to_p3(float* rgb);

/**
 * Check if RGB value is within sRGB gamut.
 * @param rgb Array of 3 floats [r, g, b] in linear space.
 * @return true if within sRGB gamut, false if out of gamut.
 */
bool metal_hdr_is_in_srgb_gamut(const float* rgb);

/**
 * Clamp RGB to target gamut (simple gamut mapping).
 * @param rgb Array of 3 floats [r, g, b] in linear space. Modified in-place.
 * @param max_value Maximum value for each component (1.0 for SDR).
 */
void metal_hdr_clamp_to_gamut(float* rgb, float max_value);

/**
 * Calculate color difference (Delta E - simplified CIE76).
 * @param rgb1 First color in linear space.
 * @param rgb2 Second color in linear space.
 * @return Color difference value.
 */
float metal_hdr_calculate_color_difference(const float* rgb1, const float* rgb2);

/* ============================================================================
 * ADVANCED TONE MAPPING
 * ============================================================================ */

/**
 * Apply ACES filmic tone mapping.
 * @param rgb Array of 3 floats [r, g, b] in linear space. Modified in-place.
 */
void metal_hdr_tonemap_aces(float* rgb);

/**
 * Apply Uncharted 2 tone mapping.
 * @param rgb Array of 3 floats [r, g, b] in linear space. Modified in-place.
 * @param exposure Exposure adjustment value (1.0 = neutral).
 */
void metal_hdr_tonemap_uncharted2(float* rgb, float exposure);

/**
 * Apply custom tone curve.
 * @param rgb Array of 3 floats [r, g, b] in linear space. Modified in-place.
 * @param toe Toe (shadow) strength [0, 1].
 * @param shoulder Shoulder (highlight) strength [0, 1].
 */
void metal_hdr_tonemap_custom(float* rgb, float toe, float shoulder);

/**
 * Calculate auto-exposure value from scene luminance histogram.
 * @param histogram Array of luminance histogram bins.
 * @param bin_count Number of histogram bins.
 * @return Recommended exposure value.
 */
float metal_hdr_calculate_auto_exposure(const uint32_t* histogram, uint32_t bin_count);

/* ============================================================================
 * METAL SHADER GENERATION
 * ============================================================================ */

/**
 * Generate Metal shader code for tone mapping.
 * @param operator_type Type of tone mapping (0=Reinhard, 1=ACES, 2=Uncharted2).
 * @param out_shader_code Output buffer for shader code (caller must free).
 * @param max_size Maximum size of output buffer.
 * @return Length of generated shader code, or 0 on error.
 */
uint32_t metal_hdr_generate_tonemap_shader(uint32_t operator_type, char* out_shader_code, uint32_t max_size);

/**
 * Generate Metal shader code for color space conversion.
 * @param from_space Source color space (0=sRGB, 1=Linear, 2=P3).
 * @param to_space Target color space (0=sRGB, 1=Linear, 2=P3).
 * @param out_shader_code Output buffer for shader code (caller must free).
 * @param max_size Maximum size of output buffer.
 * @return Length of generated shader code, or 0 on error.
 */
uint32_t metal_hdr_generate_colorspace_shader(uint32_t from_space, uint32_t to_space, char* out_shader_code, uint32_t max_size);

#ifdef __cplusplus
}
#endif

#endif /* PLATFORM_MTL_HDR_H */
