#ifndef PLAYER_DAMAGE_H
#define PLAYER_DAMAGE_H
#endif

#include <audio/audio_system.h>
#include <combat/equipment.h>
#include <core/logger.h>
#include <ecs/component_ids.h> // Added for HEALTH_COMPONENT_ID
#include <ecs/components/health.h>
#include <math.h>
#include <math/vec3.h>
#include <player/player.h>
#include <player/status_effects.h>
#include <renderer/camera.h>
#include <stdlib.h>
#include <time.h>
#include <vfx/visual_effects.h>

// Damage immunity system
typedef struct {
  f32 immunity_timer;
  f32 immunity_duration;
  bool is_immune;
} DamageImmunity;

static DamageImmunity g_damage_immunity = {0};

// Damage statistics tracking
typedef struct {
  f32 total_damage_taken;
  u32 hit_count;
  f32 last_damage_time;
  f32 damage_per_second;
} DamageStats;

static DamageStats g_damage_stats = {0};

// Damage particle system
typedef struct {
  Vec3 position;
  Vec3 velocity;
  Vec3 color;
  f32 lifetime;
  f32 max_lifetime;
  f32 size;
  bool active;
} DamageParticle;

#define MAX_DAMAGE_PARTICLES 64
static DamageParticle g_damage_particles[MAX_DAMAGE_PARTICLES] = {0};

typedef struct {
  f32 defense;
  f32 toughness;
  bool has_armor;
} ArmorTotals;

// Forward declarations
static void log_damage_event(f32 damage, DamageType damage_type, Vec3 position,
                             f32 health_before, f32 health_after,
                             bool was_immune);

static bool damage_type_uses_armor(DamageType damage_type) {
  switch (damage_type) {
  case DAMAGE_TYPE_PHYSICAL:
  case DAMAGE_TYPE_MELEE:
  case DAMAGE_TYPE_RANGED:
  case DAMAGE_TYPE_EXPLOSION:
  case DAMAGE_TYPE_FALL:
    return true;
  case DAMAGE_TYPE_FIRE:
  case DAMAGE_TYPE_MAGIC:
  case DAMAGE_TYPE_LAVA:
  case DAMAGE_TYPE_DROWNING:
  case DAMAGE_TYPE_SUFFOCATION:
  case DAMAGE_TYPE_COUNT:
    return false;
  default:
    return false;
  }
}

static ArmorTotals get_player_armor_totals(Player *player,
                                           struct World *ecs_world,
                                           EquipmentComponent **out_equipment) {
  ArmorTotals totals = {0};
  if (out_equipment) {
    *out_equipment = NULL;
  }

  if (!player)
    return totals;

  if (ecs_world) {
    EquipmentComponent *equipment =
        equipment_get_component((World *)ecs_world, player->entity_id);
    if (equipment) {
      totals.defense = equipment_get_total_defense(equipment);
      totals.toughness = equipment_get_total_toughness(equipment);
      totals.has_armor = totals.defense > 0.0f;
      if (out_equipment) {
        *out_equipment = equipment;
      }
      return totals;
    }
  }

  if (player->equipped_armor) {
    Armor *armor = (Armor *)player->equipped_armor;
    for (u32 i = 0; i < ARMOR_SLOT_COUNT; i++) {
      if (armor[i].max_durability > 0 && armor[i].durability > 0) {
        totals.defense += armor[i].defense;
        totals.toughness += armor[i].toughness;
      }
    }
    totals.has_armor = totals.defense > 0.0f;
  }

  return totals;
}

static f32 calculate_armor_reduction(const ArmorTotals *totals, f32 damage) {
  if (!totals || !totals->has_armor || damage <= 0.0f)
    return 0.0f;

  f32 defense_factor = totals->defense / (totals->defense + damage);
  f32 toughness_factor = 1.0f;
  if (totals->toughness > 0.0f && damage > totals->defense) {
    f32 excess = damage - totals->defense;
    toughness_factor = 1.0f / (1.0f + (excess / (totals->toughness * 2.0f)));
  }
  f32 reduction = defense_factor * toughness_factor;
  return CLAMP(reduction, 0.0f, 0.8f);
}

