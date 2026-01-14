/**
 * =================================================================================================
 *                          NPC ENEMY MELEE
 * =================================================================================================
 *
 * Close-quarters combat AI.
 *
 * =================================================================================================
 */

#include <include/math/math_all.h>
#include <ai/npc/prebuilt/npc_enemy_melee.h>
#include <stdlib.h>

// MeleeEnemyState defined in header

// Gap Closing Implementation
void melee_gap_close(MeleeEnemyState *state, float *player_pos, float dt) {
  // Calculate direction to player
  float dx = player_pos[0] - state->position[0];
  float dy = player_pos[1] - state->position[1];
  float dz = player_pos[2] - state->position[2];
  float dist = sqrtf(dx * dx + dy * dy + dz * dz);

  if (dist > 2.0f) { // Outside attack range
    // Rush towards player
    float rush_speed = 8.0f;
    state->position[0] += (dx / dist) * rush_speed * dt;
    state->position[1] += (dy / dist) * rush_speed * dt;
    state->position[2] += (dz / dist) * rush_speed * dt;

    // Jump over obstacles if needed
    // navmesh_find_path(state->position, player_pos);
  }
}

// Attack Selection
int melee_select_attack(MeleeEnemyState *state, float distance) {
  if (state->attack_cooldown > 0.0f)
    return -1;

  if (distance < 1.5f) {
    // Close range - prefer light attacks or combos
    if (rand() % 100 < 60) {
      return 0; // Light attack
    } else {
      return 2; // Combo
    }
  } else if (distance < 3.0f) {
    // Medium range - heavy attack to close gap
    return 1;
  }
  return -1;
}

// Blocking and Dodging
void melee_defensive_actions(MeleeEnemyState *state, bool player_attacking,
                             bool player_heavy) {
  // Random chance to block
  if (player_attacking && (rand() % 100) < (int)(state->block_chance * 100)) {
    state->is_blocking = true;
    // animation_play("block");
  }

  // Dodge roll if player winds up heavy attack
  if (player_heavy && state->dodge_timer <= 0.0f) {
    // Dodge perpendicular to player direction
    state->dodge_timer = 2.0f; // Cooldown
                               // animation_play("dodge_roll");
  }
}

// Hitbox Management
void melee_update_hitbox(MeleeEnemyState *state, float anim_time) {
  // Enable weapon hitbox during attack frames
  if (state->is_attacking) {
    // Attack animation typically has active frames 0.2-0.4s
    if (anim_time > 0.2f && anim_time < 0.4f) {
      // physics_enable_hitbox(state->weapon_hitbox);
    } else {
      // physics_disable_hitbox(state->weapon_hitbox);
    }
  }
}

void melee_enemy_update(MeleeEnemyState *state, float *player_pos, float dt) {
  // Update cooldowns
  if (state->attack_cooldown > 0.0f) {
    state->attack_cooldown -= dt;
  }
  if (state->dodge_timer > 0.0f) {
    state->dodge_timer -= dt;
  }

  // Gap closing
  melee_gap_close(state, player_pos, dt);

  // Attack selection
  float dx = player_pos[0] - state->position[0];
  float dz = player_pos[2] - state->position[2];
  float dist = sqrtf(dx * dx + dz * dz);

  int attack = melee_select_attack(state, dist);
  if (attack >= 0) {
    state->current_attack = attack;
    state->is_attacking = true;
    state->attack_cooldown = 1.5f;
  }
}
