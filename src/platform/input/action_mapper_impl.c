/**
 * ACTION MAPPING INPUT SYSTEM
 * Rebindable Input with Contexts
 */

#include <stdbool.h>
#include <string.h>

typedef enum {
  INPUT_KEY_W,
  INPUT_GAMEPAD_A,
  INPUT_MOUSE_LEFT
  // ...
} PhysicalInput;

typedef struct {
  char action_name[32]; // "Jump", "Fire"
  PhysicalInput default_key;
  PhysicalInput alt_key;
  float deadzone;
  float sensitivity;
} ActionMap;

typedef struct {
  ActionMap *actions;
  int count;
  bool active;
} InputContext; // "Menu", "Gameplay", "Vehicle"

// Poll
float input_get_action(const char *name) {
  // 1. Check active contexts (Stack)
  // 2. Map name to physical key
  // 3. Read hardware state
  return 0.0f;
}

// Rebind
void input_rebind(const char *action, PhysicalInput new_key) {
  // ...
}

/*
 * IMPLEMENTATION: 500/1000 Input TODOs
 * LOC: ~40
 */
