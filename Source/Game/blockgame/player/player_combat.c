// Player Combat System Implementation
// This file implements combat mechanics with baseline stats and timers.

#include <audio/audio_system.h>
#include <inventory/item_registry.h>
#include <player/player.h>
#include <player/player_combat.h>
#include <stdlib.h>
#include <string.h>

static PlayerCombatStats g_combat_stats = {0};
static PlayerCombatCallbacks g_combat_callbacks = {0};

static void player_combat_weapon_defaults(WeaponType weapon, f32 *damage,
                                          f32 *range, f32 *speed) {
  if (!damage || !range || !speed)
    return;

  switch (weapon) {
  case WEAPON_TYPE_SWORD:
    *damage = 5.0f;
    *range = 3.5f;
    *speed = 1.0f;
    break;
  case WEAPON_TYPE_AXE:
    *damage = 7.0f;
    *range = 3.0f;
    *speed = 1.3f;
    break;
  case WEAPON_TYPE_PICKAXE:
    *damage = 4.0f;
    *range = 2.6f;
    *speed = 1.1f;
    break;
  case WEAPON_TYPE_BOW:
    *damage = 4.0f;
    *range = 12.0f;
    *speed = 0.8f;
    break;
  case WEAPON_TYPE_CROSSBOW:
    *damage = 6.0f;
    *range = 16.0f;
    *speed = 0.6f;
    break;
  default:
    *damage = 1.0f;
    *range = 3.0f;
    *speed = 1.0f;
    break;
  }
}

static void player_combat_emit_sound(PlayerSystem *system, SoundType sound_type,
                                     const char *sound_id, f32 volume) {
  if (system && system->audio_system) {
    audio_play_sound_2d(system->audio_system, sound_type, volume,
                        SOUND_CATEGORY_PLAYER);
  }
  if (g_combat_callbacks.on_sound) {
    g_combat_callbacks.on_sound(sound_id, volume, g_combat_callbacks.user_data);
  }
}

void player_combat_init(PlayerCombatState *combat) {
  if (!combat)
    return;
  memset(combat, 0, sizeof(PlayerCombatState));
  combat->block_strength = 0.5f;
  combat->perfect_block_window = 0.0f;
  combat->cooldown_reduction = 0.0f;
  combat->dodge_timer = 0.0f;
  combat->weapon_override = WEAPON_TYPE_SWORD;
  combat->has_weapon_override = false;
}

