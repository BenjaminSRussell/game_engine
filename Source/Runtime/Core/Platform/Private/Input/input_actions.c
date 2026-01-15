#include "../Public/unified_input.h"
#include <string.h>

static const char *k_input_action_names[] = {
    "MOVE_FORWARD",  "MOVE_BACKWARD", "MOVE_LEFT",   "MOVE_RIGHT",
    "JUMP",          "SPRINT",        "CROUCH",      "INTERACT",
    "ATTACK",        "BLOCK",         "INVENTORY",   "CRAFT",
    "BUILD",         "DESTROY",       "USE_ITEM",    "DROP_ITEM",
    "HOTBAR_1",      "HOTBAR_2",      "HOTBAR_3",    "HOTBAR_4",
    "HOTBAR_5",      "HOTBAR_6",      "HOTBAR_7",    "HOTBAR_8",
    "HOTBAR_9",      "HOTBAR_PREV",   "HOTBAR_NEXT", "MENU",
    "ENTER_VEHICLE", "EXIT_VEHICLE",  "CHAT",        "DEBUG",
    "SCREENSHOT",    "MAP",           "LOOK",        "ZOOM",
    "RADIAL_MENU",
};

const char *input_action_get_name(InputAction action) {
  if (action >= INPUT_ACTION_COUNT) {
    return "UNKNOWN";
  }
  return k_input_action_names[action];
}
