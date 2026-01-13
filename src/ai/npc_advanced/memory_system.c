#include "../../include/ai/npc_advanced/memory_system.h"
#include <ai/npc/perception_system.h>
#include <core/logger.h>
// ai/npc_advanced/memory_system.c
// Advanced AI Memory and Knowledge Graph system.
//
// TODO: Implement Knowledge Graph (RDF-like) for storing world facts and
// relations.
// TODO: Add support for Episodic Memory (past events) with temporal decay.
// TODO: Implement Sensory Memory for short-term spatial awareness.
// TODO: Add support for belief-propagation between agents (Knowledge Sharing).
// TODO: Implement forgetting-mechanisms based on priority and age.
// TODO: Add support for "False Memories" and deception logic for high-fidelity
// NPCs.
// TODO: Implement spatial-memory querying for "where was I last?" logic.
// TODO: Add support for hierarchical memory (Global -> Local -> Immediate).
// TODO: Research and implement Neural-Memory integration for pattern
// recognition.
// TODO: Implement a query language (SPARQL-lite) for memory retrieval.
// TODO: Add support for emotion-weighted memory priority.

#include <common.h>
#include <core/memory.h>
#include <include/math/math.h>
#include <stdlib.h>
#include <string.h>

/**
 * =================================================================================================
 *                          MEMORY SYSTEM - AGENT_AI_1 (Phase 9-11)
 * =================================================================================================
 *
 * PURPOSE: Short-term and long-term memory for NPCs.
 *
 * FEATURES:
 * - Phase 9: STM/LTM buffers with ring buffer management
 * - Phase 10: Time-based decay and STM->LTM consolidation
 * - Phase 11: Perception system integration
 * =================================================================================================
 */

// ===========================================================================================
// PHASE 9: CORE MEMORY SYSTEM
// ===========================================================================================

MemorySystem *memory_system_create(u32 max_agents) {
  MemorySystem *system =
      (MemorySystem *)MALLOC_TAGGED(sizeof(MemorySystem), MEMORY_TAG_AI);
  if (!system) {
    LOG_ERROR("Failed to allocate memory system");
    return NULL;
  }

  memset(system, 0, sizeof(MemorySystem));
  system->agent_capacity = max_agents;
  system->agents = (MemoryAgent *)MALLOC_TAGGED(
      sizeof(MemoryAgent) * max_agents, MEMORY_TAG_AI);

  if (!system->agents) {
    FREE(0);
    return NULL;
  }

  memset(system->agents, 0, sizeof(MemoryAgent) * max_agents);

  // Default configuration
  system->global_decay_rate = 0.1f;
  system->emotional_weight_multiplier = 2.0f;

  return system;
}

void memory_system_destroy(MemorySystem *system) {
  if (!system)
    return;

  if (system->agents) {
    FREE(0);
  }

  FREE(0);
}

MemoryAgent *memory_system_add_agent(MemorySystem *system, EntityID entity_id,
                                     const char *name) {
  if (!system || system->agent_count >= system->agent_capacity) {
    return NULL;
  }

  MemoryAgent *agent = &system->agents[system->agent_count++];
  memset(agent, 0, sizeof(MemoryAgent));

  agent->entity_id = entity_id;
  strncpy(agent->name, name, sizeof(agent->name) - 1);

  // Default configuration
  agent->config.stm_decay_rate = 0.2f;          // STM decays faster
  agent->config.ltm_decay_rate = 0.01f;         // LTM decays slower
  agent->config.consolidation_threshold = 0.5f; // Strength >= 0.5 moves to LTM
  agent->config.consolidation_interval = 5.0f;  // Consolidate every 5 seconds
  agent->config.importance_bias = 2.0f; // Importance slows decay significantly
  agent->config.enable_forgetting = true;

  system->total_agents++;

  return agent;
}

bool memory_system_remove_agent(MemorySystem *system, EntityID entity_id) {
  if (!system)
    return false;

  for (u32 i = 0; i < system->agent_count; i++) {
    if (system->agents[i].entity_id == entity_id) {
      // Shift remaining agents
      for (u32 j = i; j < system->agent_count - 1; j++) {
        system->agents[j] = system->agents[j + 1];
      }
      system->agent_count--;
      return true;
    }
  }

  return false;
}

MemoryAgent *memory_system_get_agent(MemorySystem *system, EntityID entity_id) {
  if (!system)
    return NULL;

  for (u32 i = 0; i < system->agent_count; i++) {
    if (system->agents[i].entity_id == entity_id) {
      return &system->agents[i];
    }
  }

  return NULL;
}

