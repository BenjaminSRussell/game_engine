// src/engine/ai/npc_combat.c
//
// Purpose: Implements NPC combat system with tactical AI, weapon usage,
// and coordinated combat behaviors. This system handles combat between NPCs
// and other entities, providing intelligent combat decision making.
//
#include <ai/npc_combat.h>
#include <ai/npc.h>
#include <ai/npc_types.h>
#include <ecs/components/npc.h>
#include <ecs/components/transform.h>
#include <ecs/components/health.h>
#include <ecs/components/rigidbody.h>
#include "engine/include/core/logger.h"
#include <core/memory.h>
#include <math/vec3.h>
#include "engine/include/math/math.h"
#include <stdlib.h>
#include <string.h>

// External references
extern f32 g_game_time;

// Initialize NPC combat system
void npc_combat_init(NPCCombatSystem *system, struct World *ecs, 
                     struct PhysicsWorld *physics, NPCSystem *npc_system) {
  if (!system || !ecs || !physics || !npc_system)
    return;

  memset(system, 0, sizeof(NPCCombatSystem));
  system->ecs = ecs;
  system->physics = physics;
  system->npc_system = npc_system;
  
  // Initialize combat states array
  system->combat_states = malloc(sizeof(NPCCombatState) * 256);
  system->max_states = 256;
  system->active_count = 0;
  
  // Set default combat settings
  system->combat_update_interval = 0.1f; // 100ms updates
  system->threat_decay_rate = 1.0f; // 1 threat per second
  system->max_combat_range = 32.0f;
  
  system->is_initialized = true;
  
  LOG_INFO("NPC combat system initialized");
}

// Free combat system
void npc_combat_free(NPCCombatSystem *system) {
  if (!system)
    return;

  free(system->combat_states);
  memset(system, 0, sizeof(NPCCombatSystem));
  LOG_INFO("NPC combat system freed");
}

// Update all active combat
void npc_combat_update(NPCCombatSystem *system, f32 delta_time) {
  if (!system || !system->is_initialized)
    return;

  for (u32 i = 0; i < system->active_count; i++) {
    NPCCombatState *state = &system->combat_states[i];
    
    // Update combat timers
    state->time_since_last_attack += delta_time;
    state->time_since_seen_target += delta_time;
    state->combo_timer -= delta_time;
    
    // Reset combo if timer expired
    if (state->combo_timer <= 0.0f) {
      state->combo_count = 0;
      state->combo_timer = 0.0f;
    }
    
    // Update threat assessment
    npc_combat_update_threats(system, state->current_target, delta_time);
    
    // Update tactical behavior
    npc_combat_update_tactic(system, state->current_target, delta_time);
    
    // Check if we should attack
    if (npc_combat_can_attack(system, state->current_target)) {
      npc_combat_attack(system, state->current_target, state->current_target);
    }
  }
}

// Get combat state for entity
NPCCombatState *npc_combat_get_state(NPCCombatSystem *system, Entity entity) {
  if (!system)
    return NULL;

  for (u32 i = 0; i < system->active_count; i++) {
    if (system->combat_states[i].current_target.id == entity.id) {
      return &system->combat_states[i];
    }
  }
  return NULL;
}

// Create combat state for NPC
void npc_combat_create_state(NPCCombatSystem *system, Entity entity, NPCType type) {
  if (!system || system->active_count >= system->max_states)
    return;

  NPCCombatState *state = &system->combat_states[system->active_count++];
  memset(state, 0, sizeof(NPCCombatState));
  
  state->current_target = entity;
  state->combat_behavior = NPC_COMBAT_MELEE; // Default to melee
  
  // Set combat stats based on NPC type
  NPCStats stats = npc_get_stats(type);
  state->attack_damage = stats.damage;
  state->attack_speed = 1.0f; // 1 attack per second default
  state->attack_range = 2.0f; // Melee range default
  state->critical_chance = 0.05f; // 5% critical chance default
  state->accuracy = 0.8f; // 80% accuracy default
  
  // Set combat behavior based on NPC type
  switch (type) {
  case NPC_TYPE_ZOMBIE:
    state->combat_behavior = NPC_COMBAT_MELEE;
    state->preferred_tactic = NPC_TACTIC_AGGRESSIVE;
    break;
    
  case NPC_TYPE_SKELETON:
    state->combat_behavior = NPC_COMBAT_RANGED;
    state->preferred_tactic = NPC_TACTIC_KITE;
    state->has_ranged_weapon = true;
    state->attack_range = 16.0f;
    break;
    
  case NPC_TYPE_CREEPER:
    state->combat_behavior = NPC_COMBAT_EXPLOSIVE;
    state->preferred_tactic = NPC_TACTIC_AMBUSH;
    break;
    
  case NPC_TYPE_VILLAGER:
    state->combat_behavior = NPC_COMBAT_DEFENSIVE;
    state->preferred_tactic = NPC_TACTIC_RETREAT;
    break;
    
  default:
    state->combat_behavior = NPC_COMBAT_MELEE;
    state->preferred_tactic = NPC_TACTIC_DEFENSIVE;
    break;
  }
  
  state->current_tactic = state->preferred_tactic;
  
  LOG_DEBUG("Created combat state for NPC %u (type %d)", entity.id, type);
}

