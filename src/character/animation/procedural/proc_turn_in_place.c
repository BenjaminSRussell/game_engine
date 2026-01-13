#include "include/animation/animation_system.h"
#include "include/common.h"
#include "include/math/math.h"
#include <math.h>

typedef struct {
  float current_yaw;
  float target_yaw;
  bool is_turning;
  float turn_speed;
} TurnContext;

static TurnContext g_turn = {0};

void proc_turn_in_place_init(void) {
  g_turn.turn_speed = 180.0f; // degrees/sec
}

void proc_turn_in_place_update(float dt, float character_yaw, float desired_yaw,
                               float velocity) {
  g_turn.current_yaw = character_yaw;
  g_turn.target_yaw = desired_yaw;

  float delta = g_turn.target_yaw - g_turn.current_yaw;
  while (delta > 180.0f)
    delta -= 360.0f;
  while (delta < -180.0f)
    delta += 360.0f;

  // Trigger turn-in-place when standing still and large angle difference
  if (fabsf(delta) > 45.0f && velocity < 0.1f) {
    g_turn.is_turning = true;

    // Select animation based on turn direction and magnitude
    // Play turn_left_90, turn_right_90, etc.

    // Procedurally rotate if no animation
    float turn_amount = (delta > 0 ? 1.0f : -1.0f) * g_turn.turn_speed * dt;
    g_turn.current_yaw += turn_amount;
  } else {
    g_turn.is_turning = false;
  }
}

int proc_turn_is_turning(void) { return g_turn.is_turning; }
