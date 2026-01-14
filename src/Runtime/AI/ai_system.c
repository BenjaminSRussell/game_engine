/*
 * ai_system.c
 *
 * Advanced AI system implementation
 * Part of the Advanced 3D Rendering Engine
 *
 * Copyright (c) 2024 Game Engine Project
 * Licensed under MIT License
 */

#include "ai_system.h"
#include <stdlib.h>
#include <string.h>
#include <math.h>
#include <time.h>
#include <pthread.h>

/* ============================================================================
 * CONSTANTS
 * ============================================================================ */

#define AI_SYSTEM_MAX_NPCS 1024
#define AI_SYSTEM_MAX_PERCEPTIONS 512
#define AI_SYSTEM_MAX_INTERESTS 256
#define AI_SYSTEM_MAX_GROUPS 64
#define INFLUENCE_MAP_SIZE 32

/* ============================================================================
 * INTERNAL STRUCTURES
 * ============================================================================ */

typedef struct {
    NPCId npc_id;
    bool active;
    NPCState state;
    TaskType current_task;
    float x, y, z;
    float target_x, target_y, target_z;
    float health, max_health;
    float morale;
    uint32_t skill_level;
    uint32_t group_id;

    /* Perception data */
    PerceptionInfo* perceptions;
    uint32_t perception_count;
    uint32_t max_perceptions;

    /* Path following */
    PathPoint* current_path;
    uint32_t path_length;
    uint32_t path_index;

    /* Task priority */
    TaskPriority current_priority;

    /* Timing */
    float time_since_task_start;
} NPC;

typedef struct {
    uint32_t group_id;
    NPCId* members;
    uint32_t member_count;
    uint32_t max_members;
} NPCGroup;

typedef struct AISystem {
    NPC* npcs;
    uint32_t npc_count;
    uint32_t max_npcs;

    NPCGroup* groups;
    uint32_t group_count;

    /* Interest points */
    InterestPoint* interests;
    uint32_t interest_count;
    uint32_t max_interests;

    /* Influence map */
    InfluenceMapCell influence_map[INFLUENCE_MAP_SIZE * INFLUENCE_MAP_SIZE];

    /* Configuration */
    uint32_t update_rate;
    uint32_t optimization_level;

    /* Statistics */
    AIStats stats;

    pthread_mutex_t lock;
} AISystem;

/* ============================================================================
 * UTILITY FUNCTIONS
 * ============================================================================ */

static float distance_3d(float x1, float y1, float z1, float x2, float y2, float z2) {
    float dx = x2 - x1;
    float dy = y2 - y1;
    float dz = z2 - z1;
    return sqrtf(dx * dx + dy * dy + dz * dz);
}

static NPC* find_npc(AISystem* ai, NPCId npc_id) {
    if (!ai) {
        return NULL;
    }

    for (uint32_t i = 0; i < ai->npc_count; i++) {
        if (ai->npcs[i].npc_id == npc_id && ai->npcs[i].active) {
            return &ai->npcs[i];
        }
    }

    return NULL;
}

static NPCGroup* find_group(AISystem* ai, uint32_t group_id) {
    if (!ai) {
        return NULL;
    }

    for (uint32_t i = 0; i < ai->group_count; i++) {
        if (ai->groups[i].group_id == group_id) {
            return &ai->groups[i];
        }
    }

    return NULL;
}

/* ============================================================================
 * DECISION MAKING SYSTEM
 * ============================================================================ */

static TaskType evaluate_situation(AISystem* ai, NPC* npc, DecisionContext* context) {
    if (!npc || !context) {
        return TASK_IDLE_ACTIVITY;
    }

    /* Health-based decisions */
    float health_ratio = context->current_health / context->max_speed;  /* Simplified */
    if (health_ratio < 0.3f) {
        return TASK_FLEEING;
    }

    /* Perception-based decisions */
    if (context->has_target) {
        /* Check threat level of visible entities */
        float max_threat = 0.0f;
        for (uint32_t i = 0; i < npc->perception_count; i++) {
            if (npc->perceptions[i].threat_level > max_threat) {
                max_threat = npc->perceptions[i].threat_level;
            }
        }

        if (max_threat > 0.7f) {
            return TASK_ATTACK;
        } else if (max_threat > 0.3f) {
            return TASK_INVESTIGATE;
        }
    }

    /* Morale and behavior */
    if (npc->morale > 0.8f && context->time_since_last_action > 10.0f) {
        return TASK_PATROL;
    }

    return TASK_IDLE_ACTIVITY;
}

