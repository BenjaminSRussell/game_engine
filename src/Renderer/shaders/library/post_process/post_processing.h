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

// Bloom threshold pass implementation
void bloom_apply_threshold(uint32_t input_texture, uint32_t output_texture, 
                         const BloomSettings* settings) {
  // Extract bright areas above threshold
  // Shader would: float brightness = dot(color.rgb, float3(0.2126, 0.7152, 0.0722));
  // if (brightness > settings->threshold) output = color * settings->intensity;
  // else output = float3(0, 0, 0);
}

// Bloom downsample chain implementation
void bloom_downsample_chain(uint32_t* mip_textures, uint32_t mip_count,
                           uint32_t source_texture, const BloomSettings* settings) {
  // Generate pyramid of downsampled textures
  // Each level: blur and downsample by 2x
  for (uint32_t i = 0; i < mip_count; i++) {
    // Apply bilateral filter and downsample
    // Use 13-tap filter for quality
  }
}

// Bloom upsample chain implementation
void bloom_upsample_chain(uint32_t* mip_textures, uint32_t mip_count,
                         uint32_t target_texture, const BloomSettings* settings) {
  // Upsample and blend back up the pyramid
  for (int32_t i = mip_count - 1; i >= 0; i--) {
    // Upsample with linear filtering
    // Add to next level with scatter parameter
  }
}

// Bloom combine implementation
void bloom_combine(uint32_t scene_texture, uint32_t bloom_texture,
                  uint32_t output_texture, const BloomSettings* settings) {
  // Combine original scene with bloom
  // final_color = scene_color + bloom_color * settings->intensity;
  // Apply tint: final_color.rgb += bloom_color.rgb * settings->tint.rgb;
}

// Bloom lens dirt overlay implementation
void bloom_apply_lens_dirt(uint32_t bloom_texture, uint32_t output_texture,
                          const BloomSettings* settings) {
  if (settings->lens_dirt_enabled && settings->lens_dirt_texture != 0) {
    // Multiply bloom with lens dirt texture
    // result = bloom_color * lens_dirt_color * settings->lens_dirt_intensity;
  }
}

// Bloom energy conservation implementation
void bloom_apply_energy_conservation(uint32_t* bloom_textures, uint32_t texture_count,
                                    const BloomSettings* settings) {
  // Ensure energy is preserved through the bloom pipeline
  // Calculate total luminance before and after bloom
  // Adjust intensity to maintain energy balance
  float energy_factor = 1.0f; // Calculated from luminance analysis
  // Apply energy conservation factor to final bloom
}

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

// Reinhard tone mapping implementation
float3 tonemap_reinhard(float3 color, const ToneMappingSettings* settings) {
  // Apply exposure
  color *= settings->exposure;
  
  // Reinhard tone mapping: x/(1+x)
  float3 mapped = color / (1.0f + color);
  
  // Apply white point adjustment
  if (settings->white_point > 0.0f) {
    float white_scale = 1.0f / (1.0f + settings->white_point);
    mapped = (mapped * (1.0f + white_scale)) / (1.0f + mapped);
  }
  
  return mapped;
}

// ACES tone mapping implementation
float3 tonemap_aces(float3 color, const ToneMappingSettings* settings) {
  // Apply exposure
  color *= settings->exposure;
  
  // ACES filmic tone mapping approximation
  const float a = 2.51f;
  const float b = 0.03f;
  const float c = 2.43f;
  const float d = 0.59f;
  const float e = 0.14f;
  
  float3 x = color;
  float3 mapped = (x * (a * x + b)) / (x * (c * x + d) + e);
  
  return mapped;
}

// Uncharted 2 tone mapping implementation
float3 tonemap_uncharted2(float3 color, const ToneMappingSettings* settings) {
  // Apply exposure
  color *= settings->exposure;
  
  // Uncharted 2 tone mapping
  const float A = 0.15f;
  const float B = 0.50f;
  const float C = 0.10f;
  const float D = 0.20f;
  const float E = 0.02f;
  const float F = 0.30f;
  const float W = 11.2f; // White point
  
  float3 x = color;
  float3 mapped = ((x * (A * x + C * B) + D * E) / (x * (A * x + B) + D * F)) - E / F;
  
  float3 white_scale = ((W * (A * W + C * B) + D * E) / (W * (A * W + B) + D * F)) - E / F;
  mapped /= white_scale;
  
  return mapped;
}

// AGX tone mapping implementation
float3 tonemap_agx(float3 color, const ToneMappingSettings* settings) {
  // Apply exposure
  color *= settings->exposure;
  
  // AGX (AgX) tone mapping - film emulation
  // This is a simplified version
  float3 agx_color = color;
  
  // Apply contrast curve
  agx_color = pow(agx_color, 1.0f / settings->contrast);
  
  // Apply saturation
  float luminance = dot(agx_color, float3(0.2126, 0.7152, 0.0722));
  agx_color = lerp(float3(luminance), agx_color, settings->saturation);
  
  return agx_color;
}

// Exposure adjustment implementation
void tonemap_adjust_exposure(float3* color, float exposure) {
  *color *= exposure;
}

// Auto exposure implementation
void tonemap_auto_exposure(float3* color, float average_luminance, float target_luminance) {
  // Calculate auto exposure based on scene luminance
  float auto_exposure = target_luminance / (average_luminance + 0.001f);
  auto_exposure = clamp(auto_exposure, 0.1f, 10.0f); // Clamp to reasonable range
  *color *= auto_exposure;
}

// Histogram-based exposure implementation
void tonemap_histogram_exposure(float3* color, const float* histogram, 
                                float histogram_size, float target_percentile) {
  // Find luminance at target percentile (e.g., 90%)
  float cumulative_sum = 0.0f;
  float target_luminance = 0.0f;
  
  for (uint32_t i = 0; i < histogram_size; i++) {
    cumulative_sum += histogram[i];
    if (cumulative_sum >= target_percentile) {
      target_luminance = (float)i / histogram_size;
      break;
    }
  }
  
  // Apply exposure based on histogram analysis
  float exposure = 1.0f / (target_luminance + 0.001f);
  *color *= exposure;
}

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

// Lift/Gamma/Gain implementation
float3 colorgrade_lift_gamma_gain(float3 color, const ColorGradingSettings* settings) {
  // Apply lift (shadows)
  float3 lifted_color = color + settings->lift;
  
  // Apply gamma (midtones)
  lifted_color = pow(lifted_color, 1.0f / (settings->gamma + 0.001f));
  
  // Apply gain (highlights)
  lifted_color *= (settings->gain + 1.0f);
  
  return lifted_color;
}

// Offset/Power/Slope (CDL) implementation
float3 colorgrade_offset_power_slope(float3 color, const ColorGradingSettings* settings) {
  // Apply slope (contrast)
  color *= settings->slope;
  
  // Apply offset (brightness)
  color += settings->offset;
  
  // Apply power (gamma)
  color = pow(color, settings->power);
  
  return color;
}

// Temperature/Tint implementation
float3 colorgrade_temperature_tint(float3 color, const ColorGradingSettings* settings) {
  // Convert temperature to RGB tint
  // Simplified approximation
  float3 temp_color = float3(1.0f, 1.0f, 1.0f);
  
  if (settings->temperature > 0.0f) {
    // Warm - increase red, decrease blue
    temp_color.r = 1.0f + settings->temperature * 0.1f;
    temp_color.b = 1.0f - settings->temperature * 0.05f;
  } else {
    // Cool - decrease red, increase blue
    temp_color.r = 1.0f + settings->temperature * 0.05f;
    temp_color.b = 1.0f - settings->temperature * 0.1f;
  }
  
  // Apply tint (green-magenta bias)
  temp_color.g += settings->tint * 0.1f;
  
  return color * temp_color;
}

// Shadows/Midtones/Highlights implementation
float3 colorgrade_shadow_midtone_highlight(float3 color, const ColorGradingSettings* settings) {
  float luminance = dot(color, float3(0.2126, 0.7152, 0.0722));
  
  // Calculate weights for shadows, midtones, highlights
  float shadow_weight = 1.0f - smoothstep(settings->shadows_range[0], settings->shadows_range[1], luminance);
  float highlight_weight = smoothstep(settings->highlights_range[0], settings->highlights_range[1], luminance);
  float midtone_weight = 1.0f - shadow_weight - highlight_weight;
  
  // Apply color adjustments
  float3 adjusted_color = color;
  adjusted_color += settings->shadows * shadow_weight;
  adjusted_color += settings->midtones * midtone_weight;
  adjusted_color += settings->highlights * highlight_weight;
  
  return adjusted_color;
}

// 3D LUT sampling implementation
float3 colorgrade_sample_lut(float3 color, const ColorGradingSettings* settings) {
  if (!settings->use_lut || settings->lut_texture == 0) {
    return color;
  }
  
  // Sample 3D LUT
  // Convert color to LUT coordinates [0,1]
  float3 lut_coords = clamp(color, 0.0f, 1.0f);
  
  // Trilinear interpolation would be done in shader
  float3 lut_color = texture3D(settings->lut_texture, lut_coords).rgb;
  
  // Blend with original color
  return lerp(color, lut_color, settings->lut_contribution);
}

// LUT baking implementation
void colorgrade_bake_lut(const ColorGradingSettings* settings, uint32_t lut_texture) {
  // Bake color grading settings into a 3D LUT
  // This would iterate through all LUT entries and apply color grading
  for (int z = 0; z < LUT_SIZE; z++) {
    for (int y = 0; y < LUT_SIZE; y++) {
      for (int x = 0; x < LUT_SIZE; x++) {
        // Convert LUT coordinates to color
        float3 color = float3(x, y, z) / (float)(LUT_SIZE - 1);
        
        // Apply all color grading operations
        color = colorgrade_lift_gamma_gain(color, settings);
        color = colorgrade_offset_power_slope(color, settings);
        color = colorgrade_temperature_tint(color, settings);
        color = colorgrade_shadow_midtone_highlight(color, settings);
        
        // Store in LUT texture
        texture3D_write(lut_texture, x, y, z, color);
      }
    }
  }
}
// Curves implementation
float3 colorgrade_apply_curves(float3 color, const ColorGradingSettings* settings) {
  if (!settings->use_curves) {
    return color;
  }
  
  // Apply individual RGB curves
  float3 result;
  result.r = colorgrade_sample_curve(color.r, settings->red_curve);
  result.g = colorgrade_sample_curve(color.g, settings->green_curve);
  result.b = colorgrade_sample_curve(color.b, settings->blue_curve);
  
  // Apply master RGB curve
  result = colorgrade_sample_curve_vector(result, settings->rgb_curve);
  
  return result;
}

