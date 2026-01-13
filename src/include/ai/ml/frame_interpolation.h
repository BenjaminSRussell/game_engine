// include/engine/ml/frame_interpolation.h
//
// Purpose: Neural frame interpolation system for smooth 30fps->60fps conversion
// This system uses machine learning to generate intermediate frames for smoother gameplay
//
// Key Features:
// - Real-time 30fps to 60fps frame interpolation using neural networks
// - Motion-aware interpolation with optical flow estimation
// - Artifact reduction for clean intermediate frames
// - Adaptive quality based on motion complexity
// - GPU acceleration with Metal/Vulkan ML
// - Integration with rendering pipeline
//
// Performance Targets:
// - <8ms interpolation time for 1080p frames
// - <50MB VRAM usage for interpolation models
// - Maintain stable 60fps output at 1080p
// - <2% frame interpolation artifacts
//
// Ownership: FrameInterpolator owns all interpolation models and resources
// Invariants: Input frames must be valid, model must be loaded
//

#ifndef FRAME_INTERPOLATION_H
#define FRAME_INTERPOLATION_H

#include "include/common.h"
#include <ai/ml/ml_core.h>
#include "engine/include/core/logger.h"
#include <stdbool.h>
#include <stdint.h>

// ============================================================================
// INTERPOLATION CONFIGURATION
// ============================================================================

typedef enum {
    INTERPOL_MODEL_RIFE = 0,          // Real-time Intermediate Flow Estimation
    INTERPOL_MODEL_SuperSloMo,       // Super SloMo network
    INTERPOL_MODEL_DAIN,             // Depth-Aware Video Frame Interpolation
    INTERPOL_MODEL_MEMC,             // Motion Estimation and Compensation
    INTERPOL_MODEL_CUSTOM            // Custom interpolation model
} InterpolationModel;

typedef enum {
    INTERPOL_QUALITY_ULTRA_FAST = 0,  // Fastest, lower quality
    INTERPOL_QUALITY_FAST,            // Fast, good quality
    INTERPOL_QUALITY_BALANCED,         // Balanced quality/performance
    INTERPOL_QUALITY_HIGH,             // High quality
    INTERPOL_QUALITY_ULTRA            // Maximum quality
} InterpolationQuality;

typedef enum {
    INTERPOL_MODE_BILINEAR = 0,       // Simple bilinear interpolation
    INTERPOL_MODE_OPTICAL_FLOW,        // Optical flow-based
    INTERPOL_MODE_NEURAL,              // Neural network interpolation
    INTERPOL_MODE_HYBRID               // Hybrid approach
} InterpolationMode;

// ============================================================================
// INTERPOLATION PARAMETERS
// ============================================================================

typedef struct {
    u32 input_width;
    u32 input_height;
    u32 output_width;
    u32 output_height;
    InterpolationModel model;
    InterpolationQuality quality;
    InterpolationMode fallback_mode;
    f32 motion_threshold;
    f32 artifact_reduction_strength;
    bool enable_temporal_consistency;
    bool enable_adaptive_quality;
    f32 performance_budget_ms;
} InterpolationConfig;

// ============================================================================
// MOTION ESTIMATION
// ============================================================================

typedef struct {
    void *flow_field_x;              // Horizontal optical flow
    void *flow_field_y;              // Vertical optical flow
    u32 width;
    u32 height;
    f32 confidence_threshold;
    bool is_valid;
} OpticalFlowData;

typedef struct {
    void *previous_frame;
    void *current_frame;
    void *next_frame;
    OpticalFlowData *forward_flow;
    OpticalFlowData *backward_flow;
    f32 motion_magnitude;
    f32 motion_complexity;
    bool is_high_motion;
} MotionEstimationData;

// ============================================================================
// FRAME INTERPOLATOR
// ============================================================================

typedef struct {
    // ML system integration
    MLSystem *ml_system;
    void *interpolation_model;
    
    // Configuration
    InterpolationConfig config;
    
    // Frame buffers
    void *input_frames[3];           // Previous, current, next frames
    void *interpolated_frame;
    void *intermediate_frames[2];     // Multiple intermediate frames
    
    // Motion estimation
    MotionEstimationData motion_data;
    
    // GPU resources
    void *compute_command_buffer;
    void *texture_cache;
    
    // Performance monitoring
    f64 total_interpolation_time;
    u64 total_frames_interpolated;
    f32 average_interpolation_time;
    f32 current_fps;
    
    // Adaptive quality
    f32 current_quality_factor;
    u32 quality_adjustment_frames;
    bool is_performance_limited;
    
    // Temporal consistency
    void *frame_history[4];
    u32 history_index;
    f32 temporal_weight;
    
    bool initialized;
} FrameInterpolator;

// ============================================================================
// PUBLIC API - SYSTEM MANAGEMENT
// ============================================================================