static void apply_armor_durability_damage(Player *player,
                                          EquipmentComponent *equipment,
                                          f32 damage) {
  if (damage <= 0.0f)
    return;

  u32 armor_damage = (u32)(damage * 0.1f);
  if (armor_damage == 0)
    return;
  u32 step = armor_damage / ARMOR_SLOT_COUNT;
  if (step == 0) {
    step = 1;
  }

  if (equipment) {
    for (u32 i = 0; i < ARMOR_SLOT_COUNT; i++) {
      if (equipment_has_armor(equipment, (ArmorSlot)i)) {
        equipment_damage_armor(equipment, (ArmorSlot)i, step);
      }
    }
    return;
  }

  if (player && player->equipped_armor) {
    Armor *armor = (Armor *)player->equipped_armor;
    for (u32 i = 0; i < ARMOR_SLOT_COUNT; i++) {
      if (armor[i].durability > 0) {
        armor[i].durability =
            (armor[i].durability > step) ? (armor[i].durability - step) : 0;
      }
    }
  }
}

// Play damage sound effects with variation
static void play_damage_sound(AudioSystem *audio_system, f32 damage_amount,
                              DamageType damage_type) {
  if (!audio_system || damage_amount <= 0.0f)
    return;

  // Check immunity cooldown to prevent sound spam
  if (g_damage_immunity.is_immune)
    return;

  const char *sound_name = "player_hurt";
  f32 pitch = 1.0f;
  f32 volume = 0.8f;

  // Select sound based on damage type and amount
  switch (damage_type) {
  case DAMAGE_TYPE_FALL:
    sound_name = "player_fall";
    pitch = 0.8f + (damage_amount * 0.05f); // Lower pitch for harder falls
    volume = MIN(1.0f, 0.6f + damage_amount * 0.1f);
    break;
  case DAMAGE_TYPE_LAVA:
    sound_name = "player_burn";
    pitch = 0.9f;
    volume = 0.9f;
    break;
  case DAMAGE_TYPE_FIRE:
    sound_name = "player_burn_light";
    pitch = 1.1f;
    volume = 0.7f;
    break;
  case DAMAGE_TYPE_DROWNING:
    sound_name = "player_gasp";
    pitch = 0.7f;
    volume = 0.8f;
    break;
  case DAMAGE_TYPE_SUFFOCATION:
    sound_name = "player_choking";
    pitch = 0.6f;
    volume = 0.9f;
    break;
  default:
    // Vary pitch based on damage amount for generic hurt
    pitch = 0.9f + (damage_amount * 0.02f);
    volume = MIN(1.0f, 0.7f + damage_amount * 0.05f);
    break;
  }

  // Add slight randomization to pitch
  pitch *= (0.95f + (rand() % 100) * 0.001f);

  // Play the sound
  SoundType sound_enum = SOUND_PLAYER_HURT;

  // Simple mapping since we don't have all specific sounds in enum yet
  switch (damage_type) {
  case DAMAGE_TYPE_LAVA:
  case DAMAGE_TYPE_FIRE:
    sound_enum = SOUND_FIRE_BURN;
    break;
  case DAMAGE_TYPE_DROWNING:
    sound_enum = SOUND_WATER_SPLASH;
    break;
  case DAMAGE_TYPE_PHYSICAL:
  case DAMAGE_TYPE_MELEE:
  case DAMAGE_TYPE_RANGED:
  case DAMAGE_TYPE_MAGIC:
  case DAMAGE_TYPE_EXPLOSION:
  case DAMAGE_TYPE_FALL:
  case DAMAGE_TYPE_SUFFOCATION:
  case DAMAGE_TYPE_POISON:
  case DAMAGE_TYPE_WITHER:
  case DAMAGE_TYPE_STARVATION:
  case DAMAGE_TYPE_COUNT:
    sound_enum = SOUND_PLAYER_HURT;
    break;
  }

  audio_play_sound_2d(audio_system, sound_enum, volume, SOUND_CATEGORY_PLAYER);
}

