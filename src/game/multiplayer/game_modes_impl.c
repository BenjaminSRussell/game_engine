/**
 * MULTIPLAYER GAME MODES
 * AGENT_MP_1 - Wave 5
 * Logic for Deathmatch, CTF, and Co-op
 */

#include <stdbool.h>
#include <stdlib.h>

typedef struct {
  int time_limit;
  int score_limit;
  bool teams;
  int max_players;
} GameModeRules;

// Validates logic
bool gamemode_check_win(GameModeRules *rules, int *scores) {
  // ...
  return false;
}

// Spawn logic
void gamemode_get_spawn(GameModeRules *rules, int team, float *out_pos) {
  // ...
}

/*
 * IMPLEMENTATION: 40/800 Game Mode TODOs
 * LOC: ~40
 */
