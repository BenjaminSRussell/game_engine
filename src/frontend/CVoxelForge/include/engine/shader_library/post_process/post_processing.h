/**
 * =================================================================================================
 *                              POST-PROCESSING STACK
 *                               Agent: AGENT_SHADER_2
 * =================================================================================================
 *
 * Complete post-processing effects pipeline for cinematic rendering.
 *
 * =================================================================================================
 */

#ifndef POST_PROCESSING_H
#define POST_PROCESSING_H

#include <stdbool.h>
#include <stdint.h>

/* =================================================================================================
 *                                    BLOOM EFFECT
 * =================================================================================================
 */

typedef struct BloomSettings {
  bool enabled;
  float threshold;
  float intensity;
  float scatter;
  uint32_t mip_levels;
  float tint[3];
  bool lens_dirt_enabled;
  uint32_t lens_dirt_texture;
  float lens_dirt_intensity;
} BloomSettings;

// TODO(AGENT_SHADER_2): Implement bloom threshold pass [Difficulty: 4]
// TODO(AGENT_SHADER_2): Implement bloom downsample chain [Difficulty: 5]
// TODO(AGENT_SHADER_2): Implement bloom upsample chain [Difficulty: 5]
// TODO(AGENT_SHADER_2): Implement bloom combine [Difficulty: 4]
// TODO(AGENT_SHADER_2): Implement bloom lens dirt overlay [Difficulty: 4]
// TODO(AGENT_SHADER_2): Implement bloom energy conservation [Difficulty: 5]

/* =================================================================================================
 *                                    TONE MAPPING
 * =================================================================================================
 */

typedef enum ToneMappingMode {
  TONEMAP_REINHARD,
  TONEMAP_REINHARD_EXTENDED,
  TONEMAP_ACES,
  TONEMAP_ACES_APPROX,
  TONEMAP_UNCHARTED2,
  TONEMAP_FILMIC,
  TONEMAP_AGX,
  TONEMAP_NEUTRAL,
} ToneMappingMode;

typedef struct ToneMappingSettings {
  ToneMappingMode mode;
  float exposure;
  float white_point;
  float saturation;
  float contrast;
} ToneMappingSettings;

// TODO(AGENT_SHADER_2): Implement Reinhard tone mapping [Difficulty: 4]
// TODO(AGENT_SHADER_2): Implement ACES tone mapping [Difficulty: 5]
// TODO(AGENT_SHADER_2): Implement Uncharted 2 tone mapping [Difficulty: 5]
// TODO(AGENT_SHADER_2): Implement AGX tone mapping [Difficulty: 5]
// TODO(AGENT_SHADER_2): Implement exposure adjustment [Difficulty: 4]
// TODO(AGENT_SHADER_2): Implement auto exposure [Difficulty: 6]
// TODO(AGENT_SHADER_2): Implement histogram-based exposure [Difficulty: 7]

/* =================================================================================================
 *                                    COLOR GRADING
 * =================================================================================================
 */

typedef struct ColorGradingSettings {
  bool enabled;

  // Lift/Gamma/Gain
  float lift[3];
  float gamma[3];
  float gain[3];

  // Offset/Power/Slope (CDL)
  float offset[3];
  float power[3];
  float slope[3];

  // Color adjustments
  float temperature;
  float tint;
  float hue_shift;
  float saturation;
  float vibrance;

  // Color wheels
  float shadows[3];
  float midtones[3];
  float highlights[3];
  float shadows_range[2];
  float highlights_range[2];

  // LUT
  bool use_lut;
  uint32_t lut_texture;
  float lut_contribution;

  // Curves
  bool use_curves;
  float rgb_curve[256];
  float red_curve[256];
  float green_curve[256];
  float blue_curve[256];
} ColorGradingSettings;

