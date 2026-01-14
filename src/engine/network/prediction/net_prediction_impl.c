/**
 * CLIENT PREDICTION AND RECONCILIATION - COMPLETE IMPLEMENTATION
 * Network prediction with rollback, replay, and smoothing for responsive
 * gameplay
 */

#include "include/core/logger.h"
#include "include/network/network_prediction.h"
#include <math.h>
#include <stdlib.h>
#include <string.h>

#define MAX_PREDICTION_HISTORY 64
#define MAX_ENTITIES 1024
#define SMOOTHING_FACTOR 0.1f
#define RECONCILIATION_THRESHOLD 0.5f
#define ROLLBACK_BUFFER_SIZE 32

// Structs are defined in include/network/network_prediction.h

// Forward declarations
void prediction_predict_full_state(ClientPrediction *pred, uint32_t entity_id,
                                   const PredictionFrame *current_state,
                                   float delta_time,
                                   PredictionFrame *predicted_state);
void prediction_rollback_and_replay_full_state(ClientPrediction *pred,
                                               uint32_t entity_id,
                                               uint32_t confirmed_frame);
Vec3 prediction_predict_position(ClientPrediction *pred, uint32_t entity_id,
                                 Vec3 current_pos, Vec3 current_vel,
                                 Vec3 current_acc, float delta_time);

// Helper functions (Internal pointer-based implementations to avoid conflict
// with Math Library value-based functions)
static float impl_vec3_distance(const Vec3 *a, const Vec3 *b) {
  float dx = a->x - b->x;
  float dy = a->y - b->y;
  float dz = a->z - b->z;
  return sqrtf(dx * dx + dy * dy + dz * dz);
}

static Vec3 impl_vec3_lerp(const Vec3 *a, const Vec3 *b, float t) {
  Vec3 result;
  result.x = a->x + (b->x - a->x) * t;
  result.y = a->y + (b->y - a->y) * t;
  result.z = a->z + (b->z - a->z) * t;
  return result;
}

static Vec3 impl_vec3_normalize(const Vec3 *v) {
  float length = sqrtf(v->x * v->x + v->y * v->y + v->z * v->z);
  if (length < 0.0001f)
    return *v;

  float inv_length = 1.0f / length;
  return (Vec3){v->x * inv_length, v->y * inv_length, v->z * inv_length};
}

static float impl_vec3_length(const Vec3 *v) {
  return sqrtf(v->x * v->x + v->y * v->y + v->z * v->z);
}

static Vec3 impl_vec3_scale(const Vec3 *v, float scale) {
  return (Vec3){v->x * scale, v->y * scale, v->z * scale};
}

static void impl_vec3_clamp(Vec3 *v, float max_magnitude) {
  float magnitude = impl_vec3_length(v);
  if (magnitude > max_magnitude && magnitude > 0.0f) {
    float scale = max_magnitude / magnitude;
    v->x *= scale;
    v->y *= scale;
    v->z *= scale;
  }
}

static Quat impl_quat_multiply(const Quat *a, const Quat *b) {
  Quat result;
  result.x = a->x * b->w + a->w * b->x + a->y * b->z - a->z * b->y;
  result.y = a->y * b->w + a->w * b->y + a->z * b->x - a->x * b->z;
  result.z = a->z * b->w + a->w * b->z + a->x * b->y - a->y * b->x;
  result.w = a->w * b->w - a->x * b->x - a->y * b->y - a->z * b->z;
  return result;
}

static Quat impl_quat_normalize(const Quat *q) {
  float length = sqrtf(q->x * q->x + q->y * q->y + q->z * q->z + q->w * q->w);
  if (length < 0.0001f)
    return *q;

  float inv_length = 1.0f / length;
  return (Quat){q->x * inv_length, q->y * inv_length, q->z * inv_length,
                q->w * inv_length};
}

static Quat impl_quat_from_axis_angle(const Vec3 *axis, float angle) {
  float half_angle = angle * 0.5f;
  float sin_half = sinf(half_angle);
  float cos_half = cosf(half_angle);

  return (Quat){axis->x * sin_half, axis->y * sin_half, axis->z * sin_half,
                cos_half};
}

static float impl_quat_angle(const Quat *a, const Quat *b) {
  // Compute dot product
  float dot = a->x * b->x + a->y * b->y + a->z * b->z + a->w * b->w;

  // Clamp to valid range
  dot = fmaxf(-1.0f, fminf(1.0f, dot));

  // Return angle in radians
  return acosf(dot);
}

