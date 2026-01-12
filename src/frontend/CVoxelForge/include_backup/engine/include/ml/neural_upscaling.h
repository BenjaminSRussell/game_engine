// include/engine/ml/neural_upscaling.h
//
// Purpose: Neural super-resolution upscaling system for real-time rendering
// This system provides AI-powered image upscaling for enhanced visual quality
//
// Key Features:
// - Real-time 720p -> 4K upscaling using neural networks
// - Multiple upscaling models (ESRGAN, SRCNN, FSRCNN)
// - Temporal consistency for stable video upscaling
// - Adaptive quality based on performance budget
// - GPU acceleration with Metal/Vulkan ML
// - Integration with rendering pipeline
//
// Performance Targets:
// - <5ms inference time for 1080p -> 4K upscaling
// - <16MB VRAM usage per upscaling model
// - Maintain 60fps at 4K output resolution
// - Temporal consistency with <1% flicker
//
// Ownership: NeuralUpscaler owns all upscaling models and resources
// Invariants: Input and output textures must be valid, model must be loaded
//
#ifndef NEURAL_UPSCALING_H
#define NEURAL_UPSCALING_H

#include "../../common.h"
#include "ml_core.h"
#include <stdbool.h>
#include <stdint.h>

// ============================================================================
// UPSCALING CONFIGURATION
// ============================================================================

typedef enum {
    UPSCALE_MODEL_ESRGAN = 0,        // Enhanced Super-Resolution GAN
    UPSCALE_MODEL_SRCNN,             // Super-Resolution Convolutional Neural Network
    UPSCALE_MODEL_FSRCNN,            // Fast Super-Resolution Convolutional Neural Network
    UPSCALE_MODEL_REAL_ESRGAN,       // Real-ESRGAN (general purpose)
    UPSCALE_MODEL_LAPSRN,            // Laplacian Pyramid Super-Resolution Network
    UPSCALE_MODEL_CUSTOM             // Custom upscaling model
} UpscaleModel;

typedef enum {
    UPSCALE_QUALITY_ULTRA_FAST = 0,  // 2x upscaling, minimal quality
    UPSCALE_QUALITY_FAST,            // 2x upscaling, good quality
    UPSCALE_QUALITY_BALANCED,         // 3x upscaling, balanced quality/performance
    UPSCALE_QUALITY_HIGH,             // 4x upscaling, high quality
    UPSCALE_QUALITY_ULTRA            // 4x upscaling, maximum quality
} UpscaleQuality;

typedef enum {
    UPSCALE_FILTER_NEAREST = 0,      // Nearest neighbor (fastest)
    UPSCALE_FILTER_BILINEAR,         // Bilinear interpolation
    UPSCALE_FILTER_BICUBIC,          // Bicubic interpolation
    UPSCALE_FILTER_LANCZOS,          // Lanczos resampling
    UPSCALE_FILTER_NEURAL            // Neural network upscaling
} UpscaleFilter;

// ============================================================================
// UPSCALING PARAMETERS
// ============================================================================

typedef struct {
    u32 input_width;
    u32 input_height;
    u32 output_width;
    u32 output_height;
    UpscaleModel model;
    UpscaleQuality quality;
    UpscaleFilter fallback_filter;
    f32 sharpness_strength;
    f32 noise_reduction;
    bool enable_temporal_consistency;
    bool enable_adaptive_quality;
    f32 performance_budget_ms;
} UpscaleConfig;

// ============================================================================
// TEMPORAL CONSISTENCY
// ============================================================================

typedef struct {
    void *previous_frame;
    void *motion_vectors;
    f32 temporal_weight;
    u32 frame_history_count;
    u32 max_history_frames;
    bool is_valid;
} TemporalConsistencyData;

// ============================================================================
// NEURAL UPSCALER
// ============================================================================