/* ============================================================================
 * PERCEPTION SYSTEM
 * ============================================================================ */

static void update_perception_data(AISystem* ai, NPC* npc) {
    if (!npc || npc->perception_count == 0) {
        return;
    }

    /* Decay perception over time */
    for (uint32_t i = 0; i < npc->perception_count; i++) {
        npc->perceptions[i].threat_level *= 0.95f;  /* Decay */
    }

    /* Remove dead perceptions */
    uint32_t write_idx = 0;
    for (uint32_t i = 0; i < npc->perception_count; i++) {
        if (npc->perceptions[i].threat_level > 0.01f) {
            if (write_idx != i) {
                npc->perceptions[write_idx] = npc->perceptions[i];
            }
            write_idx++;
        }
    }

    npc->perception_count = write_idx;
}

/* ============================================================================
 * PUBLIC API IMPLEMENTATION
 * ============================================================================ */

AISystem* ai_system_create(uint32_t max_npcs) {
    if (max_npcs == 0) {
        max_npcs = AI_SYSTEM_MAX_NPCS;
    }

    AISystem* ai = (AISystem*)malloc(sizeof(AISystem));
    if (!ai) {
        return NULL;
    }

    memset(ai, 0, sizeof(AISystem));

    ai->npcs = (NPC*)calloc(max_npcs, sizeof(NPC));
    if (!ai->npcs) {
        free(ai);
        return NULL;
    }

    ai->max_npcs = max_npcs;
    ai->groups = (NPCGroup*)calloc(AI_SYSTEM_MAX_GROUPS, sizeof(NPCGroup));
    if (!ai->groups) {
        free(ai->npcs);
        free(ai);
        return NULL;
    }

    ai->interests = (InterestPoint*)calloc(AI_SYSTEM_MAX_INTERESTS, sizeof(InterestPoint));
    if (!ai->interests) {
        free(ai->npcs);
        free(ai->groups);
        free(ai);
        return NULL;
    }

    ai->update_rate = 20;  /* 20 updates per second */
    ai->optimization_level = 2;

    pthread_mutex_init(&ai->lock, NULL);

    return ai;
}

void ai_system_destroy(AISystem* ai) {
    if (!ai) {
        return;
    }

    for (uint32_t i = 0; i < ai->npc_count; i++) {
        if (ai->npcs[i].perceptions) {
            free(ai->npcs[i].perceptions);
        }
        if (ai->npcs[i].current_path) {
            free(ai->npcs[i].current_path);
        }
    }

    for (uint32_t i = 0; i < ai->group_count; i++) {
        if (ai->groups[i].members) {
            free(ai->groups[i].members);
        }
    }

    pthread_mutex_destroy(&ai->lock);
    free(ai->npcs);
    free(ai->groups);
    free(ai->interests);
    free(ai);
}

NPCId ai_system_register_npc(
    AISystem* ai,
    float initial_x,
    float initial_y,
    float initial_z,
    uint32_t skill_level) {

    if (!ai || ai->npc_count >= ai->max_npcs) {
        return 0;
    }

    pthread_mutex_lock(&ai->lock);

    NPC* npc = &ai->npcs[ai->npc_count];
    npc->npc_id = ai->npc_count + 1;
    npc->active = true;
    npc->state = NPC_STATE_IDLE;
    npc->current_task = TASK_IDLE_ACTIVITY;
    npc->x = initial_x;
    npc->y = initial_y;
    npc->z = initial_z;
    npc->health = 100.0f;
    npc->max_health = 100.0f;
    npc->morale = 1.0f;
    npc->skill_level = skill_level;
    npc->group_id = 0;

    /* Allocate perception buffer */
    npc->max_perceptions = AI_SYSTEM_MAX_PERCEPTIONS;
    npc->perceptions = (PerceptionInfo*)calloc(npc->max_perceptions, sizeof(PerceptionInfo));

    NPCId npc_id = npc->npc_id;
    ai->npc_count++;

    pthread_mutex_unlock(&ai->lock);

    return npc_id;
}