Memory *memory_create_entry(MemoryAgent *agent, MemoryType type,
                            EntityID subject) {
  if (!agent)
    return NULL;

  Memory *memory = (Memory *)malloc(sizeof(Memory));
  if (!memory)
    return NULL;

  memset(memory, 0, sizeof(Memory));
  memory->type = type;
  memory->subject_entity = subject;
  memory->strength = 1.0f; // Start at full strength
  memory->confidence = 1.0f;
  memory->importance = MEMORY_IMPORTANCE_HIGH;
  memory->is_active = true;
  memory->in_long_term = false;

  // Set decay rate based on importance
  switch (memory->importance) {
  case MEMORY_IMPORTANCE_TRIVIAL:
    memory->decay_rate = agent->config.stm_decay_rate * 2.0f;
    break;
  case MEMORY_IMPORTANCE_LOW:
    memory->decay_rate = agent->config.stm_decay_rate;
    break;
  case MEMORY_IMPORTANCE_MEDIUM:
    memory->decay_rate = agent->config.stm_decay_rate * 0.5f;
    break;
  case MEMORY_IMPORTANCE_HIGH:
    memory->decay_rate = agent->config.stm_decay_rate * 0.25f;
    break;
  case MEMORY_IMPORTANCE_CRITICAL:
    memory->decay_rate = 0.0f; // Never decay
    break;
  }

  return memory;
}

bool memory_add_to_stm(MemoryAgent *agent, const Memory *memory) {
  if (!agent || !memory)
    return false;

  // Ring buffer insertion
  u32 insert_idx = agent->stm.next_insert_index;
  agent->stm.entries[insert_idx] = *memory;

  agent->stm.next_insert_index = (insert_idx + 1) % MEMORY_STM_CAPACITY;

  if (agent->stm.count < MEMORY_STM_CAPACITY) {
    agent->stm.count++;
  }

  agent->stats.total_memories_created++;

  return true;
}

bool memory_add_to_ltm(MemoryAgent *agent, const Memory *memory) {
  if (!agent || !memory)
    return false;

  if (agent->ltm.count >= MEMORY_LTM_CAPACITY) {
    LOG_WARN("LTM full for agent %s", agent->name);
    return false;
  }

  Memory *ltm_memory = &agent->ltm.entries[agent->ltm.count++];
  *ltm_memory = *memory;
  ltm_memory->in_long_term = true;
  ltm_memory->decay_rate = agent->config.ltm_decay_rate;

  agent->stats.total_consolidations++;

  return true;
}

// ===========================================================================================
// PHASE 10: MEMORY DECAY & CONSOLIDATION
// ===========================================================================================

f32 memory_calculate_decay(const Memory *memory, f64 time_since_creation,
                           f32 base_decay_rate, f32 importance_bias) {
  if (!memory)
    return 0.0f;

  // Calculate decay based on time and importance
  f32 importance_factor = 1.0f;

  switch (memory->importance) {
  case MEMORY_IMPORTANCE_TRIVIAL:
    importance_factor = 2.0f;
    break;
  case MEMORY_IMPORTANCE_LOW:
    importance_factor = 1.0f;
    break;
  case MEMORY_IMPORTANCE_MEDIUM:
    importance_factor = 0.5f;
    break;
  case MEMORY_IMPORTANCE_HIGH:
    importance_factor = 0.25f;
    break;
  case MEMORY_IMPORTANCE_CRITICAL:
    return 0.0f; // No decay
  }

  // Exponential decay: strength = e^(-decay_rate * time * importance_factor)
  f32 decay_amount =
      base_decay_rate * (f32)time_since_creation * importance_factor;

  // Emotional weight reduces decay
  decay_amount *= (1.0f - memory->emotional_weight * 0.5f);

  return decay_amount;
}

void memory_decay_all(MemoryAgent *agent, f64 delta_time) {
  if (!agent)
    return;

  // Decay STM
  for (u32 i = 0; i < agent->stm.count; i++) {
    Memory *mem = &agent->stm.entries[i];
    if (!mem->is_active)
      continue;

    f32 decay =
        memory_calculate_decay(mem, delta_time, agent->config.stm_decay_rate,
                               agent->config.importance_bias);
    mem->strength -= decay;

    if (mem->strength <= 0.0f) {
      mem->is_active = false;
      agent->stats.total_memories_decayed++;
    }
  }

  // Decay LTM
  if (agent->config.enable_forgetting) {
    for (u32 i = 0; i < agent->ltm.count; i++) {
      Memory *mem = &agent->ltm.entries[i];
      if (!mem->is_active)
        continue;

      f32 decay =
          memory_calculate_decay(mem, delta_time, agent->config.ltm_decay_rate,
                                 agent->config.importance_bias);
      mem->strength -= decay;

      if (mem->strength <= 0.0f) {
        mem->is_active = false;
        agent->stats.total_memories_decayed++;
      }
    }
  }
}

