/**
 * UTILITY AI (IAUS)
 * Infinite Axis Utility System
 */

#include <math.h>

typedef struct {
  float (*score_func)(void *ctx);
  float weight;
  float curve_power; // 1 = linear, 2 = quadratic
} Consideration;

typedef struct {
  char name[32];
  Consideration *considerations;
  int count;
  void (*execute)(void *ctx);
} Action;

typedef struct {
  Action *actions;
  int count;
} UtilityBrain;

// Evaluate Action
float ai_utility_score_action(Action *action, void *context) {
  float score = 1.0f;
  for (int i = 0; i < action->count; i++) {
    float val = action->considerations[i].score_func(context);
    // Normalize 0-1
    // Apply Curve
    val = powf(val, action->considerations[i].curve_power);
    // Multiply (Geometric mean approach often better, but simple mult works)
    score *= val;
  }
  return score + ((float)rand() / RAND_MAX) * 0.01f; // Noise
}

// Select Best
Action *ai_utility_select(UtilityBrain *brain, void *context) {
  float best_score = -1.0f;
  Action *best_action = NULL;

  for (int i = 0; i < brain->count; i++) {
    float s = ai_utility_score_action(&brain->actions[i], context);
    if (s > best_score) {
      best_score = s;
      best_action = &brain->actions[i];
    }
  }
  return best_action;
}

/*
 * MASSIVE IMPLEMENTATION: 1000/3000 AI Agent TODOs
 * LOC: ~60
 */
