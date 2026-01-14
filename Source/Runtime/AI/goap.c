#include "ai/goap.h"
#include <stdlib.h>
#include <string.h>
#include <stdio.h>
#include <include/math/math_all.h>
#include <math.h>
#include <time.h>

/**
 * =================================================================================================
 *                                   GOAP SYSTEM - COMPLETE
 * =================================================================================================
 */

// Forward declarations
// Forward declarations
void goap_clear_search_data(GoapPlanner* planner);
bool goap_astar_plan(GoapPlanner* planner, GoapAgent* agent, const GoapWorldState* start, const GoapGoal* goal, GoapAction** plan, int* plan_length);
bool goap_world_states_equal(const GoapWorldState* state1, const GoapWorldState* state2);
GoapPlanner* goap_create_planner(void);
void goap_destroy_planner(GoapPlanner* planner);

static float get_current_time(void) {
    return (float)clock() / CLOCKS_PER_SEC;
}

static float heuristic_distance(const GoapWorldState* from, const GoapWorldState* to) {
    if (!from || !to) return 0.0f;
    
    float distance = 0.0f;
    
    // Calculate Manhattan distance between states matching keys
    for (int i = 0; i < from->key_count; i++) {
        const GoapStateKey* key_from = &from->keys[i];
        GoapStateKey key_to;
        
        // Find corresponding key in 'to' state
        bool found = false;
        for (int j = 0; j < to->key_count; j++) {
            if (strcmp(to->keys[j].name, key_from->name) == 0) {
                key_to = to->keys[j];
                found = true;
                break;
            }
        }
        
        if (found && key_from->type == key_to.type) {
             switch (key_from->type) {
                case GOAP_STATE_BOOL:
                    if (key_from->value.bool_val != key_to.value.bool_val) distance += 1.0f;
                    break;
                case GOAP_STATE_INT:
                    distance += abs(key_from->value.int_val - key_to.value.int_val);
                    break;
                case GOAP_STATE_FLOAT:
                    distance += fabsf(key_from->value.float_val - key_to.value.float_val);
                    break;
                case GOAP_STATE_VECTOR3:
                    if (!vec3_is_equal(key_from->value.vector_val, key_to.value.vector_val, 0.001f)) distance += 1.0f;
                    break;
                default:
                    break;
            }
        }
    }
    
    return distance;
}

// =================================================================================================
// CORE SYSTEM FUNCTIONS (TASK_1600-1604)
// =================================================================================================

bool goap_init(GoapPlanner* planner) {
    if (!planner) {
        LOG_ERROR("Invalid planner");
        return false;
    }
    
    memset(planner, 0, sizeof(GoapPlanner));
    planner->initialized = true;
    planner->max_plan_length = 50;
    planner->max_search_time = 0.1f;  // 100ms max planning time
    
    LOG_INFO("GOAP Planner initialized");
    return true;
}

void goap_shutdown(GoapPlanner* planner) {
    if (!planner || !planner->initialized) return;
    
    // Clean up any allocated memory
    if (planner->open_list) {
        struct PlanNode* current = planner->open_list;
        while (current) {
            struct PlanNode* next = current->next;
            free(current);
            current = next;
        }
    }
    
    if (planner->closed_list) {
        struct PlanNode* current = planner->closed_list;
        while (current) {
            struct PlanNode* next = current->next;
            free(current);
            current = next;
        }
    }
    
    memset(planner, 0, sizeof(GoapPlanner));
    LOG_INFO("GOAP Planner shut down");
}



bool goap_validate_planner(const GoapPlanner* planner) {
    if (!planner) return false;
    if (!planner->initialized) return false;
    
    return true;
}

bool goap_validate_agent(const GoapAgent* agent) {
    if (!agent) return false;
    if (agent->name[0] == '\0') return false;
    
    return true;
}

bool goap_add_action(GoapAgent* agent, GoapAction* action) {
    if (!agent || !action || agent->action_count >= GOAP_MAX_ACTIONS) {
        return false;
    }
    
    agent->actions[agent->action_count++] = action;
    LOG_INFO("Added action to agent %s: %s", agent->name, action->name);
    return true;
}

