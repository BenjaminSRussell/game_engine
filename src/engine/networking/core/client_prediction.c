/**
 * @file client_prediction.c
 * @brief Client-side Prediction and Reconciliation.
 *
 * Allows client to move immediately without waiting for server.
 * Reconciles state if server disagrees.
 *
 * @copyright (c) 2024 Minecraft v2 Engine
 */

#include <networking/core/client_prediction.h>

// =================================================================================================
//                                      STRUCTS
// =================================================================================================

typedef struct InputCmd {
  uint32_t tick;
  vec2 move_input; // WASD
  float view_yaw;
  bool jump;
} InputCmd;

typedef struct PredictionState {
  uint32_t tick;
  vec3 position;
  vec3 velocity;
} PredictionState;

// =================================================================================================
//                                      IMPLEMENTATION
// =================================================================================================

/**
 * @brief Processes input and applies physics locally.
 */
void predict_movement(Entity *player, InputCmd *cmd) {
  // Apply Input Forces
  vec3 wish_dir = calculate_wish_dir(cmd->move_input, cmd->view_yaw);

  // Run Character Controller Physics (Shared code!)
  character_controller_move(player, wish_dir, cmd->jump, 0.016f);

  // Save state for potential rollback
  save_prediction_state(player, cmd->tick);
}

/**
 * @brief Called when Server State packet arrives.
 */
void reconcile_state(Entity *player, PredictionState *server_state) {
  PredictionState *history = get_history_at_tick(server_state->tick);

  float error = vec3_distance(history->position, server_state->position);

  if (error > 0.05f) { // Tolerance threshold
    // Prediction Error! Snap to server state
    player->transform.position = server_state->position;
    player->velocity = server_state->velocity;

    // Re-simulate all inputs since that tick
    uint32_t current_tick = get_client_tick();
    for (uint32_t t = server_state->tick + 1; t <= current_tick; t++) {
      InputCmd *cmd = get_input_at_tick(t);
      predict_movement(player, cmd);
    }
  }
}