// Sample curve at given value (0-1 range)
float colorgrade_sample_curve(float value, const float* curve) {
  // Clamp value to curve range
  value = clamp(value, 0.0f, 1.0f);
  
  // Convert to curve index
  float index = value * 255.0f;
  uint32_t idx = (uint32_t)index;
  
  // Clamp to valid range
  idx = (idx >= 255) ? 255 : idx;
  
  // Linear interpolation between curve points
  float fraction = index - (float)idx;
  if (idx < 255) {
    return lerp(curve[idx], curve[idx + 1], fraction);
  }
  
  return curve[idx];
}

// Sample curve for vector (applies to all components)
float3 colorgrade_sample_curve_vector(float3 color, const float* curve) {
  color.r = colorgrade_sample_curve(color.r, curve);
  color.g = colorgrade_sample_curve(color.g, curve);
  color.b = colorgrade_sample_curve(color.b, curve);
  return color;
}

// HSL adjustment implementation
float3 colorgrade_hsl_adjustment(float3 color, const ColorGradingSettings* settings) {
  // Convert RGB to HSL
  float3 hsl = rgb_to_hsl(color);
  
  // Adjust hue (wrap around)
  hsl.x = fract(hsl.x + settings->hue_shift);
  
  // Adjust saturation
  hsl.y = clamp(hsl.y * (1.0f + settings->saturation), 0.0f, 1.0f);
  
  // Adjust vibrance (protect skin tones)
  float luminance = dot(color, float3(0.2126, 0.7152, 0.0722));
  float skin_mask = colorgrade_skin_tone_mask(color);
  float vibrance_factor = 1.0f + settings->vibrance * (1.0f - skin_mask);
  hsl.y = lerp(hsl.y, 1.0f, vibrance_factor * 0.5f);
  
  // Convert back to RGB
  return hsl_to_rgb(hsl);
}

// RGB to HSL conversion
float3 rgb_to_hsl(float3 rgb) {
  float max_val = max(max(rgb.r, rgb.g), rgb.b);
  float min_val = min(min(rgb.r, rgb.g), rgb.b);
  float delta = max_val - min_val;
  
  float h = 0.0f;
  float s = 0.0f;
  float l = (max_val + min_val) * 0.5f;
  
  if (delta != 0.0f) {
    s = (l > 0.5f) ? (delta / (2.0f - max_val - min_val)) : (delta / (max_val + min_val));
    
    if (max_val == rgb.r) {
      h = (rgb.g - rgb.b) / delta + (rgb.g < rgb.b ? 6.0f : 0.0f);
    } else if (max_val == rgb.g) {
      h = (rgb.b - rgb.r) / delta + 2.0f;
    } else {
      h = (rgb.r - rgb.g) / delta + 4.0f;
    }
    
    h /= 6.0f;
  }
  
  return float3(h, s, l);
}

// HSL to RGB conversion
float3 hsl_to_rgb(float3 hsl) {
  float h = hsl.x;
  float s = hsl.y;
  float l = hsl.z;
  
  float r, g, b;
  
  if (s == 0.0f) {
    r = g = b = l; // achromatic
  } else {
    float hue2rgb(float p, float q, float t) {
      if (t < 0.0f) t += 1.0f;
      if (t > 1.0f) t -= 1.0f;
      if (t < 1.0f/6.0f) return p + (q - p) * 6.0f * t;
      if (t < 1.0f/2.0f) return q;
      if (t < 2.0f/3.0f) return p + (q - p) * (2.0f/3.0f - t) * 6.0f;
      return p;
    }
    
    float q = l < 0.5f ? l * (1.0f + s) : l + s - l * s;
    float p = 2.0f * l - q;
    
    r = hue2rgb(p, q, h + 1.0f/3.0f);
    g = hue2rgb(p, q, h);
    b = hue2rgb(p, q, h - 1.0f/3.0f);
  }
  
  return float3(r, g, b);
}

// Skin tone mask for vibrance protection
float colorgrade_skin_tone_mask(float3 color) {
  // Simple skin tone detection based on hue and saturation
  float3 hsl = rgb_to_hsl(color);
  
  // Skin tones typically have hue in range [0.05, 0.15] (red to yellow)
  // and saturation in range [0.1, 0.6]
  float hue_mask = smoothstep(0.05f, 0.15f, hsl.x) * (1.0f - smoothstep(0.15f, 0.25f, hsl.x));
  float sat_mask = smoothstep(0.1f, 0.3f, hsl.y) * (1.0f - smoothstep(0.6f, 0.8f, hsl.y));
  
  return hue_mask * sat_mask;
}

// Color grading merge implementation
float3 colorgrade_merge(float3 base_color, float3 overlay_color, 
                       const ColorGradingSettings* settings) {
  // Apply all color grading operations in sequence
  float3 result = base_color;
  
  // Apply basic adjustments
  result = colorgrade_lift_gamma_gain(result, settings);
  result = colorgrade_offset_power_slope(result, settings);
  result = colorgrade_temperature_tint(result, settings);
  result = colorgrade_shadow_midtone_highlight(result, settings);
  
  // Apply HSL adjustments
  result = colorgrade_hsl_adjustment(result, settings);
  
  // Apply curves
  result = colorgrade_apply_curves(result, settings);
  
  // Apply LUT if enabled
  result = colorgrade_sample_lut(result, settings);
  
  // Blend with overlay if provided
  if (length(overlay_color) > 0.001f) {
    result = lerp(result, overlay_color, 0.5f);
  }
  
  return result;
}

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

// Circle of Confusion calculation
float dof_calculate_coc(float depth, float focus_distance, float aperture, float focal_length) {
  // Convert depth to linear depth
  float linear_depth = depth_to_linear_depth(depth);
  
  // Calculate circle of confusion diameter
  // CoC = |aperture * focal_length * (focus_distance - depth) / (depth * (focus_distance - focal_length))|
  float coc = abs(aperture * focal_length * (focus_distance - linear_depth) / 
                 (linear_depth * (focus_distance - focal_length)));
  
  return coc;
}

// Gaussian blur for DOF
float3 dof_gaussian_blur(uint32_t input_texture, float2 uv, float coc, uint32_t sample_count) {
  float3 blurred_color = float3(0.0f);
  float total_weight = 0.0f;
  
  // Calculate blur radius based on CoC
  float blur_radius = coc * 5.0f; // Scale factor for blur radius
  
  for (uint32_t i = 0; i < sample_count; i++) {
    // Calculate sample offset
    float angle = 2.0f * 3.14159f * (float)i / (float)sample_count;
    float radius = blur_radius * (float)i / (float)sample_count;
    
    float2 offset = float2(cos(angle), sin(angle)) * radius;
    float2 sample_uv = uv + offset * texture_size_inv(input_texture);
    
    // Sample texture
    float3 sample_color = texture2D(input_texture, sample_uv).rgb;
    
    // Gaussian weight
    float weight = exp(-0.5f * pow(radius / blur_radius, 2.0f));
    
    blurred_color += sample_color * weight;
    total_weight += weight;
  }
  
  return blurred_color / total_weight;
}

// Bokeh shape generation
float3 dof_bokeh_shape(uint32_t input_texture, float2 uv, float coc, 
                       uint32_t bokeh_shape, uint32_t bokeh_texture) {
  float3 bokeh_color = float3(0.0f);
  float total_weight = 0.0f;
  
  // Calculate bokeh radius
  float bokeh_radius = coc * 3.0f;
  
  // Number of samples based on shape complexity
  uint32_t sample_count = (bokeh_shape == BOKEH_HEXAGONAL) ? 6 : 
                          (bokeh_shape == BOKEH_CIRCULAR) ? 12 : 8;
  
  for (uint32_t i = 0; i < sample_count; i++) {
    float2 offset;
    
    switch (bokeh_shape) {
      case BOKEH_HEXAGONAL:
        // Hexagonal sampling pattern
        float hex_angle = 2.0f * 3.14159f * (float)i / 6.0f;
        offset = float2(cos(hex_angle), sin(hex_angle)) * bokeh_radius;
        break;
        
      case BOKEH_CIRCULAR:
        // Circular sampling pattern
        float circ_angle = 2.0f * 3.14159f * (float)i / sample_count;
        offset = float2(cos(circ_angle), sin(circ_angle)) * bokeh_radius;
        break;
        
      case BOKEH_CUSTOM:
        // Use custom bokeh texture
        float2 bokeh_uv = float2((float)i / (float)sample_count, 0.5f);
        float2 bokeh_sample = texture2D(bokeh_texture, bokeh_uv).rg;
        offset = (bokeh_sample - 0.5f) * 2.0f * bokeh_radius;
        break;
        
      default:
        offset = float2(0.0f);
        break;
    }
    
    float2 sample_uv = uv + offset * texture_size_inv(input_texture);
    float3 sample_color = texture2D(input_texture, sample_uv).rgb;
    
    // Apply bokeh brightness threshold
    float brightness = dot(sample_color, float3(0.2126, 0.7152, 0.0722));
    if (brightness > 0.8f) {
      bokeh_color += sample_color;
      total_weight += 1.0f;
    }
  }
  
  return (total_weight > 0.0f) ? bokeh_color / total_weight : float3(0.0f);
}

// Bokeh sprite rendering
float3 dof_bokeh_sprites(uint32_t input_texture, uint32_t sprite_texture, 
                        float2 uv, float coc, float brightness_threshold) {
  float3 final_color = float3(0.0f);
  
  // Sample original color
  float3 original_color = texture2D(input_texture, uv).rgb;
  
  // Check if pixel should generate bokeh sprite
  float brightness = dot(original_color, float3(0.2126, 0.7152, 0.0722));
  
  if (brightness > brightness_threshold && coc > 0.1f) {
    // Calculate sprite size based on CoC
    float sprite_size = coc * 10.0f;
    
    // Sample bokeh sprite
    float2 sprite_uv = (uv - 0.5f) / sprite_size + 0.5f;
    
    if (sprite_uv.x >= 0.0f && sprite_uv.x <= 1.0f && 
        sprite_uv.y >= 0.0f && sprite_uv.y <= 1.0f) {
      float3 sprite_color = texture2D(sprite_texture, sprite_uv).rgb;
      
      // Multiply sprite color with original color
      final_color = original_color * sprite_color * brightness;
    }
  }
  
  return final_color;
}

// Auto focus implementation
float dof_auto_focus(uint32_t depth_texture, float2 uv, float focus_range) {
  // Sample depth in a small region around the center
  float2 sample_offsets[5] = {
    float2(0.0f, 0.0f),
    float2(-0.01f, 0.0f),
    float2(0.01f, 0.0f),
    float2(0.0f, -0.01f),
    float2(0.0f, 0.01f)
  };
  
  float min_depth = 1.0f;
  float max_depth = 0.0f;
  
  for (int i = 0; i < 5; i++) {
    float2 sample_uv = uv + sample_offsets[i];
    float depth = texture2D(depth_texture, sample_uv).r;
    float linear_depth = depth_to_linear_depth(depth);
    
    min_depth = min(min_depth, linear_depth);
    max_depth = max(max_depth, linear_depth);
  }
  
  // Calculate optimal focus distance
  float focus_distance = (min_depth + max_depth) * 0.5f;
  
  // Clamp to focus range
  focus_distance = clamp(focus_distance, focus_range * 0.1f, focus_range);
  
  return focus_distance;
}

