/*
 * ai_system.h
 *
 * Advanced AI system for NPC behavior, decision making, and learning
 * Part of the Advanced 3D Rendering Engine
 *
 * Copyright (c) 2024 Game Engine Project
 * Licensed under MIT License
 *
 * Provides functionality for:
 * - Behavior trees for NPC decision-making
 * - State machines for complex behaviors
 * - Goal-oriented action planning (GOAP)
 * - Influence maps for spatial reasoning
 * - Optimization with interest system
 */

#ifndef AI_SYSTEM_H
#define AI_SYSTEM_H

#include <stdint.h>
#include <stdbool.h>
#include <stddef.h>

#ifdef __cplusplus
extern "C" {
#endif

/* ============================================================================
 * TYPE DEFINITIONS
 * ============================================================================ */

typedef uint32_t NPCId;
typedef uint32_t TaskId;

/* NPC states */
typedef enum {
    NPC_STATE_IDLE = 0,
    NPC_STATE_PATROLLING,
    NPC_STATE_PURSUING,
    NPC_STATE_ATTACKING,
    NPC_STATE_FLEEING,
    NPC_STATE_IDLE_ACTIVITY,
    NPC_STATE_CUSTOM,
} NPCState;

/* Task types */
typedef enum {
    TASK_IDLE_ACTIVITY = 0,
    TASK_MOVE_TO,
    TASK_FOLLOW_PLAYER,
    TASK_PATROL,
    TASK_ATTACK,
    TASK_DEFEND,
    TASK_INVESTIGATE,
    TASK_GATHER_RESOURCE,
    TASK_FLEEING,
    TASK_CUSTOM,
} TaskType;

/* Priority levels */
typedef enum {
    PRIORITY_LOW = 0,
    PRIORITY_NORMAL,
    PRIORITY_HIGH,
    PRIORITY_CRITICAL,
} TaskPriority;

/* Decision context */
typedef struct {
    float max_speed;
    float current_health;
    float max_health;
    float vision_distance;
    float hearing_distance;
    bool has_target;
    uint32_t visible_entity_count;
    float time_since_last_action;
} DecisionContext;

/* NPC state information */
typedef struct {
    NPCId npc_id;
    NPCState state;
    TaskType current_task;
    float x, y, z;          /* Position */
    float target_x, target_y, target_z;  /* Target position */
    float health;
    float morale;
    uint32_t perception_score;
    uint32_t skill_level;
} NPCStateInfo;

/* Behavior tree node */
typedef struct BehaviorNode {
    uint32_t node_id;
    uint32_t node_type;    /* Composite, Sequence, Selector, Leaf */
    bool (*evaluate)(struct BehaviorNode* node, DecisionContext* context);
    int (*execute)(struct BehaviorNode* node, NPCStateInfo* npc);
    struct BehaviorNode* children;
    uint32_t child_count;
    void* data;
} BehaviorNode;

/* AI system context */
typedef struct AISystem AISystem;

/* Perception info */
typedef struct {
    uint32_t entity_id;
    float distance;
    float threat_level;
    bool is_ally;
} PerceptionInfo;

/* Interest point */
typedef struct {
    float x, y, z;
    float interest_level;
    uint32_t interest_type;  /* Food, Danger, Ally, etc */
    float decay_rate;
} InterestPoint;

/* AI statistics */
typedef struct {
    uint32_t active_npcs;
    uint32_t total_decisions_made;
    uint32_t avg_decision_time_ms;
    uint32_t pathfinding_calls;
    uint32_t perception_checks;
    double total_ai_time_ms;
} AIStats;

/* ============================================================================
 * API FUNCTIONS
 * ============================================================================ */

/* Create and destroy AI system */
AISystem* ai_system_create(uint32_t max_npcs);
void ai_system_destroy(AISystem* ai);

/* NPC Registration */
NPCId ai_system_register_npc(
    AISystem* ai,
    float initial_x,
    float initial_y,
    float initial_z,
    uint32_t skill_level
);

int ai_system_unregister_npc(AISystem* ai, NPCId npc_id);

/* State and task management */
int ai_system_set_npc_state(AISystem* ai, NPCId npc_id, NPCState state);
NPCState ai_system_get_npc_state(AISystem* ai, NPCId npc_id);

int ai_system_assign_task(
    AISystem* ai,
    NPCId npc_id,
    TaskType task,
    float target_x,
    float target_y,
    float target_z,
    TaskPriority priority
);

int ai_system_update_npc_position(
    AISystem* ai,
    NPCId npc_id,
    float x,
    float y,
    float z
);

int ai_system_update_npc_health(
    AISystem* ai,
    NPCId npc_id,
    float health
);

/* Decision making */
TaskType ai_system_make_decision(
    AISystem* ai,
    NPCId npc_id,
    DecisionContext* context
);

int ai_system_get_npc_state_info(
    AISystem* ai,
    NPCId npc_id,
    NPCStateInfo* info
);

/* Perception system */
int ai_system_add_perception(
    AISystem* ai,
    NPCId npc_id,
    const PerceptionInfo* perception
);

int ai_system_clear_perceptions(AISystem* ai, NPCId npc_id);

int ai_system_get_perceived_entities(
    AISystem* ai,
    NPCId npc_id,
    PerceptionInfo** entities,
    uint32_t* entity_count
);

/* Interest system */
int ai_system_add_interest_point(
    AISystem* ai,
    const InterestPoint* point
);

int ai_system_get_nearby_interests(
    AISystem* ai,
    float search_x,
    float search_y,
    float search_z,
    float search_radius,
    InterestPoint** interests,
    uint32_t* count
);

void ai_system_decay_interests(AISystem* ai, float delta_time);

/* Behavior trees */
BehaviorNode* ai_system_create_behavior_tree(uint32_t node_count);
void ai_system_destroy_behavior_tree(BehaviorNode* root);

int ai_system_execute_behavior_tree(
    BehaviorNode* root,
    NPCStateInfo* npc,
    DecisionContext* context
);

/* Pathfinding integration */
typedef struct {
    float x, y, z;
} PathPoint;

int ai_system_request_path(
    AISystem* ai,
    NPCId npc_id,
    float target_x,
    float target_y,
    float target_z,
    PathPoint** path,
    uint32_t* path_length
);

void ai_system_free_path(PathPoint* path);

/* Group behavior */
int ai_system_add_npc_to_group(
    AISystem* ai,
    NPCId group_id,
    NPCId npc_id
);

int ai_system_remove_npc_from_group(AISystem* ai, NPCId npc_id);

int ai_system_get_group_members(
    AISystem* ai,
    NPCId group_id,
    NPCId** members,
    uint32_t* count
);

/* Spatial reasoning */
typedef struct {
    float influence_value;
    uint32_t grid_x;
    uint32_t grid_y;
} InfluenceMapCell;

int ai_system_get_influence_map(
    AISystem* ai,
    InfluenceMapCell** cells,
    uint32_t* width,
    uint32_t* height
);

void ai_system_update_influence_map(AISystem* ai, float delta_time);

/* System update */
int ai_system_update(AISystem* ai, float delta_time);

/* Configuration */
int ai_system_set_update_rate(AISystem* ai, uint32_t updates_per_second);
int ai_system_set_optimization_level(AISystem* ai, uint32_t level);

/* Statistics */
int ai_system_get_statistics(AISystem* ai, AIStats* stats);
void ai_system_reset_statistics(AISystem* ai);

#ifdef __cplusplus
}
#endif

#endif // AI_SYSTEM_H