void memory_consolidate_stm_to_ltm(MemoryAgent *agent) {
  if (!agent)
    return;

  // Move strong STM memories to LTM
  for (u32 i = 0; i < agent->stm.count; i++) {
    Memory *mem = &agent->stm.entries[i];

    if (!mem->is_active)
      continue;
    if (mem->in_long_term)
      continue;

    // Check if strong enough to consolidate
    if (mem->strength >= agent->config.consolidation_threshold) {
      if (memory_add_to_ltm(agent, mem)) {
        mem->in_long_term = true; // Mark as consolidated
      }
    }
  }
}

void memory_reinforce(Memory *memory, f32 strength_boost) {
  if (!memory)
    return;

  memory->strength += strength_boost;
  if (memory->strength > 1.0f) {
    memory->strength = 1.0f;
  }

  memory->access_count++;
}

void memory_forget_weak_memories(MemoryAgent *agent, f32 threshold) {
  if (!agent)
    return;

  // Clean up STM
  for (u32 i = 0; i < agent->stm.count; i++) {
    if (agent->stm.entries[i].strength < threshold) {
      agent->stm.entries[i].is_active = false;
    }
  }

  // Clean up LTM
  for (u32 i = 0; i < agent->ltm.count; i++) {
    if (agent->ltm.entries[i].strength < threshold) {
      agent->ltm.entries[i].is_active = false;
    }
  }
}

// ===========================================================================================
// PHASE 11: PERCEPTION BRIDGE
// ===========================================================================================

void memory_process_perceived_entity(MemoryAgent *agent,
                                     const PerceivedEntity *entity,
                                     f64 current_time) {
  if (!agent || !entity)
    return;

  // Create memory of entity sighting
  Memory *memory = memory_create_entry(agent, MEMORY_TYPE_ENTITY_SIGHTING,
                                       entity->entity_id);
  if (!memory)
    return;

  memory->location = entity->last_known_position;
  memory->timestamp = current_time;
  memory->last_reinforced = current_time;
  memory->confidence = entity->overall_confidence;

  // Determine threat level based on detection
  memory->data.entity_sighting.threat_level = 0.5f; // Default
  memory->data.entity_sighting.was_hostile = false;

  // Set importance based on visibility and confidence
  if (entity->overall_confidence > 0.8f) {
    memory->importance = MEMORY_IMPORTANCE_HIGH;
  } else if (entity->overall_confidence > 0.5f) {
    memory->importance = MEMORY_IMPORTANCE_MEDIUM;
  } else {
    memory->importance = MEMORY_IMPORTANCE_LOW;
  }

  snprintf(memory->description, sizeof(memory->description),
           "Saw entity %u at (%.1f, %.1f, %.1f)", entity->entity_id,
           entity->last_known_position.x, entity->last_known_position.y,
           entity->last_known_position.z);

  memory_add_to_stm(agent, memory);
  free(memory);
}

void memory_process_sound_event(MemoryAgent *agent, const Vec3 *location,
                                const char *sound_type, f32 volume,
                                f64 current_time) {
  if (!agent || !location)
    return;

  Memory *memory = memory_create_entry(agent, MEMORY_TYPE_SOUND_HEARD, 0);
  if (!memory)
    return;

  memory->location = *location;
  memory->timestamp = current_time;
  memory->last_reinforced = current_time;

  memory->data.sound_heard.sound_origin = *location;
  memory->data.sound_heard.volume = volume;
  if (sound_type) {
    strncpy(memory->data.sound_heard.sound_type, sound_type,
            sizeof(memory->data.sound_heard.sound_type) - 1);
  }

  // Louder sounds are more important
  if (volume > 0.7f) {
    memory->importance = MEMORY_IMPORTANCE_HIGH;
  } else if (volume > 0.3f) {
    memory->importance = MEMORY_IMPORTANCE_MEDIUM;
  } else {
    memory->importance = MEMORY_IMPORTANCE_LOW;
  }

  snprintf(memory->description, sizeof(memory->description),
           "Heard %s at (%.1f, %.1f, %.1f)", sound_type ? sound_type : "sound",
           location->x, location->y, location->z);

  memory_add_to_stm(agent, memory);
  free(memory);
}

