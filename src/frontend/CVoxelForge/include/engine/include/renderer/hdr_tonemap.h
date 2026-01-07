// include/render/hdr_tonemap.h
//
// Purpose: HDR (High Dynamic Range) rendering with tone mapping.
// Converts HDR values (0-infinity) to LDR (0-1) for display while preserving
// visual quality. Supports multiple tone mapping curves (ACES, Filmic,
// Reinhard, etc.).
//
#ifndef HDR_TONEMAP_H
#define HDR_TONEMAP_H

#include <common.h>
#include <math/vec4.h>

// Tone mapping algorithms
typedef enum {
  TONEMAP_LINEAR,            // No tone mapping (unsafe, may clip)
  TONEMAP_REINHARD,          // Simple Reinhard
  TONEMAP_REINHARD_EXTENDED, // Extended Reinhard with white point
  TONEMAP_ACES,              // Academy Color Encoding System (professional)
  TONEMAP_FILMIC,            // Filmic curve (cinematic)
  TONEMAP_UNREAL,            // Unreal Engine tone mapper
  TONEMAP_GRAN_TURISMO,      // Gran Turismo tone mapper
} TonemapAlgorithm;

// Color space for processing
typedef enum {
  COLORSPACE_LINEAR, // Linear color space
  COLORSPACE_SRGB,   // sRGB (gamma-encoded)
  COLORSPACE_DCI_P3, // DCI-P3 cinema color space
} ColorSpace;

// HDR tone mapper configuration
typedef struct {
  TonemapAlgorithm algorithm;
  ColorSpace color_space;

  // Exposure control
  f32 exposure;    // EV adjustment (-2 to +2)
  f32 white_point; // Reference white (for extended Reinhard)

  // Color grading
  f32 contrast;    // 0.5-2.0
  f32 saturation;  // 0.0-2.0
  f32 gamma;       // Gamma correction (typically 1.0-2.4)
  f32 temperature; // Color temperature adjustment
  f32 tint;        // Green/Magenta tint adjustment

  // Bloom parameters
  f32 bloom_strength;  // 0.0-1.0
  f32 bloom_threshold; // Luminance threshold for bloom (0.8-1.0)

  // Vignette
  f32 vignette_strength; // 0.0-1.0
  f32 vignette_radius;   // 0.5-2.0

  bool initialized;
} TonemapConfig;

// Color accumulation for HDR rendering
typedef struct {
  Vec4 hdr_color; // HDR value (R,G,B,A with values > 1.0)
  f32 luminance;  // Perceived brightness
  bool is_hdr;
} HDRColor;

// ==============================================================================
// HDR Tone Mapper Lifecycle
// ==============================================================================

// Initialize tone mapper with default configuration
void tonemap_init(TonemapConfig *config);

// Create tone mapper with specific algorithm
void tonemap_init_with_algorithm(TonemapConfig *config,
                                 TonemapAlgorithm algorithm);

// ==============================================================================
// Tone Mapping Operations
// ==============================================================================

// Apply tone mapping to HDR color
Vec4 tonemap_apply(const TonemapConfig *config, Vec4 hdr_color);

// Apply tone mapping to HDR color with bloom
Vec4 tonemap_apply_with_bloom(const TonemapConfig *config, Vec4 hdr_color,
                              Vec4 bloom_color);

// Apply complete color grading pipeline
Vec4 tonemap_apply_grading(const TonemapConfig *config, Vec4 hdr_color);

// ==============================================================================
// Tone Mapping Algorithms
// ==============================================================================

// Linear tone mapping (no processing, may clip)
Vec4 tonemap_linear(Vec4 hdr_color, f32 exposure);

// Reinhard tone mapping (simple)
Vec4 tonemap_reinhard(Vec4 hdr_color, f32 exposure);

// Extended Reinhard with white point
Vec4 tonemap_reinhard_extended(Vec4 hdr_color, f32 exposure, f32 white_point);

// ACES tone mapping (Academy Color Encoding System)
Vec4 tonemap_aces(Vec4 hdr_color);

// Filmic tone mapping (cinematic)
Vec4 tonemap_filmic(Vec4 hdr_color, f32 exposure);

// Unreal Engine tone mapping
Vec4 tonemap_unreal(Vec4 hdr_color);

// Gran Turismo tone mapping
Vec4 tonemap_gran_turismo(Vec4 hdr_color);

// ==============================================================================
// Color Space Conversions
// ==============================================================================

// Convert linear RGB to sRGB
Vec4 colorspace_linear_to_srgb(Vec4 color);

// Convert sRGB to linear RGB
Vec4 colorspace_srgb_to_linear(Vec4 color);

