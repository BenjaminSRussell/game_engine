// Player Magic System Implementation with Enhanced Cooldown System
// Roadmap: docs/PLAYER_MAGIC_ROADMAP.md.

#include <audio/audio_system.h>
#include <block/interaction.h>
#include <combat/combat.h>
#include <ecs/component_ids.h>
#include <ecs/components/transform.h>
#include <math.h>
#include <player/player.h>
#include <player/player_magic.h>
#include <player/spell_combination.h>
#include <string.h>
#include <core/logger.h>
#include <effects/vfx/particle_system.h>

// Forward declaration for global particle system
extern ParticleSystem *g_particle_system;

// Enhanced spell cooldown system
#define MAX_COOLDOWN_SPELLS 16
#define COOLDOWN_VISUAL_DURATION 0.5f
#define COOLDOWN_FLASH_INTERVAL 0.2f
#define MAX_SPELL_LEVEL 5

typedef struct {
    SpellType spell_type;
    f32 remaining_cooldown;
    f32 total_cooldown;
    f32 last_used_time;
    u32 cast_count;
    bool is_ready;
    bool show_visual_indicator;
    f32 visual_alpha;
    f32 flash_timer;
} SpellCooldown;

typedef struct {
    SpellCooldown spells[MAX_COOLDOWN_SPELLS];
    u32 active_cooldowns;
    f32 global_cooldown_modifier;
    bool visual_indicators_enabled;
    bool cooldown_reduction_enabled;
} SpellCooldownSystem;

static SpellCooldownSystem g_cooldown_system = {0};

// Cooldown management functions
static void init_spell_cooldown_system(void) {
    memset(&g_cooldown_system, 0, sizeof(g_cooldown_system));
    g_cooldown_system.global_cooldown_modifier = 1.0f;
    g_cooldown_system.visual_indicators_enabled = true;
    g_cooldown_system.cooldown_reduction_enabled = false;
    LOG_INFO("Spell cooldown system initialized");
}

static SpellCooldown* get_spell_cooldown(SpellType spell) {
    for (u32 i = 0; i < g_cooldown_system.active_cooldowns; i++) {
        if (g_cooldown_system.spells[i].spell_type == spell) {
            return &g_cooldown_system.spells[i];
        }
    }
    
    // Create new cooldown entry
    if (g_cooldown_system.active_cooldowns < MAX_COOLDOWN_SPELLS) {
        SpellCooldown *cooldown = &g_cooldown_system.spells[g_cooldown_system.active_cooldowns];
        cooldown->spell_type = spell;
        cooldown->remaining_cooldown = 0.0f;
        cooldown->total_cooldown = 0.0f;
        cooldown->last_used_time = 0.0f;
        cooldown->cast_count = 0;
        cooldown->is_ready = true;
        cooldown->show_visual_indicator = false;
        cooldown->visual_alpha = 0.0f;
        cooldown->flash_timer = 0.0f;
        g_cooldown_system.active_cooldowns++;
        return cooldown;
    }
    
    return NULL;
}

static void start_spell_cooldown(SpellType spell, f32 base_cooldown) {
    SpellCooldown *cooldown = get_spell_cooldown(spell);
    if (!cooldown) return;
    
    // Apply cooldown reduction if enabled
    f32 actual_cooldown = base_cooldown;
    if (g_cooldown_system.cooldown_reduction_enabled) {
        actual_cooldown *= g_cooldown_system.global_cooldown_modifier;
    }
    
    cooldown->remaining_cooldown = actual_cooldown;
    cooldown->total_cooldown = actual_cooldown;
    cooldown->last_used_time = 0.0f; // Would use current time
    cooldown->cast_count++;
    cooldown->is_ready = false;
    cooldown->show_visual_indicator = true;
    cooldown->visual_alpha = 1.0f;
    cooldown->flash_timer = COOLDOWN_FLASH_INTERVAL;
    
    LOG_DEBUG("Started cooldown for spell %d: %.2fs", spell, actual_cooldown);
}