static PredictionFrame *get_prediction_frame(ClientPrediction *pred,
                                             uint32_t frame_number) {
  if (!pred)
    return NULL;

  for (uint32_t i = 0; i < pred->count; i++) {
    uint32_t index = (pred->head - 1 - i) % MAX_PREDICTION_HISTORY;
    PredictionFrame *frame = &pred->history[index];
    if (frame->frame_number == frame_number) {
      return frame;
    }
  }
  return NULL;
}

static EntityPrediction *get_entity_prediction(ClientPrediction *pred,
                                               uint32_t entity_id) {
  if (!pred)
    return NULL;

  for (uint32_t i = 0; i < pred->entity_count; i++) {
    if (pred->entities[i].entity_id == entity_id) {
      return &pred->entities[i];
    }
  }

  // Create new entity prediction if not found
  if (pred->entity_count < MAX_ENTITIES) {
    EntityPrediction *entity = &pred->entities[pred->entity_count++];
    entity->entity_id = entity_id;

    // Initialize position/velocity
    entity->predicted_position = (Vec3){0, 0, 0};
    entity->predicted_velocity = (Vec3){0, 0, 0};
    entity->server_position = (Vec3){0, 0, 0};
    entity->server_velocity = (Vec3){0, 0, 0};

    // Initialize rotation
    entity->predicted_rotation = (Quat){0, 0, 0, 1};
    entity->predicted_angular_velocity = (Vec3){0, 0, 0};
    entity->server_rotation = (Quat){0, 0, 0, 1};
    entity->server_angular_velocity = (Vec3){0, 0, 0};

    // Initialize scale
    entity->predicted_scale = (Vec3){1, 1, 1};
    entity->server_scale = (Vec3){1, 1, 1};

    // Initialize health
    entity->predicted_health = 100.0f;
    entity->server_health = 100.0f;

    // Initialize animation state
    entity->predicted_animation = (AnimationState){0, 0.0f, 1.0f, 0};
    entity->server_animation = (AnimationState){0, 0.0f, 1.0f, 0};

    // Initialize physics state
    entity->predicted_physics = (PhysicsState){0, 0, 0, 0, 1.0f};
    entity->server_physics = (PhysicsState){0, 0, 0, 0, 1.0f};

    // Initialize custom data
    entity->predicted_custom_count = 0;
    entity->server_custom_count = 0;
    memset(entity->predicted_custom, 0, sizeof(entity->predicted_custom));
    memset(entity->server_custom, 0, sizeof(entity->server_custom));

    // Initialize error tracking
    entity->position_error = 0.0f;
    entity->rotation_error = 0.0f;
    entity->velocity_error = 0.0f;
    entity->health_error = 0.0f;
    entity->needs_reconciliation = false;
    entity->last_confirmed_frame = 0;
    entity->component_mask = 0xFFFF; // All components

    return entity;
  }

  return NULL;
}

// Create prediction system
ClientPrediction *prediction_create(void) {
  ClientPrediction *pred = malloc(sizeof(ClientPrediction));
  if (!pred) {
    log_error("Failed to allocate client prediction");
    return NULL;
  }

  memset(pred, 0, sizeof(ClientPrediction));
  pred->head = 0;
  pred->count = 0;
  pred->last_acknowledged_frame = 0;
  pred->mispredicted = false;
  pred->rtt = 0.1f; // Default RTT
  pred->packet_loss = 0.0f;
  pred->prediction_accuracy = 1.0f;

  // Initialize enhanced prediction features
  pred->entity_count = 0;
  pred->smoothed_position = (Vec3){0, 0, 0};
  pred->smoothed_rotation = (Quat){0, 0, 0, 1};
  pred->target_position = (Vec3){0, 0, 0};
  pred->target_rotation = (Quat){0, 0, 0, 1};
  pred->smoothing_time = 0.0f;

  // Enable all components by default with equal weights
  pred->enabled_components = 0xFFFF; // All components enabled
  for (uint32_t i = 0; i < MAX_STATE_COMPONENTS; i++) {
    pred->component_weights[i] = 1.0f; // Default equal weight
  }

  log_info("Enhanced client prediction system created");
  return pred;
}