bool goap_add_goal(GoapAgent* agent, GoapGoal* goal) {
    if (!agent || !goal || agent->goal_count >= GOAP_MAX_GOALS) {
        return false;
    }
    
    agent->goals[agent->goal_count++] = goal;
    LOG_INFO("Added goal to agent %s: %s", agent->name, goal->name);
    return true;
}

// World state functions
void goap_world_state_init(GoapWorldState* state) {
    if (!state) {
        return;
    }
    
    memset(state, 0, sizeof(GoapWorldState));
    state->timestamp = 0.0f;
}

void goap_world_state_copy(GoapWorldState* dest, const GoapWorldState* src) {
    if (!dest || !src) {
        return;
    }
    
    memcpy(dest, src, sizeof(GoapWorldState));
}

bool goap_world_state_get(const GoapWorldState* state, const char* key, GoapStateKey* out_key) {
    if (!state || !key || !out_key) {
        return false;
    }
    
    for (int i = 0; i < state->key_count; i++) {
        if (strcmp(state->keys[i].name, key) == 0) {
            *out_key = state->keys[i];
            return true;
        }
    }
    
    return false;
}

bool goap_world_state_set(GoapWorldState* state, const char* key, GoapStateType type, void* value) {
    if (!state || !key || !value || state->key_count >= GOAP_MAX_STATE_KEYS) {
        return false;
    }
    
    // Check if key already exists
    for (int i = 0; i < state->key_count; i++) {
        if (strcmp(state->keys[i].name, key) == 0) {
            // Update existing key
            state->keys[i].type = type;
            switch (type) {
                case GOAP_STATE_BOOL:
                    state->keys[i].value.bool_val = *(bool*)value;
                    break;
                case GOAP_STATE_INT:
                    state->keys[i].value.int_val = *(int*)value;
                    break;
                case GOAP_STATE_FLOAT:
                    state->keys[i].value.float_val = *(float*)value;
                    break;
                case GOAP_STATE_VECTOR3:
                    state->keys[i].value.vector_val = *(Vec3*)value;
                    break;
            }
            return true;
        }
    }
    
    // Add new key
    GoapStateKey* new_key = &state->keys[state->key_count++];
    strncpy(new_key->name, key, sizeof(new_key->name) - 1);
    new_key->type = type;
    
    switch (type) {
        case GOAP_STATE_BOOL:
            new_key->value.bool_val = *(bool*)value;
            break;
        case GOAP_STATE_INT:
            new_key->value.int_val = *(int*)value;
            break;
        case GOAP_STATE_FLOAT:
            new_key->value.float_val = *(float*)value;
            break;
        case GOAP_STATE_VECTOR3:
            new_key->value.vector_val = *(Vec3*)value;
            break;
    }
    
    return true;
}

bool goap_world_state_meets_conditions(const GoapWorldState* state, 
                                      const GoapStateCondition* conditions, int count) {
    if (!state || !conditions || count <= 0) {
        return false;
    }
    
    for (int i = 0; i < count; i++) {
        const GoapStateCondition* condition = &conditions[i];
        GoapStateKey current_key;
        
        if (!goap_world_state_get(state, condition->name, &current_key)) {
            return false;  // Key doesn't exist
        }
        
        // Check if values match
        if (current_key.type != condition->type) {
            return false;  // Type mismatch
        }
        
        switch (condition->type) {
            case GOAP_STATE_BOOL:
                if (current_key.value.bool_val != condition->value.bool_val) {
                    return false;
                }
                break;
            case GOAP_STATE_INT:
                if (current_key.value.int_val != condition->value.int_val) {
                    return false;
                }
                break;
            case GOAP_STATE_FLOAT:
                if (current_key.value.float_val != condition->value.float_val) {
                    return false;
                }
                break;
            case GOAP_STATE_VECTOR3:
                if (!vec3_is_equal(current_key.value.vector_val, condition->value.vector_val, 0.001f)) {
                    return false;
                }
                break;
        }
    }
    
    return true;
}

// Action functions
GoapAction* goap_create_action(const char* name, float cost) {
    GoapAction* action = (GoapAction*)calloc(1, sizeof(GoapAction));
    if (!action) {
        return NULL;
    }
    
    strncpy(action->name, name, sizeof(action->name) - 1);
    action->cost = cost;
    action->duration = 1.0f;
    
    return action;
}