// Create damage particles at position
static void create_damage_particles(Vec3 position, f32 damage_amount,
                                    DamageType damage_type) {
  // Number of particles based on damage amount
  i32 particle_count = (i32)(damage_amount * 2.0f) + 1;
  particle_count = MIN(particle_count, 8); // Cap at 8 particles

  // Particle color based on damage type
  Vec3 base_color = vec3(1.0f, 0.2f, 0.2f); // Default red

  switch (damage_type) {
  case DAMAGE_TYPE_FALL:
    base_color = vec3(0.8f, 0.4f, 0.2f); // Brownish
    break;
  case DAMAGE_TYPE_LAVA:
  case DAMAGE_TYPE_FIRE:
    base_color = vec3(1.0f, 0.6f, 0.0f); // Orange
    break;
  case DAMAGE_TYPE_DROWNING:
    base_color = vec3(0.2f, 0.4f, 0.8f); // Blue
    break;
  case DAMAGE_TYPE_SUFFOCATION:
    base_color = vec3(0.6f, 0.4f, 0.2f); // Dark brown
    break;
  case DAMAGE_TYPE_PHYSICAL:
  case DAMAGE_TYPE_MELEE:
  case DAMAGE_TYPE_RANGED:
  case DAMAGE_TYPE_MAGIC:
  case DAMAGE_TYPE_EXPLOSION:
  case DAMAGE_TYPE_POISON:
  case DAMAGE_TYPE_WITHER:
  case DAMAGE_TYPE_STARVATION:
  case DAMAGE_TYPE_COUNT:
    base_color = vec3(1.0f, 0.2f, 0.2f); // Default red
    break;
  }

  // Create particles
  for (i32 i = 0; i < particle_count; i++) {
    // Find inactive particle slot
    for (i32 j = 0; j < MAX_DAMAGE_PARTICLES; j++) {
      if (!g_damage_particles[j].active) {
        DamageParticle *particle = &g_damage_particles[j];

        // Set particle properties
        particle->position = position;
        particle->position.y += 1.0f; // Spawn at chest level

        // Random velocity outward and upward
        f32 angle = (f32)rand() / RAND_MAX * 2.0f * PI;
        f32 speed = 0.5f + (f32)rand() / RAND_MAX * 1.5f;
        particle->velocity =
            vec3(cosf(angle) * speed, 1.0f + (f32)rand() / RAND_MAX * 2.0f,
                 sinf(angle) * speed);

        // Color with slight variation
        particle->color =
            vec3(base_color.x * (0.8f + (f32)rand() / RAND_MAX * 0.4f),
                 base_color.y * (0.8f + (f32)rand() / RAND_MAX * 0.4f),
                 base_color.z * (0.8f + (f32)rand() / RAND_MAX * 0.4f));

        particle->lifetime = 0.0f;
        particle->max_lifetime = 0.5f + (f32)rand() / RAND_MAX * 0.5f;
        particle->size = 0.1f + damage_amount * 0.02f;
        particle->active = true;

        break;
      }
    }
  }
}

// Update damage particles
static void update_damage_particles(f32 delta_time) {
  for (i32 i = 0; i < MAX_DAMAGE_PARTICLES; i++) {
    DamageParticle *particle = &g_damage_particles[i];
    if (!particle->active)
      continue;

    // Update lifetime
    particle->lifetime += delta_time;
    if (particle->lifetime >= particle->max_lifetime) {
      particle->active = false;
      continue;
    }

    // Update position
    particle->position =
        vec3_add(particle->position, vec3_mul(particle->velocity, delta_time));

    // Apply gravity
    particle->velocity.y -= 9.8f * delta_time;

    // Fade out (would need particle system integration)
    f32 alpha = 1.0f - (particle->lifetime / particle->max_lifetime);
    // particle_set_alpha(particle, alpha);
  }
}

// Check if player is immune to damage
static bool is_player_immune(void) {
  return g_damage_immunity.is_immune && g_damage_immunity.immunity_timer > 0.0f;
}

