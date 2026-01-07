/*
 * tonemapping_pipeline.h
 * Unified tonemapping post-processing pipeline
 * 
 * Supports multiple tone curve operators (ACES, Reinhard, Filmic, Linear)
 * with exposure control, contrast, saturation, and optional LUT color grading.
 * 
 * Part of the Post-Processing system
 * Advanced 3D Rendering Engine
 */

#ifndef TONEMAPPING_PIPELINE_H
#define TONEMAPPING_PIPELINE_H

#include <stdint.h>
#include <stdbool.h>
#include <stddef.h>

#ifdef __cplusplus
extern "C" {
#endif

/* ============================================================================
 * TYPES
 * ============================================================================ */

typedef enum {
    TONEMAP_OPERATOR_LINEAR = 0,    // No tone mapping (clamp only)
    TONEMAP_OPERATOR_REINHARD,      // Reinhard operator
    TONEMAP_OPERATOR_ACES,          // ACES filmic (default)
    TONEMAP_OPERATOR_FILMIC,        // Unreal/Filmic operator
    TONEMAP_OPERATOR_COUNT
} tonemap_operator_t;

typedef struct tonemapping_settings {
    // Operator selection
    tonemap_operator_t operator_type;
    
    // Exposure
    float exposure;                 // Linear exposure multiplier (default: 1.0)
    bool auto_exposure;             // Enable auto-exposure (default: false)
    
    // Color adjustments (applied post-tonemap)
    float contrast;                 // Contrast adjustment (default: 1.0)
    float saturation;               // Saturation adjustment (default: 1.0)
    
    // LUT
    bool use_lut;                   // Enable LUT color grading (default: false)
    
    // Gamma
    float gamma;                    // Output gamma (default: 2.2)
} tonemapping_settings_t;

typedef struct tonemapping_pipeline tonemapping_pipeline_t;

/* ============================================================================
 * API
 * ============================================================================ */

/* Initialization */
int tonemapping_pipeline_init(void);
void tonemapping_pipeline_shutdown(void);

/* Lifecycle */
tonemapping_pipeline_t* tonemapping_pipeline_create(const tonemapping_settings_t* settings);
void tonemapping_pipeline_destroy(tonemapping_pipeline_t* pipeline);

/* Configuration */
void tonemapping_pipeline_set_settings(tonemapping_pipeline_t* pipeline, 
                                      const tonemapping_settings_t* settings);
void tonemapping_pipeline_get_settings(const tonemapping_pipeline_t* pipeline, 
                                      tonemapping_settings_t* out_settings);
tonemapping_settings_t tonemapping_pipeline_default_settings(void);

/* LUT Management */
int tonemapping_pipeline_load_lut(tonemapping_pipeline_t* pipeline, 
                                 const char* filepath);
void tonemapping_pipeline_set_lut_blend(tonemapping_pipeline_t* pipeline, 
                                       float blend_factor);

/* Processing
 * 
 * Apply tone mapping to HDR input
 * 
 * input_hdr: Input HDR texture (RGB float format)
 * output_ldr: Output LDR texture (RGB float format, 0-1 range)
 * pixel_count: Number of pixels
 */
int tonemapping_pipeline_process(tonemapping_pipeline_t* pipeline,
                                const float* input_hdr,
                                float* output_ldr,
                                uint32_t pixel_count);

/* Statistics */
void tonemapping_pipeline_get_stats(const tonemapping_pipeline_t* pipeline,
                                   float* out_avg_luminance,
                                   float* out_current_exposure,
                                   float* out_last_process_time_ms);

/* Utility - Convert exposure stops (EV) to linear multiplier */
float tonemapping_ev_to_linear(float ev);
float tonemapping_linear_to_ev(float linear);

#ifdef __cplusplus
}
#endif

#endif /* TONEMAPPING_PIPELINE_H */