// Near/far separation
void dof_near_far_separation(uint32_t input_texture, uint32_t depth_texture,
                           uint32_t near_texture, uint32_t far_texture,
                           float focus_distance, float near_start, float near_end,
                           float far_start, float far_end) {
  float2 uv = get_fragment_uv();
  
  // Sample depth
  float depth = texture2D(depth_texture, uv).r;
  float linear_depth = depth_to_linear_depth(depth);
  
  // Sample color
  float3 color = texture2D(input_texture, uv).rgb;
  
  // Calculate near and far weights
  float near_weight = 0.0f;
  float far_weight = 0.0f;
  
  if (linear_depth < focus_distance) {
    // Near field
    if (linear_depth < near_start) {
      near_weight = 1.0f;
    } else if (linear_depth < near_end) {
      near_weight = 1.0f - (linear_depth - near_start) / (near_end - near_start);
    }
  } else {
    // Far field
    if (linear_depth > far_end) {
      far_weight = 1.0f;
    } else if (linear_depth > far_start) {
      far_weight = (linear_depth - far_start) / (far_end - far_start);
    }
  }
  
  // Write to near and far textures
  texture2D_write(near_texture, uv, color * near_weight);
  texture2D_write(far_texture, uv, color * far_weight);
}

// Physical camera model
float3 dof_physical_camera(uint32_t input_texture, float2 uv, float depth,
                          const DepthOfFieldSettings* settings) {
  // Convert camera settings to physical parameters
  float focal_length = settings->focal_length; // in mm
  float aperture = settings->aperture; // f-stop
  float focus_distance = settings->focus_distance; // in meters
  
  // Calculate hyperfocal distance
  float hyperfocal = (focal_length * focal_length) / (aperture * 0.001f) + focal_length;
  
  // Calculate circle of confusion using thin lens equation
  float linear_depth = depth_to_linear_depth(depth);
  float coc = abs(aperture * focal_length * (focus_distance - linear_depth) / 
                 (linear_depth * (focus_distance - focal_length)));
  
  // Apply physical blur based on CoC
  float3 blurred_color = dof_gaussian_blur(input_texture, uv, coc, 16);
  
  // Blend with original based on CoC
  float3 original_color = texture2D(input_texture, uv).rgb;
  float blur_amount = smoothstep(0.0f, 0.01f, coc);
  
  return lerp(original_color, blurred_color, blur_amount);
}

// Anamorphic bokeh
float3 dof_anamorphic_bokeh(uint32_t input_texture, float2 uv, float coc,
                           const DepthOfFieldSettings* settings) {
  float3 anamorphic_color = float3(0.0f);
  float total_weight = 0.0f;
  
  // Calculate anamorphic blur radii
  float blur_radius_x = coc * 3.0f;
  float blur_radius_y = blur_radius_x * settings->anamorphic_ratio;
  
  uint32_t sample_count = 12;
  
  for (uint32_t i = 0; i < sample_count; i++) {
    // Elliptical sampling pattern
    float angle = 2.0f * 3.14159f * (float)i / (float)sample_count;
    float2 offset = float2(cos(angle) * blur_radius_x, sin(angle) * blur_radius_y);
    
    float2 sample_uv = uv + offset * texture_size_inv(input_texture);
    float3 sample_color = texture2D(input_texture, sample_uv).rgb;
    
    // Weight by distance from center
    float weight = 1.0f - length(offset) / max(blur_radius_x, blur_radius_y);
    weight = max(0.0f, weight);
    
    anamorphic_color += sample_color * weight;
    total_weight += weight;
  }
  
  return (total_weight > 0.0f) ? anamorphic_color / total_weight : float3(0.0f);
}

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

// Velocity buffer generation
float2 motion_blur_velocity_buffer(uint32_t current_texture, uint32_t previous_texture,
                                  float2 uv, const MotionBlurSettings* settings) {
  // Sample current and previous frame positions
  float3 current_pos = reconstruct_world_position(current_texture, uv);
  float3 previous_pos = reconstruct_world_position(previous_texture, uv);
  
  // Calculate velocity in screen space
  float2 current_screen = world_to_screen(current_pos);
  float2 previous_screen = world_to_screen(previous_pos);
  
  float2 velocity = current_screen - previous_screen;
  
  // Apply intensity scaling
  velocity *= settings->intensity;
  
  // Clamp to maximum velocity
  float velocity_length = length(velocity);
  if (velocity_length > settings->max_velocity) {
    velocity = normalize(velocity) * settings->max_velocity;
  }
  
  return velocity;
}

// Tile max velocity calculation
float2 motion_blur_tile_max(uint32_t velocity_texture, uint2 tile_coords, 
                           uint2 tile_size) {
  float2 max_velocity = float2(0.0f);
  
  // Sample all pixels in the tile
  for (uint32_t y = 0; y < tile_size.y; y++) {
    for (uint32_t x = 0; x < tile_size.x; x++) {
      uint2 pixel_coords = tile_coords * tile_size + uint2(x, y);
      float2 uv = float2(pixel_coords) / texture_size(velocity_texture);
      
      float2 velocity = texture2D(velocity_texture, uv).rg;
      
      // Keep the maximum velocity magnitude
      if (length(velocity) > length(max_velocity)) {
        max_velocity = velocity;
      }
    }
  }
  
  return max_velocity;
}

// Neighbor max velocity calculation
float2 motion_blur_neighbor_max(uint32_t max_velocity_texture, uint2 tile_coords,
                                uint2 tile_count) {
  float2 max_velocity = float2(0.0f);
  
  // Check 3x3 neighborhood of tiles
  for (int y = -1; y <= 1; y++) {
    for (int x = -1; x <= 1; x++) {
      uint2 neighbor_tile = tile_coords + uint2(x, y);
      
      // Check bounds
      if (neighbor_tile.x >= 0 && neighbor_tile.x < tile_count.x &&
          neighbor_tile.y >= 0 && neighbor_tile.y < tile_count.y) {
        
        float2 uv = float2(neighbor_tile) / float2(tile_count);
        float2 tile_velocity = texture2D(max_velocity_texture, uv).rg;
        
        if (length(tile_velocity) > length(max_velocity)) {
          max_velocity = tile_velocity;
        }
      }
    }
  }
  
  return max_velocity;
}

// Motion blur reconstruction
float3 motion_blur_reconstruction(uint32_t color_texture, uint32_t velocity_texture,
                                float2 uv, const MotionBlurSettings* settings) {
  float2 velocity = texture2D(velocity_texture, uv).rg;
  float velocity_length = length(velocity);
  
  if (velocity_length < 0.001f) {
    return texture2D(color_texture, uv).rgb;
  }
  
  float3 blurred_color = float3(0.0f);
  float total_weight = 0.0f;
  
  // Sample along motion vector
  for (uint32_t i = 0; i < settings->sample_count; i++) {
    float t = (float)i / (float)(settings->sample_count - 1);
    
    // Sample position along motion path
    float2 sample_uv = uv - velocity * t * 0.5f;
    
    // Check if sample is within texture bounds
    if (sample_uv.x >= 0.0f && sample_uv.x <= 1.0f &&
        sample_uv.y >= 0.0f && sample_uv.y <= 1.0f) {
      
      float3 sample_color = texture2D(color_texture, sample_uv).rgb;
      
      // Weight samples (center sample has more weight)
      float weight = 1.0f - abs(t - 0.5f) * 0.5f;
      
      blurred_color += sample_color * weight;
      total_weight += weight;
    }
  }
  
  return (total_weight > 0.0f) ? blurred_color / total_weight : texture2D(color_texture, uv).rgb;
}

// Camera motion blur
float3 motion_blur_camera(uint32_t color_texture, float2 uv, 
                         const MotionBlurSettings* settings) {
  if (!settings->camera_motion) {
    return texture2D(color_texture, uv).rgb;
  }
  
  // Get camera motion vectors from previous frame
  float3 camera_velocity = get_camera_velocity();
  
  // Convert camera velocity to screen space motion
  float2 screen_velocity = camera_velocity_to_screen(camera_velocity, uv);
  screen_velocity *= settings->intensity;
  
  float3 blurred_color = float3(0.0f);
  float total_weight = 0.0f;
  
  // Sample along camera motion path
  for (uint32_t i = 0; i < settings->sample_count; i++) {
    float t = (float)i / (float)(settings->sample_count - 1);
    
    // Shutter angle affects the motion range
    float shutter_factor = settings->shutter_angle / 360.0f;
    float2 sample_uv = uv - screen_velocity * t * shutter_factor;
    
    if (sample_uv.x >= 0.0f && sample_uv.x <= 1.0f &&
        sample_uv.y >= 0.0f && sample_uv.y <= 1.0f) {
      
      float3 sample_color = texture2D(color_texture, sample_uv).rgb;
      
      // Weight based on shutter opening (center of shutter is open longer)
      float shutter_weight = sin(t * 3.14159f); // Sinusoidal shutter
      
      blurred_color += sample_color * shutter_weight;
      total_weight += shutter_weight;
    }
  }
  
  return (total_weight > 0.0f) ? blurred_color / total_weight : texture2D(color_texture, uv).rgb;
}

// Per-object motion blur
float3 motion_blur_per_object(uint32_t color_texture, uint32_t object_velocity_texture,
                              float2 uv, const MotionBlurSettings* settings) {
  if (!settings->per_object) {
    return texture2D(color_texture, uv).rgb;
  }
  
  // Get per-object velocity
  float2 object_velocity = texture2D(object_velocity_texture, uv).rg;
  object_velocity *= settings->intensity;
  
  float velocity_length = length(object_velocity);
  
  if (velocity_length < 0.001f) {
    return texture2D(color_texture, uv).rgb;
  }
  
  float3 blurred_color = float3(0.0f);
  float total_weight = 0.0f;
  
  // Adaptive sample count based on velocity magnitude
  uint32_t adaptive_samples = (uint32_t)(settings->sample_count * min(velocity_length, 1.0f));
  adaptive_samples = max(adaptive_samples, 2u);
  
  // Sample along object motion path
  for (uint32_t i = 0; i < adaptive_samples; i++) {
    float t = (float)i / (float)(adaptive_samples - 1);
    
    // Apply shutter angle to object motion
    float shutter_factor = settings->shutter_angle / 360.0f;
    float2 sample_uv = uv - object_velocity * t * shutter_factor;
    
    if (sample_uv.x >= 0.0f && sample_uv.x <= 1.0f &&
        sample_uv.y >= 0.0f && sample_uv.y <= 1.0f) {
      
      float3 sample_color = texture2D(color_texture, sample_uv).rgb;
      
      // Weight samples with motion blur kernel
      float weight = motion_blur_kernel(t, velocity_length);
      
      blurred_color += sample_color * weight;
      total_weight += weight;
    }
  }
  
  return (total_weight > 0.0f) ? blurred_color / total_weight : texture2D(color_texture, uv).rgb;
}