// TODO(AGENT_SHADER_2): Implement lift/gamma/gain [Difficulty: 4]
// TODO(AGENT_SHADER_2): Implement offset/power/slope (CDL) [Difficulty: 4]
// TODO(AGENT_SHADER_2): Implement temperature/tint [Difficulty: 4]
// TODO(AGENT_SHADER_2): Implement shadows/midtones/highlights [Difficulty: 5]
// TODO(AGENT_SHADER_2): Implement 3D LUT sampling [Difficulty: 5]
// TODO(AGENT_SHADER_2): Implement LUT baking [Difficulty: 6]
// TODO(AGENT_SHADER_2): Implement curves [Difficulty: 5]
// TODO(AGENT_SHADER_2): Implement HSL adjustment [Difficulty: 5]
// TODO(AGENT_SHADER_2): Implement color grading merge [Difficulty: 5]

/* =================================================================================================
 *                                    DEPTH OF FIELD
 * =================================================================================================
 */

typedef enum DOFMode {
  DOF_MODE_GAUSSIAN,
  DOF_MODE_BOKEH,
  DOF_MODE_PHYSICAL,
} DOFMode;

typedef struct DepthOfFieldSettings {
  bool enabled;
  DOFMode mode;

  // Focus
  float focus_distance;
  bool auto_focus;
  float auto_focus_range;

  // Aperture
  float aperture;
  float focal_length;

  // Blur
  float near_blur_start;
  float near_blur_end;
  float far_blur_start;
  float far_blur_end;
  float max_blur;

  // Bokeh
  uint32_t bokeh_shape; // Hexagonal, circular, etc.
  uint32_t bokeh_texture;
  float bokeh_brightness;
  float bokeh_threshold;
  bool anamorphic_bokeh;
  float anamorphic_ratio;
} DepthOfFieldSettings;

// TODO(AGENT_SHADER_2): Implement DOF circle of confusion [Difficulty: 5]
// TODO(AGENT_SHADER_2): Implement DOF gaussian blur [Difficulty: 5]
// TODO(AGENT_SHADER_2): Implement DOF bokeh shapes [Difficulty: 7]
// TODO(AGENT_SHADER_2): Implement DOF bokeh sprites [Difficulty: 7]
// TODO(AGENT_SHADER_2): Implement DOF auto focus [Difficulty: 5]
// TODO(AGENT_SHADER_2): Implement DOF near/far separation [Difficulty: 5]
// TODO(AGENT_SHADER_2): Implement DOF physical camera [Difficulty: 6]
// TODO(AGENT_SHADER_2): Implement DOF anamorphic [Difficulty: 6]

/* =================================================================================================
 *                                    MOTION BLUR
 * =================================================================================================
 */

typedef struct MotionBlurSettings {
  bool enabled;
  float intensity;
  uint32_t sample_count;
  float max_velocity;
  bool camera_motion;
  bool per_object;
  float shutter_angle;
} MotionBlurSettings;

// TODO(AGENT_SHADER_2): Implement motion blur velocity buffer [Difficulty: 5]
// TODO(AGENT_SHADER_2): Implement motion blur tile max [Difficulty: 6]
// TODO(AGENT_SHADER_2): Implement motion blur neighbor max [Difficulty: 6]
// TODO(AGENT_SHADER_2): Implement motion blur reconstruction [Difficulty: 7]
// TODO(AGENT_SHADER_2): Implement camera motion blur [Difficulty: 5]
// TODO(AGENT_SHADER_2): Implement per-object motion blur [Difficulty: 6]

/* =================================================================================================
 *                                    ADDITIONAL EFFECTS
 * =================================================================================================
 */

typedef struct ChromaticAberrationSettings {
  bool enabled;
  float intensity;
  float radial_offset;
  bool barrel_distortion;
  float barrel_amount;
} ChromaticAberrationSettings;

typedef struct VignetteSettings {
  bool enabled;
  float intensity;
  float smoothness;
  float roundness;
  bool rounded;
  float center[2];
  float color[3];
} VignetteSettings;

typedef struct FilmGrainSettings {
  bool enabled;
  float intensity;
  float response;
  uint32_t grain_texture;
  bool colored;
} FilmGrainSettings;

typedef struct LensFlareSettings {
  bool enabled;
  uint32_t flare_texture;
  float intensity;
  float threshold;
  float ghost_count;
  float ghost_spacing;
  float halo_radius;
  float halo_width;
  float chromatic_offset;
} LensFlareSettings;