int ai_system_unregister_npc(AISystem* ai, NPCId npc_id) {
    if (!ai) {
        return -1;
    }

    pthread_mutex_lock(&ai->lock);

    NPC* npc = find_npc(ai, npc_id);
    if (!npc) {
        pthread_mutex_unlock(&ai->lock);
        return -2;
    }

    npc->active = false;

    if (npc->perceptions) {
        free(npc->perceptions);
        npc->perceptions = NULL;
    }

    if (npc->current_path) {
        free(npc->current_path);
        npc->current_path = NULL;
    }

    pthread_mutex_unlock(&ai->lock);

    return 0;
}

int ai_system_set_npc_state(AISystem* ai, NPCId npc_id, NPCState state) {
    if (!ai) {
        return -1;
    }

    NPC* npc = find_npc(ai, npc_id);
    if (!npc) {
        return -2;
    }

    npc->state = state;
    return 0;
}

NPCState ai_system_get_npc_state(AISystem* ai, NPCId npc_id) {
    NPC* npc = find_npc(ai, npc_id);
    if (!npc) {
        return NPC_STATE_IDLE;
    }

    return npc->state;
}

int ai_system_assign_task(
    AISystem* ai,
    NPCId npc_id,
    TaskType task,
    float target_x,
    float target_y,
    float target_z,
    TaskPriority priority) {

    if (!ai) {
        return -1;
    }

    NPC* npc = find_npc(ai, npc_id);
    if (!npc) {
        return -2;
    }

    npc->current_task = task;
    npc->target_x = target_x;
    npc->target_y = target_y;
    npc->target_z = target_z;
    npc->current_priority = priority;
    npc->time_since_task_start = 0.0f;

    return 0;
}

int ai_system_update_npc_position(
    AISystem* ai,
    NPCId npc_id,
    float x,
    float y,
    float z) {

    if (!ai) {
        return -1;
    }

    NPC* npc = find_npc(ai, npc_id);
    if (!npc) {
        return -2;
    }

    npc->x = x;
    npc->y = y;
    npc->z = z;

    return 0;
}

int ai_system_update_npc_health(
    AISystem* ai,
    NPCId npc_id,
    float health) {

    if (!ai) {
        return -1;
    }

    NPC* npc = find_npc(ai, npc_id);
    if (!npc) {
        return -2;
    }

    npc->health = fmaxf(0.0f, fminf(health, npc->max_health));

    /* Adjust morale based on health */
    npc->morale = npc->health / npc->max_health;

    return 0;
}

TaskType ai_system_make_decision(
    AISystem* ai,
    NPCId npc_id,
    DecisionContext* context) {

    if (!ai || !context) {
        return TASK_IDLE_ACTIVITY;
    }

    NPC* npc = find_npc(ai, npc_id);
    if (!npc) {
        return TASK_IDLE_ACTIVITY;
    }

    /* Update perception data */
    update_perception_data(ai, npc);

    /* Make decision based on situation */
    TaskType decision = evaluate_situation(ai, npc, context);

    ai->stats.total_decisions_made++;

    return decision;
}

int ai_system_get_npc_state_info(
    AISystem* ai,
    NPCId npc_id,
    NPCStateInfo* info) {

    if (!ai || !info) {
        return -1;
    }

    NPC* npc = find_npc(ai, npc_id);
    if (!npc) {
        return -2;
    }

    info->npc_id = npc_id;
    info->state = npc->state;
    info->current_task = npc->current_task;
    info->x = npc->x;
    info->y = npc->y;
    info->z = npc->z;
    info->target_x = npc->target_x;
    info->target_y = npc->target_y;
    info->target_z = npc->target_z;
    info->health = npc->health;
    info->morale = npc->morale;
    info->perception_score = npc->perception_count;
    info->skill_level = npc->skill_level;

    return 0;
}