// Destroy prediction system
void prediction_destroy(ClientPrediction *pred) {
  if (!pred)
    return;

  // Free any allocated input states
  for (uint32_t i = 0; i < MAX_PREDICTION_HISTORY; i++) {
    if (pred->history[i].input_state) {
      free(pred->history[i].input_state);
    }

    // Free custom data in history frames
    for (uint32_t j = 0; j < pred->history[i].custom_data_count; j++) {
      if (pred->history[i].custom_data[j].data) {
        free(pred->history[i].custom_data[j].data);
      }
    }
  }

  // Free custom data in entity predictions
  for (uint32_t i = 0; i < pred->entity_count; i++) {
    for (uint32_t j = 0; j < pred->entities[i].predicted_custom_count; j++) {
      if (pred->entities[i].predicted_custom[j].data) {
        free(pred->entities[i].predicted_custom[j].data);
      }
    }

    for (uint32_t j = 0; j < pred->entities[i].server_custom_count; j++) {
      if (pred->entities[i].server_custom[j].data) {
        free(pred->entities[i].server_custom[j].data);
      }
    }
  }

  for (uint32_t i = 0; i < ROLLBACK_BUFFER_SIZE; i++) {
    if (pred->rollback_buffer[i].input_state) {
      free(pred->rollback_buffer[i].input_state);
    }
  }

  free(pred);
  log_info("Enhanced client prediction system destroyed");
}

// Add prediction frame
void prediction_add_frame(ClientPrediction *pred, uint32_t frame, Vec3 pos,
                          Vec3 vel) {
  if (!pred)
    return;

  uint32_t index = pred->head % MAX_PREDICTION_HISTORY;

  // Free old input state if it exists
  if (pred->history[index].input_state) {
    free(pred->history[index].input_state);
    pred->history[index].input_state = NULL;
  }

  // Add new frame
  pred->history[index].frame_number = frame;
  pred->history[index].position = pos;
  pred->history[index].velocity = vel;
  pred->history[index].acceleration = (Vec3){0, 0, 0};
  pred->history[index].timestamp = 0.0f; // Will be set by caller
  pred->history[index].input_state = NULL;
  pred->history[index].is_confirmed = false;

  pred->head++;
  if (pred->count < MAX_PREDICTION_HISTORY) {
    pred->count++;
  }

  log_debug("Added prediction frame %u at position (%.2f, %.2f, %.2f)", frame,
            pos.x, pos.y, pos.z);
}

// Add prediction frame with input
void prediction_add_frame_with_input(ClientPrediction *pred, uint32_t frame,
                                     Vec3 pos, Vec3 vel, void *input_state,
                                     uint32_t input_size) {
  if (!pred)
    return;

  uint32_t index = pred->head % MAX_PREDICTION_HISTORY;

  // Free old input state if it exists
  if (pred->history[index].input_state) {
    free(pred->history[index].input_state);
  }

  // Add new frame with input
  pred->history[index].frame_number = frame;
  pred->history[index].position = pos;
  pred->history[index].velocity = vel;
  pred->history[index].acceleration = (Vec3){0, 0, 0};
  pred->history[index].timestamp = 0.0f;
  pred->history[index].is_confirmed = false;

  // Copy input state
  if (input_state && input_size > 0) {
    pred->history[index].input_state = malloc(input_size);
    if (pred->history[index].input_state) {
      memcpy(pred->history[index].input_state, input_state, input_size);
      pred->history[index].input_size = input_size;
    } else {
      pred->history[index].input_size = 0;
    }
  } else {
    pred->history[index].input_state = NULL;
    pred->history[index].input_size = 0;
  }

  pred->head++;
  if (pred->count < MAX_PREDICTION_HISTORY) {
    pred->count++;
  }
}

// Enhanced prediction functions
Quat prediction_predict_rotation(ClientPrediction *pred, uint32_t entity_id,
                                 Quat current_rot, Vec3 angular_vel,
                                 float delta_time) {
  if (!pred)
    return current_rot;

  EntityPrediction *entity = get_entity_prediction(pred, entity_id);
  if (!entity)
    return current_rot;

  // Simple angular integration: rot = rot * angular_vel * dt
  Quat predicted_rot = current_rot;

  // Convert angular velocity to quaternion
  float angle = impl_vec3_length(&angular_vel) * delta_time;
  if (angle > 0.0f) {
    Vec3 axis = impl_vec3_normalize(&angular_vel);
    Quat rotation_delta = impl_quat_from_axis_angle(&axis, angle);
    predicted_rot = impl_quat_multiply(&predicted_rot, &rotation_delta);
  }

  // Normalize to prevent drift
  predicted_rot = impl_quat_normalize(&predicted_rot);

  entity->predicted_rotation = predicted_rot;
  return predicted_rot;
}