void memory_process_damage_event(MemoryAgent *agent, EntityID attacker,
                                 f32 damage, f64 current_time) {
  if (!agent)
    return;

  Memory *memory =
      memory_create_entry(agent, MEMORY_TYPE_DAMAGE_TAKEN, attacker);
  if (!memory)
    return;

  memory->timestamp = current_time;
  memory->last_reinforced = current_time;
  memory->data.damage_taken.attacker = attacker;
  memory->data.damage_taken.damage_amount = damage;

  // Being attacked is always important
  memory->importance = MEMORY_IMPORTANCE_CRITICAL;
  memory->emotional_weight = 0.9f; // High emotional significance

  snprintf(memory->description, sizeof(memory->description),
           "Took %.1f damage from entity %u", damage, attacker);

  memory_add_to_stm(agent, memory);
  free(memory);
}

void memory_sync_with_perception(MemoryAgent *agent,
                                 PerceivedEntity *perceived_entities,
                                 u32 entity_count, f64 current_time) {
  if (!agent || !perceived_entities)
    return;

  for (u32 i = 0; i < entity_count; i++) {
    PerceivedEntity *entity = &perceived_entities[i];

    // Only create memories for recently detected entities
    if (entity->is_detectable) {
      memory_process_perceived_entity(agent, entity, current_time);
    }
  }

  agent->last_perception_update = current_time;
}

// ===========================================================================================
// MEMORY QUERIES
// ===========================================================================================

Memory *memory_find_by_entity(MemoryAgent *agent, EntityID entity_id,
                              u32 *count) {
  if (!agent || !count)
    return NULL;

  *count = 0;
  Memory *results = (Memory *)malloc(
      sizeof(Memory) * (MEMORY_STM_CAPACITY + MEMORY_LTM_CAPACITY));

  // Search STM
  for (u32 i = 0; i < agent->stm.count; i++) {
    if (agent->stm.entries[i].is_active &&
        agent->stm.entries[i].subject_entity == entity_id) {
      results[(*count)++] = agent->stm.entries[i];
    }
  }

  // Search LTM
  for (u32 i = 0; i < agent->ltm.count; i++) {
    if (agent->ltm.entries[i].is_active &&
        agent->ltm.entries[i].subject_entity == entity_id) {
      results[(*count)++] = agent->ltm.entries[i];
    }
  }

  return results;
}

bool memory_remembers_entity(MemoryAgent *agent, EntityID entity_id) {
  if (!agent)
    return false;

  u32 count = 0;
  Memory *memories = memory_find_by_entity(agent, entity_id, &count);

  bool remembers = (count > 0);
  if (memories)
    free(memories);

  return remembers;
}

Vec3 memory_get_last_known_location(MemoryAgent *agent, EntityID entity_id) {
  Vec3 zero = {0, 0, 0};
  if (!agent)
    return zero;

  Memory *most_recent = NULL;
  f64 latest_time = 0.0;

  // Search STM
  for (u32 i = 0; i < agent->stm.count; i++) {
    Memory *mem = &agent->stm.entries[i];
    if (mem->is_active && mem->subject_entity == entity_id &&
        mem->type == MEMORY_TYPE_ENTITY_SIGHTING) {
      if (mem->timestamp > latest_time) {
        latest_time = mem->timestamp;
        most_recent = mem;
      }
    }
  }

  // Search LTM
  for (u32 i = 0; i < agent->ltm.count; i++) {
    Memory *mem = &agent->ltm.entries[i];
    if (mem->is_active && mem->subject_entity == entity_id &&
        mem->type == MEMORY_TYPE_ENTITY_SIGHTING) {
      if (mem->timestamp > latest_time) {
        latest_time = mem->timestamp;
        most_recent = mem;
      }
    }
  }

  return most_recent ? most_recent->location : zero;
}

// ===========================================================================================
// MEMORY UPDATE
// ===========================================================================================

void memory_update_agent(MemoryAgent *agent, f64 delta_time) {
  if (!agent)
    return;

  // Decay memories
  memory_decay_all(agent, delta_time);

  // Consolidate STM to LTM periodically
  f64 time_since_consolidation = delta_time; // Simple approximation
  if (time_since_consolidation >= agent->config.consolidation_interval) {
    memory_consolidate_stm_to_ltm(agent);
    agent->ltm.last_consolidation = 0.0; // Reset timer
  } else {
    agent->ltm.last_consolidation += delta_time;
  }
}

void memory_system_update(MemorySystem *system, f64 delta_time) {
  if (!system)
    return;

  for (u32 i = 0; i < system->agent_count; i++) {
    memory_update_agent(&system->agents[i], delta_time);
  }
}

// ===========================================================================================
// DEBUG
// ===========================================================================================

