#ifndef MEMORY_SYSTEM_H
#define MEMORY_SYSTEM_H

#include "../../core/types.h"
#include "../../math/vec3.h"
#include <stdbool.h>

#ifdef __cplusplus
extern "C" {
#endif

// AGENT_AI_1 Phase 9-11: Memory System

// Forward declarations
typedef struct MemorySystem MemorySystem;
typedef struct MemoryAgent MemoryAgent;
typedef struct PerceivedEntity PerceivedEntity; // From perception_system.h

// === Phase 9: Memory System Core ===

#define MEMORY_STM_CAPACITY 32  // Short-term memory slots
#define MEMORY_LTM_CAPACITY 256 // Long-term memory slots
#define MEMORY_MAX_AGENTS 128

// Memory types
typedef enum {
  MEMORY_TYPE_ENTITY_SIGHTING,  // Saw an entity
  MEMORY_TYPE_SOUND_HEARD,      // Heard a sound
  MEMORY_TYPE_DAMAGE_TAKEN,     // Took damage
  MEMORY_TYPE_DAMAGE_DEALT,     // Dealt damage
  MEMORY_TYPE_LOCATION_VISITED, // Visited a location
  MEMORY_TYPE_ITEM_SEEN,        // Saw an item
  MEMORY_TYPE_EVENT_WITNESSED,  // Witnessed an event
  MEMORY_TYPE_CONVERSATION,     // Had a conversation
  MEMORY_TYPE_THREAT,           // Identified a threat
  MEMORY_TYPE_ALLY_SEEN,        // Saw an ally
  MEMORY_TYPE_COUNT
} MemoryType;

// Memory importance/significance
typedef enum {
  MEMORY_IMPORTANCE_TRIVIAL = 0, // Decay quickly
  MEMORY_IMPORTANCE_LOW,         // Normal decay
  MEMORY_IMPORTANCE_MEDIUM,      // Slower decay
  MEMORY_IMPORTANCE_HIGH,        // Very slow decay
  MEMORY_IMPORTANCE_CRITICAL     // Never decay (important event)
} MemoryImportance;

// Individual memory entry
typedef struct {
  MemoryType type;
  MemoryImportance importance;

  // Core data
  EntityID subject_entity; // Who/what is this memory about
  Vec3 location;           // Where did this happen
  f64 timestamp;           // When did this happen
  f64 last_reinforced;     // When was this memory last reinforced

  // Memory strength
  f32 strength;         // Memory strength (0.0 - 1.0)
  f32 confidence;       // Confidence in this memory
  f32 emotional_weight; // Emotional significance

  // Decay parameters
  f32 decay_rate;   // How fast memory decays
  u32 access_count; // How many times accessed

  // Type-specific data
  union {
    struct {
      bool was_hostile;
      f32 threat_level;
    } entity_sighting;

    struct {
      Vec3 sound_origin;
      f32 volume;
      char sound_type[32];
    } sound_heard;

    struct {
      EntityID attacker;
      f32 damage_amount;
    } damage_taken;

    struct {
      EntityID target;
      f32 damage_amount;
    } damage_dealt;

    struct {
      char location_name[64];
      bool is_safe;
    } location_visited;
  } data;

  // Metadata
  char description[128]; // Human-readable description
  bool is_active;        // Whether memory is valid
  bool in_long_term;     // Whether consolidated to LTM
} Memory;

// Short-Term Memory (STM) buffer
typedef struct {
  Memory entries[MEMORY_STM_CAPACITY];
  u32 count;
  u32 next_insert_index; // Ring buffer index
  f64 last_update;
} ShortTermMemory;

// Long-Term Memory (LTM) storage
typedef struct {
  Memory entries[MEMORY_LTM_CAPACITY];
  u32 count;
  f64 last_consolidation;
} LongTermMemory;

// Memory statistics
typedef struct {
  u32 total_memories_created;
  u32 total_memories_decayed;
  u32 total_consolidations;
  u32 total_accesses;
  f32 average_memory_strength;
  f32 average_access_count;
} MemoryStats;

// === Knowledge Graph (Phase 12) ===

typedef enum {
  RELATION_OWNS,
  RELATION_LOCATED_AT,
  RELATION_HOSTILE_TOWARDS,
  RELATION_ALLY_OF,
  RELATION_PART_OF,
  RELATION_MEMBER_OF,
  RELATION_KNOWS,
  RELATION_INTERESTED_IN
} MemoryRelation;

typedef struct {
  EntityID target_id;
  MemoryRelation relation;
  f32 strength;
  f64 timestamp;
} MemoryEdge;

typedef struct {
  EntityID entity_id;
  char name[64];
  Vec3 last_known_pos;

  MemoryEdge edges[16];
  u32 edge_count;

  bool is_persistent;
} MemoryNode;

// Memory agent (per NPC)
struct MemoryAgent {
  EntityID entity_id;
  char name[64];

  ShortTermMemory stm; // Short-term working memory
  LongTermMemory ltm;  // Long-term consolidated memory

  // Knowledge Graph
  MemoryNode nodes[64];
  u32 node_count;

  // Configuration
  struct {
    f32 stm_decay_rate;          // STM decay rate (per second)
    f32 ltm_decay_rate;          // LTM decay rate (per second)
    f32 consolidation_threshold; // Strength needed to move STM->LTM
    f32 consolidation_interval;  // How often to consolidate (seconds)
    f32 importance_bias;         // How much importance affects decay
    bool enable_forgetting;      // Whether to forget old memories
  } config;

  // Statistics
  MemoryStats stats;

  // Perception integration
  f64 last_perception_update;
};

// Main memory system
struct MemorySystem {
  MemoryAgent *agents;
  u32 agent_count;
  u32 agent_capacity;

  // Global config
  f32 global_decay_rate;
  f32 emotional_weight_multiplier;
  bool enable_debug;

  // Statistics
  u32 total_agents;
  u64 total_memories_all_agents;
};

// === Phase 9 API: Core Memory System ===

MemorySystem *memory_system_create(u32 max_agents);
void memory_system_destroy(MemorySystem *system);

MemoryAgent *memory_system_add_agent(MemorySystem *system, EntityID entity_id,
                                     const char *name);
bool memory_system_remove_agent(MemorySystem *system, EntityID entity_id);
MemoryAgent *memory_system_get_agent(MemorySystem *system, EntityID entity_id);

// Memory creation
Memory *memory_create_entry(MemoryAgent *agent, MemoryType type,
                            EntityID subject);
bool memory_add_to_stm(MemoryAgent *agent, const Memory *memory);
bool memory_add_to_ltm(MemoryAgent *agent, const Memory *memory);

// === Phase 10: Memory Decay & Consolidation ===

void memory_decay_all(MemoryAgent *agent, f64 delta_time);
void memory_consolidate_stm_to_ltm(MemoryAgent *agent);
void memory_reinforce(Memory *memory, f32 strength_boost);
void memory_forget_weak_memories(MemoryAgent *agent, f32 threshold);

// Decay calculation
f32 memory_calculate_decay(const Memory *memory, f64 time_since_creation,
                           f32 base_decay_rate, f32 importance_bias);

// === Phase 11: Perception Bridge ===

// Bridge from perception system to memory
void memory_process_perceived_entity(MemoryAgent *agent,
                                     const PerceivedEntity *entity,
                                     f64 current_time);
void memory_process_sound_event(MemoryAgent *agent, const Vec3 *location,
                                const char *sound_type, f32 volume,
                                f64 current_time);
void memory_process_damage_event(MemoryAgent *agent, EntityID attacker,
                                 f32 damage, f64 current_time);

// Integrate with perception system memory callbacks
void memory_sync_with_perception(MemoryAgent *agent,
                                 PerceivedEntity *perceived_entities,
                                 u32 entity_count, f64 current_time);

// === Memory Queries ===

// Find memories by criteria
Memory *memory_find_by_entity(MemoryAgent *agent, EntityID entity_id,
                              u32 *count);
Memory *memory_find_by_type(MemoryAgent *agent, MemoryType type, u32 *count);
Memory *memory_find_by_location(MemoryAgent *agent, const Vec3 *location,
                                f32 radius, u32 *count);
Memory *memory_find_recent(MemoryAgent *agent, f64 time_window, u32 *count);

// Get most significant memory
Memory *memory_get_most_important(MemoryAgent *agent, MemoryType type);
Memory *memory_get_most_recent(MemoryAgent *agent, MemoryType type);

// Check if entity is remembered
bool memory_remembers_entity(MemoryAgent *agent, EntityID entity_id);
f32 memory_get_entity_threat_level(MemoryAgent *agent, EntityID entity_id);
Vec3 memory_get_last_known_location(MemoryAgent *agent, EntityID entity_id);

// === Knowledge Graph API (Phase 12) ===
void memory_add_node(MemoryAgent *agent, EntityID entity_id, const char *name);
void memory_add_edge(MemoryAgent *agent, EntityID from_id, EntityID to_id,
                     MemoryRelation relation, f32 strength);
MemoryNode *memory_get_node(MemoryAgent *agent, EntityID entity_id);
bool memory_has_relation(MemoryAgent *agent, EntityID from_id, EntityID to_id,
                         MemoryRelation relation);

// === Memory Update ===

void memory_system_update(MemorySystem *system, f64 delta_time);
void memory_update_agent(MemoryAgent *agent, f64 delta_time);

// === Debug & Statistics ===

void memory_print_stm(const MemoryAgent *agent);
void memory_print_ltm(const MemoryAgent *agent);
void memory_agent_print_stats(const MemoryAgent *agent);
void memory_print_about_entity(const MemoryAgent *agent, EntityID entity_id);

const char *memory_type_to_string(MemoryType type);
const char *memory_importance_to_string(MemoryImportance importance);

#ifdef __cplusplus
}
#endif

#endif // MEMORY_SYSTEM_H
