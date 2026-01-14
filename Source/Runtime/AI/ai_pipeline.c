// src/engine/ai/ai_pipeline.c
//
// Purpose: AI pipeline implementation for efficient 2.5D and 3D model
// generation.
//
#include <ai/ai_pipeline.h>
#include "engine/include/core/logger.h"
#include <stdlib.h>
#include <string.h>

// Initialize AI pipeline
bool ai_pipeline_init(AIPipeline *pipeline, AIAssetGenerator *generator) {
  if (!pipeline || !generator) {
    LOG_ERROR("Invalid pipeline parameters");
    return false;
  }

  memset(pipeline, 0, sizeof(AIPipeline));
  pipeline->generator = generator;

  // Default config
  pipeline->config.mode = GENERATION_MODE_BOTH;
  pipeline->config.style = ASSET_STYLE_REALISTIC;
  pipeline->config.quality_target = 0.7f;
  pipeline->config.use_improvements = true;
  pipeline->config.max_iterations = 3;
  pipeline->config.parallel_generation = false;

  // Initialize state
  pipeline->state.current_stage = PIPELINE_STAGE_INPUT;
  pipeline->state.progress = 0.0f;
  pipeline->state.status_message = "Ready";

  // Initialize queues
  pipeline->pending_generations =
      (AssetConfig *)calloc(16, sizeof(AssetConfig));
  pipeline->pending_count = 0;

  pipeline->improvement_enabled = true;
  pipeline->improvement_threshold = 0.6f;

  pipeline->initialized = true;

  LOG_INFO("AI Pipeline initialized");
  return true;
}

void ai_pipeline_shutdown(AIPipeline *pipeline) {
  if (!pipeline || !pipeline->initialized) {
    return;
  }

  if (pipeline->pending_generations) {
    free(pipeline->pending_generations);
  }

  memset(pipeline, 0, sizeof(AIPipeline));
  LOG_INFO("AI Pipeline shut down");
}

// Generate asset through pipeline
bool ai_pipeline_generate(AIPipeline *pipeline, const AssetConfig *config,
                          GenerationResult *result) {
  if (!pipeline || !config || !result) {
    return false;
  }

  pipeline->state.current_stage = PIPELINE_STAGE_INPUT;
  pipeline->state.progress = 0.1f;
  pipeline->state.status_message = "Processing input";

  // Stage 1: Input processing
  AssetConfig processed_config = *config;
  pipeline->state.progress = 0.2f;

  // Stage 2: Generation
  pipeline->state.current_stage = PIPELINE_STAGE_GENERATION;
  pipeline->state.progress = 0.4f;
  pipeline->state.status_message = "Generating asset";

  *result = ai_generate_asset(pipeline->generator, &processed_config);

  if (!result->success) {
    pipeline->state.error = true;
    pipeline->state.error_message = result->error_message;
    return false;
  }

  // Stage 3: Optimization
  pipeline->state.current_stage = PIPELINE_STAGE_OPTIMIZATION;
  pipeline->state.progress = 0.6f;
  pipeline->state.status_message = "Optimizing model";

  // Stage 4: Validation
  pipeline->state.current_stage = PIPELINE_STAGE_VALIDATION;
  pipeline->state.progress = 0.8f;
  pipeline->state.status_message = "Validating";

  // Stage 5: Improvement (if needed)
  if (pipeline->config.use_improvements &&
      result->quality_score < pipeline->config.quality_target) {
    pipeline->state.current_stage = PIPELINE_STAGE_IMPROVEMENT;
    pipeline->state.progress = 0.9f;
    pipeline->state.status_message = "Improving quality";

    // Improve and regenerate (simplified)
    ai_pipeline_auto_improve(pipeline);
  }

  // Stage 6: Export
  pipeline->state.current_stage = PIPELINE_STAGE_EXPORT;
  pipeline->state.progress = 1.0f;
  pipeline->state.status_message = "Complete";

  pipeline->total_generations++;
  if (result->success) {
    pipeline->successful_generations++;
  }
  pipeline->average_quality =
      (pipeline->average_quality * (pipeline->total_generations - 1) +
       result->quality_score) /
      pipeline->total_generations;
  pipeline->total_generation_time_ms += result->generation_time_ms;

  pipeline->state.current_stage = PIPELINE_STAGE_INPUT;
  pipeline->state.progress = 0.0f;

  return result->success;
}

// Improve from feedback
void ai_pipeline_improve_from_feedback(AIPipeline *pipeline,
                                       const UserFeedback *feedback) {
  if (!pipeline || !feedback) {
    return;
  }

  ai_submit_feedback(pipeline->generator, feedback);
  ai_improve_model(pipeline->generator);

  pipeline->improvement_iterations++;
  LOG_INFO("Pipeline improved from feedback: iteration=%u",
           pipeline->improvement_iterations);
}

// Auto-improve
void ai_pipeline_auto_improve(AIPipeline *pipeline) {
  if (!pipeline || !pipeline->improvement_enabled) {
    return;
  }

  // Improve AI model
  ai_improve_model(pipeline->generator);
  pipeline->improvement_iterations++;
}

// Get improvement progress
f32 ai_pipeline_get_improvement_progress(AIPipeline *pipeline) {
  if (!pipeline) {
    return 0.0f;
  }

  // Calculate progress based on quality improvement
  f32 current_quality = ai_get_model_quality(pipeline->generator);
  f32 target_quality = pipeline->config.quality_target;

  return fminf(1.0f, current_quality / target_quality);
}

// Get statistics
void ai_pipeline_get_statistics(AIPipeline *pipeline, u32 *total,
                                u32 *successful, f32 *avg_quality,
                                u64 *avg_time) {
  if (!pipeline) {
    return;
  }

  if (total)
    *total = pipeline->total_generations;
  if (successful)
    *successful = pipeline->successful_generations;
  if (avg_quality)
    *avg_quality = pipeline->average_quality;
  if (avg_time) {
    *avg_time =
        pipeline->total_generations > 0
            ? pipeline->total_generation_time_ms / pipeline->total_generations
            : 0;
  }
}

// Set config
void ai_pipeline_set_config(AIPipeline *pipeline,
                            const PipelineConfig *config) {
  if (pipeline && config) {
    pipeline->config = *config;
  }
}

void ai_pipeline_set_improvement_enabled(AIPipeline *pipeline, bool enabled) {
  if (pipeline) {
    pipeline->improvement_enabled = enabled;
  }
}

void ai_pipeline_set_quality_target(AIPipeline *pipeline, f32 target) {
  if (pipeline) {
    pipeline->config.quality_target = fmaxf(0.0f, fminf(1.0f, target));
  }
}
