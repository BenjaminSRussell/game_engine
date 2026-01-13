/*
 * upscaler.h
 *
 * Super-resolution upscaling system (DLSS/FSR equivalent)
 * Part of the Advanced 3D Rendering Engine
 *
 * Copyright (c) 2024 Game Engine Project
 * Licensed under MIT License
 *
 * Provides functionality for:
 * - Frame upscaling from lower to higher resolution
 * - Temporal stability and anti-aliasing
 * - Motion vector integration
 * - Quality level selection (DLSS native-like quality)
 */

#ifndef UPSCALER_H
#define UPSCALER_H

#include <stdint.h>
#include <stdbool.h>
#include <stddef.h>

#ifdef __cplusplus
extern "C" {
#endif

/* ============================================================================
 * TYPE DEFINITIONS
 * ============================================================================ */

/* Upscaling quality modes */
typedef enum {
    UPSCALE_QUALITY_ULTRA = 0,      /* Highest quality, lowest performance */
    UPSCALE_QUALITY_QUALITY,         /* High quality */
    UPSCALE_QUALITY_BALANCED,        /* Balanced */
    UPSCALE_QUALITY_PERFORMANCE,     /* Performance priority */
    UPSCALE_QUALITY_ULTRA_PERFORMANCE /* Fastest */
} UpscalingQuality;

/* Upscaling algorithm */
typedef enum {
    UPSCALE_ALGORITHM_FSR = 0,       /* FidelityFX Super Resolution */
    UPSCALE_ALGORITHM_DLSS,          /* NVIDIA DLSS-like */
    UPSCALE_ALGORITHM_NATIVE,        /* Native upscaling */
    UPSCALE_ALGORITHM_XESS            /* Intel XeSS-like */
} UpscalingAlgorithm;

/* Color format */
typedef enum {
    COLOR_FORMAT_R8G8B8A8 = 0,
    COLOR_FORMAT_R16G16B16A16,
    COLOR_FORMAT_R32G32B32A32,
} ColorFormat;

/* Frame texture */
typedef struct {
    void* data;
    uint32_t width;
    uint32_t height;
    ColorFormat format;
    size_t stride;
} FrameTexture;

/* Motion vectors */
typedef struct {
    float x, y;  /* 2D motion in pixels */
} MotionVector;

/* Motion frame */
typedef struct {
    MotionVector* vectors;
    uint32_t width;
    uint32_t height;
} MotionFrame;

/* Upscaling configuration */
typedef struct {
    UpscalingAlgorithm algorithm;
    UpscalingQuality quality;
    bool use_motion_vectors;
    bool enable_sharpening;
    float sharpening_strength;
    bool enable_anti_aliasing;
    uint32_t output_width;
    uint32_t output_height;
} UpscalingConfig;

/* Upscaler context */
typedef struct Upscaler Upscaler;

/* Upscaling statistics */
typedef struct {
    uint32_t frames_upscaled;
    double average_upscale_time_ms;
    double total_upscale_time_ms;
    float upscale_ratio;
    uint64_t pixels_processed;
} UpscalingStats;

/* ============================================================================
 * API FUNCTIONS
 * ============================================================================ */

/* Create and destroy upscaler */
Upscaler* upscaler_create(const UpscalingConfig* config);
void upscaler_destroy(Upscaler* upscaler);

/* Upscaling operations */
int upscaler_process_frame(
    Upscaler* upscaler,
    const FrameTexture* input,
    FrameTexture* output
);

int upscaler_process_frame_with_motion(
    Upscaler* upscaler,
    const FrameTexture* input,
    const MotionFrame* motion,
    FrameTexture* output
);

/* Reconfigure upscaler */
int upscaler_set_quality(Upscaler* upscaler, UpscalingQuality quality);
int upscaler_set_algorithm(Upscaler* upscaler, UpscalingAlgorithm algorithm);
int upscaler_set_output_resolution(Upscaler* upscaler, uint32_t width, uint32_t height);

/* Get recommended input resolution for quality mode */
void upscaler_get_recommended_input_resolution(
    uint32_t output_width,
    uint32_t output_height,
    UpscalingQuality quality,
    uint32_t* recommended_width,
    uint32_t* recommended_height
);

/* Quality metrics */
typedef struct {
    float perceived_quality;  /* 0-1, higher is better */
    float sharpness;          /* 0-1 */
    float temporal_stability; /* 0-1 */
    float artifact_level;     /* 0-1, lower is better */
} QualityMetrics;

int upscaler_get_quality_metrics(
    Upscaler* upscaler,
    QualityMetrics* metrics
);

/* Statistics */
int upscaler_get_statistics(
    Upscaler* upscaler,
    UpscalingStats* stats
);

void upscaler_reset_statistics(Upscaler* upscaler);

/* Temporal accumulation */
int upscaler_accumulate_frame(
    Upscaler* upscaler,
    const FrameTexture* frame
);

int upscaler_reset_accumulation(Upscaler* upscaler);

/* Utility functions */
const char* upscaler_quality_to_string(UpscalingQuality quality);
const char* upscaler_algorithm_to_string(UpscalingAlgorithm algorithm);
float upscaler_get_quality_ratio(UpscalingQuality quality);
uint32_t upscaler_get_algorithm_requirements(UpscalingAlgorithm algorithm);

/* Check if algorithm is available */
bool upscaler_is_algorithm_available(UpscalingAlgorithm algorithm);

#ifdef __cplusplus
}
#endif

#endif // UPSCALER_H