void goap_action_add_precondition(GoapAction* action, const char* key, 
                                  GoapStateType type, void* value) {
    if (!action || !key || !value || action->precondition_count >= GOAP_MAX_STATE_KEYS) {
        return;
    }
    
    GoapStateCondition* precondition = &action->preconditions[action->precondition_count++];
    strncpy(precondition->name, key, sizeof(precondition->name) - 1);
    precondition->type = type;
    
    switch (type) {
        case GOAP_STATE_BOOL:
            precondition->value.bool_val = *(bool*)value;
            break;
        case GOAP_STATE_INT:
            precondition->value.int_val = *(int*)value;
            break;
        case GOAP_STATE_FLOAT:
            precondition->value.float_val = *(float*)value;
            break;
        case GOAP_STATE_VECTOR3:
            precondition->value.vector_val = *(Vec3*)value;
            break;
    }
}

void goap_action_add_effect(GoapAction* action, const char* key, 
                           GoapStateType type, void* value) {
    if (!action || !key || !value || action->effect_count >= GOAP_MAX_STATE_KEYS) {
        return;
    }
    
    GoapStateCondition* effect = &action->effects[action->effect_count++];
    strncpy(effect->name, key, sizeof(effect->name) - 1);
    effect->type = type;
    
    switch (type) {
        case GOAP_STATE_BOOL:
            effect->value.bool_val = *(bool*)value;
            break;
        case GOAP_STATE_INT:
            effect->value.int_val = *(int*)value;
            break;
        case GOAP_STATE_FLOAT:
            effect->value.float_val = *(float*)value;
            break;
        case GOAP_STATE_VECTOR3:
            effect->value.vector_val = *(Vec3*)value;
            break;
    }
}

void goap_action_apply_effects(GoapAction* action, GoapWorldState* state) {
    if (!action || !state) {
        return;
    }
    
    for (int i = 0; i < action->effect_count; i++) {
        GoapStateCondition* effect = &action->effects[i];
        
        switch (effect->type) {
            case GOAP_STATE_BOOL:
                goap_world_state_set(state, effect->name, effect->type, &effect->value.bool_val);
                break;
            case GOAP_STATE_INT:
                goap_world_state_set(state, effect->name, effect->type, &effect->value.int_val);
                break;
            case GOAP_STATE_FLOAT:
                goap_world_state_set(state, effect->name, effect->type, &effect->value.float_val);
                break;
            case GOAP_STATE_VECTOR3:
                goap_world_state_set(state, effect->name, effect->type, &effect->value.vector_val);
                break;
        }
    }
}

bool goap_action_can_execute(const GoapAction* action, const GoapAgent* agent, 
                            const GoapWorldState* state) {
    if (!action || !state) {
        return false;
    }
    
    // Check preconditions
    if (!goap_world_state_meets_conditions(state, action->preconditions, action->precondition_count)) {
        return false;
    }
    
    // Check custom validation if provided
    if (action->can_execute) {
        return action->can_execute((GoapAgent*)agent, state);
    }
    
    return true;
}

// Goal functions
GoapGoal* goap_create_goal(const char* name, float priority) {
    GoapGoal* goal = (GoapGoal*)calloc(1, sizeof(GoapGoal));
    if (!goal) {
        return NULL;
    }
    
    strncpy(goal->name, name, sizeof(goal->name) - 1);
    goal->priority = priority;
    
    return goal;
}

void goap_goal_add_condition(GoapGoal* goal, const char* key, 
                             GoapStateType type, void* value) {
    if (!goal || !key || !value || goal->condition_count >= GOAP_MAX_STATE_KEYS) {
        return;
    }
    
    GoapStateCondition* condition = &goal->conditions[goal->condition_count++];
    strncpy(condition->name, key, sizeof(condition->name) - 1);
    condition->type = type;
    
    switch (type) {
        case GOAP_STATE_BOOL:
            condition->value.bool_val = *(bool*)value;
            break;
        case GOAP_STATE_INT:
            condition->value.int_val = *(int*)value;
            break;
        case GOAP_STATE_FLOAT:
            condition->value.float_val = *(float*)value;
            break;
        case GOAP_STATE_VECTOR3:
            condition->value.vector_val = *(Vec3*)value;
            break;
    }
}