void prediction_predict_full_state(ClientPrediction *pred, uint32_t entity_id,
                                   const PredictionFrame *current_state,
                                   float delta_time,
                                   PredictionFrame *predicted_state) {
  if (!pred || !current_state || !predicted_state)
    return;

  EntityPrediction *entity = get_entity_prediction(pred, entity_id);
  if (!entity)
    return;

  // Copy current state as base
  *predicted_state = *current_state;
  predicted_state->frame_number++;
  predicted_state->timestamp += delta_time;

  // Predict position if enabled
  if (pred->enabled_components & STATE_COMPONENT_POSITION) {
    Vec3 acceleration = current_state->physics_state.force;
    if (current_state->physics_state.mass > 0.0f) {
      acceleration = impl_vec3_scale(&acceleration,
                                     1.0f / current_state->physics_state.mass);
    }

    Vec3 new_vel = current_state->velocity;
    new_vel.x += acceleration.x * delta_time;
    new_vel.y += acceleration.y * delta_time;
    new_vel.z += acceleration.z * delta_time;

    predicted_state->position.x += new_vel.x * delta_time;
    predicted_state->position.y += new_vel.y * delta_time;
    predicted_state->position.z += new_vel.z * delta_time;
    predicted_state->velocity = new_vel;
  }

  // Predict rotation if enabled
  if (pred->enabled_components & STATE_COMPONENT_ROTATION) {
    float angle =
        impl_vec3_length(&current_state->angular_velocity) * delta_time;
    if (angle > 0.0f) {
      Vec3 axis = impl_vec3_normalize(&current_state->angular_velocity);
      Quat rotation_delta = impl_quat_from_axis_angle(&axis, angle);
      predicted_state->rotation =
          impl_quat_multiply(&current_state->rotation, &rotation_delta);
      predicted_state->rotation =
          impl_quat_normalize(&predicted_state->rotation);
    }
  }

  // Predict animation state if enabled
  if (pred->enabled_components & STATE_COMPONENT_ANIMATION_STATE) {
    predicted_state->animation_state.playback_time += delta_time;
    // TODO: Handle animation transitions and blending
  }

  // Predict health if enabled (simple regeneration/damage over time)
  if (pred->enabled_components & STATE_COMPONENT_HEALTH) {
    // Simple health prediction - could be enhanced with damage over time, etc.
    // For now, keep health stable
  }

  // Update entity prediction
  entity->predicted_position = predicted_state->position;
  entity->predicted_velocity = predicted_state->velocity;
  entity->predicted_rotation = predicted_state->rotation;
  entity->predicted_angular_velocity = predicted_state->angular_velocity;
  entity->predicted_scale = predicted_state->scale;
  entity->predicted_health = predicted_state->health;
  entity->predicted_animation = predicted_state->animation_state;
  entity->predicted_physics = predicted_state->physics_state;
}

