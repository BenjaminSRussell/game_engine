// src/engine/ai/npc_ai.c
// NPC AI system with behavior trees and utility AI
#include <ai/npc_ai.h>
#include <ai/behavior_tree.h>
#include <ecs/components/npc.h>
#include <ecs/components/transform.h>
#include <ecs/components/health.h>
#include "engine/include/core/logger.h"
#include "engine/include/core/memory.h"
#include <math/vec3.h>
#include <stdlib.h>
#include <string.h>

// External references
extern f32 g_game_time;

// Initialize AI system
void npc_ai_system_init(NPCAISystem *system, struct World *ecs, 
                       struct PhysicsWorld *physics) {
  if (!system || !ecs || !physics)
    return;

  memset(system, 0, sizeof(NPCAISystem));
  system->ecs = ecs;
  system->physics = physics;
  system->contexts = malloc(sizeof(NPCAIContext) * 256);
  system->max_contexts = 256;
  system->context_count = 0;
  
  // Initialize behavior tree templates
  system->templates[NPC_AI_TEMPLATE_PASSIVE_ANIMAL] = npc_ai_create_passive_animal_tree();
  system->templates[NPC_AI_TEMPLATE_HOSTILE_MOB] = npc_ai_create_hostile_mob_tree();
  system->templates[NPC_AI_TEMPLATE_NEUTRAL_VILLAGER] = npc_ai_create_villager_tree();
  system->templates[NPC_AI_TEMPLATE_GUARD] = npc_ai_create_guard_tree();
  system->templates[NPC_AI_TEMPLATE_MERCHANT] = npc_ai_create_merchant_tree();
  system->templates[NPC_AI_TEMPLATE_CREEPER] = npc_ai_create_creeper_tree();
  
  system->global_update_interval = 0.1f; // 100ms
  system->perception_range = 16.0f;
  system->combat_range = 2.0f;
  system->is_initialized = true;
  
  LOG_INFO("NPC AI system initialized");
}

// Free AI system
void npc_ai_system_free(NPCAISystem *system) {
  if (!system)
    return;

  // Free all contexts
  for (u32 i = 0; i < system->context_count; i++) {
    if (system->contexts[i].behavior_tree) {
      bt_tree_destroy(system->contexts[i].behavior_tree);
    }
    if (system->contexts[i].blackboard) {
      bt_blackboard_destroy(system->contexts[i].blackboard);
    }
  }
  
  free(system->contexts);
  memset(system, 0, sizeof(NPCAISystem));
  LOG_INFO("NPC AI system freed");
}

// Create AI context for NPC
NPCAIContext *npc_ai_create_context(NPCAISystem *system, Entity entity, NPCType type) {
  if (!system || system->context_count >= system->max_contexts)
    return NULL;

  NPCAIContext *context = &system->contexts[system->context_count++];
  memset(context, 0, sizeof(NPCAIContext));
  
  context->entity = entity;
  context->npc_type = type;
  context->blackboard = bt_blackboard_create();
  context->current_goal = NPC_GOAL_NONE;
  context->previous_goal = NPC_GOAL_NONE;
  context->last_update_time = g_game_time;
  context->decision_interval = 0.5f; // 500ms decisions
  context->next_decision_time = g_game_time + context->decision_interval;
  context->update_frequency = 10; // Update every 10 frames
  
  // Assign appropriate behavior tree template
  NPCAITemplate template = NPC_AI_TEMPLATE_NONE;
  switch (type) {
  case NPC_TYPE_COW:
  case NPC_TYPE_PIG:
  case NPC_TYPE_CHICKEN:
    template = NPC_AI_TEMPLATE_PASSIVE_ANIMAL;
    break;
  case NPC_TYPE_ZOMBIE:
  case NPC_TYPE_SKELETON:
    template = NPC_AI_TEMPLATE_HOSTILE_MOB;
    break;
  case NPC_TYPE_VILLAGER:
    template = NPC_AI_TEMPLATE_NEUTRAL_VILLAGER;
    break;
  case NPC_TYPE_CREEPER:
    template = NPC_AI_TEMPLATE_CREEPER;
    break;
  default:
    template = NPC_AI_TEMPLATE_NEUTRAL_VILLAGER;
    break;
  }
  
  if (template != NPC_AI_TEMPLATE_NONE && system->templates[template]) {
    context->behavior_tree = bt_tree_clone(system->templates[template]);
  } else {
    context->behavior_tree = NULL;
  }
  
  LOG_DEBUG("Created AI context for NPC %u (type %d)", entity.id, type);
  return context;
}

// Update all AI contexts
void npc_ai_update(NPCAISystem *system, f32 delta_time) {
  if (!system || !system->is_initialized)
    return;

  for (u32 i = 0; i < system->context_count; i++) {
    NPCAIContext *context = &system->contexts[i];
    
    // Update based on frequency
    context->frame_counter++;
    if (context->frame_counter >= context->update_frequency) {
      npc_ai_update_single(system, context, delta_time);
      context->frame_counter = 0;
    }
  }
}