// Grant damage immunity
static void grant_damage_immunity(f32 duration) {
  g_damage_immunity.is_immune = true;
  g_damage_immunity.immunity_duration = duration;
  g_damage_immunity.immunity_timer = duration;
}

// Update damage immunity
static void update_damage_immunity(f32 delta_time) {
  if (g_damage_immunity.is_immune && g_damage_immunity.immunity_timer > 0.0f) {
    g_damage_immunity.immunity_timer -= delta_time;
    if (g_damage_immunity.immunity_timer <= 0.0f) {
      g_damage_immunity.is_immune = false;
      g_damage_immunity.immunity_timer = 0.0f;
    }
  }
}

// Update damage statistics
static void update_damage_stats(f32 damage_amount) {
  g_damage_stats.total_damage_taken += damage_amount;
  g_damage_stats.hit_count++;
  g_damage_stats.last_damage_time = (f32)clock() / CLOCKS_PER_SEC;

  // Calculate damage per second (rolling average over last 5 seconds)
  static f32 damage_history[50] = {0}; // 5 seconds at 10 updates per second
  static i32 history_index = 0;

  damage_history[history_index] = damage_amount;
  history_index = (history_index + 1) % 50;

  f32 recent_damage = 0.0f;
  for (i32 i = 0; i < 50; i++) {
    recent_damage += damage_history[i];
  }
  g_damage_stats.damage_per_second = recent_damage / 5.0f; // 5 second window
}

// Apply damage with all effects
static void apply_damage_with_effects(Player *player, struct World *ecs_world,
                                      AudioSystem *audio_system, Camera *camera,
                                      f32 damage, DamageType damage_type) {
  if (!player || !ecs_world || damage <= 0.0f)
    return;

  if (!status_effects_can_take_damage(&player->status_effects)) {
    return;
  }

  // Get health component
  HealthComponent *health = (HealthComponent *)ecs_get_component(
      (World *)ecs_world, (Entity){player->entity_id, 0}, HEALTH_COMPONENT_ID);
  if (!health)
    return;

  f32 health_before = health->health;
  bool was_immune = is_player_immune();

  f32 adjusted_damage = damage;

  if (status_effect_has(&player->status_effects,
                        STATUS_EFFECT_FIRE_RESISTANCE) &&
      (damage_type == DAMAGE_TYPE_FIRE || damage_type == DAMAGE_TYPE_LAVA)) {
    adjusted_damage *= 0.2f;
  }
  if (status_effect_has(&player->status_effects,
                        STATUS_EFFECT_WATER_BREATHING) &&
      damage_type == DAMAGE_TYPE_DROWNING) {
    adjusted_damage = 0.0f;
  }

  EquipmentComponent *equipment = NULL;
  if (damage_type_uses_armor(damage_type)) {
    ArmorTotals totals = get_player_armor_totals(player, ecs_world, &equipment);
    f32 reduction = calculate_armor_reduction(&totals, adjusted_damage);
    if (reduction > 0.0f) {
      adjusted_damage *= (1.0f - reduction);
      apply_armor_durability_damage(player, equipment, adjusted_damage);
    }
  }

  if (adjusted_damage <= 0.0f) {
    return;
  }

  // Check immunity
  if (was_immune) {
    log_damage_event(0.0f, damage_type, player->prev_physics_position,
                     health_before, health_before, true);
    return;
  }

  // Apply damage
  health->health -= adjusted_damage;
  if (health->health < 0.0f) {
    health->health = 0.0f;
  }

  f32 health_after = health->health;

  // Log the damage event
  log_damage_event(adjusted_damage, damage_type, player->prev_physics_position,
                   health_before, health_after, false);

  // Update statistics
  update_damage_stats(adjusted_damage);

  // Play sound effects
  play_damage_sound(audio_system, adjusted_damage, damage_type);

  // Create particle effects
  Vec3 player_pos = player->prev_physics_position;
  create_damage_particles(player_pos, adjusted_damage, damage_type);

  // Add camera shake based on damage amount
  if (camera) {
    f32 shake_intensity = fminf(adjusted_damage * 0.1f, 0.5f);
    f32 shake_duration = fminf(adjusted_damage * 0.2f, 1.0f);
    camera_add_shake(camera, shake_intensity, shake_duration);
  }

  // Grant brief immunity after taking damage
  status_effects_set_immunity(&player->status_effects, 0.5f);
  grant_damage_immunity(0.5f);
}