// Enhanced reconciliation
void prediction_reconcile_full_state(ClientPrediction *pred, uint32_t entity_id,
                                     uint32_t confirmed_frame,
                                     const PredictionFrame *server_state) {
  if (!pred || !server_state)
    return;

  EntityPrediction *entity = get_entity_prediction(pred, entity_id);
  if (!entity)
    return;

  // Store server state
  entity->server_position = server_state->position;
  entity->server_velocity = server_state->velocity;
  entity->server_rotation = server_state->rotation;
  entity->server_angular_velocity = server_state->angular_velocity;
  entity->server_scale = server_state->scale;
  entity->server_health = server_state->health;
  entity->server_animation = server_state->animation_state;
  entity->server_physics = server_state->physics_state;

  // Copy custom server data
  entity->server_custom_count = server_state->custom_data_count;
  for (uint32_t i = 0;
       i < server_state->custom_data_count && i < MAX_STATE_COMPONENTS; i++) {
    entity->server_custom[i] = server_state->custom_data[i];
  }

  entity->last_confirmed_frame = confirmed_frame;

  // Calculate prediction errors for all components
  float position_error =
      impl_vec3_distance(&entity->predicted_position, &server_state->position);
  float rotation_error =
      impl_quat_angle(&entity->predicted_rotation, &server_state->rotation);
  float velocity_error =
      impl_vec3_distance(&entity->predicted_velocity, &server_state->velocity);
  float health_error = fabsf(entity->predicted_health - server_state->health);

  entity->position_error = position_error;
  entity->rotation_error = rotation_error;
  entity->velocity_error = velocity_error;
  entity->health_error = health_error;

  // Check if reconciliation is needed (weighted error calculation)
  float total_error = 0.0f;
  float weight_sum = 0.0f;

  if (pred->enabled_components & STATE_COMPONENT_POSITION) {
    total_error += position_error * pred->component_weights[0];
    weight_sum += pred->component_weights[0];
  }
  if (pred->enabled_components & STATE_COMPONENT_ROTATION) {
    total_error += rotation_error * pred->component_weights[2];
    weight_sum += pred->component_weights[2];
  }
  if (pred->enabled_components & STATE_COMPONENT_VELOCITY) {
    total_error += velocity_error * pred->component_weights[1];
    weight_sum += pred->component_weights[1];
  }
  if (pred->enabled_components & STATE_COMPONENT_HEALTH) {
    total_error += health_error * pred->component_weights[5];
    weight_sum += pred->component_weights[5];
  }

  if (weight_sum > 0.0f) {
    total_error /= weight_sum;
  }

  float reconciliation_threshold = 0.5f; // Configurable

  if (total_error > reconciliation_threshold) {
    entity->needs_reconciliation = true;
    pred->mispredicted = true;

    log_debug("Multi-component prediction error %.3f for entity %u, triggering "
              "reconciliation",
              total_error, entity_id);

    // Start rollback and replay for full state
    prediction_rollback_and_replay_full_state(pred, entity_id, confirmed_frame);
  } else {
    // Smooth correction instead of hard snap
    entity->needs_reconciliation = false;
    pred->target_position = server_state->position;
    pred->target_rotation = server_state->rotation;
    pred->smoothing_time = 0.0f;
  }

  // Update prediction accuracy metrics
  float accuracy = 1.0f - fminf(total_error / 10.0f, 1.0f);
  pred->prediction_accuracy =
      (pred->prediction_accuracy * 0.9f) + (accuracy * 0.1f);
}

// Rollback and replay
void prediction_rollback_and_replay(ClientPrediction *pred, uint32_t entity_id,
                                    uint32_t confirmed_frame) {
  if (!pred)
    return;

  EntityPrediction *entity = get_entity_prediction(pred, entity_id);
  if (!entity)
    return;

  // Find the confirmed frame
  PredictionFrame *confirmed_frame_data =
      get_prediction_frame(pred, confirmed_frame);
  if (!confirmed_frame_data) {
    log_error("Cannot find confirmed frame %u for rollback", confirmed_frame);
    return;
  }

  // Reset to confirmed state
  entity->predicted_position = entity->server_position;
  entity->predicted_velocity = entity->server_velocity;

  // Replay unconfirmed frames
  for (uint32_t i = 0; i < pred->count; i++) {
    uint32_t index = (pred->head - 1 - i) % MAX_PREDICTION_HISTORY;
    PredictionFrame *frame = &pred->history[index];

    if (frame->frame_number > confirmed_frame && !frame->is_confirmed) {
      // Re-predict this frame
      Vec3 new_pos = prediction_predict_position(
          pred, entity_id, entity->predicted_position,
          entity->predicted_velocity, frame->acceleration,
          0.016f); // Assume 60 FPS

      // Update frame with new prediction
      frame->position = new_pos;
    }
  }

  log_debug("Rollback and replay completed for entity %u", entity_id);
}

// Update smoothing
void prediction_update_smoothing(ClientPrediction *pred, float delta_time) {
  if (!pred)
    return;

  if (pred->mispredicted) {
    pred->smoothing_time += delta_time;

    // Smooth interpolation over 0.2 seconds
    float t = fminf(pred->smoothing_time / 0.2f, 1.0f);
    pred->smoothed_position =
        impl_vec3_lerp(&pred->smoothed_position, &pred->target_position, t);

    if (t >= 1.0f) {
      pred->mispredicted = false;
      pred->smoothed_position = pred->target_position;
    }
  } else {
    pred->smoothed_position = pred->target_position;
  }
}

// Get smoothed position
Vec3 prediction_get_smoothed_position(ClientPrediction *pred,
                                      uint32_t entity_id) {
  if (!pred)
    return (Vec3){0, 0, 0};

  EntityPrediction *entity = get_entity_prediction(pred, entity_id);
  if (!entity)
    return (Vec3){0, 0, 0};

  if (pred->mispredicted) {
    return pred->smoothed_position;
  } else {
    return entity->predicted_position;
  }
}

