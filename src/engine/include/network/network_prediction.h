// Networking/Network Prediction
#ifndef NETWORK_PREDICTION_H
#define NETWORK_PREDICTION_H

#include "include/common.h"
#include "include/math/vec3.h"

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
void prediction_add_frame(ClientPrediction *pred, u32 frame, Vec3 pos,
                          Vec3 vel);
void prediction_reconcile(ClientPrediction *pred, u32 server_frame,
                          Vec3 server_pos);

#ifdef __cplusplus
}
#endif

#endif
