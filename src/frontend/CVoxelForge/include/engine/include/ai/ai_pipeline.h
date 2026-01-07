// include/engine/ai_pipeline.h
//
// Purpose: AI pipeline for efficient 2.5D and 3D model generation with
// systematic improvements based on user feedback.
//
#ifndef AI_PIPELINE_H
#define AI_PIPELINE_H

#include <common.h>
#include "ai_asset_generator.h"

// Pipeline stage
typedef enum {
    PIPELINE_STAGE_INPUT,          // User input processing
    PIPELINE_STAGE_GENERATION,     // AI generation
    PIPELINE_STAGE_OPTIMIZATION,   // Model optimization
    PIPELINE_STAGE_VALIDATION,     // Validation
    PIPELINE_STAGE_IMPROVEMENT,    // AI improvement
    PIPELINE_STAGE_EXPORT          // Export
} PipelineStage;

// Generation mode
typedef enum {
    GENERATION_MODE_3D,            // 3D only
    GENERATION_MODE_2_5D,          // 2.5D only
    GENERATION_MODE_BOTH           // Both 3D and 2.5D
} GenerationMode;

// Pipeline configuration
typedef struct {
    GenerationMode mode;
    AssetStyle style;
    f32 quality_target;            // Target quality (0.0-1.0)
    bool use_improvements;         // Use AI improvements
    u32 max_iterations;            // Max improvement iterations
    bool parallel_generation;      // Generate in parallel
} PipelineConfig;

// Pipeline state
typedef struct {
    PipelineStage current_stage;
    f32 progress;                 // 0.0-1.0
    const char* status_message;
    bool error;
    const char* error_message;
} PipelineState;

// AI Pipeline
typedef struct {
    AIAssetGenerator* generator;
    PipelineConfig config;
    PipelineState state;
    
    // Generation queue
    AssetConfig* pending_generations;
    u32 pending_count;
    
    // Improvement system
    bool improvement_enabled;
    f32 improvement_threshold;    // Quality threshold for improvement
    u32 improvement_iterations;
    
    // Statistics
    u32 total_generations;
    u32 successful_generations;
    f32 average_quality;
    u64 total_generation_time_ms;
    
    bool initialized;
} AIPipeline;

// Lifecycle
bool ai_pipeline_init(AIPipeline* pipeline, AIAssetGenerator* generator);
void ai_pipeline_shutdown(AIPipeline* pipeline);

// Pipeline execution
bool ai_pipeline_generate(AIPipeline* pipeline, const AssetConfig* config, GenerationResult* result);
bool ai_pipeline_generate_async(AIPipeline* pipeline, const AssetConfig* config);
bool ai_pipeline_is_complete(AIPipeline* pipeline);
GenerationResult ai_pipeline_get_result(AIPipeline* pipeline);

// Configuration
void ai_pipeline_set_config(AIPipeline* pipeline, const PipelineConfig* config);
void ai_pipeline_set_improvement_enabled(AIPipeline* pipeline, bool enabled);
void ai_pipeline_set_quality_target(AIPipeline* pipeline, f32 target);

// Improvement system
void ai_pipeline_improve_from_feedback(AIPipeline* pipeline, const UserFeedback* feedback);
void ai_pipeline_auto_improve(AIPipeline* pipeline);
f32 ai_pipeline_get_improvement_progress(AIPipeline* pipeline);

// Statistics
void ai_pipeline_get_statistics(AIPipeline* pipeline, 
                               u32* total, u32* successful, 
                               f32* avg_quality, u64* avg_time);

// Batch processing
bool ai_pipeline_generate_batch(AIPipeline* pipeline, 
                                const AssetConfig* configs, 
                                u32 count,
                                GenerationResult* results);

#endif // AI_PIPELINE_H