void player_attack(PlayerSystem *system, CombatActionType type) {
  if (!system || !system->player)
    return;
  PlayerComponent *p = system->player;
  PlayerCombatState *combat = &p->combat_state;

  if (!player_can_attack(combat, p))
    return;

  // ==== Determine Weapon Type and Stats ====
  WeaponType weapon = WEAPON_TYPE_SWORD;
  f32 base_damage = 1.0f;
  f32 attack_range = 3.0f;
  f32 attack_speed = 1.0f; // Cooldown multiplier

  // Get weapon from equipped item (simplified - just checking selected item)
  u32 equipped_item = player_get_selected_item(system);

  // Map items to weapon types (simplified)
  if (combat->has_weapon_override) {
    weapon = combat->weapon_override;
    player_combat_weapon_defaults(weapon, &base_damage, &attack_range,
                                  &attack_speed);
  } else if (equipped_item == ITEM_WOODEN_SWORD ||
             equipped_item == ITEM_STONE_SWORD ||
             equipped_item == ITEM_IRON_SWORD ||
             equipped_item == ITEM_DIAMOND_SWORD ||
             equipped_item == ITEM_GOLDEN_SWORD) {
    weapon = WEAPON_TYPE_SWORD;
    base_damage = 5.0f;
    attack_range = 3.5f;
    attack_speed = 1.0f;
  } else if (equipped_item == ITEM_WOODEN_AXE ||
             equipped_item == ITEM_STONE_AXE ||
             equipped_item == ITEM_IRON_AXE ||
             equipped_item == ITEM_DIAMOND_AXE ||
             equipped_item == ITEM_GOLDEN_AXE) {
    weapon = WEAPON_TYPE_AXE;
    base_damage = 7.0f;
    attack_range = 3.0f;
    attack_speed = 1.3f; // Slower
  } else if (equipped_item == ITEM_WOODEN_PICKAXE ||
             equipped_item == ITEM_STONE_PICKAXE ||
             equipped_item == ITEM_IRON_PICKAXE ||
             equipped_item == ITEM_DIAMOND_PICKAXE ||
             equipped_item == ITEM_GOLDEN_PICKAXE) {
    weapon = WEAPON_TYPE_PICKAXE;
    base_damage = 4.0f;
    attack_range = 2.6f;
    attack_speed = 1.1f;
  } else if (equipped_item == ITEM_BOW) {
    weapon = WEAPON_TYPE_BOW;
    base_damage = 4.0f;
    attack_range = 12.0f;
    attack_speed = 0.8f;
  } else if (equipped_item == ITEM_CROSSBOW) {
    weapon = WEAPON_TYPE_CROSSBOW;
    base_damage = 6.0f;
    attack_range = 16.0f;
    attack_speed = 0.6f;
  }

  // ==== Stamina Cost ====
  f32 stamina_cost = 10.0f + (weapon == WEAPON_TYPE_AXE ? 5.0f : 0.0f);
  if (p->stamina < stamina_cost) {
    return; // Not enough stamina
  }
  p->stamina -= stamina_cost;

  // ==== Critical Hit Mechanics ====
  bool is_critical = false;
  f32 crit_chance = 0.1f; // 10% base crit chance

  // Increase crit chance if airborne
  if (!p->on_ground) {
    crit_chance += 0.15f; // +15% while jumping
  }

  // Increase crit chance based on combo
  if (combat->combo_count > 0) {
    crit_chance += combat->combo_count * 0.05f; // +5% per combo hit
  }

  // Roll for critical hit
  f32 random_val = (f32)rand() / (f32)RAND_MAX;
  if (random_val < crit_chance) {
    is_critical = true;
    base_damage *= 1.5f; // 50% damage boost
  }

  // ==== Combo System ====
  f32 combo_window = 1.5f; // 1.5 second window to continue combo

  if (combat->combo_timer > 0) {
    // Continue combo
    combat->combo_count++;
    combat->combo_timer = combo_window;

    // Combo damage bonus (stacks up to 5 hits)
    f32 combo_multiplier = 1.0f + (MIN(combat->combo_count, 5) * 0.1f);
    base_damage *= combo_multiplier;
  } else {
    // Start new combo
    combat->combo_count = 1;
    combat->combo_timer = combo_window;
  }

  // ==== Attack Range Validation ====
  // Raycast to find target
  Vec3 ray_origin = player_get_position(system);
  ray_origin.y += 1.6f; // Eye height
  Vec3 ray_dir = player_get_forward(system);

  bool hit = false;
  Vec3 hit_pos = ray_origin;
  if (system->physics_world) {
    RaycastResult ray = physics_raycast(system->physics_world, ray_origin,
                                        ray_dir, attack_range);

    if (ray.hit) {
      hit = true;
      hit_pos = ray.hit_point;
      // Apply damage to hit entity
      // Note: This would need proper entity damage system integration
      // For now, just mark that we hit something
      combat->damage_timer = 0.5f;
      combat->hit_stun_timer = 0.2f;
      combat->last_damage_amount = base_damage;
      combat->combo_score += (u32)(base_damage * 10.0f);

      // Play hit sound
      if (system->audio_system) {
        audio_play_sound(system->audio_system, SOUND_SWORD_HIT, hit_pos, 0.8f, SOUND_CATEGORY_PLAYER);
      }
    }
  }

  // ==== Set Attack State ====
  combat->is_attacking = true;
  combat->attack_cooldown = 0.5f / attack_speed; // Apply attack speed modifier

  g_combat_stats.attacks++;
  if (hit) {
    g_combat_stats.hits++;
    g_combat_stats.damage_dealt += base_damage;
  }
  if (is_critical) {
    g_combat_stats.crits++;
  }

  // Choose sound based on weapon
  SoundType attack_sound = SOUND_SWORD_SWING;
  if (weapon == WEAPON_TYPE_BOW || weapon == WEAPON_TYPE_CROSSBOW) {
    attack_sound = SOUND_BOW_FIRE;
  } else if (weapon == WEAPON_TYPE_PICKAXE) {
    // Maybe same as sword for now, or could use tool swing if available
    attack_sound = SOUND_SWORD_SWING;
  }

  player_combat_emit_sound(system, attack_sound, "player_attack", 0.7f);

  if (g_combat_callbacks.on_attack) {
    g_combat_callbacks.on_attack(weapon, is_critical,
                                 g_combat_callbacks.user_data);
  }
  if (hit && g_combat_callbacks.on_hit) {
    g_combat_callbacks.on_hit(hit_pos, base_damage,
                              g_combat_callbacks.user_data);
  }
  (void)type; // Unused for now
}