static void update_spell_cooldowns(f32 delta_time) {
    for (u32 i = 0; i < g_cooldown_system.active_cooldowns; i++) {
        SpellCooldown *cooldown = &g_cooldown_system.spells[i];
        
        if (cooldown->remaining_cooldown > 0.0f) {
            cooldown->remaining_cooldown -= delta_time;
            
            if (cooldown->remaining_cooldown <= 0.0f) {
                cooldown->remaining_cooldown = 0.0f;
                cooldown->is_ready = true;
                LOG_DEBUG("Spell %d is now ready", cooldown->spell_type);
            }
        }
        
        // Update visual indicators
        if (cooldown->show_visual_indicator) {
            // Fade out visual indicator
            if (cooldown->is_ready) {
                cooldown->visual_alpha -= delta_time * 2.0f;
                if (cooldown->visual_alpha <= 0.0f) {
                    cooldown->visual_alpha = 0.0f;
                    cooldown->show_visual_indicator = false;
                }
            } else {
                // Flash effect during cooldown
                cooldown->flash_timer -= delta_time;
                if (cooldown->flash_timer <= 0.0f) {
                    cooldown->flash_timer = COOLDOWN_FLASH_INTERVAL;
                    cooldown->visual_alpha = cooldown->visual_alpha > 0.5f ? 0.3f : 1.0f;
                }
            }
        }
    }
}

static bool is_spell_ready(SpellType spell) {
    SpellCooldown *cooldown = get_spell_cooldown(spell);
    return cooldown ? cooldown->is_ready : true;
}

static f32 get_spell_cooldown_progress(SpellType spell) {
    SpellCooldown *cooldown = get_spell_cooldown(spell);
    if (!cooldown || cooldown->total_cooldown <= 0.0f) {
        return 1.0f; // Ready
    }
    
    return 1.0f - (cooldown->remaining_cooldown / cooldown->total_cooldown);
}

// Visual indicator rendering
static void render_spell_cooldown_indicators(PlayerSystem *system) {
    if (!g_cooldown_system.visual_indicators_enabled || !system->player) {
        return;
    }
    
    // In a real implementation, this would render UI elements showing:
    // 1. Cooldown progress bars for each spell
    // 2. Visual flash effects when spells become ready
    // 3. Cooldown timers with remaining time
    // 4. Color-coded indicators (red = cooldown, green = ready)
    
    for (u32 i = 0; i < g_cooldown_system.active_cooldowns; i++) {
        SpellCooldown *cooldown = &g_cooldown_system.spells[i];
        
        if (cooldown->show_visual_indicator && cooldown->visual_alpha > 0.0f) {
            // Render visual indicator (placeholder implementation)
            // Would draw UI elements at appropriate screen positions
            LOG_TRACE("Rendering cooldown indicator for spell %d (alpha: %.2f)", 
                     cooldown->spell_type, cooldown->visual_alpha);
        }
    }
}

typedef struct {
  f32 mana_cost;
  f32 cooldown;
  f32 range;
  f32 magnitude;
  f32 magnitude_per_level;
  f32 duration;
  f32 duration_per_level;
  bool is_ranged;
} SpellConfig;

static const SpellConfig kSpellConfigs[SPELL_COUNT] = {
    [SPELL_FIREBALL] = {15.0f, 1.5f, 30.0f, 8.0f, 2.0f, 0.0f, 0.0f, true},
    [SPELL_HEAL] = {12.0f, 4.0f, 0.0f, 8.0f, 4.0f, 0.0f, 0.0f, false},
    [SPELL_TELEPORT] = {20.0f, 6.0f, 24.0f, 0.0f, 2.0f, 0.0f, 0.0f, false},
    [SPELL_SHIELD] = {18.0f, 10.0f, 0.0f, 0.0f, 0.0f, 3.0f, 1.0f, false},
    [SPELL_LIGHTNING] = {20.0f, 5.0f, 28.0f, 12.0f, 3.0f, 0.0f, 0.0f, true},
    [SPELL_FREEZE] = {16.0f, 5.0f, 18.0f, 0.0f, 0.0f, 2.0f, 0.5f, true},
    [SPELL_INVISIBILITY] = {25.0f, 12.0f, 0.0f, 0.0f, 0.0f, 8.0f, 2.0f, false},
    [SPELL_FLIGHT] = {30.0f, 15.0f, 0.0f, 0.0f, 0.0f, 8.0f, 2.0f, false},
};