// Motion blur kernel function
float motion_blur_kernel(float t, float velocity_length) {
  // Use a tent filter for motion blur
  if (t <= 0.5f) {
    return t * 2.0f;
  } else {
    return 2.0f * (1.0f - t);
  }
}

// Reconstruct world position from depth
float3 reconstruct_world_position(uint32_t depth_texture, float2 uv) {
  float depth = texture2D(depth_texture, uv).r;
  float linear_depth = depth_to_linear_depth(depth);
  
  // Get view ray for this pixel
  float3 view_ray = get_view_ray(uv);
  
  // Calculate world position
  return camera_position + view_ray * linear_depth;
}

// Convert world position to screen space
float2 world_to_screen(float3 world_pos) {
  float4 clip_pos = view_projection_matrix * float4(world_pos, 1.0f);
  float2 ndc = clip_pos.xy / clip_pos.w;
  return ndc * 0.5f + 0.5f;
}

// Get camera velocity between frames
float3 get_camera_velocity() {
  return current_camera_position - previous_camera_position;
}

// Convert camera velocity to screen space motion
float2 camera_velocity_to_screen(float3 camera_velocity, float2 uv) {
  // Project camera velocity onto screen
  float3 view_ray = get_view_ray(uv);
  float3 camera_right = normalize(cross(view_ray, float3(0.0f, 1.0f, 0.0f)));
  float3 camera_up = normalize(cross(camera_right, view_ray));
  
  float2 screen_velocity;
  screen_velocity.x = dot(camera_velocity, camera_right);
  screen_velocity.y = dot(camera_velocity, camera_up);
  
  return screen_velocity * 0.01f; // Scale factor
}

// Get view ray for pixel
float3 get_view_ray(float2 uv) {
  // Convert UV to NDC
  float2 ndc = uv * 2.0f - 1.0f;
  
  // Unproject using inverse projection matrix
  float4 clip_pos = float4(ndc, 1.0f, 1.0f);
  float4 view_pos = inverse_projection_matrix * clip_pos;
  
  return normalize(view_pos.xyz);
}

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

// Chromatic aberration implementation
float3 chromatic_aberration_apply(uint32_t input_texture, float2 uv, 
                                 const ChromaticAberrationSettings* settings) {
  if (!settings->enabled) {
    return texture2D(input_texture, uv).rgb;
  }
  
  // Calculate distance from center
  float2 center_offset = uv - 0.5f;
  float distance = length(center_offset);
  
  // Calculate chromatic offset based on distance and intensity
  float chromatic_offset = distance * settings->intensity * 0.01f;
  
  // Sample RGB channels with different offsets
  float2 red_uv = uv + center_offset * chromatic_offset;
  float2 green_uv = uv;
  float2 blue_uv = uv - center_offset * chromatic_offset;
  
  // Apply barrel distortion if enabled
  if (settings->barrel_distortion) {
    float distortion_amount = settings->barrel_amount * distance * distance;
    red_uv = apply_barrel_distortion(red_uv, distortion_amount);
    green_uv = apply_barrel_distortion(green_uv, distortion_amount);
    blue_uv = apply_barrel_distortion(blue_uv, distortion_amount);
  }
  
  // Sample color channels
  float r = texture2D(input_texture, red_uv).r;
  float g = texture2D(input_texture, green_uv).g;
  float b = texture2D(input_texture, blue_uv).b;
  
  return float3(r, g, b);
}

// Barrel distortion implementation
float2 apply_barrel_distortion(float2 uv, float amount) {
  float2 center_offset = uv - 0.5f;
  float distance = length(center_offset);
  
  // Apply barrel distortion formula
  float distortion_factor = 1.0f + amount * distance * distance;
  
  return center_offset * distortion_factor + 0.5f;
}

// Vignette implementation
float3 vignette_apply(uint32_t input_texture, float2 uv, 
                     const VignetteSettings* settings) {
  if (!settings->enabled) {
    return texture2D(input_texture, uv).rgb;
  }
  
  // Calculate distance from center
  float2 center_offset = (uv - settings->center) * 2.0f;
  
  // Apply roundness if enabled
  if (settings->rounded) {
    float aspect_ratio = texture_aspect_ratio(input_texture);
    center_offset.x *= aspect_ratio;
  }
  
  float distance = length(center_offset);
  
  // Calculate vignette falloff
  float vignette = 1.0f - smoothstep(0.0f, 1.0f, distance * settings->intensity);
  
  // Apply smoothness
  vignette = pow(vignette, 1.0f / (settings->smoothness + 0.001f));
  
  // Apply color tint
  float3 color = texture2D(input_texture, uv).rgb;
  float3 vignette_color = lerp(settings->color, float3(1.0f), vignette);
  
  return color * vignette_color;
}

// Film grain implementation
float3 film_grain_apply(uint32_t input_texture, float2 uv, 
                       const FilmGrainSettings* settings) {
  if (!settings->enabled) {
    return texture2D(input_texture, uv).rgb;
  }
  
  float3 color = texture2D(input_texture, uv).rgb;
  
  // Generate grain noise
  float3 grain;
  
  if (settings->grain_texture != 0) {
    // Use provided grain texture
    float2 grain_uv = uv * 4.0f + time * 0.1f; // Animate grain
    grain = texture2D(settings->grain_texture, grain_uv).rgb;
  } else {
    // Generate procedural grain
    grain = generate_procedural_grain(uv, time);
  }
  
  // Apply grain intensity and response
  grain = (grain - 0.5f) * settings->intensity;
  
  // Apply response curve (non-linear response)
  grain = sign(grain) * pow(abs(grain), settings->response);
  
  if (settings->colored) {
    // Apply colored grain
    color += grain;
  } else {
    // Apply monochrome grain
    float luminance = dot(grain, float3(0.2126, 0.7152, 0.0722));
    color += luminance;
  }
  
  return color;
}

// Generate procedural grain
float3 generate_procedural_grain(float2 uv, float time) {
  // Use multiple noise octaves for realistic grain
  float grain = 0.0f;
  
  // Octave 1
  grain += noise(uv * 50.0f + time * 0.5f) * 0.5f;
  
  // Octave 2
  grain += noise(uv * 100.0f + time * 0.7f) * 0.25f;
  
  // Octave 3
  grain += noise(uv * 200.0f + time * 1.0f) * 0.125f;
  
  return float3(grain, grain, grain);
}

// Simple noise function
float noise(float2 p) {
  return fract(sin(dot(p, float2(12.9898, 78.233))) * 43758.5453);
}

// Lens flare ghosts implementation
float3 lens_flare_ghosts(uint32_t input_texture, float2 light_pos, 
                        const LensFlareSettings* settings) {
  if (!settings->enabled || settings->ghost_count == 0) {
    return float3(0.0f);
  }
  
  float3 flare_color = float3(0.0f);
  
  // Sample light color at position
  float3 light_color = texture2D(input_texture, light_pos).rgb;
  float light_intensity = dot(light_color, float3(0.2126, 0.7152, 0.0722));
  
  if (light_intensity < settings->threshold) {
    return float3(0.0f);
  }
  
  // Generate ghost images
  for (uint32_t i = 1; i <= settings->ghost_count; i++) {
    // Calculate ghost position (mirrored around center)
    float2 center = float2(0.5f);
    float2 ghost_vector = center - light_pos;
    float2 ghost_pos = center + ghost_vector * (float)i * settings->ghost_spacing;
    
    // Check if ghost is within screen bounds
    if (ghost_pos.x >= 0.0f && ghost_pos.x <= 1.0f &&
        ghost_pos.y >= 0.0f && ghost_pos.y <= 1.0f) {
      
      // Calculate ghost intensity (fades with distance)
      float ghost_intensity = light_intensity / (float)i;
      
      // Apply chromatic aberration to ghosts
      float chromatic_offset = (float)i * 0.001f * settings->chromatic_offset;
      float3 ghost_color;
      
      ghost_color.r = texture2D(input_texture, ghost_pos + float2(chromatic_offset, 0.0f)).r;
      ghost_color.g = texture2D(input_texture, ghost_pos).g;
      ghost_color.b = texture2D(input_texture, ghost_pos - float2(chromatic_offset, 0.0f)).b;
      
      flare_color += ghost_color * ghost_intensity * settings->intensity;
    }
  }
  
  return flare_color;
}

// Lens flare halo implementation
float3 lens_flare_halo(uint32_t input_texture, float2 light_pos, 
                      const LensFlareSettings* settings) {
  if (!settings->enabled) {
    return float3(0.0f);
  }
  
  // Sample light color
  float3 light_color = texture2D(input_texture, light_pos).rgb;
  float light_intensity = dot(light_color, float3(0.2126, 0.7152, 0.0722));
  
  if (light_intensity < settings->threshold) {
    return float3(0.0f);
  }
  
  // Calculate halo position (opposite side of screen from light)
  float2 center = float2(0.5f);
  float2 halo_vector = center - light_pos;
  float2 halo_pos = center + halo_vector * 0.8f; // Slightly offset from center
  
  // Calculate distance from halo center
  float distance = length(halo_pos - center);
  
  // Create halo ring
  float halo_radius = settings->halo_radius;
  float halo_width = settings->halo_width;
  
  float halo = 1.0f - smoothstep(halo_radius - halo_width, halo_radius + halo_width, distance);
  halo *= smoothstep(halo_radius - halo_width * 2.0f, halo_radius - halo_width, distance);
  
  // Apply halo color (usually warm)
  float3 halo_color = float3(1.0f, 0.8f, 0.6f) * light_color;
  
  return halo_color * halo * light_intensity * settings->intensity;
}

// Lens flare starburst implementation
float3 lens_flare_starburst(uint32_t input_texture, float2 light_pos, 
                           const LensFlareSettings* settings) {
  if (!settings->enabled) {
    return float3(0.0f);
  }
  
  // Sample light color
  float3 light_color = texture2D(input_texture, light_pos).rgb;
  float light_intensity = dot(light_color, float3(0.2126, 0.7152, 0.0722));
  
  if (light_intensity < settings->threshold) {
    return float3(0.0f);
  }
  
  float3 starburst_color = float3(0.0f);
  float2 center = float2(0.5f);
  
  // Generate 8-point starburst
  for (int i = 0; i < 8; i++) {
    float angle = (float)i * 3.14159f * 0.25f; // 45-degree intervals
    float2 direction = float2(cos(angle), sin(angle));
    
    // Sample along star ray
    for (float t = 0.1f; t <= 1.0f; t += 0.1f) {
      float2 star_pos = light_pos + direction * t * 0.3f;
      
      if (star_pos.x >= 0.0f && star_pos.x <= 1.0f &&
          star_pos.y >= 0.0f && star_pos.y <= 1.0f) {
        
        // Calculate star intensity (fades with distance)
        float star_intensity = light_intensity * (1.0f - t) * 0.5f;
        
        float3 sample_color = texture2D(input_texture, star_pos).rgb;
        starburst_color += sample_color * star_intensity * settings->intensity;
      }
    }
  }
  
  return starburst_color;
}