int ai_system_add_perception(
    AISystem* ai,
    NPCId npc_id,
    const PerceptionInfo* perception) {

    if (!ai || !perception) {
        return -1;
    }

    NPC* npc = find_npc(ai, npc_id);
    if (!npc || npc->perception_count >= npc->max_perceptions) {
        return -2;
    }

    npc->perceptions[npc->perception_count] = *perception;
    npc->perception_count++;

    return 0;
}

int ai_system_clear_perceptions(AISystem* ai, NPCId npc_id) {
    if (!ai) {
        return -1;
    }

    NPC* npc = find_npc(ai, npc_id);
    if (!npc) {
        return -2;
    }

    npc->perception_count = 0;
    return 0;
}

int ai_system_get_perceived_entities(
    AISystem* ai,
    NPCId npc_id,
    PerceptionInfo** entities,
    uint32_t* entity_count) {

    if (!ai || !entities || !entity_count) {
        return -1;
    }

    NPC* npc = find_npc(ai, npc_id);
    if (!npc) {
        return -2;
    }

    *entities = npc->perceptions;
    *entity_count = npc->perception_count;

    return 0;
}

int ai_system_add_interest_point(
    AISystem* ai,
    const InterestPoint* point) {

    if (!ai || !point || ai->interest_count >= ai->max_interests) {
        return -1;
    }

    ai->interests[ai->interest_count] = *point;
    ai->interest_count++;

    return 0;
}

int ai_system_get_nearby_interests(
    AISystem* ai,
    float search_x,
    float search_y,
    float search_z,
    float search_radius,
    InterestPoint** interests,
    uint32_t* count) {

    if (!ai || !interests || !count) {
        return -1;
    }

    InterestPoint* nearby = (InterestPoint*)malloc(
        ai->interest_count * sizeof(InterestPoint)
    );
    if (!nearby) {
        return -2;
    }

    uint32_t found = 0;
    for (uint32_t i = 0; i < ai->interest_count; i++) {
        float dist = distance_3d(
            search_x, search_y, search_z,
            ai->interests[i].x, ai->interests[i].y, ai->interests[i].z
        );

        if (dist < search_radius) {
            nearby[found] = ai->interests[i];
            found++;
        }
    }

    *interests = nearby;
    *count = found;

    return 0;
}

void ai_system_decay_interests(AISystem* ai, float delta_time) {
    if (!ai) {
        return;
    }

    uint32_t write_idx = 0;
    for (uint32_t i = 0; i < ai->interest_count; i++) {
        ai->interests[i].interest_level -= ai->interests[i].decay_rate * delta_time;

        if (ai->interests[i].interest_level > 0.01f) {
            if (write_idx != i) {
                ai->interests[write_idx] = ai->interests[i];
            }
            write_idx++;
        }
    }

    ai->interest_count = write_idx;
}

BehaviorNode* ai_system_create_behavior_tree(uint32_t node_count) {
    if (node_count == 0) {
        return NULL;
    }

    BehaviorNode* root = (BehaviorNode*)calloc(node_count, sizeof(BehaviorNode));
    if (!root) {
        return NULL;
    }

    return root;
}

void ai_system_destroy_behavior_tree(BehaviorNode* root) {
    if (!root) {
        return;
    }

    free(root);
}

int ai_system_execute_behavior_tree(
    BehaviorNode* root,
    NPCStateInfo* npc,
    DecisionContext* context) {

    if (!root || !npc || !context) {
        return -1;
    }

    /* Simplified behavior tree execution */
    if (root->evaluate && root->evaluate(root, context)) {
        if (root->execute) {
            return root->execute(root, npc);
        }
    }

    return 0;
}

int ai_system_request_path(
    AISystem* ai,
    NPCId npc_id,
    float target_x,
    float target_y,
    float target_z,
    PathPoint** path,
    uint32_t* path_length) {

    if (!ai || !path || !path_length) {
        return -1;
    }

    /* Simplified pathfinding - create straight line path */
    *path_length = 3;
    *path = (PathPoint*)malloc(3 * sizeof(PathPoint));

    if (!*path) {
        return -2;
    }

    NPC* npc = find_npc(ai, npc_id);
    if (!npc) {
        free(*path);
        return -3;
    }

    (*path)[0].x = npc->x;
    (*path)[0].y = npc->y;
    (*path)[0].z = npc->z;

    (*path)[1].x = (npc->x + target_x) / 2;
    (*path)[1].y = (npc->y + target_y) / 2;
    (*path)[1].z = (npc->z + target_z) / 2;

    (*path)[2].x = target_x;
    (*path)[2].y = target_y;
    (*path)[2].z = target_z;

    ai->stats.pathfinding_calls++;

    return 0;
}