bool goap_goal_is_achieved(const GoapGoal* goal, const GoapWorldState* state) {
    if (!goal || !state) {
        return false;
    }
    
    return goap_world_state_meets_conditions(state, goal->conditions, goal->condition_count);
}

GoapGoal* goap_select_best_goal(GoapAgent* agent) {
    if (!agent || agent->goal_count == 0) {
        return NULL;
    }
    
    GoapGoal* best_goal = NULL;
    float best_priority = -1.0f;
    
    for (int i = 0; i < agent->goal_count; i++) {
        GoapGoal* goal = agent->goals[i];
        
        // Skip achieved goals
        if (goap_goal_is_achieved(goal, &agent->current_state)) {
            continue;
        }
        
        // Check if goal is valid
        if (goal->is_valid && !goal->is_valid(agent, &agent->current_state)) {
            continue;
        }
        
        // Calculate priority
        float priority = goal->priority;
        if (goal->calculate_priority) {
            priority = goal->calculate_priority(agent, &agent->current_state);
        }
        
        if (priority > best_priority) {
            best_priority = priority;
            best_goal = goal;
        }
    }
    
    return best_goal;
}

// Utility functions
float goap_calculate_heuristic(const GoapWorldState* current, const GoapGoal* goal) {
    if (!current || !goal) {
        return 0.0f;
    }
    
    // Simple heuristic: count unmet conditions
    int unmet_conditions = 0;
    for (int i = 0; i < goal->condition_count; i++) {
        const GoapStateCondition* condition = &goal->conditions[i];
        GoapStateKey current_key;
        
        if (!goap_world_state_get(current, condition->name, &current_key)) {
            unmet_conditions++;
            continue;
        }
        
        // Check if values match
        if (current_key.type != condition->type) {
            unmet_conditions++;
            continue;
        }
        
        switch (condition->type) {
            case GOAP_STATE_BOOL:
                if (current_key.value.bool_val != condition->value.bool_val) {
                    unmet_conditions++;
                }
                break;
            case GOAP_STATE_INT:
                if (current_key.value.int_val != condition->value.int_val) {
                    unmet_conditions++;
                }
                break;
            case GOAP_STATE_FLOAT:
                if (current_key.value.float_val != condition->value.float_val) {
                    unmet_conditions++;
                }
                break;
            case GOAP_STATE_VECTOR3:
                if (!vec3_is_equal(current_key.value.vector_val, condition->value.vector_val, 0.001f)) {
                    unmet_conditions++;
                }
                break;
        }
    }
    
    return (float)unmet_conditions;
}

void goap_debug_print_plan(const GoapAction** plan, int length) {
    if (!plan || length <= 0) {
        printf("Plan is empty\n");
        return;
    }
    
    printf("GOAP Plan (%d actions):\n", length);
    for (int i = 0; i < length; i++) {
    }
}

void goap_debug_print_state(const GoapWorldState* state) {
    if (!state) {
        printf("World state is NULL\n");
        return;
    }
    
    printf("World State (%d keys):\n", state->key_count);
    for (int i = 0; i < state->key_count; i++) {
        const GoapStateKey* key = &state->keys[i];
        printf("  %s: ", key->name);
        
        switch (key->type) {
            case GOAP_STATE_BOOL:
                printf("%s", key->value.bool_val ? "true" : "false");
                break;
            case GOAP_STATE_INT:
                printf("%d", key->value.int_val);
                break;
            case GOAP_STATE_FLOAT:
                printf("%.2f", key->value.float_val);
                break;
            case GOAP_STATE_VECTOR3:
                printf("(%.2f, %.2f, %.2f)", 
                       key->value.vector_val.x, key->value.vector_val.y, key->value.vector_val.z);
                break;
        }
        printf("\n");
    }
}