static Vec3 player_magic_get_direction(PlayerSystem *system, Vec3 origin,
                                       Vec3 target) {
  Vec3 to_target = vec3_sub(target, origin);
  f32 length = vec3_length(to_target);
  if (length > 0.001f) {
    return vec3_div(to_target, length);
  }

  if (system && system->camera) {
    return system->camera->front;
  }

  return vec3(0.0f, 0.0f, -1.0f);
}

static bool player_magic_can_teleport(PlayerSystem *system, Vec3 origin,
                                      Vec3 target, f32 range,
                                      Vec3 *out_target) {
  Vec3 to_target = vec3_sub(target, origin);
  f32 distance = vec3_length(to_target);
  if (distance < 0.001f) {
    return false;
  }

  Vec3 direction = vec3_div(to_target, distance);
  if (distance > range) {
    target = vec3_add(origin, vec3_mul(direction, range));
  }

  if (system && system->chunk_manager) {
    i32 x = (i32)floorf(target.x);
    i32 y = (i32)floorf(target.y);
    i32 z = (i32)floorf(target.z);
    if (block_get(system->chunk_manager, x, y, z) != BLOCK_AIR) {
      return false;
    }
    if (block_get(system->chunk_manager, x, y + 1, z) != BLOCK_AIR) {
      return false;
    }
  }

  if (out_target) {
    *out_target = target;
  }
  return true;
}

void player_magic_init(PlayerMagicComponent *magic) {
  if (!magic)
    return;
  memset(magic, 0, sizeof(PlayerMagicComponent));

  magic->max_mana = 100.0f;
  magic->current_mana = magic->max_mana;
  magic->mana_regen_rate = 6.0f;
  magic->spell_points = 0;

  for (int i = 0; i < SPELL_COUNT; i++) {
    magic->spells[i].type = (SpellType)i;
    magic->spells[i].level = 1;
    magic->spells[i].cooldown = 0.0f;
    magic->spells[i].cast_time = 0.0f;
    magic->spells[i].mana_cost = kSpellConfigs[i].mana_cost;
    magic->spells[i].is_channeling = false;
    magic->spells[i].channel_time = 0.0f;
  }
  memset(&magic->stats, 0, sizeof(SpellStats));
}

