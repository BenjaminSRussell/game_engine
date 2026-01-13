// src/engine/ai/asset_generator.c
//
// Purpose: AI-powered asset generation implementation with local ML model.
// Generates modular 3D and 2.5D assets in 25 different styles.
//
#include <ai/ai_asset_generator.h>
#include <core/logger.h>
#include <include/math/math.h>
#include <include/rendering/mesh.h>
#include <rendering/texture_system.h>
#include <stdlib.h>
#include <string.h>

// Local AI model structure (simplified - would use actual ML library)
typedef struct {
  // Model weights (simplified representation)
  f32 *weights;
  u32 weight_count;

  // Style embeddings (25 styles)
  f32 style_embeddings[ASSET_STYLE_COUNT][128];

  // Training data
  u32 training_samples;
  f32 *training_data;

  // Generation parameters
  f32 temperature;    // Creativity temperature
  f32 style_strength; // Style adherence
} LocalAIModel;

// Initialize AI asset generator
bool ai_asset_generator_init(AIAssetGenerator *generator) {
  if (!generator) {
    LOG_ERROR("Invalid generator parameter");
    return false;
  }

  memset(generator, 0, sizeof(AIAssetGenerator));

  // Initialize AI model
  LocalAIModel *model = (LocalAIModel *)calloc(1, sizeof(LocalAIModel));
  if (!model) {
    LOG_ERROR("Failed to allocate AI model");
    return false;
  }

  // Initialize model weights (simplified - would load from file)
  model->weight_count = 10000; // Example size
  model->weights = (f32 *)calloc(model->weight_count, sizeof(f32));
  if (!model->weights) {
    free(model);
    LOG_ERROR("Failed to allocate model weights");
    return false;
  }

  // Initialize style embeddings (25 styles, 128 dimensions each)
  for (u32 i = 0; i < ASSET_STYLE_COUNT; i++) {
    for (u32 j = 0; j < 128; j++) {
      model->style_embeddings[i][j] = ((f32)rand() / RAND_MAX) * 2.0f - 1.0f;
    }
  }

  model->temperature = 0.7f;
  model->style_strength = 0.8f;

  generator->ai_model.model_data = model;
  generator->ai_model.initialized = true;

  // Initialize component library
  generator->component_capacity = 1024;
  generator->component_library = (AssetComponent *)calloc(
      generator->component_capacity, sizeof(AssetComponent));

  // Initialize generation queue
  generator->queue_capacity = 64;
  generator->generation_queue =
      (AssetConfig *)calloc(generator->queue_capacity, sizeof(AssetConfig));

  // Initialize feedback system
  generator->feedback_capacity = 256;
  generator->feedback_history = (UserFeedback *)calloc(
      generator->feedback_capacity, sizeof(UserFeedback));

  generator->initialized = true;

  LOG_INFO("AI Asset Generator initialized");
  LOG_INFO("  - Component capacity: %u", generator->component_capacity);
  LOG_INFO("  - Style templates: %u", ASSET_STYLE_COUNT);
  LOG_INFO("  - AI model initialized");

  return true;
}

void ai_asset_generator_shutdown(AIAssetGenerator *generator) {
  if (!generator || !generator->initialized) {
    return;
  }

  // Free AI model
  if (generator->ai_model.model_data) {
    LocalAIModel *model = (LocalAIModel *)generator->ai_model.model_data;
    if (model->weights) {
      free(model->weights);
    }
    free(model);
  }

  // Free components
  for (u32 i = 0; i < generator->component_count; i++) {
    AssetComponent *comp = &generator->component_library[i];
    if (comp->snap_points) {
      free(comp->snap_points);
    }
    // Meshes and textures are managed elsewhere
  }

  if (generator->component_library) {
    free(generator->component_library);
  }

  if (generator->generation_queue) {
    free(generator->generation_queue);
  }

  if (generator->feedback_history) {
    free(generator->feedback_history);
  }

  memset(generator, 0, sizeof(AIAssetGenerator));
  LOG_INFO("AI Asset Generator shut down");
}