typedef struct {
    // ML system integration
    MLSystem *ml_system;
    void *upscale_model;
    
    // Configuration
    UpscaleConfig config;
    
    // GPU resources
    void *input_texture;
    void *output_texture;
    void *intermediate_texture;
    void *compute_command_buffer;
    
    // Temporal consistency
    TemporalConsistencyData temporal_data;
    
    // Performance monitoring
    f64 total_upscale_time;
    u64 total_frames_upscaled;
    f32 average_upscale_time;
    f32 current_fps;
    
    // Adaptive quality
    f32 current_quality_factor;
    u32 quality_adjustment_frames;
    bool is_performance_limited;
    
    bool initialized;
} NeuralUpscaler;

// ============================================================================
// PUBLIC API - SYSTEM MANAGEMENT
// ============================================================================

// Upscaler creation and management
NeuralUpscaler *neural_upscaler_create(MLSystem *ml_system);
void neural_upscaler_destroy(NeuralUpscaler *upscaler);
bool neural_upscaler_initialize(NeuralUpscaler *upscaler, const UpscaleConfig *config);
void neural_upscaler_shutdown(NeuralUpscaler *upscaler);

// Configuration
bool neural_upscaler_set_model(NeuralUpscaler *upscaler, UpscaleModel model);
bool neural_upscaler_set_quality(NeuralUpscaler *upscaler, UpscaleQuality quality);
bool neural_upscaler_set_resolution(NeuralUpscaler *upscaler, u32 input_width, u32 input_height, 
                                   u32 output_width, u32 output_height);
void neural_upscaler_enable_temporal_consistency(NeuralUpscaler *upscaler, bool enable);
void neural_upscaler_enable_adaptive_quality(NeuralUpscaler *upscaler, bool enable);

// ============================================================================
// PUBLIC API - UPSCALING OPERATIONS
// ============================================================================

// Main upscaling function
bool neural_upscaler_process_frame(NeuralUpscaler *upscaler, void *input_texture, void *output_texture);
bool neural_upscaler_process_frame_async(NeuralUpscaler *upscaler, void *input_texture, void *output_texture,
                                        void (*callback)(bool success, void *user_data), void *user_data);

// Batch processing for multiple frames
bool neural_upscaler_process_batch(NeuralUpscaler *upscaler, void **input_textures, void **output_textures, u32 frame_count);

// ============================================================================
// PUBLIC API - TEMPORAL CONSISTENCY
// ============================================================================

// Temporal consistency management
bool neural_upscaler_update_temporal_data(NeuralUpscaler *upscaler, void *current_frame, void *motion_vectors);
void neural_upscaler_reset_temporal_history(NeuralUpscaler *upscaler);
void neural_upscaler_set_temporal_weight(NeuralUpscaler *upscaler, f32 weight);

// ============================================================================
// PUBLIC API - ADAPTIVE QUALITY
// ============================================================================

// Adaptive quality management
void neural_upscaler_update_performance_metrics(NeuralUpscaler *upscaler, f32 frame_time_ms);
void neural_upscaler_adjust_quality(NeuralUpscaler *upscaler);
void neural_upscaler_set_performance_budget(NeuralUpscaler *upscaler, f32 budget_ms);

// ============================================================================
// PUBLIC API - PERFORMANCE MONITORING
// ============================================================================

// Performance statistics
typedef struct {
    f64 average_upscale_time_ms;
    f32 current_fps;
    u64 total_frames_processed;
    f32 gpu_utilization;
    f32 memory_usage_mb;
    UpscaleQuality current_quality;
    bool is_performance_limited;
    f32 quality_factor;
} UpscalePerformanceStats;

UpscalePerformanceStats *neural_upscaler_get_performance_stats(NeuralUpscaler *upscaler);
void neural_upscaler_reset_performance_stats(NeuralUpscaler *upscaler);
void neural_upscaler_print_performance_report(NeuralUpscaler *upscaler);

// ============================================================================
// PUBLIC API - UTILITY FUNCTIONS
// ============================================================================

// Model information
const char *neural_upscaler_get_model_name(UpscaleModel model);
u32 neural_upscaler_get_scale_factor(UpscaleModel model);
f32 neural_upscaler_get_quality_factor(UpscaleQuality quality);
f64 neural_upscaler_get_estimated_time(UpscaleModel model, UpscaleQuality quality, u32 input_pixels);

