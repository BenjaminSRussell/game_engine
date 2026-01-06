#ifndef COLOR_GRADING_H
#define COLOR_GRADING_H

#include <stdint.h>

/**
 * Color Grading System
 * Professional color correction and grading tools
 */

typedef struct {
    // Lift/Gamma/Gain
    float lift[3];
    float gamma[3];
    float gain[3];
    
    // Saturation/Vibrance
    float saturation;
    float vibrance;
    
    // Contrast
    float contrast;
    
    // Color wheels (shadows/midtones/highlights)
    float shadows_tint[3];
    float midtones_tint[3];
    float highlights_tint[3];
    
    // Temperature and tint
    float temperature;
    float tint;
    
    // 3D LUT
    uint32_t lut_texture_id;
    float lut_intensity;
    
    // Exposure
    float exposure;
} ColorGradingParams;

// Initialize
ColorGradingParams color_grading_default(void);

// Apply pipeline
void apply_color_grading(float* color, const ColorGradingParams* params);

// Individual operations
void apply_lift_gamma_gain(float* color, const ColorGradingParams* params);
void apply_saturation(float* color, float saturation);
void apply_contrast(float* color, float contrast);
void apply_color_wheels(float* color, const ColorGradingParams* params);
void apply_temperature_tint(float* color, float temperature, float tint);

// Presets
ColorGradingParams color_grading_preset_warm(void);
ColorGradingParams color_grading_preset_cool(void);
ColorGradingParams color_grading_preset_cinematic(void);
ColorGradingParams color_grading_preset_vibrant(void);

#endif // COLOR_GRADING_H