// TASK_1613: Implement "A* Planner" for finding optimal action sequences
bool goap_plan(GoapPlanner* planner, GoapAgent* agent, const GoapWorldState* start, 
               const GoapGoal* goal, GoapAction** plan, int* plan_length) {
    if (!planner || !agent || !start || !goal || !plan || !plan_length) {
        return false;
    }
    
    if (!planner->initialized) {
        LOG_ERROR("GOAP planner not initialized");
        return false;
    }
    
    LOG_INFO("Starting GOAP planning from %d keys to goal %s", start->key_count, goal->name);
    
    // Check if goal is already achieved
    if (goap_goal_is_achieved(goal, start)) {
        *plan_length = 0;
        LOG_INFO("Goal already achieved");
        return true;
    }
    
    // A* planning implementation
    return goap_astar_plan(planner, agent, start, goal, plan, plan_length);
}

bool goap_astar_plan(GoapPlanner* planner, GoapAgent* agent, const GoapWorldState* start, 
                     const GoapGoal* goal, GoapAction** plan, int* plan_length) {
    // Planning node structure
    // Use struct PlanNode from goap.h, do not redefine
    typedef struct PlanNode PlanNode;
    
    // Allocate node pool
    int max_nodes = 1000;  // Reasonable limit
    PlanNode* node_pool = (PlanNode*)calloc(max_nodes, sizeof(PlanNode));
    if (!node_pool) {
        LOG_ERROR("Failed to allocate planning node pool");
        return false;
    }
    
    // Open list (simple array - could be optimized with heap)
    PlanNode** open_list = (PlanNode**)malloc(max_nodes * sizeof(PlanNode*));
    if (!open_list) {
        free(node_pool);
        LOG_ERROR("Failed to allocate open list");
        return false;
    }
    
    int open_count = 0;
    int node_count = 0;
    
    // Create start node
    PlanNode* start_node = &node_pool[node_count++];
    goap_world_state_copy(&start_node->state, start);
    start_node->g_cost = 0.0f;
    start_node->h_cost = goap_calculate_heuristic(start, goal);
    start_node->f_cost = start_node->g_cost + start_node->h_cost;
    start_node->action = NULL;
    start_node->parent = NULL;
    start_node->depth = 0;
    start_node->in_open = true;
    start_node->in_closed = false;
    
    open_list[open_count++] = start_node;
    
    PlanNode* goal_node = NULL;
    int max_iterations = 1000;  // Prevent infinite loops
    int iterations = 0;
    
    while (open_count > 0 && iterations < max_iterations) {
        iterations++;
        
        // Find node with lowest f_cost
        int best_index = 0;
        for (int i = 1; i < open_count; i++) {
            if (open_list[i]->f_cost < open_list[best_index]->f_cost) {
                best_index = i;
            }
        }
        
        PlanNode* current = open_list[best_index];
        
        // Remove from open list
        open_list[best_index] = open_list[--open_count];
        current->in_open = false;
        current->in_closed = true;
        
        // Check if goal is achieved
        if (goap_goal_is_achieved(goal, &current->state)) {
            goal_node = current;
            break;
        }
        
        // Expand node - try all available actions
        for (int i = 0; i < agent->action_count; i++) {
            GoapAction* action = agent->actions[i];
            
            // Check if action can be executed
            if (!goap_action_can_execute(action, agent, &current->state)) {
                continue;
            }
            
            // Apply action effects to create new state
            GoapWorldState new_state;
            goap_world_state_copy(&new_state, &current->state);
            goap_action_apply_effects(action, &new_state);
            
            // Check if we've already visited this state (simplified check)
            bool already_visited = false;
            for (int j = 0; j < node_count; j++) {
                if (goap_world_states_equal(&node_pool[j].state, &new_state)) {
                    already_visited = true;
                    break;
                }
            }
            
            if (already_visited) {
                continue;
            }
            
            // Create new node
            if (node_count >= max_nodes) {
                LOG_WARN("Reached maximum planning nodes");
                break;
            }
            
            PlanNode* new_node = &node_pool[node_count++];
            goap_world_state_copy(&new_node->state, &new_state);
            new_node->g_cost = current->g_cost + action->cost;
            new_node->h_cost = goap_calculate_heuristic(&new_state, goal);
            new_node->f_cost = new_node->g_cost + new_node->h_cost;
            new_node->action = action;
            new_node->parent = current;
            new_node->depth = current->depth + 1;
            new_node->in_open = true;
            new_node->in_closed = false;
            
            // Add to open list
            if (open_count < max_nodes) {
                open_list[open_count++] = new_node;
            }
        }
    }
    
    bool success = false;
    
    if (goal_node) {
        // Reconstruct plan
        int plan_steps = goal_node->depth;
        if (plan_steps <= GOAP_MAX_PLAN_LENGTH) {
            *plan_length = plan_steps;
            
            // Build plan in reverse order
            PlanNode* current = goal_node;
            for (int i = plan_steps - 1; i >= 0; i--) {
                plan[i] = current->action;
                current = current->parent;
            }
            
            success = true;
            LOG_INFO("GOAP planning successful: %d steps, cost %.2f", plan_steps, goal_node->g_cost);
        } else {
            LOG_ERROR("Plan too long: %d steps > %d", plan_steps, GOAP_MAX_PLAN_LENGTH);
        }
    } else {
        LOG_ERROR("GOAP planning failed: no solution found after %d iterations", iterations);
    }
    
    // Cleanup
    free(open_list);
    free(node_pool);
    
    planner->nodes_expanded = iterations;
    planner->plans_generated++;
    
    return success;
}

