// include/game/game_hooks.h
//
// Purpose: Defines mod API hooks for game-specific events.
// This header provides hook points for mods to subscribe to game events
// like world initialization, game mode changes, player actions, and world ticks.
//
// Public APIs:
// - `game_hook_world_init`: Triggered when the game world is initialized
// - `game_hook_game_mode_changed`: Triggered when game mode changes (Survival->Creative, etc.)
// - `game_hook_world_tick`: Triggered at the start of each game tick
// - `game_hook_player_action`: Triggered when player performs block/item action
// - `game_hook_world_loaded`: Triggered when a saved world is loaded
// - `game_hook_world_saved`: Triggered when world is saved
//

#ifndef GAME_GAME_HOOKS_H
#define GAME_GAME_HOOKS_H

#include "stdint.h"

#ifdef __cplusplus
extern "C" {
#endif

// Forward declarations
struct ModRegistry;
struct GameState;
struct GameModeState;

// Event data structures
typedef struct {
    struct GameState *game_state;
    struct GameModeState *mode_state;
} GameInitEventData;

typedef struct {
    int old_mode;
    int new_mode;
    int difficulty;
} GameModeChangeEventData;

typedef struct {
    float delta_time;
    uint64_t tick_count;
} GameWorldTickEventData;

typedef struct {
    int action_type;  // 0 = place, 1 = break, 2 = use
    float x, y, z;    // position
} PlayerActionEventData;

// Hook registration
void game_hook_init(struct ModRegistry *registry);

// Hook triggers (called by game systems)
void game_hook_trigger_world_init(struct GameState *state, struct GameModeState *mode);
void game_hook_trigger_game_mode_changed(int old_mode, int new_mode, int difficulty);
void game_hook_trigger_world_tick(float dt, uint64_t tick_count);
void game_hook_trigger_player_action(int action_type, float x, float y, float z);
void game_hook_trigger_world_loaded(const char *world_name);
void game_hook_trigger_world_saved(const char *world_name);

#ifdef __cplusplus
}
#endif

#endif /* GAME_GAME_HOOKS_H */