// Generate asset from configuration
GenerationResult ai_generate_asset(AIAssetGenerator *generator,
                                   const AssetConfig *config) {
  GenerationResult result = {0};

  if (!generator || !generator->initialized || !config) {
    result.error_message = "Invalid parameters";
    return result;
  }

  LOG_INFO("Generating asset: style=%d, complexity=%.2f", config->style,
           config->complexity_target);

  // Get AI model
  LocalAIModel *model = (LocalAIModel *)generator->ai_model.model_data;
  if (!model) {
    result.error_message = "AI model not initialized";
    return result;
  }

  // Generate component using AI
  AssetComponent *component =
      ai_create_component(generator, "Generated", config->style);
  if (!component) {
    result.error_message = "Failed to create component";
    return result;
  }

  // Apply AI generation (simplified - would use actual ML inference)
  component->style = config->style;
  component->complexity = config->complexity_target;

  // Generate snap points based on style
  // Realistic: fewer snap points, more organic
  // Blocky: many snap points, grid-based
  u32 snap_count = 6; // Default: top, bottom, 4 sides
  if (config->style == ASSET_STYLE_BLOCKY) {
    snap_count = 8; // More connection points
  } else if (config->style == ASSET_STYLE_ORGANIC) {
    snap_count = 4; // Fewer, more flexible
  }

  // Generate snap points
  for (u32 i = 0; i < snap_count; i++) {
    Vec3 pos = vec3(0, 0, 0);
    Vec3 normal = vec3(0, 1, 0);

    // Position snap points based on type
    switch (i) {
    case 0:
      pos = vec3(0, config->target_size.y * 0.5f, 0);
      normal = vec3(0, 1, 0);
      break; // Top
    case 1:
      pos = vec3(0, -config->target_size.y * 0.5f, 0);
      normal = vec3(0, -1, 0);
      break; // Bottom
    case 2:
      pos = vec3(-config->target_size.x * 0.5f, 0, 0);
      normal = vec3(-1, 0, 0);
      break; // Left
    case 3:
      pos = vec3(config->target_size.x * 0.5f, 0, 0);
      normal = vec3(1, 0, 0);
      break; // Right
    case 4:
      pos = vec3(0, 0, -config->target_size.z * 0.5f);
      normal = vec3(0, 0, -1);
      break; // Front
    case 5:
      pos = vec3(0, 0, config->target_size.z * 0.5f);
      normal = vec3(0, 0, 1);
      break; // Back
    }

    ai_add_snap_point(component, (SnapPointType)i, pos, normal);
  }

  // Generate mesh (simplified - would use AI to generate actual geometry)
  // For now, create placeholder mesh
  component->mesh = NULL;      // Would be generated by AI
  component->mesh_2_5d = NULL; // Would be generated for 2.5D

  // Generate texture/skin (simplified)
  component->texture_id = 0; // Would be generated by AI

  // Set bounds
  component->bounds_min =
      vec3(-config->target_size.x * 0.5f, -config->target_size.y * 0.5f,
           -config->target_size.z * 0.5f);
  component->bounds_max =
      vec3(config->target_size.x * 0.5f, config->target_size.y * 0.5f,
           config->target_size.z * 0.5f);
  component->center = vec3(0, 0, 0);

  // Calculate quality score (simplified)
  result.quality_score = 0.7f + (config->complexity_target * 0.3f);
  component->generation_quality = result.quality_score;

  // Add to result
  result.components = (AssetComponent *)malloc(sizeof(AssetComponent));
  result.components[0] = *component;
  result.component_count = 1;
  result.success = true;
  result.generation_time_ms = 100; // Placeholder

  LOG_INFO("Asset generated successfully: quality=%.2f", result.quality_score);

  return result;
}