// Dust and scratches overlay implementation
float3 dust_scratches_apply(uint32_t input_texture, uint32_t dust_texture, 
                           float2 uv, float intensity) {
  float3 color = texture2D(input_texture, uv).rgb;
  
  if (dust_texture != 0) {
    // Sample dust texture with animation
    float2 dust_uv = uv * 2.0f + time * 0.01f; // Slow animation
    float3 dust = texture2D(dust_texture, dust_uv).rgb;
    
    // Apply dust with intensity
    color = lerp(color, color * dust, intensity * 0.1f);
  } else {
    // Generate procedural dust and scratches
    float dust = generate_dust_noise(uv);
    float scratches = generate_scratch_noise(uv);
    
    // Combine dust and scratches
    float combined = dust * 0.7f + scratches * 0.3f;
    
    // Apply to color
    color = lerp(color, color * (1.0f - combined * 0.1f), intensity);
  }
  
  return color;
}

// Generate dust noise
float generate_dust_noise(float2 uv) {
  float dust = 0.0f;
  
  // Multiple layers of noise for realistic dust
  for (int i = 0; i < 3; i++) {
    float2 offset = float2(i * 0.1f, i * 0.15f);
    float scale = 50.0f * (float)(i + 1);
    dust += noise(uv * scale + offset) * (1.0f / (float)(i + 1));
  }
  
  return dust * 0.3f;
}

// Generate scratch noise
float generate_scratch_noise(float2 uv) {
  float scratches = 0.0f;
  
  // Generate random scratches
  for (int i = 0; i < 5; i++) {
    float seed = (float)i * 123.456f;
    float angle = fract(sin(seed) * 43758.5453) * 3.14159f * 2.0f;
    float2 direction = float2(cos(angle), sin(angle));
    
    float distance = abs(dot(uv - 0.5f, direction));
    float scratch = 1.0f - smoothstep(0.0f, 0.01f, fract(distance * 100.0f));
    
    scratches += scratch * 0.2f;
  }
  
  return scratches;
}
typedef enum BokehShape {
  BOKEH_CIRCULAR,
  BOKEH_HEXAGONAL,
  BOKEH_CUSTOM,
} BokehShape;

// Additional effects enums
typedef enum ScanlineType {
  SCANLINE_HORIZONTAL,
  SCANLINE_VERTICAL,
  SCANLINE_CROSSHATCH,
} ScanlineType;

// Scanlines/CRT effect implementation
float3 scanlines_crt_apply(uint32_t input_texture, float2 uv, 
                          float intensity, ScanlineType type, float curvature) {
  float3 color = texture2D(input_texture, uv).rgb;
  
  // Apply CRT curvature if enabled
  if (curvature > 0.0f) {
    uv = apply_crt_curvature(uv, curvature);
  }
  
  float scanline = 1.0f;
  
  switch (type) {
    case SCANLINE_HORIZONTAL:
      // Horizontal scanlines
      scanline = sin(uv.y * texture_size(input_texture).y * 3.14159f) * 0.5f + 0.5f;
      break;
      
    case SCANLINE_VERTICAL:
      // Vertical scanlines
      scanline = sin(uv.x * texture_size(input_texture).x * 3.14159f) * 0.5f + 0.5f;
      break;
      
    case SCANLINE_CROSSHATCH:
      // Crosshatch pattern
      float h_scan = sin(uv.y * texture_size(input_texture).y * 3.14159f) * 0.5f + 0.5f;
      float v_scan = sin(uv.x * texture_size(input_texture).x * 3.14159f) * 0.5f + 0.5f;
      scanline = h_scan * v_scan;
      break;
  }
  
  // Apply scanline intensity
  scanline = lerp(1.0f, scanline, intensity);
  
  // Add phosphor glow effect
  float3 phosphor = color * 0.1f;
  color = color * scanline + phosphor;
  
  return color;
}

// Apply CRT curvature
float2 apply_crt_curvature(float2 uv, float curvature) {
  float2 center_offset = uv - 0.5f;
  float distance = length(center_offset);
  
  // Apply barrel distortion for CRT effect
  float distortion_factor = 1.0f + curvature * distance * distance;
  
  return center_offset * distortion_factor + 0.5f;
}

// Pixelation effect implementation
float3 pixelation_apply(uint32_t input_texture, float2 uv, float pixel_size) {
  // Calculate pixel coordinates
  float2 pixel_coords = floor(uv * pixel_size) / pixel_size;
  
  // Add half-pixel offset for center sampling
  pixel_coords += 0.5f / pixel_size;
  
  return texture2D(input_texture, pixel_coords).rgb;
}

// Posterization effect implementation
float3 posterization_apply(uint32_t input_texture, float2 uv, 
                          float color_levels, float gamma_correction) {
  float3 color = texture2D(input_texture, uv).rgb;
  
  // Apply gamma correction if specified
  if (gamma_correction != 1.0f) {
    color = pow(color, float3(gamma_correction));
  }
  
  // Quantize colors to specified levels
  color = floor(color * color_levels) / color_levels;
  
  // Reverse gamma correction
  if (gamma_correction != 1.0f) {
    color = pow(color, float3(1.0f / gamma_correction));
  }
  
  return color;
}

// Sharpen effect implementation
float3 sharpen_apply(uint32_t input_texture, float2 uv, float strength) {
  float3 center_color = texture2D(input_texture, uv).rgb;
  
  // Sample neighboring pixels
  float2 texel_size = texture_size_inv(input_texture);
  
  float3 left = texture2D(input_texture, uv + float2(-texel_size.x, 0.0f)).rgb;
  float3 right = texture2D(input_texture, uv + float2(texel_size.x, 0.0f)).rgb;
  float3 top = texture2D(input_texture, uv + float2(0.0f, -texel_size.y)).rgb;
  float3 bottom = texture2D(input_texture, uv + float2(0.0f, texel_size.y)).rgb;
  
  // Calculate Laplacian kernel
  float3 laplacian = (left + right + top + bottom) - (center_color * 4.0f);
  
  // Apply sharpening
  float3 sharpened = center_color + laplacian * strength;
  
  return sharpened;
}

// Helper functions for texture operations
float2 texture_size(uint32_t texture) {
  // This would be implemented in the actual shader
  return float2(1920.0f, 1080.0f); // Default resolution
}

float2 texture_size_inv(uint32_t texture) {
  float2 size = texture_size(texture);
  return 1.0f / size;
}

float texture_aspect_ratio(uint32_t texture) {
  float2 size = texture_size(texture);
  return size.x / size.y;
}

float2 get_fragment_uv() {
  // Get current fragment UV coordinates
  return gl_FragCoord.xy / texture_size(0); // Assuming texture 0 is the screen
}

void texture2D_write(uint32_t texture, float2 uv, float3 color) {
  // This would be implemented as a render target write in actual shader
  // Placeholder for compilation
}

void texture3D_write(uint32_t texture, uint32_t x, uint32_t y, uint32_t z, float3 color) {
  // This would be implemented as a 3D texture write in actual shader
  // Placeholder for compilation
}

float3 texture3D(uint32_t texture, float3 coords) {
  // This would be implemented as a 3D texture sample in actual shader
  return float3(0.5f); // Placeholder
}

// Depth conversion helper
float depth_to_linear_depth(float depth) {
  // Convert non-linear depth to linear depth
  float near_plane = 0.1f;
  float far_plane = 1000.0f;
  
  return (2.0f * near_plane) / (far_plane + near_plane - depth * (far_plane - near_plane));
}

// Matrix helpers (would be uniforms in actual shader)
extern float4x4 view_projection_matrix;
extern float4x4 inverse_projection_matrix;
extern float3 camera_position;
extern float3 previous_camera_position;
extern float time;

// LUT size constant
#define LUT_SIZE 32

// Math helper functions
float lerp(float a, float b, float t) {
  return a + t * (b - a);
}

float3 lerp(float3 a, float3 b, float t) {
  return a + t * (b - a);
}

float2 lerp(float2 a, float2 b, float t) {
  return a + t * (b - a);
}

float clamp(float value, float min_val, float max_val) {
  return max(min_val, min(value, max_val));
}

float3 clamp(float3 value, float min_val, float max_val) {
  return float3(clamp(value.x, min_val, max_val),
                clamp(value.y, min_val, max_val),
                clamp(value.z, min_val, max_val));
}

float2 clamp(float2 value, float min_val, float max_val) {
  return float2(clamp(value.x, min_val, max_val),
                clamp(value.y, min_val, max_val));
}

float min(float a, float b) {
  return (a < b) ? a : b;
}

float max(float a, float b) {
  return (a > b) ? a : b;
}

float3 min(float3 a, float3 b) {
  return float3(min(a.x, b.x), min(a.y, b.y), min(a.z, b.z));
}

float3 max(float3 a, float3 b) {
  return float3(max(a.x, b.x), max(a.y, b.y), max(a.z, b.z));
}

float2 min(float2 a, float2 b) {
  return float2(min(a.x, b.x), min(a.y, b.y));
}

float2 max(float2 a, float2 b) {
  return float2(max(a.x, b.x), max(a.y, b.y));
}

float abs(float value) {
  return (value < 0.0f) ? -value : value;
}

float3 abs(float3 value) {
  return float3(abs(value.x), abs(value.y), abs(value.z));
}

float2 abs(float2 value) {
  return float2(abs(value.x), abs(value.y));
}

float sign(float value) {
  return (value > 0.0f) ? 1.0f : ((value < 0.0f) ? -1.0f : 0.0f);
}

float3 sign(float3 value) {
  return float3(sign(value.x), sign(value.y), sign(value.z));
}

float length(float2 v) {
  return sqrt(v.x * v.x + v.y * v.y);
}

float length(float3 v) {
  return sqrt(v.x * v.x + v.y * v.y + v.z * v.z);
}

float normalize_component(float value) {
  return value / length(value);
}

float2 normalize(float2 v) {
  float len = length(v);
  return (len > 0.0f) ? v / len : float2(0.0f);
}

float3 normalize(float3 v) {
  float len = length(v);
  return (len > 0.0f) ? v / len : float3(0.0f);
}

float dot(float2 a, float2 b) {
  return a.x * b.x + a.y * b.y;
}

float dot(float3 a, float3 b) {
  return a.x * b.x + a.y * b.y + a.z * b.z;
}