// Perform melee attack
bool npc_combat_melee_attack(NPCCombatSystem *system, Entity attacker, Entity target) {
  if (!system || !system->ecs)
    return false;

  // Get components
  TransformComponent *attacker_transform = (TransformComponent *)world_get_component(
      system->ecs, attacker, TRANSFORM_COMPONENT_ID);
  TransformComponent *target_transform = (TransformComponent *)world_get_component(
      system->ecs, target, TRANSFORM_COMPONENT_ID);
  HealthComponent *target_health = (HealthComponent *)world_get_component(
      system->ecs, target, HEALTH_COMPONENT_ID);
  
  if (!attacker_transform || !target_transform || !target_health) {
    return false;
  }

  // Check range
  f32 distance = vec3_length(vec3_sub(attacker_transform->position, 
                                      target_transform->position));
  if (distance > 3.0f) { // Melee range
    return false;
  }

  // Get combat state
  NPCCombatState *state = npc_combat_get_state(system, attacker);
  if (!state) {
    return false;
  }

  // Check attack cooldown
  if (state->time_since_last_attack < (1.0f / state->attack_speed)) {
    return false;
  }

  // Calculate damage
  f32 damage = state->attack_damage;
  
  // Apply combo multiplier
  if (state->combo_count > 0) {
    damage *= (1.0f + state->combo_count * 0.1f);
  }
  
  // Critical hit chance
  bool is_critical = (rand() / (float)RAND_MAX) < state->critical_chance;
  if (is_critical) {
    damage *= 2.0f;
  }
  
  // Apply accuracy check
  bool hit = (rand() / (float)RAND_MAX) < state->accuracy;
  if (hit) {
    // Apply damage
    target_health->health -= damage;
    if (target_health->health < 0) {
      target_health->health = 0;
    }
    
    // Update combo
    state->combo_count++;
    state->combo_timer = 1.0f; // 1 second combo window
    
    // Apply knockback
    RigidBodyComponent *target_rb = (RigidBodyComponent *)world_get_component(
        system->ecs, target, RIGIDBODY_COMPONENT_ID);
    if (target_rb && target_rb->body) {
      Vec3 knockback = vec3_normalize(vec3_sub(target_transform->position, 
                                            attacker_transform->position));
      knockback = vec3_mul(knockback, 5.0f);
      rigid_body_apply_impulse(target_rb->body, knockback);
    }
    
    // Update statistics
    system->total_attacks++;
    system->total_hits++;
    if (is_critical) {
      system->total_criticals++;
    }
    
    // Trigger event
    npc_combat_on_attack_hit(system, attacker, target, damage);
    
    LOG_DEBUG("NPC %u hit NPC %u for %.1f damage%s", 
              attacker.id, target.id, damage, is_critical ? " (CRITICAL)" : "");
  } else {
    // Miss
    npc_combat_on_attack_miss(system, attacker, target);
    system->total_attacks++;
    
    LOG_DEBUG("NPC %u missed NPC %u", attacker.id, target.id);
  }
  
  // Reset attack timer
  state->time_since_last_attack = 0.0f;
  
  return hit;
}