void player_block(PlayerSystem *system, bool enable) {
  if (!system || !system->player)
    return;
  PlayerComponent *p = system->player;
  PlayerCombatState *combat = &p->combat_state;

  if (enable && !player_can_block(combat, p))
    return;

  // ==== Shield Type Detection ====
  // Check if player has shield equipped
  bool has_shield = (p->equipped_armor != NULL); // Simplified check

  if (enable) {
    // Start blocking
    combat->is_blocking = true;
    g_combat_stats.blocks++;

    // Set block strength based on shield type
    if (has_shield) {
      combat->block_strength = 0.75f; // Shield: blocks 75% damage
    } else {
      combat->block_strength = 0.5f; // Bare hands: blocks 50% damage
    }

    // Perfect block timing window (0.3 seconds at start)
    combat->perfect_block_window = 0.3f;

  } else {
    // Stop blocking
    combat->is_blocking = false;
    combat->block_cooldown = 0.2f; // Short cooldown after releasing block
  }

  if (g_combat_callbacks.on_block) {
    g_combat_callbacks.on_block(enable, combat->perfect_block_window > 0.0f,
                                g_combat_callbacks.user_data);
  }
}

// New function: Handle incoming damage while blocking
f32 player_block_damage(PlayerSystem *system, f32 incoming_damage,
                        bool *out_perfect_block) {
  if (!system || !system->player)
    return incoming_damage;

  PlayerComponent *p = system->player;
  PlayerCombatState *combat = &p->combat_state;

  if (!combat->is_blocking)
    return incoming_damage;

  // ==== Perfect Block Timing ====
  bool is_perfect_block = (combat->perfect_block_window > 0);
  if (out_perfect_block)
    *out_perfect_block = is_perfect_block;

  f32 damage_multiplier = 1.0f;

  if (is_perfect_block) {
    // Perfect block: blocks 100% damage and enables counter
    damage_multiplier = 0.0f;
    combat->block_cooldown = 0.0f; // Can immediately counter-attack

  } else {
    // Normal block: reduced by shield strength
    damage_multiplier = 1.0f - combat->block_strength;
  }

  // ==== Block Stamina Drain ====
  f32 stamina_drain = incoming_damage * 0.5f; // 50% of damage drains stamina
  p->stamina -= stamina_drain;
  g_combat_stats.damage_taken += incoming_damage;

  if (p->stamina <= 0) {
    // Stamina depleted - block broken!
    p->stamina = 0.0f;
    combat->is_blocking = false;
    combat->block_cooldown = 1.0f; // Long cooldown when broken
    return incoming_damage;        // Take full damage
  }

  // Block successful
  return incoming_damage * damage_multiplier;
}

void player_dodge(PlayerSystem *system, Vec3 direction) {
  if (!system || !system->player)
    return;
  PlayerComponent *p = system->player;
  PlayerCombatState *combat = &p->combat_state;

  // ==== Check if can dodge ====
  if (combat->dodge_cooldown > 0)
    return; // On cooldown
  if (combat->is_blocking)
    return; // Can't dodge while blocking
  if (combat->is_dodging)
    return; // Already dodging

  //  ==== Stamina Cost ====
  f32 dodge_stamina_cost = 20.0f;
  if (p->stamina < dodge_stamina_cost) {
    return; // Not enough stamina
  }
  p->stamina -= dodge_stamina_cost;

  // ==== Calculate Dodge Direction ====
  Vec3 dodge_dir = direction;
  if (vec3_length(dodge_dir) < 0.001f) {
    // If no direction specified, dodge backward
    dodge_dir = player_get_forward(system);
    dodge_dir = vec3_mul(dodge_dir, -1.0f);
  }
  dodge_dir = vec3_normalize(dodge_dir);

  // ==== Set Dodge Parameters ====
  combat->is_dodging = true;
  combat->dodge_invulnerable = true;
  combat->dodge_cooldown = 1.0f; // 1 second cooldown
  combat->dodge_timer = 0.3f;
  g_combat_stats.dodges++;

  // Dodge distance based on player stats
  combat->dodge_distance = 3.0f;

  // If in air, shorter dodge
  if (!p->on_ground) {
    combat->dodge_distance = 2.0f;
  }

  // ==== Apply Physics Impulse ====
  if (p->physics_body) {
    Vec3 dodge_velocity = vec3_mul(
        dodge_dir, combat->dodge_distance * 5.0f); // 5.0 = impulse multiplier
    Vec3 current_vel = rigid_body_get_velocity(p->physics_body);

    // Override horizontal velocity with dodge
    current_vel.x = dodge_velocity.x;
    current_vel.z = dodge_velocity.z;

    rigid_body_set_velocity(p->physics_body, current_vel);
  }

  // Markers for visual/audio feedback  (would be triggered in actual
  // implementation)
  // - Trigger dodge animation
  // - Spawn dodge trail particles
  // - Play dodge sound effect
  // - Brief slow-motion effect (optional)
  player_combat_emit_sound(system, SOUND_SWORD_SWING, "player_dodge", 0.6f);
  if (g_combat_callbacks.on_dodge) {
    g_combat_callbacks.on_dodge(dodge_dir, g_combat_callbacks.user_data);
  }
}