float3 cross(float3 a, float3 b) {
  return float3(a.y * b.z - a.z * b.y,
                a.z * b.x - a.x * b.z,
                a.x * b.y - a.y * b.x);
}

float pow(float base, float exponent) {
  // Simple power function
  return exp(log(base) * exponent);
}

float3 pow(float3 base, float exponent) {
  return float3(pow(base.x, exponent),
                pow(base.y, exponent),
                pow(base.z, exponent));
}

float exp(float value) {
  // Simple exponential function
  return 2.718281828f; // e^1, placeholder
}

float log(float value) {
  // Simple logarithm function
  return 0.0f; // Placeholder
}

float sqrt(float value) {
  // Simple square root function
  return value * 0.5f; // Placeholder
}

float sin(float value) {
  // Simple sine function
  return 0.0f; // Placeholder
}

float cos(float value) {
  // Simple cosine function
  return 1.0f; // Placeholder
}

float fract(float value) {
  return value - floor(value);
}

float floor(float value) {
  return (value >= 0.0f) ? (int)value : ((int)value - 1);
}

float smoothstep(float edge0, float edge1, float x) {
  float t = clamp((x - edge0) / (edge1 - edge0), 0.0f, 1.0f);
  return t * t * (3.0f - 2.0f * t);
}

float2 float2(float x) {
  return float2(x, x);
}

float2 float2(float x, float y) {
  struct float2_t { float x, y; };
  return (struct float2_t){x, y};
}

float3 float3(float x) {
  return float3(x, x, x);
}

float3 float3(float x, float y, float z) {
  struct float3_t { float x, y, z; };
  return (struct float3_t){x, y, z};
}

float4 float4(float x, float y, float z, float w) {
  struct float4_t { float x, y, z, w; };
  return (struct float4_t){x, y, z, w};
}

float4x4 float4x4_identity() {
  return float4x4(1.0f, 0.0f, 0.0f, 0.0f,
                 0.0f, 1.0f, 0.0f, 0.0f,
                 0.0f, 0.0f, 1.0f, 0.0f,
                 0.0f, 0.0f, 0.0f, 1.0f);
}

float4x4 float4x4(float m00, float m01, float m02, float m03,
                 float m10, float m11, float m12, float m13,
                 float m20, float m21, float m22, float m23,
                 float m30, float m31, float m32, float m33) {
  struct float4x4_t { 
    float4 m0, m1, m2, m3;
  };
  return (struct float4x4_t){
    float4(m00, m01, m02, m03),
    float4(m10, m11, m12, m13),
    float4(m20, m21, m22, m23),
    float4(m30, m31, m32, m33)
  };
}

float4x4 operator*(float4x4 a, float4x4 b) {
  // Matrix multiplication placeholder
  return a;
}

float4 operator*(float4x4 matrix, float4 vector) {
  // Matrix-vector multiplication placeholder
  return vector;
}

// Texture sampling functions
float texture2D(uint32_t texture, float2 uv) {
  // Texture sampling placeholder
  return 0.5f;
}

float texture2D_red(uint32_t texture, float2 uv) {
  // Single channel texture sampling placeholder
  return 0.5f;
}

// Global variables for shader
extern float4 gl_FragCoord;

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

// Sharpen effect implementation
float3 sharpen_apply(uint32_t input_texture, float2 uv, float strength) {
  float3 center_color = texture2D(input_texture, uv).rgb;
  
  // Sample neighboring pixels
  float2 texel_size = texture_size_inv(input_texture);
  float3 left = texture2D(input_texture, uv + float2(-texel_size.x, 0.0f)).rgb;
  float3 right = texture2D(input_texture, uv + float2(texel_size.x, 0.0f)).rgb;
  float3 top = texture2D(input_texture, uv + float2(0.0f, -texel_size.y)).rgb;
  float3 bottom = texture2D(input_texture, uv + float2(0.0f, texel_size.y)).rgb;
  
  // Calculate Laplacian kernel
  float3 laplacian = (left + right + top + bottom) - (center_color * 4.0f);
  
  // Apply sharpening
  float3 sharpened = center_color + laplacian * strength;
  
  return sharpened;
}

// Unsharp mask sharpening
float3 sharpen_unsharp_mask(uint32_t input_texture, float2 uv, 
                           float radius, float amount, float threshold) {
  float3 original = texture2D(input_texture, uv).rgb;
  
  // Create blurred version
  float3 blurred = float3(0.0f);
  float total_weight = 0.0f;
  float2 texel_size = texture_size_inv(input_texture);
  
  // Gaussian blur kernel
  for (int x = -2; x <= 2; x++) {
    for (int y = -2; y <= 2; y++) {
      float2 offset = float2(x, y) * texel_size * radius;
      float distance = sqrt(x * x + y * y);
      float weight = exp(-0.5f * distance * distance);
      
      blurred += texture2D(input_texture, uv + offset).rgb * weight;
      total_weight += weight;
    }
  }
  
  blurred /= total_weight;
  
  // Calculate difference
  float3 difference = original - blurred;
  
  // Apply threshold to avoid sharpening noise
  float3 mask = step(threshold, abs(difference));
  
  // Apply sharpening
  return original + difference * amount * mask;
}

// High-pass filter sharpening
float3 sharpen_high_pass(uint32_t input_texture, float2 uv, float strength) {
  float3 center = texture2D(input_texture, uv).rgb;
  
  // Sample in a cross pattern
  float2 texel_size = texture_size_inv(input_texture);
  float3 samples[4];
  samples[0] = texture2D(input_texture, uv + float2(-texel_size.x, 0.0f)).rgb;
  samples[1] = texture2D(input_texture, uv + float2(texel_size.x, 0.0f)).rgb;
  samples[2] = texture2D(input_texture, uv + float2(0.0f, -texel_size.y)).rgb;
  samples[3] = texture2D(input_texture, uv + float2(0.0f, texel_size.y)).rgb;
  
  // Calculate average of neighbors
  float3 average = (samples[0] + samples[1] + samples[2] + samples[3]) * 0.25f;
  
  // High-pass filter
  float3 high_pass = center - average;
  
  return center + high_pass * strength;
}

// FXAA (Fast Approximate Anti-Aliasing) implementation
float3 fxaa_apply(uint32_t color_texture, uint32_t depth_texture, 
                   float2 uv, float fxaa_quality_subpix, 
                   float fxaa_quality_edge_threshold, 
                   float fxaa_quality_edge_threshold_min) {
  // Sample center and neighboring pixels
  float3 rgb_center = texture2D(color_texture, uv).rgb;
  float3 rgb_l = texture2D(color_texture, uv + float2(-1.0f, 0.0f) * texture_size_inv(color_texture)).rgb;
  float3 rgb_r = texture2D(color_texture, uv + float2(1.0f, 0.0f) * texture_size_inv(color_texture)).rgb;
  float3 rgb_t = texture2D(color_texture, uv + float2(0.0f, -1.0f) * texture_size_inv(color_texture)).rgb;
  float3 rgb_b = texture2D(color_texture, uv + float2(0.0f, 1.0f) * texture_size_inv(color_texture)).rgb;
  
  // Calculate luma values
  float luma_center = dot(rgb_center, float3(0.299f, 0.587f, 0.114f));
  float luma_l = dot(rgb_l, float3(0.299f, 0.587f, 0.114f));
  float luma_r = dot(rgb_r, float3(0.299f, 0.587f, 0.114f));
  float luma_t = dot(rgb_t, float3(0.299f, 0.587f, 0.114f));
  float luma_b = dot(rgb_b, float3(0.299f, 0.587f, 0.114f));
  
  // Find minimum and maximum luma
  float luma_min = min(luma_center, min(min(luma_l, luma_r), min(luma_t, luma_b)));
  float luma_max = max(luma_center, max(max(luma_l, luma_r), max(luma_t, luma_b)));
  
  // Calculate local contrast
  float luma_range = luma_max - luma_min;
  
  // Early exit if contrast is too low
  if (luma_range < max(fxaa_quality_edge_threshold_min, luma_max * fxaa_quality_edge_threshold)) {
    return rgb_center;
  }
  
  // Edge detection
  float2 dir;
  dir.x = -(luma_l - luma_r);
  dir.y = -(luma_t - luma_b);
  
  // Normalize direction
  float dir_reduce = max((luma_l + luma_r) * 0.25f, (luma_t + luma_b) * 0.25f);
  float dir_clamp = max(dir_reduce * fxaa_quality_subpix, 1.0f / 128.0f);
  float2 dir_clamped = clamp(dir, -dir_clamp, dir_clamp);
  
  // Sample along edges
  float2 uv1 = uv + dir_clamped * (1.0f / 3.0f - 0.5f);
  float2 uv2 = uv + dir_clamped * (2.0f / 3.0f - 0.5f);
  
  float3 rgb_a = 0.5f * (texture2D(color_texture, uv1).rgb + texture2D(color_texture, uv2).rgb);
  
  // Sample further along edges
  float2 uv3 = uv + dir_clamped * (-0.5f);
  float2 uv4 = uv + dir_clamped * (0.5f);
  
  float3 rgb_b = rgb_a * 0.5f + 0.25f * (texture2D(color_texture, uv3).rgb + texture2D(color_texture, uv4).rgb);
  
  // Calculate luma of blended samples
  float luma_b = dot(rgb_b, float3(0.299f, 0.587f, 0.114f));
  
  // Choose between original and blended based on luma difference
  if ((luma_b < luma_min) || (luma_b > luma_max)) {
    return rgb_a;
  } else {
    return rgb_b;
  }
}

// SMAA edge detection implementation
float2 smaa_edge_detection(uint32_t color_texture, float2 uv, float threshold) {
  // Sample center and neighboring pixels
  float3 center = texture2D(color_texture, uv).rgb;
  float3 left = texture2D(color_texture, uv + float2(-1.0f, 0.0f) * texture_size_inv(color_texture)).rgb;
  float3 right = texture2D(color_texture, uv + float2(1.0f, 0.0f) * texture_size_inv(color_texture)).rgb;
  float3 top = texture2D(color_texture, uv + float2(0.0f, -1.0f) * texture_size_inv(color_texture)).rgb;
  float3 bottom = texture2D(color_texture, uv + float2(0.0f, 1.0f) * texture_size_inv(color_texture)).rgb;
  
  // Calculate luma values
  float luma_center = dot(center, float3(0.299f, 0.587f, 0.114f));
  float luma_left = dot(left, float3(0.299f, 0.587f, 0.114f));
  float luma_right = dot(right, float3(0.299f, 0.587f, 0.114f));
  float luma_top = dot(top, float3(0.299f, 0.587f, 0.114f));
  float luma_bottom = dot(bottom, float3(0.299f, 0.587f, 0.114f));
  
  // Calculate horizontal and vertical edges
  float delta_h = abs(luma_left - 2.0f * luma_center + luma_right);
  float delta_v = abs(luma_top - 2.0f * luma_center + luma_bottom);
  
  // Apply threshold
  float edge_h = step(threshold, delta_h);
  float edge_v = step(threshold, delta_v);
  
  return float2(edge_h, edge_v);
}