bool player_cast_spell(PlayerSystem *system, SpellType spell, Vec3 target) {
  if (!system || !system->player || spell >= SPELL_COUNT)
    return false;

  // Check spell cooldown first
  if (!is_spell_ready(spell)) {
    player_magic_record_cast_attempt(magic, spell, false);
    LOG_DEBUG("Spell %d is on cooldown", spell);
    return false;
  }

  PlayerMagicComponent *magic = &system->player->magic_component;
  if (!player_can_cast_spell(magic, spell)) {
    player_magic_record_cast_attempt(magic, spell, false);
    return false;
  }

  TransformComponent *transform = ecs_get_component(
      (World *)system->ecs_world, (Entity){system->player->entity_id, 0},
      TRANSFORM_COMPONENT_ID);
  if (!transform)
    return false;

  SpellState *spell_state = &magic->spells[spell];
  const SpellConfig *config = &kSpellConfigs[spell];

  f32 magnitude = config->magnitude + config->magnitude_per_level * (spell_state->level - 1);
  f32 duration = config->duration + config->duration_per_level * (spell_state->level - 1);
  f32 range = config->range;
  if (spell == SPELL_TELEPORT) {
    range += config->magnitude_per_level * (spell_state->level - 1);
  }

  if (spell == SPELL_TELEPORT) {
    Vec3 validated_target = target;
    if (!player_magic_can_teleport(system, transform->position, target,
                                   range, &validated_target)) {
      return false;
    }
    target = validated_target;
  }

  spell_state->type = spell;
  spell_state->target_position = target;
  spell_state->is_channeling = false;
  spell_state->channel_time = 0.0f;

  magic->current_mana = MAX(0.0f, magic->current_mana - spell_state->mana_cost);
  spell_state->cooldown = config->cooldown;
  spell_state->cast_time = 0.0f;

  // Start cooldown using enhanced system
  start_spell_cooldown(spell, config->cooldown);

  // Determine animation duration
  f32 anim_duration = 0.45f;
  switch (spell) {
  case SPELL_HEAL:
    anim_duration = 0.6f;
    break;
  case SPELL_TELEPORT:
    anim_duration = 0.4f;
    break;
  case SPELL_SHIELD:
  case SPELL_INVISIBILITY:
    anim_duration = 0.55f;
    break;
  default:
    anim_duration = 0.45f;
    break;
  }

  // Record successful cast with enhanced statistics
  player_magic_record_cast_attempt(magic, spell, true);
  magic->stats.total_mana_consumed += spell_state->mana_cost;
  magic->stats.session_mana_consumed += spell_state->mana_cost;
  magic->stats.spell_details[spell].mana_consumed += spell_state->mana_cost;
  magic->stats.total_cast_time += anim_duration;
  magic->stats.spell_details[spell].cast_time += anim_duration;
  player_trigger_action_animation(system->player, SPIRIT_ANIM_INTERACT,
                                  anim_duration);
  spell_state->cast_time = anim_duration;

  Vec3 direction =
      player_magic_get_direction(system, transform->position, target);

  if (system->audio_system) {
    SoundType sound = SOUND_SWORD_SWING;
    f32 volume = 0.7f;
    switch (spell) {
    case SPELL_FIREBALL:
      sound = SOUND_FIRE_BURN;
      volume = 0.8f;
      break;
    case SPELL_LIGHTNING:
      sound = SOUND_THUNDER_01;
      volume = 0.9f;
      break;
    case SPELL_HEAL:
      sound = SOUND_PLAYER_HEAL;
      volume = 0.8f;
      break;
    case SPELL_TELEPORT:
      sound = SOUND_WIND_LIGHT;
      volume = 0.6f;
      break;
    case SPELL_SHIELD:
      sound = SOUND_CRAFTING_SUCCESS;
      volume = 0.6f;
      break;
    default:
      break;
    }
    audio_play_sound(system->audio_system, sound, transform->position, volume,
                     SOUND_CATEGORY_PLAYER);
  }

  switch (spell) {
  case SPELL_FIREBALL:
    if (g_particle_system) {
        particle_emit_burst(g_particle_system, PARTICLE_TYPE_FLAME, transform->position, direction, 2.0f, 20, 1.0f);
    }
    break;
  case SPELL_LIGHTNING:
    if (g_particle_system) {
        particle_emit_burst(g_particle_system, PARTICLE_TYPE_LIGHTNING, target, vec3_zero(), 1.0f, 10, 0.5f);
    }
    break;
  case SPELL_HEAL:
    if (g_particle_system) {
        particle_emit_burst(g_particle_system, PARTICLE_TYPE_HEART, transform->position, vec3(0.0f, 1.0f, 0.0f), 1.0f, 10, 1.0f);
    }
    break;
  case SPELL_TELEPORT:
    if (g_particle_system) {
        particle_emit_burst(g_particle_system, PARTICLE_TYPE_PORTAL, transform->position, vec3_zero(), 0.5f, 50, 1.0f);
        particle_emit_burst(g_particle_system, PARTICLE_TYPE_PORTAL, target, vec3_zero(), 0.5f, 50, 1.0f);
    }
    break;
  case SPELL_SHIELD:
    if (g_particle_system) {
        particle_emit_burst(g_particle_system, PARTICLE_TYPE_ENCHANT, transform->position, vec3_zero(), 1.0f, 30, 2.0f);
    }
    break;
  case SPELL_INVISIBILITY:
    if (g_particle_system) {
        particle_emit_burst(g_particle_system, PARTICLE_TYPE_SMOKE, transform->position, vec3_zero(), 1.0f, 20, 1.5f);
    }
    break;
  case SPELL_FLIGHT:
    if (g_particle_system) {
        particle_emit_burst(g_particle_system, PARTICLE_TYPE_DUST, transform->position, vec3(0.0f, -1.0f, 0.0f), 2.0f, 20, 1.0f);
    }
    break;
  default:
    break;
  }

  switch (spell) {
  case SPELL_FIREBALL:
  case SPELL_LIGHTNING:
  case SPELL_FREEZE:
    if (system->combat_system && system->ecs_world) {
      combat_ranged_attack(system->combat_system, system->ecs_world,
                           system->player->entity_id, direction, range);
      player_magic_record_damage_dealt(magic, spell, magnitude, false);
    }
    break;

  case SPELL_HEAL:
    player_heal(system, magnitude);
    player_magic_record_healing_done(magic, spell, magnitude);
    status_effect_remove(&system->player->status_effects, STATUS_EFFECT_POISON);
    status_effect_remove(&system->player->status_effects,
                         STATUS_EFFECT_WEAKNESS);
    status_effect_remove(&system->player->status_effects,
                         STATUS_EFFECT_SLOWNESS);
    break;

  case SPELL_TELEPORT:
    transform->position = target;
    break;

  case SPELL_SHIELD:
    status_effects_set_immunity(&system->player->status_effects,
                                duration);
    break;

  case SPELL_INVISIBILITY:
    status_effect_add(&system->player->status_effects,
                      STATUS_EFFECT_INVISIBILITY, duration, 1.0f);
    break;

  case SPELL_FLIGHT:
    player_fly(system, true);
    spell_state->is_channeling = true;
    spell_state->channel_time = duration;
    magic->active_spell_count += 1;
    break;

  default:
    break;
  }

  return true;
}