// General attack function
bool npc_combat_attack(NPCCombatSystem *system, Entity attacker, Entity target) {
  if (!system)
    return false;

  NPCCombatState *state = npc_combat_get_state(system, attacker);
  if (!state) {
    return false;
  }

  switch (state->combat_behavior) {
  case NPC_COMBAT_MELEE:
    return npc_combat_melee_attack(system, attacker, target);
    
  case NPC_COMBAT_RANGED:
    // TODO: Implement ranged attack
    return false;
    
  case NPC_COMBAT_EXPLOSIVE:
    // TODO: Implement explosive attack
    return false;
    
  default:
    return npc_combat_melee_attack(system, attacker, target);
  }
}

// Check if NPC can attack
bool npc_combat_can_attack(NPCCombatSystem *system, Entity entity) {
  NPCCombatState *state = npc_combat_get_state(system, entity);
  if (!state) {
    return false;
  }

  // Check if in combat and not on cooldown
  return state->is_in_combat && 
         state->time_since_last_attack >= (1.0f / state->attack_speed);
}

// Update tactical behavior
void npc_combat_update_tactic(NPCCombatSystem *system, Entity entity, f32 delta_time) {
  NPCCombatState *state = npc_combat_get_state(system, entity);
  if (!state) {
    return;
  }

  // Simple tactic updates based on situation
  switch (state->current_tactic) {
  case NPC_TACTIC_AGGRESSIVE:
    // Move towards target and attack
    break;
    
  case NPC_TACTIC_DEFENSIVE:
    // Wait for openings, block more
    break;
    
  case NPC_TACTIC_KITE:
    // Attack from distance, retreat when close
    break;
    
  case NPC_TACTIC_FLANK:
    // Try to get behind target
    state->circling_angle += delta_time * 2.0f; // Circle around
    break;
    
  default:
    break;
  }
}

// Update threat assessment
void npc_combat_update_threats(NPCCombatSystem *system, Entity entity, f32 delta_time) {
  if (!system || entity.id == 0)
    return;

  // Decay threat over time
  NPCCombatState *state = npc_combat_get_state(system, entity);
  if (state && state->threat_level > 0.0f) {
    state->threat_level -= delta_time * system->threat_decay_rate;
    if (state->threat_level < 0.0f) {
      state->threat_level = 0.0f;
    }
  }
}

// Combat event callbacks
void npc_combat_on_attack_hit(NPCCombatSystem *system, Entity attacker, 
                             Entity target, f32 damage) {
  system->total_combat_events++;
  
  // Update target's combat state
  NPCCombatState *target_state = npc_combat_get_state(system, target);
  if (target_state) {
    target_state->last_attacker = attacker;
    target_state->threat_level += damage;
    target_state->is_in_combat = true;
  }
  
  // Start combat if not already active
  NPCCombatState *attacker_state = npc_combat_get_state(system, attacker);
  if (attacker_state && !attacker_state->is_in_combat) {
    attacker_state->is_in_combat = true;
    npc_combat_on_combat_start(system, attacker, target);
  }
}

void npc_combat_on_attack_miss(NPCCombatSystem *system, Entity attacker, Entity target) {
  system->total_combat_events++;
  LOG_DEBUG("Attack missed");
}

void npc_combat_on_combat_start(NPCCombatSystem *system, Entity entity, Entity target) {
  LOG_DEBUG("Combat started between %u and %u", entity.id, target.id);
}

void npc_combat_on_combat_end(NPCCombatSystem *system, Entity entity) {
  NPCCombatState *state = npc_combat_get_state(system, entity);
  if (state) {
    state->is_in_combat = false;
    state->current_target.id = 0;
    state->threat_level = 0.0f;
  }
  
  LOG_DEBUG("Combat ended for entity %u", entity.id);
}

// Utility functions
u32 npc_combat_get_active_combat_count(NPCCombatSystem *system) {
  return system ? system->active_count : 0;
}

void npc_combat_debug_print_state(NPCCombatSystem *system, Entity entity) {
  NPCCombatState *state = npc_combat_get_state(system, entity);
  if (!state) {
    printf("No combat state for entity %u\n", entity.id);
    return;
  }

  printf("Combat State for entity %u:\n", entity.id);
  printf("  In Combat: %s\n", state->is_in_combat ? "Yes" : "No");
  printf("  Target: %u\n", state->current_target.id);
  printf("  Threat Level: %.2f\n", state->threat_level);
  printf("  Combat Behavior: %d\n", state->combat_behavior);
  printf("  Current Tactic: %d\n", state->current_tactic);
  printf("  Attack Damage: %.2f\n", state->attack_damage);
  printf("  Combo Count: %u\n", state->combo_count);
}