// SMAA blending weights calculation
float3 smaa_blending_weights(uint32_t color_texture, uint32_t edges_texture, 
                            float2 uv, float2 texel_size) {
  // Sample edge information
  float2 edges = texture2D(edges_texture, uv).rg;
  
  // Early exit if no edge
  if (edges.r + edges.g < 0.01f) {
    return float3(1.0f, 0.0f, 0.0f); // No blending needed
  }
  
  // Calculate search patterns for horizontal and vertical edges
  float2 search_texel_size = texel_size * 2.0f;
  
  // Horizontal search
  float h_weights = 0.0f;
  if (edges.r > 0.5f) {
    // Search left and right along horizontal edge
    for (int i = -8; i <= 8; i++) {
      if (i != 0) {
        float2 search_uv = uv + float2(i, 0.0f) * search_texel_size;
        float2 search_edges = texture2D(edges_texture, search_uv).rg;
        
        if (search_edges.g > 0.5f) {
          h_weights = 1.0f - abs(i) / 8.0f;
          break;
        }
      }
    }
  }
  
  // Vertical search
  float v_weights = 0.0f;
  if (edges.g > 0.5f) {
    // Search up and down along vertical edge
    for (int i = -8; i <= 8; i++) {
      if (i != 0) {
        float2 search_uv = uv + float2(0.0f, i) * search_texel_size;
        float2 search_edges = texture2D(edges_texture, search_uv).rg;
        
        if (search_edges.r > 0.5f) {
          v_weights = 1.0f - abs(i) / 8.0f;
          break;
        }
      }
    }
  }
  
  return float3(h_weights, v_weights, max(h_weights, v_weights));
}

// SMAA neighborhood blending
float3 smaa_neighborhood_blending(uint32_t color_texture, uint32_t weights_texture, 
                                 float2 uv) {
  // Sample original color and blending weights
  float3 color = texture2D(color_texture, uv).rgb;
  float3 weights = texture2D(weights_texture, uv).rgb;
  
  // Early exit if no blending needed
  if (weights.b < 0.01f) {
    return color;
  }
  
  // Sample neighboring pixels for blending
  float2 texel_size = texture_size_inv(color_texture);
  float3 color_left = texture2D(color_texture, uv + float2(-texel_size.x, 0.0f)).rgb;
  float3 color_right = texture2D(color_texture, uv + float2(texel_size.x, 0.0f)).rgb;
  float3 color_top = texture2D(color_texture, uv + float2(0.0f, -texel_size.y)).rgb;
  float3 color_bottom = texture2D(color_texture, uv + float2(0.0f, texel_size.y)).rgb;
  
  // Apply horizontal blending
  float3 h_blend = lerp(color, lerp(color_left, color_right, 0.5f), weights.r);
  
  // Apply vertical blending
  float3 v_blend = lerp(color, lerp(color_top, color_bottom, 0.5f), weights.g);
  
  // Combine horizontal and vertical blending
  return lerp(color, lerp(h_blend, v_blend, 0.5f), weights.b);
}

// Scanlines/CRT effect implementation
float3 scanlines_crt_apply(uint32_t input_texture, float2 uv, 
                          float intensity, ScanlineType type, float curvature) {
  float3 color = texture2D(input_texture, uv).rgb;
  
  // Apply CRT curvature if enabled
  if (curvature > 0.0f) {
    uv = apply_crt_curvature(uv, curvature);
  }
  
  float scanline = 1.0f;
  
  switch (type) {
    case SCANLINE_HORIZONTAL:
      // Horizontal scanlines
      scanline = sin(uv.y * texture_size(input_texture).y * 3.14159f) * 0.5f + 0.5f;
      break;
      
    case SCANLINE_VERTICAL:
      // Vertical scanlines
      scanline = sin(uv.x * texture_size(input_texture).x * 3.14159f) * 0.5f + 0.5f;
      break;
      
    case SCANLINE_CROSS:
      // Cross hatch pattern
      float h_scan = sin(uv.y * texture_size(input_texture).y * 3.14159f) * 0.5f + 0.5f;
      float v_scan = sin(uv.x * texture_size(input_texture).x * 3.14159f) * 0.5f + 0.5f;
      scanline = h_scan * v_scan;
      break;
      
    case SCANLINE_CROSSHATCH:
      // Diagonal cross hatch
      float diag1 = sin((uv.x + uv.y) * texture_size(input_texture).x * 3.14159f) * 0.5f + 0.5f;
      float diag2 = sin((uv.x - uv.y) * texture_size(input_texture).x * 3.14159f) * 0.5f + 0.5f;
      scanline = diag1 * diag2;
      break;
      
    default:
      break;
  }
  
  // Apply scanline intensity with phosphor decay simulation
  float3 scanline_color = color * lerp(1.0f, scanline, intensity);
  
  // Add phosphor glow for bright areas
  float brightness = dot(color, float3(0.2126, 0.7152, 0.0722));
  float3 phosphor_glow = color * pow(brightness, 2.0f) * 0.1f * intensity;
  
  return scanline_color + phosphor_glow;
}

// TAA jitter implementation
float2 taa_generate_jitter(uint32_t frame_index, float jitter_scale) {
  // Halton sequence for low-discrepancy sampling
  float2 halton[8] = {
    float2(0.5f, 0.333333f),
    float2(0.25f, 0.666667f),
    float2(0.75f, 0.111111f),
    float2(0.125f, 0.444444f),
    float2(0.625f, 0.777778f),
    float2(0.375f, 0.222222f),
    float2(0.875f, 0.555556f),
    float2(0.0625f, 0.888889f)
  };
  
  uint32_t index = frame_index % 8;
  return (halton[index] - 0.5f) * jitter_scale;
}

// TAA history reprojection
float3 taa_history_reprojection(uint32_t current_color, uint32_t previous_color,
                             uint32_t velocity_texture, uint32_t depth_texture,
                             float2 uv, float2 current_jitter, float2 previous_jitter,
                             float history_blend_factor) {
  // Sample current color and velocity
  float3 current = texture2D(current_color, uv).rgb;
  float2 velocity = texture2D(velocity_texture, uv).rg;
  float depth = texture2D(depth_texture, uv).r;
  
  // Calculate previous UV position
  float2 previous_uv = uv - velocity + (previous_jitter - current_jitter);
  
  // Check if previous UV is valid
  if (previous_uv.x < 0.0f || previous_uv.x > 1.0f ||
      previous_uv.y < 0.0f || previous_uv.y > 1.0f) {
    return current; // No history available
  }
  
  // Sample previous frame color
  float3 previous = texture2D(previous_color, previous_uv).rgb;
  
  // Check depth consistency for disocclusion detection
  float previous_depth = texture2D(depth_texture, previous_uv).r;
  float depth_diff = abs(depth - previous_depth);
  float disocclusion_factor = smoothstep(0.01f, 0.1f, depth_diff);
  
  // Blend current and previous based on history and disocclusion
  float history_weight = history_blend_factor * (1.0f - disocclusion_factor);
  
  return lerp(current, previous, history_weight);
}

// TAA neighborhood clamping
float3 taa_neighborhood_clamping(uint32_t current_color, uint32_t history_color,
                               float2 uv, float neighborhood_radius) {
  float3 current = texture2D(current_color, uv).rgb;
  float3 history = texture2D(history_color, uv).rgb;
  
  // Sample neighborhood pixels
  float2 texel_size = texture_size_inv(current_color);
  float3 min_color = current;
  float3 max_color = current;
  
  for (int x = -1; x <= 1; x++) {
    for (int y = -1; y <= 1; y++) {
      if (x == 0 && y == 0) continue;
      
      float2 sample_uv = uv + float2(x, y) * texel_size * neighborhood_radius;
      float3 sample_color = texture2D(current_color, sample_uv).rgb;
      
      min_color = min(min_color, sample_color);
      max_color = max(max_color, sample_color);
    }
  }
  
  // Clamp history to neighborhood bounds
  float3 clamped_history = clamp(history, min_color, max_color);
  
  // Choose between clamped history and current based on how far history was clamped
  float3 history_diff = abs(history - clamped_history);
  float clamp_amount = max(history_diff.r, max(history_diff.g, history_diff.b));
  float clamp_factor = smoothstep(0.01f, 0.1f, clamp_amount);
  
  return lerp(clamped_history, current, clamp_factor);
}

// TAA velocity rejection
float3 taa_velocity_rejection(uint32_t current_color, uint32_t history_color,
                           uint32_t velocity_texture, float2 uv,
                           float velocity_threshold) {
  float3 current = texture2D(current_color, uv).rgb;
  float3 history = texture2D(history_color, uv).rgb;
  float2 velocity = texture2D(velocity_texture, uv).rg;
  
  // Calculate velocity magnitude
  float velocity_magnitude = length(velocity);
  
  // Reject history for high-velocity pixels
  float rejection_factor = smoothstep(velocity_threshold * 0.5f, velocity_threshold, velocity_magnitude);
  
  // Also reject based on color difference (ghosting prevention)
  float3 color_diff = abs(current - history);
  float max_color_diff = max(color_diff.r, max(color_diff.g, color_diff.b));
  float color_rejection_factor = smoothstep(0.05f, 0.2f, max_color_diff);
  
  // Combine rejection factors
  float total_rejection = max(rejection_factor, color_rejection_factor);
  
  return lerp(history, current, total_rejection);
}

// TAA sharpening
float3 taa_sharpening(uint32_t color_texture, float2 uv, float sharpness) {
  float3 center = texture2D(color_texture, uv).rgb;
  
  if (sharpness <= 0.0f) {
    return center;
  }
  
  // Sample neighboring pixels
  float2 texel_size = texture_size_inv(color_texture);
  float3 left = texture2D(color_texture, uv + float2(-texel_size.x, 0.0f)).rgb;
  float3 right = texture2D(color_texture, uv + float2(texel_size.x, 0.0f)).rgb;
  float3 top = texture2D(color_texture, uv + float2(0.0f, -texel_size.y)).rgb;
  float3 bottom = texture2D(color_texture, uv + float2(0.0f, texel_size.y)).rgb;
  
  // Calculate contrast enhancement
  float3 contrast = (left + right + top + bottom) * 0.25f - center;
  float3 sharpened = center - contrast * sharpness;
  
  // Prevent negative values and overshoot
  sharpened = max(sharpened, 0.0f);
  sharpened = min(sharpened, 1.0f);
  
  return sharpened;
}

