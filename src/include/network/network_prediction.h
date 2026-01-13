// Networking/Network Prediction
#ifndef NETWORK_PREDICTION_H
#define NETWORK_PREDICTION_H

#include "include/common.h"
#include "math/vec3.h"

#define MAX_PREDICTION_HISTORY 64

typedef struct {
  u32 frame_number;
  Vec3 position;
  Vec3 velocity;
  f32 timestamp;
  void *input_state; // Serialized input
} PredictionFrame;

typedef struct {
  PredictionFrame history[MAX_PREDICTION_HISTORY];
  u32 head;
  u32 count;

  u32 last_acknowledged_frame;
  bool mispredicted;

} ClientPrediction;

#ifdef __cplusplus
extern "C" {
#endif

ClientPrediction *prediction_create(void);
void prediction_destroy(ClientPrediction *pred);
void prediction_add_frame(ClientPrediction *pred, u32 frame, Vec3 pos, Vec3 vel);
void prediction_add_frame_with_input(ClientPrediction *pred, u32 frame, Vec3 pos, 
                                   Vec3 vel, void *input_state, u32 input_size);
void prediction_reconcile(ClientPrediction *pred, u32 server_frame, Vec3 server_pos);
bool prediction_get_frame(ClientPrediction *pred, u32 frame, PredictionFrame *out_frame);
Vec3 prediction_get_current_position(ClientPrediction *pred);
Vec3 prediction_get_current_velocity(ClientPrediction *pred);
bool prediction_was_mispredicted(ClientPrediction *pred);
u32 prediction_get_last_acknowledged_frame(ClientPrediction *pred);
void prediction_clear_old_frames(ClientPrediction *pred, u32 before_frame);
u32 prediction_get_frames_to_resimulate(ClientPrediction *pred, PredictionFrame *frames, u32 max_frames);

#ifdef __cplusplus
}
#endif

#endif
