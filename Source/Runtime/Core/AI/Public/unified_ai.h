#ifndef UNIFIED_AI_H
#define UNIFIED_AI_H

#include <core/types.h>
#include <math/vec3.h>

// Opaque types
typedef struct AISystem AISystem;
typedef struct AINPC AINPC;
typedef struct AIBehaviorNode AIBehaviorNode;

// IDs
typedef u32 NPCId;

// Enums
typedef enum {
  AI_STATE_IDLE,
  AI_STATE_PATROL,
  AI_STATE_COMBAT,
  AI_STATE_FLEE,
  AI_STATE_INVESTIGATE,
  AI_STATE_DEAD
} AIState;

typedef enum {
  AI_TASK_NONE,
  AI_TASK_MOVE_TO,
  AI_TASK_ATTACK_TARGET,
  AI_TASK_WAIT,
  AI_TASK_INTERACT
} AITaskType;

typedef struct {
  u32 max_npcs;
  u32 update_rate;
} AIConfig;

typedef struct {
  NPCId id;
  AIState state;
  Vec3 position;
  f32 health;
  f32 morale;
} AINPCInfo;

// Core System
AISystem *ai_system_create(AIConfig config);
void ai_system_destroy(AISystem *system);
void ai_system_update(AISystem *system, f32 delta_time);

// NPC Management
NPCId ai_npc_create(AISystem *system, Vec3 position);
void ai_npc_destroy(AISystem *system, NPCId id);
bool ai_npc_get_info(AISystem *system, NPCId id, AINPCInfo *out_info);
void ai_npc_set_state(AISystem *system, NPCId id, AIState state);

// Navigation
void ai_npc_move_to(AISystem *system, NPCId id, Vec3 target);
void ai_npc_stop(AISystem *system, NPCId id);

// Perception
void ai_npc_report_stimulus(AISystem *system, NPCId observer_id,
                            Vec3 stimulus_pos, f32 intensity);

// Behavior Tree (Simplified Public API)
AIBehaviorNode *ai_bt_create_sequence(AIBehaviorNode **children, u32 count);
AIBehaviorNode *ai_bt_create_selector(AIBehaviorNode **children, u32 count);
void ai_bt_destroy(AIBehaviorNode *node);

#endif // UNIFIED_AI_H