bool player_channel_spell(PlayerSystem *system, SpellType spell,
                          f32 delta_time) {
  if (!system || !system->player || spell >= SPELL_COUNT)
    return false;

  PlayerMagicComponent *magic = &system->player->magic_component;
  SpellState *spell_state = &magic->spells[spell];
  if (!spell_state->is_channeling)
    return false;

  const SpellConfig *config = &kSpellConfigs[spell];
  f32 drain_rate = config->mana_cost * 0.25f;
  magic->current_mana -= drain_rate * delta_time;
  if (magic->current_mana <= 0.0f) {
    magic->current_mana = 0.0f;
    spell_state->is_channeling = false;
    spell_state->channel_time = 0.0f;
    if (spell == SPELL_FLIGHT) {
      player_fly(system, false);
    }
    if (magic->active_spell_count > 0) {
      magic->active_spell_count -= 1;
    }
    return false;
  }

  spell_state->channel_time += delta_time;
  return true;
}

void player_cancel_spell(PlayerSystem *system) {
  if (!system || !system->player)
    return;
  PlayerMagicComponent *magic = &system->player->magic_component;

  for (int i = 0; i < SPELL_COUNT; i++) {
    if (magic->spells[i].is_channeling) {
      if (i == SPELL_FLIGHT) {
        player_fly(system, false);
      }
      magic->spells[i].is_channeling = false;
      magic->spells[i].channel_time = 0.0f;
    }
  }
  magic->active_spell_count = 0;
}

void player_magic_update(PlayerSystem *system, f32 delta_time) {
  if (!system || !system->player)
    return;

  PlayerMagicComponent *magic = &system->player->magic_component;

  // Regenerate mana
  if (magic->current_mana < magic->max_mana) {
    magic->current_mana += magic->mana_regen_rate * delta_time;
    if (magic->current_mana > magic->max_mana) {
      magic->current_mana = magic->max_mana;
    }
  }

  // Update spell cooldowns using enhanced system
  update_spell_cooldowns(delta_time);
  
  // Render visual indicators
  render_spell_cooldown_indicators(system);

  // Update channeling/active durations
  for (int i = 0; i < SPELL_COUNT; i++) {
    if (!magic->spells[i].is_channeling) {
      continue;
    }

    magic->spells[i].channel_time -= delta_time;
    if (magic->spells[i].channel_time <= 0.0f) {
      magic->spells[i].is_channeling = false;
      magic->spells[i].channel_time = 0.0f;
      if (i == SPELL_FLIGHT) {
        player_fly(system, false);
      }
      if (magic->active_spell_count > 0) {
        magic->active_spell_count -= 1;
      }
    }
  }
}

bool player_can_cast_spell(PlayerMagicComponent *magic, SpellType spell) {
  if (!magic || spell >= SPELL_COUNT)
    return false;

  // Check enhanced cooldown system
  if (!is_spell_ready(spell))
    return false;

  SpellState *spell_state = &magic->spells[spell];

  if (!player_has_mana(magic, spell_state->mana_cost))
    return false;

  if (magic->casting_disabled)
    return false;

  if (spell_state->is_channeling)
    return false;

  return true;
}

bool player_has_mana(PlayerMagicComponent *magic, f32 amount) {
  if (!magic)
    return false;
  return magic->current_mana >= amount;
}