// Public API functions
void player_update_damage_systems(f32 delta_time) {
  update_damage_immunity(delta_time);
  update_damage_particles(delta_time);
}

DamageStats player_get_damage_stats(void) { return g_damage_stats; }

void player_reset_damage_stats(void) {
  memset(&g_damage_stats, 0, sizeof(DamageStats));
}

bool player_is_damage_immune(void) { return is_player_immune(); }

void player_damage_detailed(PlayerSystem *system, f32 damage, DamageType type) {
  if (!system || !system->player || damage <= 0.0f)
    return;

  apply_damage_with_effects(system->player, system->ecs_world,
                            system->audio_system, system->camera, damage, type);
}

// Apply fall damage to player
void player_apply_fall_damage(Player *player, struct World *ecs_world,
                              AudioSystem *audio_system, Camera *camera) {
  if (!player || !ecs_world)
    return;

  // Calculate fall distance
  f32 fall_distance = player->fall_start_y - player->prev_physics_position.y;

  // No damage if fell less than 3 blocks
  if (fall_distance < 3.0f) {
    return;
  }

  // Calculate damage: 1 damage per block after 3 blocks
  f32 damage = fall_distance - 3.0f;

  // Apply damage with effects
  apply_damage_with_effects(player, ecs_world, audio_system, camera, damage,
                            DAMAGE_TYPE_FALL);

  LOG_INFO("Player took %.1f fall damage (fell %.1f blocks)", damage,
           fall_distance);
}

// Apply lava damage
void player_apply_lava_damage(Player *player, struct World *ecs_world,
                              AudioSystem *audio_system, Camera *camera,
                              f32 delta_time) {
  if (!player || !ecs_world || !player->in_lava)
    return;

  // Lava deals 4 damage per second
  f32 damage = 4.0f * delta_time;

  // Apply damage with effects
  apply_damage_with_effects(player, ecs_world, audio_system, camera, damage,
                            DAMAGE_TYPE_LAVA);

  // Set player on fire
  if (damage > 0.0f) {
    status_effect_add(&player->status_effects, STATUS_EFFECT_ON_FIRE, 8.0f,
                      1.0f);
  }
}

// Apply fire damage
void player_apply_fire_damage(Player *player, struct World *ecs_world,
                              AudioSystem *audio_system, Camera *camera,
                              f32 delta_time) {
  if (!player || !ecs_world)
    return;

  // Fire deals 1 damage per second
  f32 damage = 1.0f * delta_time;

  // Apply damage with effects
  apply_damage_with_effects(player, ecs_world, audio_system, camera, damage,
                            DAMAGE_TYPE_FIRE);
}

// Apply drowning damage
void player_apply_drowning_damage(Player *player, struct World *ecs_world,
                                  AudioSystem *audio_system, Camera *camera,
                                  f32 delta_time) {
  if (!player || !ecs_world)
    return;

  // Only deal damage if oxygen is depleted
  if (player->oxygen > 0.0f)
    return;

  // Drowning deals 2 damage per second
  f32 damage = 2.0f * delta_time;

  // Apply damage with effects
  apply_damage_with_effects(player, ecs_world, audio_system, camera, damage,
                            DAMAGE_TYPE_DROWNING);
}

