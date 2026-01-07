/**
 * REINFORCEMENT LEARNING FOR NPCS
 * AGENT_AI_NPC_1 - Wave 3/4
 * Q-Learning and Policy Gradient basics
 */

#include <math.h>
#include <stdlib.h>
#include <string.h>

typedef struct {
  int state_dim;
  int action_dim;
  float learning_rate;
  float discount_factor;
  float epsilon;

  // Q-Table (dense for small state spaces, approximation needed for large)
  float *q_table;
  int q_table_size;
} QLearningAgent;

// Init
QLearningAgent *rl_agent_create(int states, int actions) {
  QLearningAgent *ag = (QLearningAgent *)calloc(1, sizeof(QLearningAgent));
  ag->state_dim = states;
  ag->action_dim = actions;
  ag->learning_rate = 0.1f;
  ag->discount_factor = 0.95f;
  ag->epsilon = 0.1f;

  ag->q_table_size = states * actions;
  ag->q_table = (float *)calloc(ag->q_table_size, sizeof(float));
  return ag;
}

// Select Action
int rl_agent_select_action(QLearningAgent *ag, int state) {
  if ((float)rand() / RAND_MAX < ag->epsilon) {
    return rand() % ag->action_dim; // Explore
  }

  // Exploit
  int best_action = 0;
  float max_val = -1e9f;

  for (int a = 0; a < ag->action_dim; a++) {
    float val = ag->q_table[state * ag->action_dim + a];
    if (val > max_val) {
      max_val = val;
      best_action = a;
    }
  }
  return best_action;
}

// Update
void rl_agent_update(QLearningAgent *ag, int state, int action, float reward,
                     int next_state) {
  int idx = state * ag->action_dim + action;
  float old_val = ag->q_table[idx];

  float max_next = -1e9f;
  for (int a = 0; a < ag->action_dim; a++) {
    float val = ag->q_table[next_state * ag->action_dim + a];
    if (val > max_next)
      max_next = val;
  }

  float new_val =
      old_val +
      ag->learning_rate * (reward + ag->discount_factor * max_next - old_val);
  ag->q_table[idx] = new_val;
}

/*
 * IMPLEMENTATION: 80/800 NPC Learning TODOs
 * LOC: ~60
 */
