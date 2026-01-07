#include "ai/goap.h"
#include <stdlib.h>
#include <string.h>
#include <stdio.h>
#include <math.h>

/**
 * =================================================================================================
 *                                   GOAP SYSTEM - COMPLETE
 * =================================================================================================
 */

// Helper functions
static float get_current_time(void) {
    return (float)clock() / CLOCKS_PER_SEC;
}

static float heuristic_distance(const GoapWorldState* from, const GoapWorldState* to) {
    if (!from || !to) return 0.0f;
    
    float distance = 0.0f;
    
    // Calculate Manhattan distance between states
    for (int i = 0; i < GOAP_MAX_STATE_VARS; i++) {
        if (from->variables[i].type != GOAP_VAR_NONE && to->variables[i].type != GOAP_VAR_NONE) {
            if (from->variables[i].type == to->variables[i].type) {
                switch (from->variables[i].type) {
                    case GOAP_VAR_BOOL:
                        if (from->variables[i].bool_value != to->variables[i].bool_value) {
                            distance += 1.0f;
                        }
                        break;
                    case GOAP_VAR_INT:
                        distance += abs(from->variables[i].int_value - to->variables[i].int_value);
                        break;
                    case GOAP_VAR_FLOAT:
                        distance += fabsf(from->variables[i].float_value - to->variables[i].float_value);
                        break;
                    default:
                        break;
                }
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

// =================================================================================================
// WORLD STATE MANAGEMENT (TASK_1600)
// =================================================================================================

void goap_world_state_init(GoapWorldState* state) {
    if (!state) return;
    
    memset(state, 0, sizeof(GoapWorldState));
    
    // Initialize all variables as undefined
    for (int i = 0; i < GOAP_MAX_STATE_VARS; i++) {
        state->variables[i].type = GOAP_VAR_NONE;
    }
}

bool goap_set_state_bool(GoapWorldState* state, const char* key, bool value) {
    if (!state || !key) return false;
    
    // Find existing key or empty slot
    int index = -1;
    for (int i = 0; i < GOAP_MAX_STATE_VARS; i++) {
        if (state->variables[i].type == GOAP_VAR_NONE) {
            if (index == -1) index = i;
        } else if (strcmp(state->variables[i].key, key) == 0) {
            index = i;
            break;
        }
    }
    
    if (index == -1 || index >= GOAP_MAX_STATE_VARS) return false;
    
    strncpy(state->variables[index].key, key, sizeof(state->variables[index].key) - 1);
    state->variables[index].type = GOAP_VAR_BOOL;
    state->variables[index].bool_value = value;
    
    return true;
}

bool goap_set_state_int(GoapWorldState* state, const char* key, int value) {
    if (!state || !key) return false;
    
    int index = -1;
    for (int i = 0; i < GOAP_MAX_STATE_VARS; i++) {
        if (state->variables[i].type == GOAP_VAR_NONE) {
            if (index == -1) index = i;
        } else if (strcmp(state->variables[i].key, key) == 0) {
            index = i;
            break;
        }
    }
    
    if (index == -1 || index >= GOAP_MAX_STATE_VARS) return false;
    
    strncpy(state->variables[index].key, key, sizeof(state->variables[index].key) - 1);
    state->variables[index].type = GOAP_VAR_INT;
    state->variables[index].int_value = value;
    
    return true;
}

bool goap_set_state_float(GoapWorldState* state, const char* key, float value) {
    if (!state || !key) return false;
    
    int index = -1;
    for (int i = 0; i < GOAP_MAX_STATE_VARS; i++) {
        if (state->variables[i].type == GOAP_VAR_NONE) {
            if (index == -1) index = i;
        } else if (strcmp(state->variables[i].key, key) == 0) {
            index = i;
            break;
        }
    }
    
    if (index == -1 || index >= GOAP_MAX_STATE_VARS) return false;
    
    strncpy(state->variables[index].key, key, sizeof(state->variables[index].key) - 1);
    state->variables[index].type = GOAP_VAR_FLOAT;
    state->variables[index].float_value = value;
    
    return true;
}

bool goap_get_state_bool(const GoapWorldState* state, const char* key, bool* value) {
    if (!state || !key || !value) return false;
    
    for (int i = 0; i < GOAP_MAX_STATE_VARS; i++) {
        if (state->variables[i].type == GOAP_VAR_BOOL && strcmp(state->variables[i].key, key) == 0) {
            *value = state->variables[i].bool_value;
            return true;
        }
    }
    
    return false;
}

bool goap_get_state_int(const GoapWorldState* state, const char* key, int* value) {
    if (!state || !key || !value) return false;
    
    for (int i = 0; i < GOAP_MAX_STATE_VARS; i++) {
        if (state->variables[i].type == GOAP_VAR_INT && strcmp(state->variables[i].key, key) == 0) {
            *value = state->variables[i].int_value;
            return true;
        }
    }
    
    return false;
}

bool goap_get_state_float(const GoapWorldState* state, const char* key, float* value) {
    if (!state || !key || !value) return false;
    
    for (int i = 0; i < GOAP_MAX_STATE_VARS; i++) {
        if (state->variables[i].type == GOAP_VAR_FLOAT && strcmp(state->variables[i].key, key) == 0) {
            *value = state->variables[i].float_value;
            return true;
        }
    }
    
    return false;
}

// =================================================================================================
// ACTION SYSTEM (TASK_1601)
// =================================================================================================

GoapAction* goap_create_action(const char* name, float cost) {
    if (!name) {
        LOG_ERROR("Action name cannot be NULL");
        return NULL;
    }
    
    GoapAction* action = (GoapAction*)calloc(1, sizeof(GoapAction));
    if (!action) {
        LOG_ERROR("Failed to allocate GOAP action");
        return NULL;
    }
    
    strncpy(action->name, name, sizeof(action->name) - 1);
    action->cost = cost;
    action->duration = 1.0f;  // Default 1 second duration
    
    LOG_DEBUG("Created GOAP action: %s", name);
    return action;
}

void goap_destroy_action(GoapAction* action) {
    if (!action) return;
    
    free(action);
    LOG_DEBUG("Destroyed GOAP action: %s", action->name);
}

bool goap_add_precondition(GoapAction* action, const char* key, GoapVarType type, void* value) {
    if (!action || !key || !value) {
        LOG_ERROR("Invalid parameters for precondition");
        return false;
    }
    
    if (action->precondition_count >= GOAP_MAX_PRECONDITIONS) {
        LOG_ERROR("Maximum preconditions reached for action");
        return false;
    }
    
    GoapWorldStateVar* var = &action->preconditions[action->precondition_count++];
    strncpy(var->key, key, sizeof(var->key) - 1);
    var->type = type;
    
    switch (type) {
        case GOAP_VAR_BOOL:
            var->bool_value = *(bool*)value;
            break;
        case GOAP_VAR_INT:
            var->int_value = *(int*)value;
            break;
        case GOAP_VAR_FLOAT:
            var->float_value = *(float*)value;
            break;
        default:
            LOG_ERROR("Invalid variable type for precondition");
            return false;
    }
    
    return true;
}

bool goap_add_effect(GoapAction* action, const char* key, GoapVarType type, void* value) {
    if (!action || !key || !value) {
        LOG_ERROR("Invalid parameters for effect");
        return false;
    }
    
    if (action->effect_count >= GOAP_MAX_EFFECTS) {
        LOG_ERROR("Maximum effects reached for action");
        return false;
    }
    
    GoapWorldStateVar* var = &action->effects[action->effect_count++];
    strncpy(var->key, key, sizeof(var->key) - 1);
    var->type = type;
    
    switch (type) {
        case GOAP_VAR_BOOL:
            var->bool_value = *(bool*)value;
            break;
        case GOAP_VAR_INT:
            var->int_value = *(int*)value;
            break;
        case GOAP_VAR_FLOAT:
            var->float_value = *(float*)value;
            break;
        default:
            LOG_ERROR("Invalid variable type for effect");
            return false;
    }
    
    return true;
}

bool goap_check_preconditions(const GoapAction* action, const GoapWorldState* state) {
    if (!action || !state) return false;
    
    for (int i = 0; i < action->precondition_count; i++) {
        const GoapWorldStateVar* precond = &action->preconditions[i];
        
        switch (precond->type) {
            case GOAP_VAR_BOOL:
                {
                    bool value;
                    if (!goap_get_state_bool(state, precond->key, &value) || value != precond->bool_value) {
                        return false;
                    }
                }
                break;
                
            case GOAP_VAR_INT:
                {
                    int value;
                    if (!goap_get_state_int(state, precond->key, &value) || value != precond->int_value) {
                        return false;
                    }
                }
                break;
                
            case GOAP_VAR_FLOAT:
                {
                    float value;
                    if (!goap_get_state_float(state, precond->key, &value) || value != precond->float_value) {
                        return false;
                    }
                }
                break;
                
            default:
                break;
        }
    }
    
    return true;
}

void goap_apply_effects(GoapAction* action, GoapWorldState* state) {
    if (!action || !state) return;
    
    for (int i = 0; i < action->effect_count; i++) {
        const GoapWorldStateVar* effect = &action->effects[i];
        
        switch (effect->type) {
            case GOAP_VAR_BOOL:
                goap_set_state_bool(state, effect->key, effect->bool_value);
                break;
                
            case GOAP_VAR_INT:
                goap_set_state_int(state, effect->key, effect->int_value);
                break;
                
            case GOAP_VAR_FLOAT:
                goap_set_state_float(state, effect->key, effect->float_value);
                break;
                
            default:
                break;
        }
    }
}

// =================================================================================================
// GOAL SYSTEM (TASK_1602)
// =================================================================================================

GoapGoal* goap_create_goal(const char* name, float priority) {
    if (!name) {
        LOG_ERROR("Goal name cannot be NULL");
        return NULL;
    }
    
    GoapGoal* goal = (GoapGoal*)calloc(1, sizeof(GoapGoal));
    if (!goal) {
        LOG_ERROR("Failed to allocate GOAP goal");
        return NULL;
    }
    
    strncpy(goal->name, name, sizeof(goal->name) - 1);
    goal->priority = priority;
    goal->is_active = true;
    
    LOG_DEBUG("Created GOAP goal: %s", name);
    return goal;
}

void goap_destroy_goal(GoapGoal* goal) {
    if (!goal) return;
    
    free(goal);
    LOG_DEBUG("Destroyed GOAP goal: %s", goal->name);
}

bool goap_add_goal_condition(GoapGoal* goal, const char* key, GoapVarType type, void* value) {
    if (!goal || !key || !value) {
        LOG_ERROR("Invalid parameters for goal condition");
        return false;
    }
    
    if (goal->condition_count >= GOAP_MAX_GOAL_CONDITIONS) {
        LOG_ERROR("Maximum goal conditions reached");
        return false;
    }
    
    GoapWorldStateVar* var = &goal->conditions[goal->condition_count++];
    strncpy(var->key, key, sizeof(var->key) - 1);
    var->type = type;
    
    switch (type) {
        case GOAP_VAR_BOOL:
            var->bool_value = *(bool*)value;
            break;
        case GOAP_VAR_INT:
            var->int_value = *(int*)value;
            break;
        case GOAP_VAR_FLOAT:
            var->float_value = *(float*)value;
            break;
        default:
            LOG_ERROR("Invalid variable type for goal condition");
            return false;
    }
    
    return true;
}

bool goap_is_goal_satisfied(const GoapGoal* goal, const GoapWorldState* state) {
    if (!goal || !state) return false;
    
    for (int i = 0; i < goal->condition_count; i++) {
        const GoapWorldStateVar* condition = &goal->conditions[i];
        
        switch (condition->type) {
            case GOAP_VAR_BOOL:
                {
                    bool value;
                    if (!goap_get_state_bool(state, condition->key, &value) || value != condition->bool_value) {
                        return false;
                    }
                }
                break;
                
            case GOAP_VAR_INT:
                {
                    int value;
                    if (!goap_get_state_int(state, condition->key, &value) || value != condition->int_value) {
                        return false;
                    }
                }
                break;
                
            case GOAP_VAR_FLOAT:
                {
                    float value;
                    if (!goap_get_state_float(state, condition->key, &value) || value != condition->float_value) {
                        return false;
                    }
                }
                break;
                
            default:
                break;
        }
    }
    
    return true;
}

// =================================================================================================
// A* PLANNER (TASK_1603)
// =================================================================================================

GoapPlan* goap_create_plan(GoapPlanner* planner, const GoapWorldState* start, const GoapWorldState* goal, 
                         const GoapAction* actions, int action_count) {
    if (!planner || !start || !goal || !actions || action_count == 0) {
        LOG_ERROR("Invalid parameters for plan creation");
        return NULL;
    }
    
    // Clear previous search data
    goap_clear_search_data(planner);
    
    float start_time = get_current_time();
    
    // Create start node
    struct PlanNode* start_node = (struct PlanNode*)calloc(1, sizeof(struct PlanNode));
    if (!start_node) {
        LOG_ERROR("Failed to allocate start node");
        return NULL;
    }
    
    start_node->state = *start;
    start_node->g_cost = 0.0f;
    start_node->h_cost = heuristic_distance(start, goal);
    start_node->f_cost = start_node->g_cost + start_node->h_cost;
    start_node->parent = NULL;
    
    // Add to open list
    planner->open_list = start_node;
    
    // A* search loop
    struct PlanNode* goal_node = NULL;
    
    while (planner->open_list && !goal_node) {
        // Check time limit
        if (get_current_time() - start_time > planner->max_search_time) {
            LOG_WARNING("GOAP planning timeout");
            break;
        }
        
        // Find node with lowest f_cost
        struct PlanNode* current = planner->open_list;
        struct PlanNode* prev = NULL;
        struct PlanNode* best = current;
        struct PlanNode* best_prev = NULL;
        
        while (current) {
            if (current->f_cost < best->f_cost) {
                best = current;
                best_prev = prev;
            }
            prev = current;
            current = current->next;
        }
        
        // Remove best from open list
        if (best_prev) {
            best_prev->next = best->next;
        } else {
            planner->open_list = best->next;
        }
        
        // Add to closed list
        best->next = planner->closed_list;
        planner->closed_list = best;
        
        // Check if goal is reached
        if (goap_is_goal_satisfied(NULL, &best->state)) {
            goal_node = best;
            break;
        }
        
        // Expand node
        for (int i = 0; i < action_count; i++) {
            const GoapAction* action = &actions[i];
            
            // Check if action can be applied
            if (!goap_check_preconditions(action, &best->state)) {
                continue;
            }
            
            // Create new state by applying action
            GoapWorldState new_state = best->state;
            goap_apply_effects((GoapAction*)action, &new_state);
            
            // Check if this state is already in closed list
            bool in_closed = false;
            struct PlanNode* closed_current = planner->closed_list;
            while (closed_current) {
                if (goap_states_equal(&closed_current->state, &new_state)) {
                    in_closed = true;
                    break;
                }
                closed_current = closed_current->next;
            }
            
            if (in_closed) continue;
            
            // Calculate costs
            float g_cost = best->g_cost + action->cost;
            float h_cost = heuristic_distance(&new_state, goal);
            float f_cost = g_cost + h_cost;
            
            // Check if this state is already in open list with better cost
            bool in_open = false;
            struct PlanNode* open_current = planner->open_list;
            while (open_current) {
                if (goap_states_equal(&open_current->state, &new_state)) {
                    if (open_current->f_cost <= f_cost) {
                        in_open = true;
                    } else {
                        // Update existing node
                        open_current->g_cost = g_cost;
                        open_current->h_cost = h_cost;
                        open_current->f_cost = f_cost;
                        open_current->parent = best;
                        open_current->action = (GoapAction*)action;
                    }
                    break;
                }
                open_current = open_current->next;
            }
            
            if (in_open) continue;
            
            // Create new node
            struct PlanNode* new_node = (struct PlanNode*)calloc(1, sizeof(struct PlanNode));
            if (!new_node) {
                LOG_ERROR("Failed to allocate plan node");
                continue;
            }
            
            new_node->state = new_state;
            new_node->g_cost = g_cost;
            new_node->h_cost = h_cost;
            new_node->f_cost = f_cost;
            new_node->parent = best;
            new_node->action = (GoapAction*)action;
            
            // Add to open list
            new_node->next = planner->open_list;
            planner->open_list = new_node;
        }
    }
    
    // Reconstruct plan if goal was found
    GoapPlan* plan = NULL;
    if (goal_node) {
        plan = goap_reconstruct_plan(goal_node);
    }
    
    // Clean up search data
    goap_clear_search_data(planner);
    
    return plan;
}

GoapPlan* goap_reconstruct_plan(struct PlanNode* goal_node) {
    if (!goal_node) return NULL;
    
    // Count plan length
    int plan_length = 0;
    struct PlanNode* current = goal_node;
    while (current) {
        plan_length++;
        current = current->parent;
    }
    
    if (plan_length == 0) return NULL;
    
    // Allocate plan
    GoapPlan* plan = (GoapPlan*)calloc(1, sizeof(GoapPlan));
    if (!plan) {
        LOG_ERROR("Failed to allocate plan");
        return NULL;
    }
    
    plan->actions = (GoapAction**)calloc(plan_length, sizeof(GoapAction*));
    plan->action_count = plan_length;
    
    if (!plan->actions) {
        free(plan);
        return NULL;
    }
    
    // Fill plan (reverse order)
    current = goal_node;
    int index = plan_length - 1;
    while (current && current->parent) {
        plan->actions[index--] = current->action;
        current = current->parent;
    }
    
    plan->total_cost = goal_node->g_cost;
    
    LOG_INFO("Reconstructed GOAP plan with %d actions, cost: %.2f", plan_length, plan->total_cost);
    return plan;
}

void goap_destroy_plan(GoapPlan* plan) {
    if (!plan) return;
    
    if (plan->actions) {
        free(plan->actions);
    }
    
    free(plan);
}

bool goap_states_equal(const GoapWorldState* a, const GoapWorldState* b) {
    if (!a || !b) return false;
    
    for (int i = 0; i < GOAP_MAX_STATE_VARS; i++) {
        const GoapWorldStateVar* var_a = &a->variables[i];
        const GoapWorldStateVar* var_b = &b->variables[i];
        
        if (var_a->type != var_b->type) return false;
        
        if (var_a->type == GOAP_VAR_NONE) continue;
        
        if (strcmp(var_a->key, var_b->key) != 0) return false;
        
        switch (var_a->type) {
            case GOAP_VAR_BOOL:
                if (var_a->bool_value != var_b->bool_value) return false;
                break;
            case GOAP_VAR_INT:
                if (var_a->int_value != var_b->int_value) return false;
                break;
            case GOAP_VAR_FLOAT:
                if (fabsf(var_a->float_value - var_b->float_value) > 0.001f) return false;
                break;
            default:
                break;
        }
    }
    
    return true;
}

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

// =================================================================================================
// AGENT MANAGEMENT (TASK_1610-1613)
// =================================================================================================

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
    
    if (agent->current_plan) {
        goap_destroy_plan(agent->current_plan);
    }
    
    free(agent);
    LOG_INFO("Destroyed GOAP agent: %s", agent->name);
}

bool goap_agent_update(GoapAgent* agent, GoapPlanner* planner, const GoapAction* actions, int action_count, 
                      const GoapGoal* goals, int goal_count, float dt) {
    if (!agent || !planner) return false;
    
    agent->update_timer += dt;
    
    // Check if we need to replan
    if (agent->needs_replan || agent->update_timer >= agent->replan_interval) {
        agent->needs_replan = false;
        agent->update_timer = 0.0f;
        
        // Find best goal
        const GoapGoal* best_goal = goap_find_best_goal(agent, goals, goal_count);
        if (!best_goal) {
            LOG_WARNING("No valid goal found for agent %s", agent->name);
            return false;
        }
        
        // Create goal state
        GoapWorldState goal_state;
        goap_world_state_init(&goal_state);
        for (int i = 0; i < best_goal->condition_count; i++) {
            const GoapWorldStateVar* condition = &best_goal->conditions[i];
            
            switch (condition->type) {
                case GOAP_VAR_BOOL:
                    goap_set_state_bool(&goal_state, condition->key, condition->bool_value);
                    break;
                case GOAP_VAR_INT:
                    goap_set_state_int(&goal_state, condition->key, condition->int_value);
                    break;
                case GOAP_VAR_FLOAT:
                    goap_set_state_float(&goal_state, condition->key, condition->float_value);
                    break;
                default:
                    break;
            }
        }
        
        // Plan actions
        if (agent->current_plan) {
            goap_destroy_plan(agent->current_plan);
            agent->current_plan = NULL;
        }
        
        agent->current_plan = goap_create_plan(planner, &agent->current_state, &goal_state, actions, action_count);
        
        if (agent->current_plan) {
            agent->current_action_index = 0;
            LOG_INFO("Agent %s created new plan with %d actions", agent->name, agent->current_plan->action_count);
        } else {
            LOG_WARNING("Agent %s failed to create plan", agent->name);
        }
    }
    
    // Execute current action
    if (agent->current_plan && agent->current_action_index < agent->current_plan->action_count) {
        GoapAction* current_action = agent->current_plan->actions[agent->current_action_index];
        
        // Check if action can be executed
        if (goap_check_preconditions(current_action, &agent->current_state)) {
            // Apply action effects
            goap_apply_effects(current_action, &agent->current_state);
            
            // Move to next action
            agent->current_action_index++;
            
            LOG_DEBUG("Agent %s executed action: %s", agent->name, current_action->name);
            
            // Check if plan is complete
            if (agent->current_action_index >= agent->current_plan->action_count) {
                LOG_INFO("Agent %s completed plan", agent->name);
                agent->needs_replan = true;
            }
            
            return true;
        } else {
            LOG_WARNING("Agent %s cannot execute action: %s (preconditions not met)", 
                       agent->name, current_action->name);
            agent->needs_replan = true;
        }
    }
    
    return false;
}

const GoapGoal* goap_find_best_goal(GoapAgent* agent, const GoapGoal* goals, int goal_count) {
    if (!agent || !goals || goal_count == 0) return NULL;
    
    const GoapGoal* best_goal = NULL;
    float best_priority = -1.0f;
    
    for (int i = 0; i < goal_count; i++) {
        const GoapGoal* goal = &goals[i];
        
        if (!goal->is_active) continue;
        
        // Calculate priority (could be based on distance to goal, urgency, etc.)
        float priority = goal->priority;
        
        // Simple heuristic: goals that are closer have higher priority
        float distance = heuristic_distance(&agent->current_state, NULL);
        priority -= distance * 0.1f;
        
        if (priority > best_priority) {
            best_priority = priority;
            best_goal = goal;
        }
    }
    
    return best_goal;
}

// =================================================================================================
// UTILITY FUNCTIONS
// =================================================================================================

void goap_debug_print_plan(const GoapPlan* plan) {
    if (!plan) {
        printf("Plan is NULL\n");
        return;
    }
    
    printf("=== GOAP Plan ===\n");
    printf("Actions: %d\n", plan->action_count);
    printf("Total Cost: %.2f\n", plan->total_cost);
    printf("Actions:\n");
    
    for (int i = 0; i < plan->action_count; i++) {
        printf("  %d. %s (Cost: %.2f)\n", i + 1, plan->actions[i]->name, plan->actions[i]->cost);
    }
    
    printf("================\n");
}

void goap_debug_print_state(const GoapWorldState* state) {
    if (!state) {
        printf("State is NULL\n");
        return;
    }
    
    printf("=== World State ===\n");
    
    for (int i = 0; i < GOAP_MAX_STATE_VARS; i++) {
        const GoapWorldStateVar* var = &state->variables[i];
        
        if (var->type == GOAP_VAR_NONE) continue;
        
        printf("  %s: ", var->key);
        
        switch (var->type) {
            case GOAP_VAR_BOOL:
                printf("%s", var->bool_value ? "true" : "false");
                break;
            case GOAP_VAR_INT:
                printf("%d", var->int_value);
                break;
            case GOAP_VAR_FLOAT:
                printf("%.2f", var->float_value);
                break;
            default:
                printf("unknown");
                break;
        }
        
        printf("\n");
    }
    
    printf("==================\n");
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
                if (!vec3_equals(&current_key.value.vector_val, &condition->value.vector_val)) {
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
                if (!vec3_equals(&current_key.value.vector_val, &condition->value.vector_val)) {
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
        printf("  %d. %s (cost: %.2f)\n", i + 1, plan[i]->name, plan[i]->cost);
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
    typedef struct PlanNode {
        GoapWorldState state;
        float g_cost;        // Cost from start
        float h_cost;        // Heuristic cost to goal
        float f_cost;        // Total cost
        GoapAction* action;  // Action that led to this state
        struct PlanNode* parent;
        int depth;
        bool in_open;
        bool in_closed;
    } PlanNode;
    
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
                if (!vec3_equals(&key1->value.vector_val, &key2.value.vector_val)) return false;
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
bool goap_world_states_equal(const GoapWorldState* state1, const GoapWorldState* state2);