// Apply suffocation damage (when inside solid block)
void player_apply_suffocation_damage(Player *player, struct World *ecs_world,
                                     ChunkManager *chunk_manager,
                                     BlockRegistry *block_registry,
                                     AudioSystem *audio_system, Camera *camera,
                                     f32 delta_time) {
  if (!player || !ecs_world || !chunk_manager || !block_registry)
    return;

  // Check if player's head is inside a solid block
  Vec3 head_pos = player->prev_physics_position;
  head_pos.y += 1.6f; // Player height

  i32 x = (i32)floorf(head_pos.x);
  i32 y = (i32)floorf(head_pos.y);
  i32 z = (i32)floorf(head_pos.z);

  ChunkPos cp = world_to_chunk_pos(x, y, z);
  Chunk *chunk = chunk_manager_get(chunk_manager, cp);
  if (!chunk)
    return;

  i32 local_x = x - cp.x * CHUNK_SIZE;
  i32 local_y = y - cp.y * CHUNK_SIZE;
  i32 local_z = z - cp.z * CHUNK_SIZE;

  BlockID block = chunk_get_block(chunk, local_x, local_y, local_z);
  if (block == BLOCK_AIR)
    return;

  const BlockType *block_type = block_registry_get(block_registry, block);
  if (!block_type || !block_is_solid(block_type))
    return;

  // Player is suffocating - deal 1 damage per second
  f32 damage = 1.0f * delta_time;

  // Apply damage with effects
  apply_damage_with_effects(player, ecs_world, audio_system, camera, damage,
                            DAMAGE_TYPE_SUFFOCATION);

  LOG_DEBUG("Player suffocating in block %d", block);
}

// Update fall damage tracking
void player_update_fall_tracking(Player *player) {
  if (!player)
    return;

  // If just landed
  if (player->on_ground && !player->was_on_ground) {
    // Apply fall damage if fell significant distance
    // Note: This function should be called with ecs_world parameter
    // For now, just track that we landed
    player->fall_start_y = player->prev_physics_position.y;
  }

  // If just left ground, record height
  if (!player->on_ground && player->was_on_ground) {
    player->fall_start_y = player->prev_physics_position.y;
  }

  player->was_on_ground = player->on_ground;
}

// ============================================================================
// Armor Mitigation Helpers
// ============================================================================

f32 player_apply_armor_damage(Player *player, f32 base_damage,
                              DamageType damage_type) {
  if (!player || base_damage <= 0.0f)
    return 0.0f;

  if (!damage_type_uses_armor(damage_type)) {
    return base_damage;
  }

  ArmorTotals totals = get_player_armor_totals(player, NULL, NULL);
  f32 reduction = calculate_armor_reduction(&totals, base_damage);
  f32 reduced_damage = base_damage * (1.0f - reduction);

  if (reduced_damage < base_damage * 0.01f) {
    reduced_damage = base_damage * 0.01f;
  }

  apply_armor_durability_damage(player, NULL, reduced_damage);
  return reduced_damage;
}

f32 player_get_armor_protection(Player *player, DamageType damage_type) {
  if (!player || !damage_type_uses_armor(damage_type))
    return 0.0f;

  ArmorTotals totals = get_player_armor_totals(player, NULL, NULL);
  f32 reduction = calculate_armor_reduction(&totals, 10.0f);
  return reduction * 100.0f;
}

// ============================================================================
// Damage Logging System for Debugging
// ============================================================================

typedef struct {
  f32 timestamp;
  f32 damage_amount;
  DamageType damage_type;
  Vec3 position;
  f32 player_health_before;
  f32 player_health_after;
  bool was_immune;
} DamageLogEntry;

#define MAX_DAMAGE_LOG_ENTRIES 256
static DamageLogEntry g_damage_log[MAX_DAMAGE_LOG_ENTRIES] = {0};
static u32 g_damage_log_index = 0;
static u32 g_damage_log_count = 0;
static bool g_damage_logging_enabled = true;