// Public API for spell cooldown system
void player_magic_init_cooldown_system(void) {
    init_spell_cooldown_system();
}

void player_magic_enable_visual_indicators(bool enable) {
    g_cooldown_system.visual_indicators_enabled = enable;
    LOG_INFO("Spell visual indicators %s", enable ? "enabled" : "disabled");
}

void player_magic_set_cooldown_modifier(f32 modifier) {
    g_cooldown_system.global_cooldown_modifier = modifier;
    LOG_INFO("Spell cooldown modifier set to %.2f", modifier);
}

void player_magic_enable_cooldown_reduction(bool enable) {
    g_cooldown_system.cooldown_reduction_enabled = enable;
    LOG_INFO("Spell cooldown reduction %s", enable ? "enabled" : "disabled");
}

bool player_magic_is_spell_ready(SpellType spell) {
    return is_spell_ready(spell);
}

f32 player_magic_get_cooldown_progress(SpellType spell) {
    return get_spell_cooldown_progress(spell);
}

void player_magic_get_cooldown_stats(SpellType spell, f32 *remaining, f32 *total) {
    SpellCooldown *cooldown = get_spell_cooldown(spell);
    if (cooldown) {
        if (remaining) *remaining = cooldown->remaining_cooldown;
        if (total) *total = cooldown->total_cooldown;
    } else {
        if (remaining) *remaining = 0.0f;
        if (total) *total = 0.0f;
    }
}

void player_magic_add_spell_points(PlayerMagicComponent *magic, u32 points) {
    if (magic) {
        magic->spell_points += points;
        LOG_INFO("Added %u spell points. Total: %u", points, magic->spell_points);
    }
}

bool player_magic_upgrade_spell(PlayerMagicComponent *magic, SpellType spell) {
    if (!magic || spell >= SPELL_COUNT) return false;

    if (magic->spell_points == 0) {
        LOG_WARN("Cannot upgrade spell: No spell points available");
        return false;
    }

    SpellState *state = &magic->spells[spell];
    if (state->level >= MAX_SPELL_LEVEL) {
        LOG_WARN("Cannot upgrade spell %d: Max level reached (%d)", spell, MAX_SPELL_LEVEL);
        return false;
    }

    magic->spell_points--;
    state->level++;
    LOG_INFO("Upgraded spell %d to level %u. Remaining points: %u", spell, state->level, magic->spell_points);

    return true;
}

// ============================================================================
// ENHANCED STATISTICS TRACKING IMPLEMENTATION
// ============================================================================

void player_magic_record_cast_attempt(PlayerMagicComponent *magic, SpellType spell, bool success) {
    if (!magic || spell >= SPELL_COUNT) return;
    
    if (!success) {
        magic->stats.failed_casts++;
        magic->stats.spell_details[spell].interrupts++;
        LOG_DEBUG("Recorded failed cast for spell %d", spell);
        return;
    }
    
    magic->stats.total_spells_cast++;
    magic->stats.spell_counts[spell]++;
    magic->stats.session_spells_cast++;
    
    // Update per-spell statistics
    magic->stats.spell_details[spell].last_used_time = 0.0f; // Would use current time
    magic->stats.spell_details[spell].total_use_time += 0.0f; // Would track actual cast time
    
    LOG_DEBUG("Recorded successful cast for spell %d (total: %u)", 
              spell, magic->stats.spell_counts[spell]);
}

void player_magic_record_damage_dealt(PlayerMagicComponent *magic, SpellType spell, f32 damage, bool critical) {
    if (!magic || spell >= SPELL_COUNT || damage <= 0.0f) return;
    
    magic->stats.total_damage_dealt += damage;
    magic->stats.spell_details[spell].damage_dealt += damage;
    
    if (critical) {
        magic->stats.critical_hits++;
        magic->stats.spell_details[spell].critical_hits++;
        LOG_DEBUG("Recorded critical hit for spell %d: %.1f damage", spell, damage);
    } else {
        LOG_DEBUG("Recorded damage for spell %d: %.1f damage", spell, damage);
    }
}

void player_magic_record_healing_done(PlayerMagicComponent *magic, SpellType spell, f32 healing) {
    if (!magic || spell >= SPELL_COUNT || healing <= 0.0f) return;
    
    magic->stats.total_healing_done += healing;
    magic->stats.spell_details[spell].healing_done += healing;
    
    LOG_DEBUG("Recorded healing for spell %d: %.1f HP", spell, healing);
}

