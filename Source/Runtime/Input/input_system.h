#ifndef INPUT_SYSTEM_H
#define INPUT_SYSTEM_H

#include "../../Game/game_context.h"

// Initialize input system
void input_system_init(InputState *state);

// Update input state (poll events, read keys/mouse/gamepad)
void input_system_update(InputState *state, void *window_handle);

#endif