// Set network quality metrics
void prediction_set_network_quality(ClientPrediction *pred, float rtt,
                                    float packet_loss) {
  if (!pred)
    return;

  pred->rtt = rtt;
  pred->packet_loss = packet_loss;

  // Adjust prediction parameters based on network quality
  if (rtt > 0.2f || packet_loss > 0.1f) {
    // Poor network quality - increase smoothing
    log_debug("Poor network quality detected (RTT: %.3f, Loss: %.3f)", rtt,
              packet_loss);
  }
}

// Get prediction statistics
void prediction_get_stats(ClientPrediction *pred, float *accuracy, float *rtt,
                          float *packet_loss, uint32_t *mispredictions) {
  if (!pred)
    return;

  if (accuracy)
    *accuracy = pred->prediction_accuracy;
  if (rtt)
    *rtt = pred->rtt;
  if (packet_loss)
    *packet_loss = pred->packet_loss;
  if (mispredictions)
    *mispredictions = pred->mispredicted ? 1 : 0;
}

// Clear prediction history
void prediction_clear_history(ClientPrediction *pred) {
  if (!pred)
    return;

  // Free input states
  for (uint32_t i = 0; i < MAX_PREDICTION_HISTORY; i++) {
    if (pred->history[i].input_state) {
      free(pred->history[i].input_state);
      pred->history[i].input_state = NULL;
    }
  }

  pred->head = 0;
  pred->count = 0;
  pred->last_acknowledged_frame = 0;
  pred->mispredicted = false;

  log_info("Prediction history cleared");
}

void prediction_reconcile(ClientPrediction *pred, u32 entity_id,
                          u32 confirmed_frame, Vec3 server_pos,
                          Vec3 server_vel) {
  if (!pred) {
    return;
  }

  // Create a temporary server state frame
  PredictionFrame server_state = {0};
  server_state.frame_number = confirmed_frame;
  server_state.position = server_pos;
  server_state.velocity = server_vel;
  // Default other values
  server_state.scale = (Vec3){1, 1, 1};
  server_state.health = 100.0f;
  server_state.component_mask =
      STATE_COMPONENT_POSITION | STATE_COMPONENT_VELOCITY;

  // Delegate to full state reconciliation
  prediction_reconcile_full_state(pred, entity_id, confirmed_frame,
                                  &server_state);
}

bool prediction_get_frame(ClientPrediction *pred, u32 frame,
                          PredictionFrame *out_frame) {
  if (!pred || !out_frame) {
    return false;
  }

  for (u32 i = 0; i < pred->count; i++) {
    u32 index = (pred->head - pred->count + i) % MAX_PREDICTION_HISTORY;
    if (pred->history[index].frame_number == frame) {
      *out_frame = pred->history[index];
      return true;
    }
  }

  return false;
}

Vec3 prediction_get_current_position(ClientPrediction *pred) {
  if (!pred || pred->count == 0) {
    return (Vec3){0, 0, 0};
  }

  u32 index = (pred->head - 1) % MAX_PREDICTION_HISTORY;
  return pred->history[index].position;
}

Vec3 prediction_get_current_velocity(ClientPrediction *pred) {
  if (!pred || pred->count == 0) {
    return (Vec3){0, 0, 0};
  }

  u32 index = (pred->head - 1) % MAX_PREDICTION_HISTORY;
  return pred->history[index].velocity;
}

bool prediction_was_mispredicted(ClientPrediction *pred) {
  return pred ? pred->mispredicted : false;
}

u32 prediction_get_last_acknowledged_frame(ClientPrediction *pred) {
  return pred ? pred->last_acknowledged_frame : 0;
}

void prediction_clear_old_frames(ClientPrediction *pred, u32 before_frame) {
  if (!pred) {
    return;
  }

  u32 removed = 0;
  for (u32 i = 0; i < pred->count; i++) {
    u32 index = (pred->head - pred->count + i) % MAX_PREDICTION_HISTORY;
    if (pred->history[index].frame_number < before_frame) {
      if (pred->history[index].input_state) {
        free(pred->history[index].input_state);
        pred->history[index].input_state = NULL;
      }
      removed++;
    } else {
      break; // Frames are in order, so we can stop here
    }
  }

  if (removed > 0) {
    pred->count -= removed;
    log_debug("Cleared %u old prediction frames", removed);
  }
}