// Create component
AssetComponent *ai_create_component(AIAssetGenerator *generator,
                                    const char *name, AssetStyle style) {
  if (!generator || !name) {
    return NULL;
  }

  // Find free slot
  for (u32 i = 0; i < generator->component_capacity; i++) {
    if (generator->component_library[i].component_id == 0) {
      AssetComponent *comp = &generator->component_library[i];
      comp->component_id = generator->component_count + 1;
      comp->name = name;
      comp->style = style;
      comp->snap_point_capacity = 16;
      comp->snap_points =
          (SnapPoint *)calloc(comp->snap_point_capacity, sizeof(SnapPoint));

      generator->component_count++;
      return comp;
    }
  }

  // Expand if needed
  u32 new_capacity = generator->component_capacity * 2;
  AssetComponent *new_lib = (AssetComponent *)realloc(
      generator->component_library, new_capacity * sizeof(AssetComponent));
  if (new_lib) {
    generator->component_library = new_lib;
    memset(&generator->component_library[generator->component_capacity], 0,
           (new_capacity - generator->component_capacity) *
               sizeof(AssetComponent));
    generator->component_capacity = new_capacity;

    AssetComponent *comp =
        &generator->component_library[generator->component_count];
    comp->component_id = generator->component_count + 1;
    comp->name = name;
    comp->style = style;
    comp->snap_point_capacity = 16;
    comp->snap_points =
        (SnapPoint *)calloc(comp->snap_point_capacity, sizeof(SnapPoint));

    generator->component_count++;
    return comp;
  }

  return NULL;
}

// Add snap point
bool ai_add_snap_point(AssetComponent *component, SnapPointType type,
                       Vec3 position, Vec3 normal) {
  if (!component || !component->snap_points) {
    return false;
  }

  if (component->snap_point_count >= component->snap_point_capacity) {
    u32 new_capacity = component->snap_point_capacity * 2;
    SnapPoint *new_points = (SnapPoint *)realloc(
        component->snap_points, new_capacity * sizeof(SnapPoint));
    if (!new_points) {
      return false;
    }
    component->snap_points = new_points;
    component->snap_point_capacity = new_capacity;
  }

  SnapPoint *snap = &component->snap_points[component->snap_point_count];
  snap->type = type;
  snap->position = position;
  snap->normal = normal;
  snap->radius = 0.1f; // Default snap radius
  snap->connection_id = component->snap_point_count;
  snap->occupied = false;

  component->snap_point_count++;
  return true;
}

// Check if components can snap together
bool ai_can_snap(AssetComponent *component1, AssetComponent *component2,
                 u32 snap1_id, u32 snap2_id) {
  if (!component1 || !component2) {
    return false;
  }

  if (snap1_id >= component1->snap_point_count ||
      snap2_id >= component2->snap_point_count) {
    return false;
  }

  SnapPoint *snap1 = &component1->snap_points[snap1_id];
  SnapPoint *snap2 = &component2->snap_points[snap2_id];

  // Check if already occupied
  if (snap1->occupied || snap2->occupied) {
    return false;
  }

  // Check compatibility (simplified - would check types, sizes, etc.)
  // Opposite normals should align
  Vec3 dir = vec3_sub(snap1->normal, snap2->normal);
  f32 dot = vec3_dot(snap1->normal, snap2->normal);

  // Normals should be opposite (dot product near -1)
  return dot < -0.5f;
}

// Submit feedback for AI improvement
void ai_submit_feedback(AIAssetGenerator *generator,
                        const UserFeedback *feedback) {
  if (!generator || !feedback) {
    return;
  }

  if (generator->feedback_count >= generator->feedback_capacity) {
    u32 new_capacity = generator->feedback_capacity * 2;
    UserFeedback *new_feedback = (UserFeedback *)realloc(
        generator->feedback_history, new_capacity * sizeof(UserFeedback));
    if (new_feedback) {
      generator->feedback_history = new_feedback;
      generator->feedback_capacity = new_capacity;
    } else {
      return;
    }
  }

  generator->feedback_history[generator->feedback_count] = *feedback;
  generator->feedback_count++;

  // Update AI model quality
  LocalAIModel *model = (LocalAIModel *)generator->ai_model.model_data;
  if (model) {
    generator->ai_model.average_quality =
        (generator->ai_model.average_quality * (generator->feedback_count - 1) +
         feedback->quality_rating) /
        generator->feedback_count;
  }

  LOG_INFO("Feedback submitted: quality=%.2f, component_id=%u",
           feedback->quality_rating, feedback->component_id);
}

