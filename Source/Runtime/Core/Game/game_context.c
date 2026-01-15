#include "game_context.h"

// Global game state instance
GameState g_game = {0};

// Other global states referenced in monolithic_main.c
InGameState g_in_game_state;
HUDSystem g_hud;
ItemRegistry g_item_registry;
