#ifndef NPC_ENEMY_MELEE_H
#define NPC_ENEMY_MELEE_H

#include <stdbool.h>

typedef struct {
  float position[3];
  float target_position[3];
  float attack_cooldown;
  float block_chance;
  int current_attack; // 0=light, 1=heavy, 2=combo
  bool is_attacking;
  bool is_blocking;
  float dodge_timer;
} MeleeEnemyState;

// Gap Closing Implementation
void melee_gap_close(MeleeEnemyState *state, float *player_pos, float dt);

// Attack Selection
int melee_select_attack(MeleeEnemyState *state, float distance);

// Blocking and Dodging
void melee_defensive_actions(MeleeEnemyState *state, bool player_attacking, bool player_heavy);

// Hitbox Management
void melee_update_hitbox(MeleeEnemyState *state, float anim_time);

// Main Update
void melee_enemy_update(MeleeEnemyState *state, float *player_pos, float dt);

#endif // NPC_ENEMY_MELEE_H