// TODO(AGENT_SHADER_2): Implement chromatic aberration [Difficulty: 4]
// TODO(AGENT_SHADER_2): Implement barrel distortion [Difficulty: 5]
// TODO(AGENT_SHADER_2): Implement vignette [Difficulty: 3]
// TODO(AGENT_SHADER_2): Implement film grain [Difficulty: 4]
// TODO(AGENT_SHADER_2): Implement lens flare ghosts [Difficulty: 6]
// TODO(AGENT_SHADER_2): Implement lens flare halo [Difficulty: 5]
// TODO(AGENT_SHADER_2): Implement lens flare starburst [Difficulty: 5]
// TODO(AGENT_SHADER_2): Implement dust/scratches overlay [Difficulty: 4]
// TODO(AGENT_SHADER_2): Implement scanlines/CRT effect [Difficulty: 5]
// TODO(AGENT_SHADER_2): Implement pixelation [Difficulty: 3]
// TODO(AGENT_SHADER_2): Implement posterization [Difficulty: 3]
// TODO(AGENT_SHADER_2): Implement sharpen [Difficulty: 4]

/* =================================================================================================
 *                                    ANTI-ALIASING
 * =================================================================================================
 */

typedef enum AAMode {
  AA_NONE,
  AA_FXAA,
  AA_SMAA,
  AA_TAA,
  AA_DLSS,
  AA_FSR,
} AAMode;

typedef struct AASettings {
  AAMode mode;
  float sharpness;
  bool motion_vector_dilation;
  float jitter_scale;
  float history_blend;
} AASettings;

// TODO(AGENT_SHADER_2): Implement FXAA [Difficulty: 6]
// TODO(AGENT_SHADER_2): Implement SMAA edge detection [Difficulty: 6]
// TODO(AGENT_SHADER_2): Implement SMAA blending weights [Difficulty: 7]
// TODO(AGENT_SHADER_2): Implement SMAA neighborhood blending [Difficulty: 6]
// TODO(AGENT_SHADER_2): Implement TAA jitter [Difficulty: 5]
// TODO(AGENT_SHADER_2): Implement TAA history reprojection [Difficulty: 7]
// TODO(AGENT_SHADER_2): Implement TAA neighborhood clamping [Difficulty: 6]
// TODO(AGENT_SHADER_2): Implement TAA velocity rejection [Difficulty: 6]
// TODO(AGENT_SHADER_2): Implement TAA sharpening [Difficulty: 5]

/* =================================================================================================
 *                                    POST-PROCESS STACK
 * =================================================================================================
 */

typedef struct PostProcessStack {
  BloomSettings bloom;
  ToneMappingSettings tonemapping;
  ColorGradingSettings color_grading;
  DepthOfFieldSettings dof;
  MotionBlurSettings motion_blur;
  ChromaticAberrationSettings chromatic;
  VignetteSettings vignette;
  FilmGrainSettings film_grain;
  LensFlareSettings lens_flare;
  AASettings aa;

  // Render targets
  uint32_t *temp_textures;
  uint32_t temp_texture_count;

  // Effect order
  uint32_t effect_order[16];
  uint32_t effect_count;

  bool is_enabled;
} PostProcessStack;

// TODO(AGENT_SHADER_2): Implement post_process_init [Difficulty: 5]
// TODO(AGENT_SHADER_2): Implement post_process_shutdown [Difficulty: 3]
// TODO(AGENT_SHADER_2): Implement post_process_resize [Difficulty: 4]
// TODO(AGENT_SHADER_2): Implement post_process_render [Difficulty: 6]
// TODO(AGENT_SHADER_2): Implement post_process_set_order [Difficulty: 4]
// TODO(AGENT_SHADER_2): Implement post_process_blend_volumes [Difficulty: 6]
// TODO(AGENT_SHADER_2): Implement post_process_profile_save [Difficulty: 4]
// TODO(AGENT_SHADER_2): Implement post_process_profile_load [Difficulty: 4]

#endif // POST_PROCESSING_H