// Improve AI model based on feedback
void ai_improve_model(AIAssetGenerator *generator) {
  if (!generator || generator->feedback_count == 0) {
    return;
  }

  LocalAIModel *model = (LocalAIModel *)generator->ai_model.model_data;
  if (!model) {
    return;
  }

  // Simplified improvement: adjust model parameters based on feedback
  f32 avg_quality = 0.0f;
  for (u32 i = 0; i < generator->feedback_count; i++) {
    avg_quality += generator->feedback_history[i].quality_rating;
  }
  avg_quality /= generator->feedback_count;

  // Adjust temperature based on quality
  if (avg_quality < 0.5f) {
    model->temperature *= 0.95f; // Reduce creativity if quality is low
  } else if (avg_quality > 0.8f) {
    model->temperature *= 1.05f; // Increase creativity if quality is high
  }
  model->temperature = fmaxf(0.1f, fminf(1.0f, model->temperature));

  generator->ai_model.training_iterations++;
  LOG_INFO(
      "AI model improved: iteration=%u, avg_quality=%.2f, temperature=%.2f",
      generator->ai_model.training_iterations, avg_quality, model->temperature);
}

// Get model quality
f32 ai_get_model_quality(AIAssetGenerator *generator) {
  return generator ? generator->ai_model.average_quality : 0.0f;
}

// Generate from description
GenerationResult ai_generate_from_description(AIAssetGenerator *generator,
                                              const char *description,
                                              AssetStyle style) {
  GenerationResult result = {0};

  if (!generator || !description) {
    result.error_message = "Invalid parameters";
    return result;
  }

  // Parse description and create config
  AssetConfig config = {0};
  config.style = style;
  config.description = description;
  config.complexity_target = 0.5f; // Default
  config.require_2_5d = true;
  config.require_3d = true;
  config.target_size = vec3(1.0f, 1.0f, 1.0f);
  config.detail_level = 0.7f;

  // Simple keyword-based complexity estimation
  if (strstr(description, "simple") || strstr(description, "basic")) {
    config.complexity_target = 0.3f;
  } else if (strstr(description, "complex") ||
             strstr(description, "detailed")) {
    config.complexity_target = 0.8f;
  }

  return ai_generate_asset(generator, &config);
}

// Find component
AssetComponent *ai_find_component(AIAssetGenerator *generator,
                                  u32 component_id) {
  if (!generator) {
    return NULL;
  }

  for (u32 i = 0; i < generator->component_count; i++) {
    if (generator->component_library[i].component_id == component_id) {
      return &generator->component_library[i];
    }
  }

  return NULL;
}

// Destroy component
void ai_destroy_component(AIAssetGenerator *generator,
                          AssetComponent *component) {
  if (!generator || !component) {
    return;
  }

  if (component->snap_points) {
    free(component->snap_points);
  }

  memset(component, 0, sizeof(AssetComponent));
  generator->component_count--;
}

// Connect components
bool ai_connect_components(AssetComponent *base, AssetComponent *attachment,
                           u32 base_snap_id, u32 attach_snap_id) {
  if (!base || !attachment) {
    return false;
  }

  if (!ai_can_snap(base, attachment, base_snap_id, attach_snap_id)) {
    return false;
  }

  base->snap_points[base_snap_id].occupied = true;
  attachment->snap_points[attach_snap_id].occupied = true;

  return true;
}

// Get component count
u32 ai_get_component_count(AIAssetGenerator *generator) {
  return generator ? generator->component_count : 0;
}