// Update single NPC AI
void npc_ai_update_single(NPCAISystem *system, NPCAIContext *context, f32 delta_time) {
  if (!context || !context->behavior_tree)
    return;

  // Update memory and perception
  npc_ai_update_memory(system, context, delta_time);
  npc_ai_perceive_environment(system, context);
  
  // Check if we need to make a new decision
  if (g_game_time >= context->next_decision_time) {
    // Select best goal based on current situation
    NPCAIGoal new_goal = npc_ai_select_goal(system, context);
    
    if (new_goal != context->current_goal) {
      context->previous_goal = context->current_goal;
      context->current_goal = new_goal;
      
      // Update blackboard with new goal
      bt_blackboard_set_int(context->blackboard, "current_goal", (int)new_goal);
      
      LOG_DEBUG("NPC %u changed goal from %d to %d", 
                context->entity.id, context->current_goal, new_goal);
    }
    
    context->next_decision_time = g_game_time + context->decision_interval;
  }
  
  // Execute behavior tree
  if (context->behavior_tree) {
    bt_tree_execute(context->behavior_tree, context->blackboard, delta_time);
  }
}

// Create behavior tree templates (simplified versions)
BehaviorTree *npc_ai_create_passive_animal_tree(void) {
  // TODO: Implement full passive animal behavior tree
  return NULL; // Placeholder
}

BehaviorTree *npc_ai_create_hostile_mob_tree(void) {
  // TODO: Implement hostile mob behavior tree
  return NULL; // Placeholder
}

BehaviorTree *npc_ai_create_villager_tree(void) {
  // TODO: Implement villager behavior tree
  return NULL; // Placeholder
}

BehaviorTree *npc_ai_create_guard_tree(void) {
  // TODO: Implement guard behavior tree
  return NULL; // Placeholder
}

BehaviorTree *npc_ai_create_merchant_tree(void) {
  // TODO: Implement merchant behavior tree
  return NULL; // Placeholder
}

BehaviorTree *npc_ai_create_creeper_tree(void) {
  // TODO: Implement creeper behavior tree
  return NULL; // Placeholder
}

// Goal selection utilities
NPCAIGoal npc_ai_select_goal(NPCAISystem *system, NPCAIContext *context) {
  // Simple goal selection based on NPC type and current state
  NPCComponent *npc = (NPCComponent *)world_get_component(
      system->ecs, context->entity, NPC_COMPONENT_ID);
  if (!npc)
    return NPC_GOAL_NONE;

  // Check health first
  HealthComponent *health = (HealthComponent *)world_get_component(
      system->ecs, context->entity, HEALTH_COMPONENT_ID);
  if (health && health->health < health->max_health * 0.3f) {
    return NPC_GOAL_FLEE;
  }

  // Check needs
  if (npc->hunger < 20.0f) {
    return NPC_GOAL_EAT;
  }
  if (npc->energy < 15.0f) {
    return NPC_GOAL_SLEEP;
  }

  // Type-specific goal selection
  switch (context->npc_type) {
  case NPC_TYPE_COW:
  case NPC_TYPE_PIG:
  case NPC_TYPE_CHICKEN:
    return NPC_GOAL_SURVIVE;
    
  case NPC_TYPE_ZOMBIE:
  case NPC_TYPE_SKELETON:
    return NPC_GOAL_HUNT;
    
  case NPC_TYPE_VILLAGER:
    if (npc->hunger < 50.0f) {
      return NPC_GOAL_WORK;
    }
    return NPC_GOAL_SOCIALIZE;
    
  case NPC_TYPE_CREEPER:
    return NPC_GOAL_HUNT;
    
  default:
    return NPC_GOAL_SURVIVE;
  }
}

// Memory and perception updates
void npc_ai_update_memory(NPCAISystem *system, NPCAIContext *context, f32 delta_time) {
  context->memory.time_since_attack += delta_time;
  context->memory.time_since_seen_player += delta_time;
  
  // Decay threat level over time
  if (context->memory.threat_level > 0.0f) {
    context->memory.threat_level -= delta_time * 0.5f;
    if (context->memory.threat_level < 0.0f) {
      context->memory.threat_level = 0.0f;
    }
  }
}

void npc_ai_perceive_environment(NPCAISystem *system, NPCAIContext *context) {
  // TODO: Implement perception system
  // For now, just update basic awareness
}

// Blackboard utilities
BTBlackboard *npc_ai_get_blackboard(NPCAISystem *system, Entity entity) {
  NPCAIContext *context = npc_ai_get_context(system, entity);
  return context ? context->blackboard : NULL;
}

// Utility functions
u32 npc_ai_get_active_context_count(NPCAISystem *system) {
  return system ? system->context_count : 0;
}

NPCAIContext *npc_ai_get_context(NPCAISystem *system, Entity entity) {
  if (!system)
    return NULL;

  for (u32 i = 0; i < system->context_count; i++) {
    if (system->contexts[i].entity.id == entity.id) {
      return &system->contexts[i];
    }
  }
  return NULL;
}