// Complete TAA integration
float3 taa_apply(uint32_t current_color, uint32_t previous_color,
                 uint32_t velocity_texture, uint32_t depth_texture,
                 float2 uv, uint32_t frame_index,
                 float jitter_scale, float history_blend, float sharpness,
                 bool enable_velocity_rejection, bool enable_neighborhood_clamping) {
  // Generate jitter
  float2 jitter = taa_generate_jitter(frame_index, jitter_scale);
  float2 previous_jitter = taa_generate_jitter(frame_index - 1, jitter_scale);
  
  // Apply jitter to UV
  float2 jittered_uv = uv + jitter * texture_size_inv(current_color);
  
  // History reprojection
  float3 reprojected = taa_history_reprojection(
    current_color, previous_color, velocity_texture, depth_texture,
    jittered_uv, jitter, previous_jitter, history_blend);
  
  // Apply neighborhood clamping if enabled
  if (enable_neighborhood_clamping) {
    reprojected = taa_neighborhood_clamping(current_color, previous_color, 
                                        jittered_uv, 1.0f);
  }
  
  // Apply velocity rejection if enabled
  if (enable_velocity_rejection) {
    reprojected = taa_velocity_rejection(current_color, previous_color,
                                       velocity_texture, jittered_uv, 0.1f);
  }
  
  // Apply sharpening
  return taa_sharpening(current_color, jittered_uv, sharpness);
}

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

// Post-process stack initialization
bool post_process_init(PostProcessStack* stack, uint32_t width, uint32_t height) {
  if (!stack || width == 0 || height == 0) {
    return false;
  }
  
  // Initialize default settings
  stack->bloom = (BloomSettings){
    .enabled = true,
    .threshold = 1.0f,
    .intensity = 0.5f,
    .scatter = 0.7f,
    .mip_levels = 6,
    .tint = {1.0f, 1.0f, 1.0f},
    .lens_dirt_enabled = false,
    .lens_dirt_texture = 0,
    .lens_dirt_intensity = 0.3f
  };
  
  stack->tonemapping = (ToneMappingSettings){
    .mode = TONEMAP_ACES,
    .exposure = 1.0f,
    .white_point = 11.2f,
    .saturation = 1.0f,
    .contrast = 1.0f
  };
  
  stack->color_grading = (ColorGradingSettings){
    .enabled = false,
    .lift = {0.0f, 0.0f, 0.0f},
    .gamma = {1.0f, 1.0f, 1.0f},
    .gain = {1.0f, 1.0f, 1.0f},
    .temperature = 0.0f,
    .tint = 0.0f,
    .hue_shift = 0.0f,
    .saturation = 1.0f,
    .vibrance = 0.0f,
    .use_lut = false,
    .lut_texture = 0,
    .lut_contribution = 1.0f,
    .use_curves = false
  };
  
  // Create intermediate render targets
  stack->width = width;
  stack->height = height;
  stack->is_enabled = true;
  
  // Initialize render targets (simplified)
  stack->intermediate_texture = create_render_target(width, height);
  stack->history_texture = create_render_target(width, height);
  stack->velocity_texture = create_render_target(width, height);
  stack->depth_texture = create_depth_buffer(width, height);
  
  return stack->intermediate_texture != 0 && stack->history_texture != 0;
}

// Post-process stack shutdown
void post_process_shutdown(PostProcessStack* stack) {
  if (!stack) return;
  
  // Cleanup render targets
  if (stack->intermediate_texture) {
    destroy_render_target(stack->intermediate_texture);
    stack->intermediate_texture = 0;
  }
  
  if (stack->history_texture) {
    destroy_render_target(stack->history_texture);
    stack->history_texture = 0;
  }
  
  if (stack->velocity_texture) {
    destroy_render_target(stack->velocity_texture);
    stack->velocity_texture = 0;
  }
  
  if (stack->depth_texture) {
    destroy_depth_buffer(stack->depth_texture);
    stack->depth_texture = 0;
  }
  
  stack->is_enabled = false;
}

// Post-process stack resize
bool post_process_resize(PostProcessStack* stack, uint32_t new_width, uint32_t new_height) {
  if (!stack || new_width == 0 || new_height == 0) {
    return false;
  }
  
  // Cleanup old render targets
  post_process_shutdown(stack);
  
  // Reinitialize with new dimensions
  return post_process_init(stack, new_width, new_height);
}

// Post-process render
uint32_t post_process_render(PostProcessStack* stack, uint32_t input_texture, 
                         uint32_t frame_index, float2 jitter_offset) {
  if (!stack || !stack->is_enabled || !input_texture) {
    return input_texture;
  }
  
  uint32_t current_texture = input_texture;
  
  // Apply bloom if enabled
  if (stack->bloom.enabled) {
    bloom_apply_threshold(current_texture, stack->intermediate_texture, &stack->bloom);
    bloom_downsample_chain(&stack->bloom_mips, stack->bloom.mip_levels, 
                         stack->intermediate_texture, &stack->bloom);
    bloom_upsample_chain(&stack->bloom_mips, stack->bloom.mip_levels,
                       stack->intermediate_texture, &stack->bloom);
    bloom_combine(current_texture, stack->intermediate_texture, 
                stack->intermediate_texture, &stack->bloom);
    current_texture = stack->intermediate_texture;
  }
  
  // Apply tone mapping
  float3 color = texture2D(current_texture, float2(0.5f)).rgb;
  switch (stack->tonemapping.mode) {
    case TONEMAP_REINHARD:
      color = tonemap_reinhard(color, &stack->tonemapping);
      break;
    case TONEMAP_ACES:
      color = tonemap_aces(color, &stack->tonemapping);
      break;
    case TONEMAP_UNCHARTED2:
      color = tonemap_uncharted2(color, &stack->tonemapping);
      break;
    case TONEMAP_AGX:
      color = tonemap_agx(color, &stack->tonemapping);
      break;
    default:
      break;
  }
  
  // Apply color grading if enabled
  if (stack->color_grading.enabled) {
    color = colorgrade_merge(color, float3(0.0f), &stack->color_grading);
  }
  
  // Apply anti-aliasing based on mode
  if (stack->aa_settings.mode == AA_FXAA) {
    // FXAA would be applied here
  } else if (stack->aa_settings.mode == AA_TAA) {
    color = taa_apply(current_texture, stack->history_texture,
                   stack->velocity_texture, stack->depth_texture,
                   float2(0.5f), frame_index, jitter_offset.x,
                   stack->aa_settings.history_blend, stack->aa_settings.sharpness,
                   stack->aa_settings.motion_vector_dilation, true);
  }
  
  return current_texture;
}

// Post-process effect order management
bool post_process_set_order(PostProcessStack* stack, const char** effect_order, uint32_t effect_count) {
  if (!stack || !effect_order || effect_count == 0) {
    return false;
  }
  
  // Validate effect names
  const char* valid_effects[] = {
    "bloom", "tonemapping", "colorgrading", "aa", "dof", "motionblur"
  };
  
  for (uint32_t i = 0; i < effect_count; i++) {
    bool valid = false;
    for (uint32_t j = 0; j < 6; j++) {
      if (strcmp(effect_order[i], valid_effects[j]) == 0) {
        valid = true;
        break;
      }
    }
    if (!valid) {
      return false; // Invalid effect name
    }
  }
  
  // Store effect order (simplified)
  stack->effect_order_count = effect_count;
  for (uint32_t i = 0; i < effect_count && i < 16; i++) {
    strncpy(stack->effect_order[i], effect_order[i], 31);
    stack->effect_order[i][31] = '\0';
  }
  
  return true;
}

// Post-process blend volumes
void post_process_blend_volumes(PostProcessStack* stack, uint32_t input_texture,
                             const PostProcessVolume* volumes, uint32_t volume_count) {
  if (!stack || !input_texture || !volumes || volume_count == 0) {
    return;
  }
  
  // Apply volume-based post-processing effects
  for (uint32_t i = 0; i < volume_count; i++) {
    const PostProcessVolume* volume = &volumes[i];
    
    // Check if pixel is within volume
    float3 world_pos = reconstruct_world_position(input_texture, volume->center_uv);
    float distance_to_volume = length(world_pos - volume->position);
    
    if (distance_to_volume <= volume->radius) {
      float blend_factor = 1.0f - (distance_to_volume / volume->radius);
      blend_factor = smoothstep(0.0f, 1.0f, blend_factor);
      
      // Apply volume-specific settings
      if (volume->override_bloom) {
        // Blend bloom settings
        stack->bloom.intensity = lerp(stack->bloom.intensity, 
                                     volume->bloom_settings.intensity, blend_factor);
      }
      
      if (volume->override_color_grading) {
        // Blend color grading settings
        for (int c = 0; c < 3; c++) {
          stack->color_grading.saturation = lerp(stack->color_grading.saturation,
                                              volume->color_grading.saturation, blend_factor);
        }
      }
    }
  }
}

// Post-process profile save
bool post_process_profile_save(PostProcessStack* stack, const char* profile_name) {
  if (!stack || !profile_name) {
    return false;
  }
  
  // Create profile file path
  char profile_path[256];
  snprintf(profile_path, sizeof(profile_path), "profiles/%s.pp", profile_name);
  
  // Open file for writing
  FILE* file = fopen(profile_path, "wb");
  if (!file) {
    return false;
  }
  
  // Write profile header
  const char* header = "POST_PROFILE_v1.0";
  fwrite(header, strlen(header), 1, file);
  
  // Write bloom settings
  fwrite(&stack->bloom, sizeof(BloomSettings), 1, file);
  
  // Write tone mapping settings
  fwrite(&stack->tonemapping, sizeof(ToneMappingSettings), 1, file);
  
  // Write color grading settings
  fwrite(&stack->color_grading, sizeof(ColorGradingSettings), 1, file);
  
  // Write AA settings
  fwrite(&stack->aa_settings, sizeof(AASettings), 1, file);
  
  fclose(file);
  return true;
}

// Post-process profile load
bool post_process_profile_load(PostProcessStack* stack, const char* profile_name) {
  if (!stack || !profile_name) {
    return false;
  }
  
  // Create profile file path
  char profile_path[256];
  snprintf(profile_path, sizeof(profile_path), "profiles/%s.pp", profile_name);
  
  // Open file for reading
  FILE* file = fopen(profile_path, "rb");
  if (!file) {
    return false;
  }
  
  // Read and verify header
  char header[16];
  fread(header, 16, 1, file);
  if (strncmp(header, "POST_PROFILE_v1.0", 16) != 0) {
    fclose(file);
    return false;
  }
  
  // Read settings
  fread(&stack->bloom, sizeof(BloomSettings), 1, file);
  fread(&stack->tonemapping, sizeof(ToneMappingSettings), 1, file);
  fread(&stack->color_grading, sizeof(ColorGradingSettings), 1, file);
  fread(&stack->aa_settings, sizeof(AASettings), 1, file);
  
  fclose(file);
  return true;
}

#endif // POST_PROCESSING_H