// Utility functions
bool neural_upscaler_validate_config(const UpscaleConfig *config);
void neural_upscaler_get_recommended_config(u32 target_width, u32 target_height, f32 target_fps,
                                           UpscaleConfig *config);

// ============================================================================
// INTEGRATION HELPERS
// ============================================================================

// Rendering pipeline integration
typedef struct {
    void *render_context;
    void *command_queue;
    void *texture_registry;
    f32 render_scale_factor;
    bool is_hdr;
} RenderIntegrationContext;

bool neural_upscaler_integrate_with_renderer(NeuralUpscaler *upscaler, const RenderIntegrationContext *render_ctx);
void neural_upscaler_update_render_integration(NeuralUpscaler *upscaler, const RenderIntegrationContext *render_ctx);

// ============================================================================
// ERROR HANDLING
// ============================================================================

typedef enum {
    UPSCALE_ERROR_NONE = 0,
    UPSCALE_ERROR_INVALID_PARAMETER,
    UPSCALE_ERROR_MODEL_NOT_LOADED,
    UPSCALE_ERROR_TEXTURE_INVALID,
    UPSCALE_ERROR_UPSCALING_FAILED,
    UPSCALE_ERROR_OUT_OF_MEMORY,
    UPSCALE_ERROR_GPU_UNAVAILABLE,
    UPSCALE_ERROR_PERFORMANCE_LIMITED
} UpscaleError;

const char *neural_upscaler_get_error_string(UpscaleError error);

// ============================================================================
// UTILITY MACROS
// ============================================================================

// Configuration creation macros
#define NEURAL_UPSCALE_CONFIG_2X(input_w, input_h) \
    (UpscaleConfig){ \
        .input_width = input_w, \
        .input_height = input_h, \
        .output_width = input_w * 2, \
        .output_height = input_h * 2, \
        .model = UPSCALE_MODEL_FSRCNN, \
        .quality = UPSCALE_QUALITY_FAST, \
        .fallback_filter = UPSCALE_FILTER_BICUBIC, \
        .sharpness_strength = 1.0f, \
        .noise_reduction = 0.1f, \
        .enable_temporal_consistency = true, \
        .enable_adaptive_quality = true, \
        .performance_budget_ms = 5.0f \
    }

#define NEURAL_UPSCALE_CONFIG_4X(input_w, input_h) \
    (UpscaleConfig){ \
        .input_width = input_w, \
        .input_height = input_h, \
        .output_width = input_w * 4, \
        .output_height = input_h * 4, \
        .model = UPSCALE_MODEL_ESRGAN, \
        .quality = UPSCALE_QUALITY_HIGH, \
        .fallback_filter = UPSCALE_FILTER_LANCZOS, \
        .sharpness_strength = 1.2f, \
        .noise_reduction = 0.15f, \
        .enable_temporal_consistency = true, \
        .enable_adaptive_quality = true, \
        .performance_budget_ms = 8.0f \
    }

// Error checking macros
#define UPSCALE_CHECK_ERROR(condition, error) \
    do { \
        if (!(condition)) { \
            LOG_ERROR("Neural Upscaler Error: %s", neural_upscaler_get_error_string(error)); \
            return error; \
        } \
    } while(0)

#define UPSCALE_CHECK_NULL_PARAM(param) \
    UPSCALE_CHECK_ERROR((param) != NULL, UPSCALE_ERROR_INVALID_PARAMETER)

// Performance macros
#define UPSCALE_START_TIMER(upscaler) \
    f64 start_time = performance_get_time()

#define UPSCALE_END_TIMER(upscaler) \
    do { \
        f64 end_time = performance_get_time(); \
        f64 duration = end_time - start_time; \
        upscaler->total_upscale_time += duration; \
        upscaler->total_frames_upscaled++; \
        upscaler->average_upscale_time = upscaler->total_upscale_time / upscaler->total_frames_upscaled; \
    } while(0)

#endif // NEURAL_UPSCALING_H