// Convert linear RGB to DCI-P3
Vec4 colorspace_linear_to_dci_p3(Vec4 color);

// Convert DCI-P3 to linear RGB
Vec4 colorspace_dci_p3_to_linear(Vec4 color);

// ==============================================================================
// Luminance and Color Analysis
// ==============================================================================

// Calculate perceived luminance (Y in YUV)
f32 luminance_calculate(Vec4 color);

// Calculate luminance using Rec. 709 standard
f32 luminance_rec709(Vec4 color);

// Calculate luminance using Rec. 2020 standard
f32 luminance_rec2020(Vec4 color);

// Calculate relative luminance for contrast
f32 luminance_relative(Vec4 color);

// ==============================================================================
// Color Grading
// ==============================================================================

// Apply exposure adjustment
Vec4 grading_exposure(Vec4 color, f32 exposure_ev);

// Apply contrast adjustment
Vec4 grading_contrast(Vec4 color, f32 contrast);

// Apply saturation adjustment
Vec4 grading_saturation(Vec4 color, f32 saturation);

// Apply gamma correction
Vec4 grading_gamma(Vec4 color, f32 gamma);

// Apply color temperature (warm/cool adjustment)
Vec4 grading_temperature(Vec4 color, f32 temperature); // -100 to +100

// Apply tint (green/magenta adjustment)
Vec4 grading_tint(Vec4 color, f32 tint); // -100 to +100

// Apply lift (shadow adjustment)
Vec4 grading_lift(Vec4 color, f32 lift);

// ==============================================================================
// Bloom and Light Effects
// ==============================================================================

// Extract bloom contribution from HDR color
Vec4 extract_bloom(Vec4 hdr_color, f32 threshold);

// Apply bloom to color
Vec4 apply_bloom(Vec4 base_color, Vec4 bloom_color, f32 strength);

// Apply lens flare effect
Vec4 apply_lens_flare(Vec4 color, Vec4 light_position, f32 intensity);

// ==============================================================================
// Vignette and Edge Effects
// ==============================================================================

// Apply vignette (darkening at edges)
Vec4 apply_vignette(Vec4 color, f32 strength, f32 radius);

// Calculate vignette factor for coordinates
f32 vignette_factor(Vec4 uv, f32 radius);

// ==============================================================================
// Histogram and Analysis
// ==============================================================================

// Histogram bin for exposure analysis
typedef struct {
  u32 luminance_histogram[256]; // Luminance distribution
  f32 average_luminance;
  f32 max_luminance;
  f32 min_luminance;
} LuminanceHistogram;

// Build luminance histogram from image data
void histogram_build(Vec4 *pixels, u32 pixel_count,
                     LuminanceHistogram *out_histogram);

// Calculate exposure from histogram
f32 histogram_calculate_exposure(const LuminanceHistogram *histogram);

// ==============================================================================
// Configuration Management
// ==============================================================================

// Set tone mapping algorithm
void tonemap_set_algorithm(TonemapConfig *config, TonemapAlgorithm algorithm);

// Set exposure (EV)
void tonemap_set_exposure(TonemapConfig *config, f32 exposure_ev);

// Set contrast
void tonemap_set_contrast(TonemapConfig *config, f32 contrast);

// Set saturation
void tonemap_set_saturation(TonemapConfig *config, f32 saturation);

// Set gamma
void tonemap_set_gamma(TonemapConfig *config, f32 gamma);

// Set bloom parameters
void tonemap_set_bloom(TonemapConfig *config, f32 strength, f32 threshold);

// Set vignette parameters
void tonemap_set_vignette(TonemapConfig *config, f32 strength, f32 radius);

// Reset to default configuration
void tonemap_reset_to_default(TonemapConfig *config);

// ==============================================================================
// Presets
// ==============================================================================

// Create cinema preset (high contrast, warm tones)
TonemapConfig tonemap_preset_cinema(void);

// Create neutral preset (balanced colors)
TonemapConfig tonemap_preset_neutral(void);

// Create vivid preset (high saturation, vibrant)
TonemapConfig tonemap_preset_vivid(void);

// Create bright preset (high exposure)
TonemapConfig tonemap_preset_bright(void);

// ==============================================================================
// Serialization
// ==============================================================================

// Export tone mapping configuration
bool tonemap_serialize(const TonemapConfig *config, u8 *buffer, u32 buffer_size,
                       u32 *out_size);

// Import tone mapping configuration
bool tonemap_deserialize(u8 *buffer, u32 size, TonemapConfig *out_config);

#endif // HDR_TONEMAP_H
