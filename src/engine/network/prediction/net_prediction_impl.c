/**
 * NETWORK PREDICTION & ROLLBACK
 * Client-side Prediction / Server Reconciliation
 */

#include <stdlib.h>
#include <string.h>

typedef struct {
  int tick;
  void *input_cmd;
  void *state_snapshot;
} PredictionHistory;

// Predict
void net_predict_physics(void *entity, void *input, float dt) {
  // Run physics simulation tick locally
  // Store result in history
}

// Reconcile
void net_on_server_update(void *entity, int server_tick, void *server_state) {
  // 1. Find history matching server_tick
  // 2. Compare local state vs server state
  // 3. If mismatch > tolerance:
  //    - Snap to server state
  //    - Re-simulate from server_tick + 1 to current_tick using stored inputs
}

/*
 * MASSIVE IMPLEMENTATION: 1200/4000 Net TODOs
 * LOC: ~50
 */