// Interpolator creation and management
FrameInterpolator *frame_interpolator_create(MLSystem *ml_system);
void frame_interpolator_destroy(FrameInterpolator *interpolator);
bool frame_interpolator_initialize(FrameInterpolator *interpolator, const InterpolationConfig *config);
void frame_interpolator_shutdown(FrameInterpolator *interpolator);

// Configuration
bool frame_interpolator_set_model(FrameInterpolator *interpolator, InterpolationModel model);
bool frame_interpolator_set_quality(FrameInterpolator *interpolator, InterpolationQuality quality);
bool frame_interpolator_set_resolution(FrameInterpolator *interpolator, u32 input_width, u32 input_height,
                                      u32 output_width, u32 output_height);
void frame_interpolator_enable_temporal_consistency(FrameInterpolator *interpolator, bool enable);
void frame_interpolator_enable_adaptive_quality(FrameInterpolator *interpolator, bool enable);

// ============================================================================
// PUBLIC API - INTERPOLATION OPERATIONS
// ============================================================================

// Main interpolation function
bool frame_interpolator_process_frames(FrameInterpolator *interpolator, void *prev_frame, void *curr_frame, 
                                       void *next_frame, void *output_frame);
bool frame_interpolator_process_frames_async(FrameInterpolator *interpolator, void *prev_frame, void *curr_frame,
                                            void *next_frame, void *output_frame,
                                            void (*callback)(bool success, void *user_data), void *user_data);

// Single frame interpolation (for real-time)
bool frame_interpolator_interpolate_frame(FrameInterpolator *interpolator, void *prev_frame, void *curr_frame,
                                         void *output_frame, f32 interpolation_factor);

// ============================================================================
// PUBLIC API - MOTION ESTIMATION
// ============================================================================

// Motion estimation operations
bool frame_interpolator_estimate_motion(FrameInterpolator *interpolator, void *prev_frame, void *curr_frame);
bool frame_interpolator_estimate_bidirectional_motion(FrameInterpolator *interpolator, void *prev_frame, 
                                                     void *curr_frame, void *next_frame);
OpticalFlowData *frame_interpolator_get_optical_flow(FrameInterpolator *interpolator);

// Motion analysis
f32 frame_interpolator_get_motion_magnitude(FrameInterpolator *interpolator);
f32 frame_interpolator_get_motion_complexity(FrameInterpolator *interpolator);
bool frame_interpolator_is_high_motion_scene(FrameInterpolator *interpolator);

// ============================================================================
// PUBLIC API - ADAPTIVE QUALITY
// ============================================================================

// Adaptive quality management
void frame_interpolator_update_performance_metrics(FrameInterpolator *interpolator, f32 frame_time_ms);
void frame_interpolator_adjust_quality(FrameInterpolator *interpolator);
void frame_interpolator_set_performance_budget(FrameInterpolator *interpolator, f32 budget_ms);

// Quality analysis
f32 frame_interpolator_get_quality_factor(FrameInterpolator *interpolator);
bool frame_interpolator_should_skip_interpolation(FrameInterpolator *interpolator);

// ============================================================================
// PUBLIC API - TEMPORAL CONSISTENCY
// ============================================================================

// Temporal consistency management
bool frame_interpolator_update_frame_history(FrameInterpolator *interpolator, void *current_frame);
void frame_interpolator_reset_frame_history(FrameInterpolator *interpolator);
void frame_interpolator_set_temporal_weight(FrameInterpolator *interpolator, f32 weight);

// Artifact reduction
bool frame_interpolator_reduce_artifacts(FrameInterpolator *interpolator, void *frame);
void frame_interpolator_set_artifact_reduction_strength(FrameInterpolator *interpolator, f32 strength);

// ============================================================================
// PUBLIC API - PERFORMANCE MONITORING
// ============================================================================

// Performance statistics
typedef struct {
    f64 average_interpolation_time_ms;
    f32 current_fps;
    u64 total_frames_processed;
    f32 gpu_utilization;
    f32 memory_usage_mb;
    InterpolationQuality current_quality;
    bool is_performance_limited;
    f32 quality_factor;
    f32 motion_magnitude;
    f32 artifact_rate;
} InterpolationPerformanceStats;

InterpolationPerformanceStats *frame_interpolator_get_performance_stats(FrameInterpolator *interpolator);
void frame_interpolator_reset_performance_stats(FrameInterpolator *interpolator);
void frame_interpolator_print_performance_report(FrameInterpolator *interpolator);

// ============================================================================
// PUBLIC API - UTILITY FUNCTIONS
// ============================================================================

// Model information
const char *frame_interpolator_get_model_name(InterpolationModel model);
f32 frame_interpolator_quality_to_factor(InterpolationQuality quality);

// ... (in later implementation)


f64 frame_interpolator_get_estimated_time(InterpolationModel model, InterpolationQuality quality, u32 input_pixels);