// Component management functions
void prediction_enable_components(ClientPrediction *pred,
                                  uint32_t component_mask) {
  if (!pred)
    return;
  pred->enabled_components = component_mask;
}

void prediction_set_component_weight(ClientPrediction *pred,
                                     uint32_t component_id, float weight) {
  if (!pred || component_id >= MAX_STATE_COMPONENTS)
    return;
  pred->component_weights[component_id] = weight;
}

void prediction_get_entity_state(ClientPrediction *pred, uint32_t entity_id,
                                 PredictionFrame *state) {
  if (!pred || !state)
    return;

  EntityPrediction *entity = get_entity_prediction(pred, entity_id);
  if (!entity) {
    memset(state, 0, sizeof(PredictionFrame));
    return;
  }

  state->position = entity->server_position;
  state->velocity = entity->server_velocity;
  state->rotation = entity->server_rotation;
  state->angular_velocity = entity->server_angular_velocity;
  state->scale = entity->server_scale;
  state->health = entity->server_health;
  state->animation_state = entity->server_animation;
  state->physics_state = entity->server_physics;

  // Copy custom server data
  state->custom_data_count = entity->server_custom_count;
  for (uint32_t i = 0;
       i < entity->server_custom_count && i < MAX_STATE_COMPONENTS; i++) {
    state->custom_data[i] = entity->server_custom[i];
  }

  state->component_mask = entity->component_mask;
}

// Error and quality metrics
void prediction_get_entity_errors(ClientPrediction *pred, uint32_t entity_id,
                                  float *pos_error, float *rot_error,
                                  float *vel_error, float *health_error) {
  if (!pred)
    return;

  EntityPrediction *entity = get_entity_prediction(pred, entity_id);
  if (!entity) {
    if (pos_error)
      *pos_error = 0.0f;
    if (rot_error)
      *rot_error = 0.0f;
    if (vel_error)
      *vel_error = 0.0f;
    if (health_error)
      *health_error = 0.0f;
    return;
  }

  if (pos_error)
    *pos_error = entity->position_error;
  if (rot_error)
    *rot_error = entity->rotation_error;
  if (vel_error)
    *vel_error = entity->velocity_error;
  if (health_error)
    *health_error = entity->health_error;
}

float prediction_get_entity_accuracy(ClientPrediction *pred,
                                     uint32_t entity_id) {
  if (!pred)
    return 0.0f;

  EntityPrediction *entity = get_entity_prediction(pred, entity_id);
  if (!entity)
    return 0.0f;

  // Calculate weighted accuracy based on enabled components
  float total_error = 0.0f;
  float weight_sum = 0.0f;

  if (pred->enabled_components & STATE_COMPONENT_POSITION) {
    total_error += entity->position_error * pred->component_weights[0];
    weight_sum += pred->component_weights[0];
  }
  if (pred->enabled_components & STATE_COMPONENT_ROTATION) {
    total_error += entity->rotation_error * pred->component_weights[2];
    weight_sum += pred->component_weights[2];
  }
  if (pred->enabled_components & STATE_COMPONENT_VELOCITY) {
    total_error += entity->velocity_error * pred->component_weights[1];
    weight_sum += pred->component_weights[1];
  }
  if (pred->enabled_components & STATE_COMPONENT_HEALTH) {
    total_error += entity->health_error * pred->component_weights[5];
    weight_sum += pred->component_weights[5];
  }

  if (weight_sum > 0.0f) {
    total_error /= weight_sum;
  }

  return 1.0f - fminf(total_error / 10.0f, 1.0f);
}