// Log damage event
static void log_damage_event(f32 damage, DamageType damage_type, Vec3 position,
                             f32 health_before, f32 health_after,
                             bool was_immune) {
  if (!g_damage_logging_enabled)
    return;

  DamageLogEntry *entry = &g_damage_log[g_damage_log_index];

  entry->timestamp = (f32)clock() / CLOCKS_PER_SEC;
  entry->damage_amount = damage;
  entry->damage_type = damage_type;
  entry->position = position;
  entry->player_health_before = health_before;
  entry->player_health_after = health_after;
  entry->was_immune = was_immune;

  // Update log index (circular buffer)
  g_damage_log_index = (g_damage_log_index + 1) % MAX_DAMAGE_LOG_ENTRIES;
  if (g_damage_log_count < MAX_DAMAGE_LOG_ENTRIES) {
    g_damage_log_count++;
  }

  // Also log to console for immediate debugging
  const char *type_names[] = {"PHYSICAL", "FIRE",     "MAGIC",      "EXPLOSION",
                              "FALL",     "DROWNING", "SUFFOCATION"};

  if (damage_type < DAMAGE_TYPE_COUNT) {
    LOG_DEBUG("DAMAGE: %.2f %s at (%.1f,%.1f,%.1f) Health: %.1f->%.1f%s",
              damage, type_names[damage_type], position.x, position.y,
              position.z, health_before, health_after,
              was_immune ? " [IMMUNE]" : "");
  }
}

// Get damage log entries
u32 player_get_damage_log(DamageLogEntry *entries, u32 max_entries) {
  if (!entries || max_entries == 0)
    return 0;

  u32 count = MIN(g_damage_log_count, max_entries);
  u32 start_index =
      (g_damage_log_index + MAX_DAMAGE_LOG_ENTRIES - g_damage_log_count) %
      MAX_DAMAGE_LOG_ENTRIES;

  for (u32 i = 0; i < count; i++) {
    u32 index = (start_index + i) % MAX_DAMAGE_LOG_ENTRIES;
    entries[i] = g_damage_log[index];
  }

  return count;
}

// Clear damage log
void player_clear_damage_log(void) {
  memset(g_damage_log, 0, sizeof(g_damage_log));
  g_damage_log_index = 0;
  g_damage_log_count = 0;
}

// Enable/disable damage logging
void player_set_damage_logging(bool enabled) {
  g_damage_logging_enabled = enabled;
  LOG_INFO("Damage logging %s", enabled ? "enabled" : "disabled");
}

// Get damage log statistics
void player_get_damage_log_stats(u32 *total_entries, f32 *total_damage,
                                 u32 *damage_by_type) {
  if (total_entries)
    *total_entries = g_damage_log_count;

  f32 total = 0.0f;
  if (damage_by_type) {
    memset(damage_by_type, 0, sizeof(u32) * DAMAGE_TYPE_COUNT);
  }

  for (u32 i = 0; i < g_damage_log_count; i++) {
    u32 index =
        (g_damage_log_index + MAX_DAMAGE_LOG_ENTRIES - g_damage_log_count + i) %
        MAX_DAMAGE_LOG_ENTRIES;
    DamageLogEntry *entry = &g_damage_log[index];

    total += entry->damage_amount;
    if (damage_by_type && entry->damage_type < DAMAGE_TYPE_COUNT) {
      damage_by_type[entry->damage_type]++;
    }
  }

  if (total_damage)
    *total_damage = total;
}

// Print damage log summary
void player_print_damage_log_summary(void) {
  if (g_damage_log_count == 0) {
    LOG_INFO("No damage events logged");
    return;
  }

  u32 damage_by_type[DAMAGE_TYPE_COUNT] = {0};
  f32 total_damage = 0.0f;
  player_get_damage_log_stats(NULL, &total_damage, damage_by_type);

  const char *type_names[] = {"Physical", "Fire",     "Magic",      "Explosion",
                              "Fall",     "Drowning", "Suffocation"};

  LOG_INFO("=== Damage Log Summary ===");
  LOG_INFO("Total events: %u", g_damage_log_count);
  LOG_INFO("Total damage: %.1f", total_damage);

  for (u32 i = 0; i < DAMAGE_TYPE_COUNT; i++) {
    if (damage_by_type[i] > 0) {
      LOG_INFO("  %s: %u hits", type_names[i], damage_by_type[i]);
    }
  }
}