const char *memory_type_to_string(MemoryType type) {
  switch (type) {
  case MEMORY_TYPE_ENTITY_SIGHTING:
    return "ENTITY_SIGHTING";
  case MEMORY_TYPE_SOUND_HEARD:
    return "SOUND_HEARD";
  case MEMORY_TYPE_DAMAGE_TAKEN:
    return "DAMAGE_TAKEN";
  case MEMORY_TYPE_DAMAGE_DEALT:
    return "DAMAGE_DEALT";
  case MEMORY_TYPE_LOCATION_VISITED:
    return "LOCATION_VISITED";
  case MEMORY_TYPE_ITEM_SEEN:
    return "ITEM_SEEN";
  case MEMORY_TYPE_EVENT_WITNESSED:
    return "EVENT_WITNESSED";
  default:
    return "UNKNOWN";
  }
}

const char *memory_importance_to_string(MemoryImportance importance) {
  switch (importance) {
  case MEMORY_IMPORTANCE_TRIVIAL:
    return "TRIVIAL";
  case MEMORY_IMPORTANCE_LOW:
    return "LOW";
  case MEMORY_IMPORTANCE_MEDIUM:
    return "MEDIUM";
  case MEMORY_IMPORTANCE_HIGH:
    return "HIGH";
  case MEMORY_IMPORTANCE_CRITICAL:
    return "CRITICAL";
  default:
    return "UNKNOWN";
  }
}

// ===========================================================================================
// PHASE 12: KNOWLEDGE GRAPH
// ===========================================================================================

void memory_add_node(MemoryAgent *agent, EntityID entity_id, const char *name) {
  if (!agent || agent->node_count >= 64)
    return;

  // Check if node already exists
  for (u32 i = 0; i < agent->node_count; i++) {
    if (agent->nodes[i].entity_id == entity_id) {
      if (name) {
        strncpy(agent->nodes[i].name, name, sizeof(agent->nodes[i].name) - 1);
      }
      return;
    }
  }

  MemoryNode *node = &agent->nodes[agent->node_count++];
  memset(node, 0, sizeof(MemoryNode));
  node->entity_id = entity_id;
  if (name) {
    strncpy(node->name, name, sizeof(node->name) - 1);
  }
}

void memory_add_edge(MemoryAgent *agent, EntityID from_id, EntityID to_id,
                     MemoryRelation relation, f32 strength) {
  if (!agent)
    return;

  MemoryNode *from_node = memory_get_node(agent, from_id);
  if (!from_node) {
    memory_add_node(agent, from_id, NULL);
    from_node = memory_get_node(agent, from_id);
  }

  if (!from_node || from_node->edge_count >= 16)
    return;

  // Check if edge already exists
  for (u32 i = 0; i < from_node->edge_count; i++) {
    if (from_node->edges[i].target_id == to_id &&
        from_node->edges[i].relation == relation) {
      from_node->edges[i].strength = strength;
      from_node->edges[i].timestamp = 0; // Should use actual time
      return;
    }
  }

  MemoryEdge *edge = &from_node->edges[from_node->edge_count++];
  edge->target_id = to_id;
  edge->relation = relation;
  edge->strength = strength;
  edge->timestamp = 0; // Should use actual time
}

MemoryNode *memory_get_node(MemoryAgent *agent, EntityID entity_id) {
  if (!agent)
    return NULL;

  for (u32 i = 0; i < agent->node_count; i++) {
    if (agent->nodes[i].entity_id == entity_id) {
      return &agent->nodes[i];
    }
  }

  return NULL;
}

bool memory_has_relation(MemoryAgent *agent, EntityID from_id, EntityID to_id,
                         MemoryRelation relation) {
  MemoryNode *from_node = memory_get_node(agent, from_id);
  if (!from_node)
    return false;

  for (u32 i = 0; i < from_node->edge_count; i++) {
    if (from_node->edges[i].target_id == to_id &&
        from_node->edges[i].relation == relation) {
      return from_node->edges[i].strength > 0.1f;
    }
  }

  return false;
}

void memory_agent_print_stats(const MemoryAgent *agent) {
  if (!agent)
    return;

  LOG_INFO("=== Memory Stats for %s ===", agent->name);
  LOG_INFO("STM Count: %u / %d", agent->stm.count, MEMORY_STM_CAPACITY);
  LOG_INFO("LTM Count: %u / %d", agent->ltm.count, MEMORY_LTM_CAPACITY);
  LOG_INFO("Node Count: %u / 64", agent->node_count);
  LOG_INFO("Total Created: %u", agent->stats.total_memories_created);
  LOG_INFO("Total Decayed: %u", agent->stats.total_memories_decayed);
  LOG_INFO("Consolidations: %u", agent->stats.total_consolidations);
}
