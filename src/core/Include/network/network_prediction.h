// Networking/Network Prediction
#ifndef NETWORK_PREDICTION_H
#define NETWORK_PREDICTION_H

#include "math/quat.h"
#include "math/vec3.h"
#include <common.h>

#define MAX_PREDICTION_HISTORY 64
#define MAX_ENTITIES 1024
#define MAX_STATE_COMPONENTS 16

// Entity state components for comprehensive prediction
typedef enum {
  STATE_COMPONENT_POSITION = 1 << 0,
  STATE_COMPONENT_VELOCITY = 1 << 1,
  STATE_COMPONENT_ROTATION = 1 << 2,
  STATE_COMPONENT_ANGULAR_VELOCITY = 1 << 3,
  STATE_COMPONENT_SCALE = 1 << 4,
  STATE_COMPONENT_HEALTH = 1 << 5,
  STATE_COMPONENT_ANIMATION_STATE = 1 << 6,
  STATE_COMPONENT_PHYSICS_STATE = 1 << 7,
  STATE_COMPONENT_CUSTOM_DATA = 1 << 8
} StateComponentMask;

// Animation state for prediction
typedef struct {
  u32 clip_id;
  f32 playback_time;
  f32 blend_weight;
  u32 state_flags;
} AnimationState;

// Physics state for prediction
typedef struct {
  Vec3 force;
  Vec3 torque;
  bool is_grounded;
  bool is_sleeping;
  f32 mass;
} PhysicsState;

// Custom data for extensible prediction
typedef struct {
  u32 data_type;
  u32 data_size;
  void *data;
} CustomStateData;

// Enhanced prediction frame with multiple state components
typedef struct {
  u32 frame_number;
  Vec3 position;
  Vec3 velocity;
  Vec3 acceleration;
  Quat rotation;
  Vec3 angular_velocity;
  Vec3 scale;
  f32 health;
  AnimationState animation_state;
  PhysicsState physics_state;
  CustomStateData custom_data[MAX_STATE_COMPONENTS];
  u32 custom_data_count;
  f32 timestamp;
  void *input_state;
  u32 input_size;
  u32 component_mask;
  bool is_confirmed;
} PredictionFrame;

// Entity prediction state
typedef struct {
  u32 entity_id;
  Vec3 predicted_position;
  Vec3 predicted_velocity;
  Quat predicted_rotation;
  Vec3 predicted_angular_velocity;
  Vec3 predicted_scale;
  f32 predicted_health;
  AnimationState predicted_animation;
  PhysicsState predicted_physics;
  CustomStateData predicted_custom[MAX_STATE_COMPONENTS];
  u32 predicted_custom_count;

  // Server state for reconciliation
  Vec3 server_position;
  Vec3 server_velocity;
  Quat server_rotation;
  Vec3 server_angular_velocity;
  Vec3 server_scale;
  f32 server_health;
  AnimationState server_animation;
  PhysicsState server_physics;
  CustomStateData server_custom[MAX_STATE_COMPONENTS];
  u32 server_custom_count;

  // Error tracking
  f32 position_error;
  f32 rotation_error;
  f32 velocity_error;
  f32 health_error;
  bool needs_reconciliation;
  u32 last_confirmed_frame;
  u32 component_mask;
} EntityPrediction;

typedef struct {
  PredictionFrame history[MAX_PREDICTION_HISTORY];
  u32 head;
  u32 count;
  u32 last_acknowledged_frame;
  bool mispredicted;

  // Entity predictions
  EntityPrediction entities[MAX_ENTITIES];
  u32 entity_count;

  // Smoothing state
  Vec3 smoothed_position;
  Quat smoothed_rotation;
  Vec3 target_position;
  Quat target_rotation;
  f32 smoothing_time;

  // Rollback buffer
  PredictionFrame rollback_buffer[32];
  uint32_t rollback_head;

  // Network quality metrics
  f32 rtt;
  f32 packet_loss;
  f32 prediction_accuracy;

  // Component prediction settings
  u32 enabled_components;
  f32 component_weights[MAX_STATE_COMPONENTS];
} ClientPrediction;

#ifdef __cplusplus
extern "C" {
#endif

// Core prediction functions
ClientPrediction *prediction_create(void);
void prediction_destroy(ClientPrediction *pred);

// Enhanced frame management
void prediction_add_frame(ClientPrediction *pred, u32 frame, Vec3 pos,
                          Vec3 vel);
void prediction_add_frame_with_input(ClientPrediction *pred, u32 frame,
                                     Vec3 pos, Vec3 vel, void *input_state,
                                     u32 input_size);
void prediction_add_enhanced_frame(ClientPrediction *pred,
                                   const PredictionFrame *frame);

// Enhanced prediction functions
Vec3 prediction_predict_position(ClientPrediction *pred, u32 entity_id,
                                 Vec3 current_pos, Vec3 current_vel,
                                 Vec3 current_acc, f32 delta_time);
Quat prediction_predict_rotation(ClientPrediction *pred, u32 entity_id,
                                 Quat current_rot, Vec3 angular_vel,
                                 f32 delta_time);
void prediction_predict_full_state(ClientPrediction *pred, u32 entity_id,
                                   const PredictionFrame *current_state,
                                   f32 delta_time,
                                   PredictionFrame *predicted_state);

// Enhanced reconciliation
void prediction_reconcile(ClientPrediction *pred, u32 entity_id,
                          u32 confirmed_frame, Vec3 server_pos,
                          Vec3 server_vel);
void prediction_reconcile_full_state(ClientPrediction *pred, u32 entity_id,
                                     u32 confirmed_frame,
                                     const PredictionFrame *server_state);

// Component management
void prediction_enable_components(ClientPrediction *pred, u32 component_mask);
void prediction_set_component_weight(ClientPrediction *pred, u32 component_type,
                                     f32 weight);
bool prediction_is_component_enabled(ClientPrediction *pred,
                                     u32 component_type);

// Custom data support
void prediction_add_custom_data(ClientPrediction *pred, u32 entity_id,
                                u32 data_type, void *data, u32 data_size);
void *prediction_get_custom_data(ClientPrediction *pred, u32 entity_id,
                                 u32 data_type);

// Enhanced state access
void prediction_get_predicted_state(ClientPrediction *pred, u32 entity_id,
                                    PredictionFrame *state);
void prediction_get_smoothed_state(ClientPrediction *pred, u32 entity_id,
                                   PredictionFrame *state);
void prediction_get_server_state(ClientPrediction *pred, u32 entity_id,
                                 PredictionFrame *state);

// Error and quality metrics
void prediction_get_entity_errors(ClientPrediction *pred, u32 entity_id,
                                  f32 *pos_error, f32 *rot_error,
                                  f32 *vel_error, f32 *health_error);
f32 prediction_get_entity_accuracy(ClientPrediction *pred, u32 entity_id);

// Legacy compatibility functions
bool prediction_get_frame(ClientPrediction *pred, u32 frame,
                          PredictionFrame *out_frame);
Vec3 prediction_get_current_position(ClientPrediction *pred);
Vec3 prediction_get_current_velocity(ClientPrediction *pred);
bool prediction_was_mispredicted(ClientPrediction *pred);
u32 prediction_get_last_acknowledged_frame(ClientPrediction *pred);
void prediction_clear_old_frames(ClientPrediction *pred, u32 before_frame);
u32 prediction_get_frames_to_resimulate(ClientPrediction *pred,
                                        PredictionFrame *frames,
                                        u32 max_frames);

#ifdef __cplusplus
}
#endif

#endif