void player_magic_record_interrupt(PlayerMagicComponent *magic, SpellType spell) {
    if (!magic || spell >= SPELL_COUNT) return;
    
    magic->stats.interrupts++;
    magic->stats.spell_details[spell].interrupts++;
    
    LOG_DEBUG("Recorded interrupt for spell %d", spell);
}

void player_magic_record_combo(PlayerMagicComponent *magic, u32 combo_count, f32 combo_time) {
    if (!magic || combo_count == 0) return;
    
    magic->stats.combos_completed++;
    
    if (combo_count > magic->stats.highest_combo) {
        magic->stats.highest_combo = combo_count;
        LOG_DEBUG("New highest combo: %u spells", combo_count);
    }
    
    if (combo_time > magic->stats.longest_combo_time) {
        magic->stats.longest_combo_time = combo_time;
        LOG_DEBUG("New longest combo duration: %.2f seconds", combo_time);
    }
    
    LOG_DEBUG("Recorded combo: %u spells in %.2f seconds", combo_count, combo_time);
}

void player_magic_update_session_stats(PlayerMagicComponent *magic) {
    if (!magic) return;
    
    // Calculate session efficiency (damage per mana point)
    if (magic->stats.session_mana_consumed > 0.0f) {
        magic->stats.session_efficiency = magic->stats.total_damage_dealt / magic->stats.session_mana_consumed;
    }
    
    // Calculate average cast time
    if (magic->stats.total_spells_cast > 0) {
        magic->stats.average_cast_time = magic->stats.total_cast_time / magic->stats.total_spells_cast;
    }
    
    LOG_DEBUG("Session stats updated - Efficiency: %.2f damage/mana, Avg cast time: %.3f seconds", 
              magic->stats.session_efficiency, magic->stats.average_cast_time);
}

void player_magic_reset_session_stats(PlayerMagicComponent *magic) {
    if (!magic) return;
    
    magic->stats.session_start_time = 0.0f; // Would use current time
    magic->stats.session_spells_cast = 0;
    magic->stats.session_mana_consumed = 0.0f;
    magic->stats.session_efficiency = 0.0f;
    
    LOG_INFO("Session statistics reset");
}

// ============================================================================
// STATISTICS QUERIES IMPLEMENTATION
// ============================================================================

const SpellStats* player_magic_get_stats(PlayerMagicComponent *magic) {
    if (!magic) return NULL;
    
    // Update session stats before returning
    player_magic_update_session_stats(magic);
    
    return &magic->stats;
}

f32 player_magic_get_spell_efficiency(PlayerMagicComponent *magic, SpellType spell) {
    if (!magic || spell >= SPELL_COUNT) return 0.0f;
    
    const struct {
        f32 damage_dealt;
        f32 mana_consumed;
    } *details = &magic->stats.spell_details[spell];
    
    if (details->mana_consumed <= 0.0f) {
        return 0.0f;
    }
    
    return details->damage_dealt / details->mana_consumed;
}

f32 player_magic_get_cast_frequency(PlayerMagicComponent *magic, SpellType spell) {
    if (!magic || spell >= SPELL_COUNT) return 0.0f;
    
    if (magic->stats.total_spells_cast == 0) {
        return 0.0f;
    }
    
    return (f32)magic->stats.spell_counts[spell] / (f32)magic->stats.total_spells_cast;
}

u32 player_magic_get_most_used_spell(PlayerMagicComponent *magic) {
    if (!magic) return SPELL_COUNT;
    
    u32 max_count = 0;
    u32 most_used = SPELL_COUNT;
    
    for (u32 i = 0; i < SPELL_COUNT; i++) {
        if (magic->stats.spell_counts[i] > max_count) {
            max_count = magic->stats.spell_counts[i];
            most_used = i;
        }
    }
    
    return most_used;
}

f32 player_magic_get_average_damage_per_cast(PlayerMagicComponent *magic) {
    if (!magic || magic->stats.total_spells_cast == 0) return 0.0f;
    
    return magic->stats.total_damage_dealt / (f32)magic->stats.total_spells_cast;
}