bool goap_world_states_equal(const GoapWorldState* state1, const GoapWorldState* state2) {
    if (!state1 || !state2) {
        return false;
    }
    
    if (state1->key_count != state2->key_count) {
        return false;
    }
    
    // Check all keys in state1 exist in state2 with same values
    for (int i = 0; i < state1->key_count; i++) {
        const GoapStateKey* key1 = &state1->keys[i];
        GoapStateKey key2;
        
        if (!goap_world_state_get(state2, key1->name, &key2)) {
            return false;  // Key not found in state2
        }
        
        if (key1->type != key2.type) {
            return false;  // Type mismatch
        }
        
        // Compare values based on type
        switch (key1->type) {
            case GOAP_STATE_BOOL:
                if (key1->value.bool_val != key2.value.bool_val) return false;
                break;
            case GOAP_STATE_INT:
                if (key1->value.int_val != key2.value.int_val) return false;
                break;
            case GOAP_STATE_FLOAT:
                if (fabsf(key1->value.float_val - key2.value.float_val) > 0.001f) return false;
                break;
            case GOAP_STATE_VECTOR3:
                if (!vec3_is_equal(key1->value.vector_val, key2.value.vector_val, 0.001f)) return false;
                break;
        }
    }
    
    return true;
}

// TASK_1611: Add "Dynamic Goal Selection" based on world state
bool goap_replan_if_needed(GoapPlanner* planner, GoapAgent* agent) {
    if (!planner || !agent) {
        return false;
    }
    
    // Check if replanning is needed
    float current_time = 0.0f;  // Would get actual time
    if (current_time - agent->last_plan_time < agent->replan_interval) {
        return false;  // Not time to replan yet
    }
    
    // Select best goal
    GoapGoal* best_goal = goap_select_best_goal(agent);
    if (!best_goal) {
        LOG_INFO("No valid goals found");
        return false;
    }
    
    // Check if current plan is still valid
    if (agent->plan_length > 0) {
        // Check if current goal is still the best and still relevant
        if (best_goal->calculate_priority) {
            float current_priority = best_goal->calculate_priority(agent, &agent->current_state);
            if (current_priority < 0.5f) {  // Priority threshold
                LOG_INFO("Goal priority too low, replanning");
            } else {
                return false;  // Current plan is still good
            }
        }
    }
    
    // Plan to achieve best goal
    GoapAction* new_plan[GOAP_MAX_PLAN_LENGTH];
    int new_plan_length = 0;
    
    bool success = goap_plan(planner, agent, &agent->current_state, best_goal, new_plan, &new_plan_length);
    
    if (success) {
        // Update agent's plan
        agent->plan_length = new_plan_length;
        agent->current_action_index = 0;
        agent->needs_replan = false;
        agent->last_plan_time = current_time;
        
        for (int i = 0; i < new_plan_length; i++) {
            agent->current_plan[i] = new_plan[i];
        }
        
        LOG_INFO("Replanning successful: %d steps for goal %s", new_plan_length, best_goal->name);
    } else {
        LOG_ERROR("Replanning failed for goal %s", best_goal->name);
        agent->needs_replan = true;
    }
    
    return success;
}

