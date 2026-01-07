#ifndef GAME_GAME_H
#define GAME_GAME_H

#include "game/mode.h"
#include "game/state.h"

#ifdef __cplusplus
extern "C" {
#endif

/* Forward declarations */
struct ECSWorld;
struct ModRegistry;

/* Initialize game systems. Returns 0 on success. */
int game_init(void);

/* Advance game by dt seconds. */
void game_tick(float dt);

/* Shutdown game systems and free resources. */
void game_shutdown(void);

/* Get a pointer to the global game state. */
struct GameState *game_state(void);

/* Get the ECS world instance. */
World *game_get_ecs_world(void);

/* Get the game mode state. */
GameModeState *game_get_mode_state(void);

/* Get the mod registry. */
struct ModRegistry *game_get_mod_registry(void);

#ifdef __cplusplus
}
#endif

#endif /* GAME_GAME_H */
