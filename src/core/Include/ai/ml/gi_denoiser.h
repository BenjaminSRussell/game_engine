// include/engine/ml/gi_denoiser.h
//
// Purpose: ML-based denoising system for ray-traced global illumination
// This system uses neural networks to reduce noise in ray-traced lighting
//
// Key Features:
// - Real-time denoising of ray-traced global illumination
// - Temporal accumulation for stable results
// - Edge-aware filtering to preserve detail
// - Adaptive denoising based on noise levels
// - GPU acceleration with Metal/Vulkan ML
// - Integration with ray tracing pipeline
//
// Performance Targets:
// - <3ms denoising time for 1080p GI buffer
// - <20MB VRAM usage for denoising models
// - Maintain 60fps with ray tracing enabled
// - <1% detail loss in denoised output
//
// Ownership: GIDenoiser owns all denoising models and resources
// Invariants: Input GI buffer must be valid, model must be loaded
//

#ifndef GI_DENOISER_H
#define GI_DENOISER_H

#include <common.h>
#include "include/ai/ml/ml_core.h"
#include <stdbool.h>
#include <stdint.h>

typedef enum {
    DENOISE_MODEL_KPCN = 0,         // Kernel-Predicting Convolutional Network
    DENOISE_MODEL_SVGF,             // Spatiotemporal Variance-Guided Filtering
    DENOISE_MODEL_NRM,              // Neural Radiance Cache Denoiser
    DENOISE_MODEL_ATTR,             // Attribute-based denoising
    DENOISE_MODEL_CUSTOM            // Custom denoising model
} DenoiseModel;

typedef struct {
    u32 width, height;
    void *noisy_gi_buffer;
    void *albedo_buffer;
    void *normal_buffer;
    void *depth_buffer;
    void *motion_vector_buffer;
    void *previous_frame;
    f32 temporal_alpha;
    bool use_temporal_accumulation;
} GIDenoiseInput;

typedef struct {
    void *denoised_gi_buffer;
    f32 noise_reduction_ratio;
    f32 detail_preservation_score;
    f64 processing_time_ms;
    bool is_valid;
} GIDenoiseOutput;

typedef struct {
    MLSystem *ml_system;
    void *denoise_model;
    DenoiseModel model_type;
    GIDenoiseInput input;
    GIDenoiseOutput output;
    void *compute_resources;
    bool initialized;
} GIDenoiser;

GIDenoiser *gi_denoiser_create(MLSystem *ml_system);
void gi_denoiser_destroy(GIDenoiser *denoiser);
bool gi_denoiser_initialize(GIDenoiser *denoiser, DenoiseModel model);
bool gi_denoise_frame(GIDenoiser *denoiser, const GIDenoiseInput *input, GIDenoiseOutput *output);

#endif // GI_DENOISER_H