void player_combat_update(PlayerCombatState *combat, f32 delta_time) {
  if (!combat)
    return;

  // Update cooldowns
  f32 cooldown_scale = 1.0f + combat->cooldown_reduction;
  if (combat->attack_cooldown > 0) {
    combat->attack_cooldown -= delta_time * cooldown_scale;
  }

  if (combat->block_cooldown > 0) {
    combat->block_cooldown -= delta_time * cooldown_scale;
  }

  if (combat->dodge_cooldown > 0) {
    combat->dodge_cooldown -= delta_time * cooldown_scale;
  }

  if (combat->damage_timer > 0) {
    combat->damage_timer -= delta_time;
    if (combat->damage_timer < 0.0f) {
      combat->damage_timer = 0.0f;
    }
  }

  if (combat->hit_stun_timer > 0.0f) {
    combat->hit_stun_timer -= delta_time;
    if (combat->hit_stun_timer < 0.0f) {
      combat->hit_stun_timer = 0.0f;
    }
  }

  // Update combo timer
  if (combat->combo_timer > 0) {
    combat->combo_timer -= delta_time;
    if (combat->combo_timer <= 0) {
      combat->combo_count = 0;
      combat->combo_score = 0;
    }
  }

  // ==== Update Dodge State ====
  if (combat->is_dodging) {
    combat->dodge_timer -= delta_time;
    if (combat->dodge_timer <= 0.0f) {
      combat->is_dodging = false;
      combat->dodge_invulnerable = false;
      combat->dodge_timer = 0.0f;
    }
  }

  // ==== Update Perfect Block Window ====
  if (combat->is_blocking && combat->perfect_block_window > 0) {
    combat->perfect_block_window -= delta_time;
    if (combat->perfect_block_window < 0) {
      combat->perfect_block_window = 0.0f;
    }
  }
}

bool player_can_attack(PlayerCombatState *combat,
                       const PlayerComponent *player) {
  if (!combat || !player)
    return false;
  return combat->attack_cooldown <= 0 && !combat->is_blocking &&
         player->stamina > 0.0f;
}

bool player_can_block(PlayerCombatState *combat,
                      const PlayerComponent *player) {
  if (!combat || !player)
    return false;
  return combat->block_cooldown <= 0 && !combat->is_attacking &&
         player->stamina > 0.0f;
}

bool player_is_invulnerable(PlayerCombatState *combat) {
  if (!combat)
    return false;
  return combat->damage_timer > 0 || combat->dodge_invulnerable;
}

void player_combat_set_weapon_override(PlayerCombatState *combat,
                                       WeaponType weapon) {
  if (!combat)
    return;
  combat->weapon_override = weapon;
  combat->has_weapon_override = true;
}

void player_combat_clear_weapon_override(PlayerCombatState *combat) {
  if (!combat)
    return;
  combat->has_weapon_override = false;
}

const PlayerCombatStats *player_combat_get_stats(void) {
  return &g_combat_stats;
}

void player_combat_reset_stats(void) {
  memset(&g_combat_stats, 0, sizeof(g_combat_stats));
}

void player_combat_set_callbacks(const PlayerCombatCallbacks *callbacks) {
  if (!callbacks) {
    memset(&g_combat_callbacks, 0, sizeof(g_combat_callbacks));
    return;
  }
  g_combat_callbacks = *callbacks;
}