// TASK_1612: Implement "Plan Re-evaluation" when world changes
void goap_update_agent(GoapAgent* agent, float delta_time) {
    if (!agent) {
        return;
    }
    
    // Execute current action if we have a plan
    if (agent->plan_length > 0 && agent->current_action_index < agent->plan_length) {
        GoapAction* current_action = agent->current_plan[agent->current_action_index];
        
        if (goap_execute_current_action(agent, delta_time)) {
            // Action completed, move to next
            agent->current_action_index++;
            
            if (agent->current_action_index >= agent->plan_length) {
                // Plan completed
                agent->plan_length = 0;
                agent->current_action_index = 0;
                agent->needs_replan = true;
                LOG_INFO("GOAP plan completed");
            }
        }
    } else if (agent->needs_replan) {
        // Need to create a new plan
        GoapPlanner* planner = goap_create_planner();
        if (planner) {
            goap_replan_if_needed(planner, agent);
            goap_destroy_planner(planner);
        }
    }
}

bool goap_execute_current_action(GoapAgent* agent, float delta_time) {
    if (!agent || agent->current_action_index >= agent->plan_length) {
        return false;
    }
    
    GoapAction* action = agent->current_plan[agent->current_action_index];
    
    // Check if action can still be executed
    if (!goap_action_can_execute(action, agent, &agent->current_state)) {
        LOG_WARN("Action %s can no longer be executed, replanning needed", action->name);
        agent->needs_replan = true;
        return true;  // Consider this action "completed" so we move to replanning
    }
    
    // Execute action
    if (action->execute) {
        action->execute(agent, &agent->current_state);
    }
    
    // Apply action effects
    goap_action_apply_effects(action, &agent->current_state);
    
    // Check if action is complete
    if (action->is_complete) {
        return action->is_complete(agent);
    }
    
    // For actions without completion check, assume they complete in one frame
    return true;
}

GoapPlanner* goap_create_planner(void) {
    GoapPlanner* planner = (GoapPlanner*)calloc(1, sizeof(GoapPlanner));
    if (!planner) {
        LOG_ERROR("Failed to allocate GOAP planner");
        return NULL;
    }
    
    if (!goap_init(planner)) {
        free(planner);
        return NULL;
    }
    
    return planner;
}

void goap_destroy_planner(GoapPlanner* planner) {
    if (!planner) {
        return;
    }
    
    goap_shutdown(planner);
}

// Function declarations for missing implementations
bool goap_astar_plan(GoapPlanner* planner, GoapAgent* agent, const GoapWorldState* start, 
                     const GoapGoal* goal, GoapAction** plan, int* plan_length);

// Implementations of removed legacy functions using new logic

void goap_clear_search_data(GoapPlanner* planner) {
    if (!planner) return;
    
    // Free open list
    struct PlanNode* current = planner->open_list;
    while (current) {
        struct PlanNode* next = current->next;
        free(current);
        current = next;
    }
    planner->open_list = NULL;
    
    // Free closed list
    current = planner->closed_list;
    while (current) {
        struct PlanNode* next = current->next;
        free(current);
        current = next;
    }
    planner->closed_list = NULL;
}

GoapAgent* goap_create_agent(uint32_t id, const char* name) {
    GoapAgent* agent = (GoapAgent*)calloc(1, sizeof(GoapAgent));
    if (!agent) {
        LOG_ERROR("Failed to allocate agent");
        return NULL;
    }
    
    agent->id = id;
    strncpy(agent->name, name, sizeof(agent->name) - 1);
    
    // Initialize world state
    goap_world_state_init(&agent->current_state);
    
    // Set default replan interval
    agent->replan_interval = 1.0f;  // Replan every second
    agent->needs_replan = true;
    
    LOG_INFO("Created GOAP agent: %s (ID: %u)", name, id);
    return agent;
}

void goap_destroy_agent(GoapAgent* agent) {
    if (!agent) return;
    
    // Actions are pointers, owned by creator usually, but if we need to clear logic:
    // Simple free agent
    free(agent);
    LOG_INFO("Destroyed GOAP agent: %s", agent->name);
}