void ai_system_free_path(PathPoint* path) {
    if (path) {
        free(path);
    }
}

int ai_system_add_npc_to_group(
    AISystem* ai,
    NPCId group_id,
    NPCId npc_id) {

    if (!ai) {
        return -1;
    }

    NPC* npc = find_npc(ai, npc_id);
    if (!npc) {
        return -2;
    }

    NPCGroup* group = find_group(ai, group_id);
    if (!group) {
        if (ai->group_count >= AI_SYSTEM_MAX_GROUPS) {
            return -3;
        }

        group = &ai->groups[ai->group_count];
        group->group_id = group_id;
        group->max_members = 32;
        group->members = (NPCId*)malloc(32 * sizeof(NPCId));
        ai->group_count++;
    }

    if (group->member_count < group->max_members) {
        group->members[group->member_count] = npc_id;
        group->member_count++;
        npc->group_id = group_id;
        return 0;
    }

    return -4;
}

int ai_system_remove_npc_from_group(AISystem* ai, NPCId npc_id) {
    if (!ai) {
        return -1;
    }

    NPC* npc = find_npc(ai, npc_id);
    if (!npc) {
        return -2;
    }

    npc->group_id = 0;
    return 0;
}

int ai_system_get_group_members(
    AISystem* ai,
    NPCId group_id,
    NPCId** members,
    uint32_t* count) {

    if (!ai || !members || !count) {
        return -1;
    }

    NPCGroup* group = find_group(ai, group_id);
    if (!group) {
        return -2;
    }

    *members = group->members;
    *count = group->member_count;

    return 0;
}

int ai_system_get_influence_map(
    AISystem* ai,
    InfluenceMapCell** cells,
    uint32_t* width,
    uint32_t* height) {

    if (!ai || !cells || !width || !height) {
        return -1;
    }

    *cells = ai->influence_map;
    *width = INFLUENCE_MAP_SIZE;
    *height = INFLUENCE_MAP_SIZE;

    return 0;
}

void ai_system_update_influence_map(AISystem* ai, float delta_time) {
    if (!ai) {
        return;
    }

    /* Simple influence map update - decay existing values */
    for (uint32_t i = 0; i < INFLUENCE_MAP_SIZE * INFLUENCE_MAP_SIZE; i++) {
        ai->influence_map[i].influence_value *= 0.95f;
    }
}

int ai_system_update(AISystem* ai, float delta_time) {
    if (!ai) {
        return -1;
    }

    pthread_mutex_lock(&ai->lock);

    /* Update all active NPCs */
    for (uint32_t i = 0; i < ai->npc_count; i++) {
        if (ai->npcs[i].active) {
            ai->npcs[i].time_since_task_start += delta_time;
        }
    }

    /* Decay interests */
    ai_system_decay_interests(ai, delta_time);

    /* Update influence map */
    ai_system_update_influence_map(ai, delta_time);

    ai->stats.active_npcs = ai->npc_count;

    pthread_mutex_unlock(&ai->lock);

    return 0;
}

int ai_system_set_update_rate(AISystem* ai, uint32_t updates_per_second) {
    if (!ai || updates_per_second == 0) {
        return -1;
    }

    ai->update_rate = updates_per_second;
    return 0;
}

int ai_system_set_optimization_level(AISystem* ai, uint32_t level) {
    if (!ai || level > 3) {
        return -1;
    }

    ai->optimization_level = level;
    return 0;
}

int ai_system_get_statistics(AISystem* ai, AIStats* stats) {
    if (!ai || !stats) {
        return -1;
    }

    memcpy(stats, &ai->stats, sizeof(AIStats));
    return 0;
}

void ai_system_reset_statistics(AISystem* ai) {
    if (!ai) {
        return;
    }

    memset(&ai->stats, 0, sizeof(AIStats));
}

/* End of ai_system.c */