// Enhanced frame management
void prediction_add_enhanced_frame(ClientPrediction *pred,
                                   const PredictionFrame *frame) {
  if (!pred || !frame)
    return;

  uint32_t index = pred->head % MAX_PREDICTION_HISTORY;

  // Free old input state and custom data if it exists
  if (pred->history[index].input_state) {
    free(pred->history[index].input_state);
    pred->history[index].input_state = NULL;
  }
  for (uint32_t i = 0; i < pred->history[index].custom_data_count; i++) {
    if (pred->history[index].custom_data[i].data) {
      free(pred->history[index].custom_data[i].data);
    }
  }

  // Add new enhanced frame
  pred->history[index] = *frame;

  // Copy input state and custom data
  if (frame->input_state && frame->input_size > 0) {
    pred->history[index].input_state = malloc(frame->input_size);
    if (pred->history[index].input_state) {
      memcpy(pred->history[index].input_state, frame->input_state,
             frame->input_size);
      pred->history[index].input_size = frame->input_size;
    } else {
      pred->history[index].input_size = 0;
    }
  }

  for (uint32_t i = 0; i < frame->custom_data_count && i < MAX_STATE_COMPONENTS;
       i++) {
    if (frame->custom_data[i].data && frame->custom_data[i].data_size > 0) {
      pred->history[index].custom_data[i].data_type =
          frame->custom_data[i].data_type;
      pred->history[index].custom_data[i].data_size =
          frame->custom_data[i].data_size;
      pred->history[index].custom_data[i].data =
          malloc(frame->custom_data[i].data_size);
      if (pred->history[index].custom_data[i].data) {
        memcpy(pred->history[index].custom_data[i].data,
               frame->custom_data[i].data, frame->custom_data[i].data_size);
      }
    }
  }

  pred->head++;
  if (pred->count < MAX_PREDICTION_HISTORY) {
    pred->count++;
  }

  log_debug("Added enhanced prediction frame %u with %u components",
            frame->frame_number, __builtin_popcount(frame->component_mask));
}

// Enhanced rollback and replay for full state
void prediction_rollback_and_replay_full_state(ClientPrediction *pred,
                                               uint32_t entity_id,
                                               uint32_t confirmed_frame) {
  if (!pred)
    return;

  EntityPrediction *entity = get_entity_prediction(pred, entity_id);
  if (!entity)
    return;

  // Find confirmed frame
  PredictionFrame *confirmed_frame_data =
      get_prediction_frame(pred, confirmed_frame);
  if (!confirmed_frame_data) {
    log_error("Cannot find confirmed frame %u for rollback", confirmed_frame);
    return;
  }

  // Reset to confirmed state
  entity->predicted_position = entity->server_position;
  entity->predicted_velocity = entity->server_velocity;
  entity->predicted_rotation = entity->server_rotation;
  entity->predicted_angular_velocity = entity->server_angular_velocity;
  entity->predicted_scale = entity->server_scale;
  entity->predicted_health = entity->server_health;
  entity->predicted_animation = entity->server_animation;
  entity->predicted_physics = entity->server_physics;

  // Copy custom server data to predicted
  entity->predicted_custom_count = entity->server_custom_count;
  for (uint32_t i = 0;
       i < entity->server_custom_count && i < MAX_STATE_COMPONENTS; i++) {
    entity->predicted_custom[i] = entity->server_custom[i];
  }

  // Replay unconfirmed frames with full state prediction
  for (uint32_t i = 0; i < pred->count; i++) {
    uint32_t index = (pred->head - 1 - i) % MAX_PREDICTION_HISTORY;
    PredictionFrame *frame = &pred->history[index];

    if (frame->frame_number > confirmed_frame && !frame->is_confirmed) {
      // Re-predict this frame
      PredictionFrame next_state;
      prediction_predict_full_state(pred, entity_id, frame, 0.016f,
                                    &next_state);

      // Update frame with new prediction
      frame->position = next_state.position;
      frame->velocity = next_state.velocity;
      frame->rotation = next_state.rotation;
      frame->angular_velocity = next_state.angular_velocity;
      frame->scale = next_state.scale;
      frame->health = next_state.health;
      frame->animation_state = next_state.animation_state;
      frame->physics_state = next_state.physics_state;

      // Copy custom data
      frame->custom_data_count = next_state.custom_data_count;
      for (uint32_t j = 0;
           j < next_state.custom_data_count && j < MAX_STATE_COMPONENTS; j++) {
        frame->custom_data[j] = next_state.custom_data[j];
      }
    }
  }

  log_debug("Full state rollback and replay completed for entity %u",
            entity_id);
}

// Get frames that need to be re-simulated after reconciliation
uint32_t prediction_get_frames_to_resimulate(ClientPrediction *pred,
                                             PredictionFrame *frames,
                                             uint32_t max_frames) {
  if (!pred || !frames || max_frames == 0) {
    return 0;
  }

  uint32_t count = 0;
  for (uint32_t i = 0; i < pred->count && count < max_frames; i++) {
    uint32_t index = (pred->head - pred->count + i) % MAX_PREDICTION_HISTORY;
    if (pred->history[index].frame_number > pred->last_acknowledged_frame) {
      frames[count] = pred->history[index];
      count++;
    }
  }

  return count;
}