// Utility functions
bool frame_interpolator_validate_config(const InterpolationConfig *config);
void frame_interpolator_get_recommended_config(u32 input_width, u32 input_height, f32 target_fps,
                                              InterpolationConfig *config);

// ============================================================================
// INTEGRATION HELPERS
// ============================================================================

// Rendering pipeline integration
typedef struct {
    void *render_context;
    void *command_queue;
    void *frame_buffer_registry;
    f32 render_scale_factor;
    bool is_hdr;
    u32 color_space;
} RenderIntegrationContext;

bool frame_interpolator_integrate_with_renderer(FrameInterpolator *interpolator, const RenderIntegrationContext *render_ctx);
void frame_interpolator_update_render_integration(FrameInterpolator *interpolator, const RenderIntegrationContext *render_ctx);

// ============================================================================
// ERROR HANDLING
// ============================================================================

typedef enum {
    INTERPOL_ERROR_NONE = 0,
    INTERPOL_ERROR_INVALID_PARAMETER,
    INTERPOL_ERROR_MODEL_NOT_LOADED,
    INTERPOL_ERROR_FRAME_INVALID,
    INTERPOL_ERROR_INTERPOLATION_FAILED,
    INTERPOL_ERROR_OUT_OF_MEMORY,
    INTERPOL_ERROR_GPU_UNAVAILABLE,
    INTERPOL_ERROR_MOTION_ESTIMATION_FAILED,
    INTERPOL_ERROR_PERFORMANCE_LIMITED
} InterpolationError;

const char *frame_interpolator_get_error_string(InterpolationError error);

// ============================================================================
// UTILITY MACROS
// ============================================================================

// Configuration creation macros
#define FRAME_INTERPOL_CONFIG_1080P_60FPS() \
    (InterpolationConfig){ \
        .input_width = 1920, \
        .input_height = 1080, \
        .output_width = 1920, \
        .output_height = 1080, \
        .model = INTERPOL_MODEL_RIFE, \
        .quality = INTERPOL_QUALITY_BALANCED, \
        .fallback_mode = INTERPOL_MODE_OPTICAL_FLOW, \
        .motion_threshold = 2.0f, \
        .artifact_reduction_strength = 0.3f, \
        .enable_temporal_consistency = true, \
        .enable_adaptive_quality = true, \
        .performance_budget_ms = 8.0f \
    }

#define FRAME_INTERPOL_CONFIG_4K_60FPS() \
    (InterpolationConfig){ \
        .input_width = 3840, \
        .input_height = 2160, \
        .output_width = 3840, \
        .output_height = 2160, \
        .model = INTERPOL_MODEL_DAIN, \
        .quality = INTERPOL_QUALITY_HIGH, \
        .fallback_mode = INTERPOL_MODE_NEURAL, \
        .motion_threshold = 1.5f, \
        .artifact_reduction_strength = 0.4f, \
        .enable_temporal_consistency = true, \
        .enable_adaptive_quality = true, \
        .performance_budget_ms = 12.0f \
    }

// Error checking macros
#define INTERPOL_CHECK_ERROR(condition, error) \
    do { \
        if (!(condition)) { \
            LOG_ERROR("Frame Interpolator Error: %s", frame_interpolator_get_error_string(error)); \
            return error; \
        } \
    } while(0)

#define INTERPOL_CHECK_NULL_PARAM(param) \
    INTERPOL_CHECK_ERROR((param) != NULL, INTERPOL_ERROR_INVALID_PARAMETER)

#define INTERPOL_CHECK_NULL_PARAM_PTR(param) \
    do { \
        if ((param) == NULL) { \
            LOG_ERROR("Frame Interpolator Error: %s", frame_interpolator_get_error_string(INTERPOL_ERROR_INVALID_PARAMETER)); \
            return NULL; \
        } \
    } while(0)

#define INTERPOL_CHECK_NULL_PARAM_BOOL(param) \
    do { \
        if ((param) == NULL) { \
            LOG_ERROR("Frame Interpolator Error: %s", frame_interpolator_get_error_string(INTERPOL_ERROR_INVALID_PARAMETER)); \
            return false; \
        } \
    } while(0)

// Performance macros
#include <time.h>
#define INTERPOL_START_TIMER(interpolator) \
    clock_t start_time = clock()

#define INTERPOL_END_TIMER(interpolator) \
    do { \
        clock_t end_time = clock(); \
        f64 duration = ((f64)(end_time - start_time)) / CLOCKS_PER_SEC; \
        interpolator->total_interpolation_time += duration; \
        interpolator->total_frames_interpolated++; \
        interpolator->average_interpolation_time = interpolator->total_interpolation_time / interpolator->total_frames_interpolated; \
    } while(0)


#endif // FRAME_INTERPOLATION_H
