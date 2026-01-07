#include "postprocess/color_grading.h"
#include <include/math/math.h>
#include <string.h>
#include <stdio.h>

/**
 * Color Grading System
 * Supports 3D LUT, lift/gamma/gain, saturation, and color wheels
 */

// Color grading parameters (already defined in existing files)
typedef struct {
    // Lift/Gamma/Gain (shadows/midtones/highlights)
    float lift[3];
    float gamma[3];
    float gain[3];
    
    // Saturation
    float saturation;
    float vibrance;
    
    // Contrast
    float contrast;
    
    // Color wheels
    float shadows_tint[3];
    float midtones_tint[3];
    float highlights_tint[3];
    
    // Temperature and tint
    float temperature;  // -1 to 1 (blue to orange)
    float tint;         // -1 to 1 (green to magenta)
    
    // 3D LUT
    uint32_t lut_texture_id;
    float lut_intensity;
    
    // Exposure
    float exposure;
} ColorGradingParams;

// Initialize default parameters
ColorGradingParams color_grading_default(void) {
    ColorGradingParams params = {0};
    
    // Neutral lift/gamma/gain
    params.lift[0] = params.lift[1] = params.lift[2] = 0.0f;
    params.gamma[0] = params.gamma[1] = params.gamma[2] = 1.0f;
    params.gain[0] = params.gain[1] = params.gain[2] = 1.0f;
    
    // Neutral saturation/contrast
    params.saturation = 1.0f;
    params.vibrance = 0.0f;
    params.contrast = 1.0f;
    
    // No tint
    params.shadows_tint[0] = params.shadows_tint[1] = params.shadows_tint[2] = 0.0f;
    params.midtones_tint[0] = params.midtones_tint[1] = params.midtones_tint[2] = 0.0f;
    params.highlights_tint[0] = params.highlights_tint[1] = params.highlights_tint[2] = 0.0f;
    
    params.temperature = 0.0f;
    params.tint = 0.0f;
    
    params.lut_texture_id = 0;
    params.lut_intensity = 1.0f;
    
    params.exposure = 0.0f;
    
    return params;
}

// Helper: clamp
static inline float clamp01(float x) {
    return x < 0.0f ? 0.0f : (x > 1.0f ? 1.0f : x);
}

// Helper: lerp
static inline float lerp(float a, float b, float t) {
    return a + t * (b - a);
}

// RGB to Luminance
static float rgb_to_luminance(float r, float g, float b) {
    return 0.2126f * r + 0.7152f * g + 0.0722f * b;
}

// Apply lift/gamma/gain
void apply_lift_gamma_gain(float* color, const ColorGradingParams* params) {
    for (int i = 0; i < 3; i++) {
        // Lift (shadows)
        color[i] = color[i] + params->lift[i];
        
        // Gamma (midtones)
        if (params->gamma[i] != 1.0f && color[i] > 0.0f) {
            color[i] = powf(color[i], 1.0f / params->gamma[i]);
        }
        
        // Gain (highlights)
        color[i] = color[i] * params->gain[i];
        
        // Clamp
        color[i] = clamp01(color[i]);
    }
}

// Apply saturation
void apply_saturation(float* color, float saturation) {
    float luma = rgb_to_luminance(color[0], color[1], color[2]);
    
    color[0] = lerp(luma, color[0], saturation);
    color[1] = lerp(luma, color[1], saturation);
    color[2] = lerp(luma, color[2], saturation);
}

// Apply contrast
void apply_contrast(float* color, float contrast) {
    const float midpoint = 0.5f;
    
    for (int i = 0; i < 3; i++) {
        color[i] = midpoint + (color[i] - midpoint) * contrast;
        color[i] = clamp01(color[i]);
    }
}

// Apply color wheels (simplified)
void apply_color_wheels(float* color, const ColorGradingParams* params) {
    float luma = rgb_to_luminance(color[0], color[1], color[2]);
    
    // Determine weights for shadows/midtones/highlights
    float shadow_weight = 1.0f - clamp01(luma * 2.0f);
    float highlight_weight = clamp01((luma - 0.5f) * 2.0f);
    float midtone_weight = 1.0f - shadow_weight - highlight_weight;
    
    // Apply tints
    for (int i = 0; i < 3; i++) {
        color[i] += params->shadows_tint[i] * shadow_weight;
        color[i] += params->midtones_tint[i] * midtone_weight;
        color[i] += params->highlights_tint[i] * highlight_weight;
        color[i] = clamp01(color[i]);
    }
}

// Apply temperature and tint
void apply_temperature_tint(float* color, float temperature, float tint) {
    // Temperature: blue(-1) to orange(+1)
    if (temperature != 0.0f) {
        color[2] += -temperature * 0.1f; // Blue channel
        color[0] += temperature * 0.1f;   // Red channel
    }
    
    // Tint: green(-1) to magenta(+1)
    if (tint != 0.0f) {
        color[1] += -tint * 0.1f;  // Green channel
        color[0] += tint * 0.05f;  // Red (magenta has red)
        color[2] += tint * 0.05f;  // Blue (magenta has blue)
    }
    
    // Clamp
    for (int i = 0; i < 3; i++) {
        color[i] = clamp01(color[i]);
    }
}

// Full color grading pipeline
void apply_color_grading(float* color, const ColorGradingParams* params) {
    // Exposure
    if (params->exposure != 0.0f) {
        float exposure_scale = powf(2.0f, params->exposure);
        color[0] *= exposure_scale;
        color[1] *= exposure_scale;
        color[2] *= exposure_scale;
    }
    
    // Lift/Gamma/Gain
    apply_lift_gamma_gain(color, params);
    
    // Saturation
    if (params->saturation != 1.0f) {
        apply_saturation(color, params->saturation);
    }
    
    // Contrast
    if (params->contrast != 1.0f) {
        apply_contrast(color, params->contrast);
    }
    
    // Color wheels
    apply_color_wheels(color, params);
    
    // Temperature/Tint
    if (params->temperature != 0.0f || params->tint != 0.0f) {
        apply_temperature_tint(color, params->temperature, params->tint);
    }
    
    // TODO: 3D LUT application would go here
    // This requires texture sampling which would be done in a shader
}

// Preset: Warm
ColorGradingParams color_grading_preset_warm(void) {
    ColorGradingParams params = color_grading_default();
    params.temperature = 0.3f;
    params.gain[0] = 1.1f;  // Boost reds
    params.gain[2] = 0.9f;  // Reduce blues
    return params;
}

// Preset: Cool
ColorGradingParams color_grading_preset_cool(void) {
    ColorGradingParams params = color_grading_default();
    params.temperature = -0.3f;
    params.gain[2] = 1.1f;  // Boost blues
    params.gain[0] = 0.9f;  // Reduce reds
    return params;
}

// Preset: Cinematic
ColorGradingParams color_grading_preset_cinematic(void) {
    ColorGradingParams params = color_grading_default();
    params.contrast = 1.15f;
    params.saturation = 0.9f;
    params.lift[2] = 0.05f;  // Slight blue crush in shadows
    params.highlights_tint[0] = 0.05f;  // Warm highlights
    return params;
}

// Preset: Vibrant
ColorGradingParams color_grading_preset_vibrant(void) {
    ColorGradingParams params = color_grading_default();
    params.saturation = 1.3f;
    params.vibrance = 0.2f;
    params.contrast = 1.1f;
    return params;
}
