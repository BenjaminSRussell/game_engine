// ai_system.c - Unified AI System Implementation
#include "ai_system.h"
#include "../include/core/logger.h"
#include <stdlib.h>
#include <string.h>
#include <math.h>

#define MAX_AGENTS 1024
#define MAX_NODES 65536
#define PATHFINDING_MAX_REQUESTS 256

// === AGENT IMPLEMENTATION ===

struct AIAgent {
    uint32_t id;
    AIAgentType type;
    AIAgentState state;
    
    // Transform
    float position[3];
    float rotation[3];
    float velocity[3];
    
    // Configuration
    AIAgentConfig config;
    
    // AI Systems
    BehaviorTree* behavior_tree;
    GoapPlanner* goap_planner;
    Path* current_path;
    uint32_t current_path_index;
    
    // Relationships
    AIAgent* target;
    AIAgent* enemies[16];
    uint32_t enemy_count;
    AIAgent* allies[16];
    uint32_t ally_count;
    
    // State
    bool is_active;
    float health;
    float max_health;
    float perception_timer;
    
    void* user_data;
};

AIAgent* ai_agent_create(uint32_t id, const AIAgentConfig* config) {
    AIAgent* agent = malloc(sizeof(AIAgent));
    if (!agent) return NULL;
    
    memset(agent, 0, sizeof(AIAgent));
    agent->id = id;
    agent->state = AI_STATE_IDLE;
    
    if (config) {
        agent->config = *config;
        agent->type = config->type;
        agent->max_health = 100.0f;
        agent->health = agent->max_health;
    }
    
    ai_vector3_set(agent->position, 0.0f, 0.0f, 0.0f);
    ai_vector3_set(agent->rotation, 0.0f, 0.0f, 0.0f);
    ai_vector3_set(agent->velocity, 0.0f, 0.0f, 0.0f);
    
    agent->is_active = true;
    
    log_info("AI agent created with ID: %u, type: %d", id, agent->type);
    return agent;
}

void ai_agent_destroy(AIAgent* agent) {
    if (!agent) return;
    
    if (agent->behavior_tree) {
        bt_destroy(agent->behavior_tree);
    }
    
    if (agent->goap_planner) {
        goap_destroy_planner(agent->goap_planner);
    }
    
    if (agent->current_path) {
        pathfinding_destroy_path(agent->current_path);
    }
    
    free(agent);
    log_info("AI agent destroyed with ID: %u", agent->id);
}

void ai_agent_update(AIAgent* agent, float dt) {
    if (!agent || !agent->is_active) return;
    
    // Update perception
    agent->perception_timer += dt;
    
    // Update behavior tree
    if (agent->behavior_tree) {
        bt_update(agent->behavior_tree, agent, dt);
    }
    
    // Update GOAP planner
    if (agent->goap_planner) {
        goap_execute_plan(agent->goap_planner, agent, dt);
    }
    
    // Update movement along path
    if (agent->current_path && agent->current_path->is_valid) {
        // Simple path following
        if (agent->current_path_index < agent->current_path->waypoint_count) {
            float* target = &agent->current_path->waypoints[agent->current_path_index * 3];
            float direction[3];
            ai_vector3_subtract(direction, target, agent->position);
            
            float distance = ai_vector3_length(direction);
            if (distance < 0.5f) {
                agent->current_path_index++;
                if (agent->current_path_index >= agent->current_path->waypoint_count) {
                    // Reached destination
                    pathfinding_destroy_path(agent->current_path);
                    agent->current_path = NULL;
                    agent->current_path_index = 0;
                }
            } else {
                // Move towards target
                ai_vector3_normalize(direction);
                ai_vector3_multiply(direction, direction, agent->config.movement_speed * dt);
                ai_vector3_add(agent->position, agent->position, direction);
                
                // Update rotation to face movement direction
                agent->rotation[1] = atan2f(direction[0], direction[2]) * 180.0f / M_PI;
            }
        }
    }
}

void ai_agent_set_state(AIAgent* agent, AIAgentState state) {
    if (!agent) return;
    agent->state = state;
}

void ai_agent_set_position(AIAgent* agent, float x, float y, float z) {
    if (!agent) return;
    ai_vector3_set(agent->position, x, y, z);
}

void ai_agent_get_position(AIAgent* agent, float* x, float* y, float* z) {
    if (!agent || !x || !y || !z) return;
    *x = agent->position[0];
    *y = agent->position[1];
    *z = agent->position[2];
}

void ai_agent_set_target(AIAgent* agent, AIAgent* target) {
    if (!agent) return;
    agent->target = target;
}

AIAgent* ai_agent_get_target(AIAgent* agent) {
    return agent ? agent->target : NULL;
}

void ai_agent_set_behavior_tree(AIAgent* agent, BehaviorTree* tree) {
    if (!agent) return;
    if (agent->behavior_tree) {
        bt_destroy(agent->behavior_tree);
    }
    agent->behavior_tree = tree;
}

void ai_agent_set_goap_planner(AIAgent* agent, GoapPlanner* planner) {
    if (!agent) return;
    if (agent->goap_planner) {
        goap_destroy_planner(agent->goap_planner);
    }
    agent->goap_planner = planner;
}

// === BEHAVIOR TREE IMPLEMENTATION ===

BehaviorTree* bt_create(void) {
    BehaviorTree* tree = malloc(sizeof(BehaviorTree));
    if (!tree) return NULL;
    
    memset(tree, 0, sizeof(BehaviorTree));
    tree->is_active = true;
    tree->update_interval = 0.1f; // 10 Hz update rate
    
    return tree;
}

void bt_destroy(BehaviorTree* tree) {
    if (!tree) return;
    
    // TODO: Recursively destroy all nodes
    if (tree->root) {
        // Free node tree
    }
    
    free(tree);
}

void bt_update(BehaviorTree* tree, AIAgent* agent, float dt) {
    if (!tree || !agent || !tree->is_active || !tree->root) return;
    
    tree->accumulated_time += dt;
    if (tree->accumulated_time < tree->update_interval) return;
    
    tree->accumulated_time = 0.0f;
    
    // Execute the root node
    tree->root->status = tree->root->data.action.execute(tree->root, agent);
}

BTNode* bt_create_action_node(const char* name, BTNodeStatus (*execute)(BTNode* node, AIAgent* agent)) {
    BTNode* node = malloc(sizeof(BTNode));
    if (!node) return NULL;
    
    memset(node, 0, sizeof(BTNode));
    node->type = BT_NODE_ACTION;
    node->status = BT_STATUS_FAILURE;
    strncpy(node->name, name, sizeof(node->name) - 1);
    
    node->data.action.execute = execute;
    node->data.action.init = NULL;
    node->data.action.cleanup = NULL;
    
    return node;
}

BTNode* bt_create_condition_node(const char* name, bool (*check)(AIAgent* agent)) {
    BTNode* node = malloc(sizeof(BTNode));
    if (!node) return NULL;
    
    memset(node, 0, sizeof(BTNode));
    node->type = BT_NODE_CONDITION;
    node->status = BT_STATUS_FAILURE;
    strncpy(node->name, name, sizeof(node->name) - 1);
    
    node->data.condition.check = check;
    
    return node;
}

BTNode* bt_create_selector_node(const char* name) {
    BTNode* node = malloc(sizeof(BTNode));
    if (!node) return NULL;
    
    memset(node, 0, sizeof(BTNode));
    node->type = BT_NODE_SELECTOR;
    node->status = BT_STATUS_FAILURE;
    strncpy(node->name, name, sizeof(node->name) - 1);
    
    node->children = malloc(sizeof(BTNode*) * 8);
    node->child_capacity = 8;
    
    return node;
}

BTNode* bt_create_sequence_node(const char* name) {
    BTNode* node = malloc(sizeof(BTNode));
    if (!node) return NULL;
    
    memset(node, 0, sizeof(BTNode));
    node->type = BT_NODE_SEQUENCE;
    node->status = BT_STATUS_FAILURE;
    strncpy(node->name, name, sizeof(node->name) - 1);
    
    node->children = malloc(sizeof(BTNode*) * 8);
    node->child_capacity = 8;
    
    return node;
}

void bt_add_child(BTNode* parent, BTNode* child) {
    if (!parent || !child) return;
    
    if (parent->child_count >= parent->child_capacity) {
        parent->child_capacity *= 2;
        parent->children = realloc(parent->children, sizeof(BTNode*) * parent->child_capacity);
    }
    
    parent->children[parent->child_count++] = child;
}

void bt_set_blackboard_float(BehaviorTree* tree, const char* key, float value) {
    if (!tree || !key) return;
    
    for (uint32_t i = 0; i < tree->blackboard.count; i++) {
        if (strcmp(tree->blackboard.entries[i].key, key) == 0) {
            tree->blackboard.entries[i].value.float_val = value;
            tree->blackboard.entries[i].type = BT_VALUE_FLOAT;
            return;
        }
    }
    
    if (tree->blackboard.count < 128) {
        strncpy(tree->blackboard.entries[tree->blackboard.count].key, key, 63);
        tree->blackboard.entries[tree->blackboard.count].value.float_val = value;
        tree->blackboard.entries[tree->blackboard.count].type = BT_VALUE_FLOAT;
        tree->blackboard.count++;
    }
}

bool bt_get_blackboard_float(BehaviorTree* tree, const char* key, float* value) {
    if (!tree || !key || !value) return false;
    
    for (uint32_t i = 0; i < tree->blackboard.count; i++) {
        if (strcmp(tree->blackboard.entries[i].key, key) == 0 &&
            tree->blackboard.entries[i].type == BT_VALUE_FLOAT) {
            *value = tree->blackboard.entries[i].value.float_val;
            return true;
        }
    }
    return false;
}

// === GOAP IMPLEMENTATION ===

GoapPlanner* goap_create_planner(void) {
    GoapPlanner* planner = malloc(sizeof(GoapPlanner));
    if (!planner) return NULL;
    
    memset(planner, 0, sizeof(GoapPlanner));
    
    // Allocate search data
    planner->search_data.capacity = 1024;
    planner->search_data.states = malloc(sizeof(GoapWorldState) * planner->search_data.capacity);
    planner->search_data.g_scores = malloc(sizeof(float) * planner->search_data.capacity);
    planner->search_data.f_scores = malloc(sizeof(float) * planner->search_data.capacity);
    planner->search_data.came_from = malloc(sizeof(GoapAction*) * planner->search_data.capacity);
    planner->search_data.closed_set = malloc(sizeof(bool) * planner->search_data.capacity);
    planner->search_data.open_set = malloc(sizeof(bool) * planner->search_data.capacity);
    
    return planner;
}

void goap_destroy_planner(GoapPlanner* planner) {
    if (!planner) return;
    
    free(planner->search_data.states);
    free(planner->search_data.g_scores);
    free(planner->search_data.f_scores);
    free(planner->search_data.came_from);
    free(planner->search_data.closed_set);
    free(planner->search_data.open_set);
    
    free(planner);
}

void goap_add_action(GoapPlanner* planner, GoapAction* action) {
    if (!planner || !action || planner->action_count >= 64) return;
    planner->actions[planner->action_count++] = action;
}

void goap_add_goal(GoapPlanner* planner, GoapGoal* goal) {
    if (!planner || !goal || planner->goal_count >= 16) return;
    planner->goals[planner->goal_count++] = goal;
}

bool goap_plan(GoapPlanner* planner, AIAgent* agent) {
    if (!planner || !agent) return false;
    
    // TODO: Implement A* planning algorithm
    // For now, just create a simple plan
    planner->plan_length = 0;
    planner->current_action_index = 0;
    planner->plan_timer = 0.0f;
    
    return true;
}

void goap_execute_plan(GoapPlanner* planner, AIAgent* agent, float dt) {
    if (!planner || !agent || planner->plan_length == 0) return;
    
    planner->plan_timer += dt;
    
    if (planner->current_action_index < planner->plan_length) {
        GoapAction* action = planner->actions[planner->current_action_index];
        
        if (planner->plan_timer >= action->duration) {
            // Execute action
            if (action->execute) {
                action->execute(agent);
            }
            
            // Apply effects
            // TODO: Apply action effects to world state
            
            planner->current_action_index++;
            planner->plan_timer = 0.0f;
        }
    }
}

// === PATHFINDING IMPLEMENTATION ===

NavigationMesh* navmesh_create(void) {
    NavigationMesh* navmesh = malloc(sizeof(NavigationMesh));
    if (!navmesh) return NULL;
    
    memset(navmesh, 0, sizeof(NavigationMesh));
    navmesh->node_capacity = 1024;
    navmesh->nodes = malloc(sizeof(PathNode) * navmesh->node_capacity);
    
    return navmesh;
}

void navmesh_destroy(NavigationMesh* navmesh) {
    if (!navmesh) return;
    
    free(navmesh->nodes);
    
    if (navmesh->spatial_grid.grid) {
        for (uint32_t i = 0; i < navmesh->spatial_grid.grid_size_x; i++) {
            free(navmesh->spatial_grid.grid[i]);
        }
        free(navmesh->spatial_grid.grid);
    }
    
    free(navmesh);
}

bool navmesh_add_node(NavigationMesh* navmesh, float x, float y, float z, bool is_walkable) {
    if (!navmesh || navmesh->node_count >= navmesh->node_capacity) return false;
    
    PathNode* node = &navmesh->nodes[navmesh->node_count++];
    node->x = x;
    node->y = y;
    node->z = z;
    node->is_walkable = is_walkable;
    node->connection_count = 0;
    node->area_id = 0;
    
    // Update bounds
    if (navmesh->node_count == 1) {
        navmesh->spatial_grid.min_x = navmesh->spatial_grid.max_x = x;
        navmesh->spatial_grid.min_y = navmesh->spatial_grid.max_y = y;
        navmesh->spatial_grid.min_z = navmesh->spatial_grid.max_z = z;
    } else {
        navmesh->spatial_grid.min_x = fminf(navmesh->spatial_grid.min_x, x);
        navmesh->spatial_grid.max_x = fmaxf(navmesh->spatial_grid.max_x, x);
        navmesh->spatial_grid.min_y = fminf(navmesh->spatial_grid.min_y, y);
        navmesh->spatial_grid.max_y = fmaxf(navmesh->spatial_grid.max_y, y);
        navmesh->spatial_grid.min_z = fminf(navmesh->spatial_grid.min_z, z);
        navmesh->spatial_grid.max_z = fmaxf(navmesh->spatial_grid.max_z, z);
    }
    
    return true;
}

bool navmesh_add_connection(NavigationMesh* navmesh, uint32_t from_node, uint32_t to_node) {
    if (!navmesh || from_node >= navmesh->node_count || to_node >= navmesh->node_count) return false;
    
    PathNode* node = &navmesh->nodes[from_node];
    if (node->connection_count >= 16) return false;
    
    node->connections[node->connection_count++] = to_node;
    return true;
}

Path* pathfinding_find_path(NavigationMesh* navmesh, const float* start, const float* end, bool can_fly, bool can_swim, float agent_radius) {
    if (!navmesh || !start || !end) return NULL;
    
    Path* path = malloc(sizeof(Path));
    if (!path) return NULL;
    
    memset(path, 0, sizeof(Path));
    path->waypoint_capacity = 32;
    path->waypoints = malloc(sizeof(float) * 3 * path->waypoint_capacity);
    
    // Simple direct path for now
    // TODO: Implement A* pathfinding
    path->waypoints[0] = start[0];
    path->waypoints[1] = start[1];
    path->waypoints[2] = start[2];
    path->waypoints[3] = end[0];
    path->waypoints[4] = end[1];
    path->waypoints[5] = end[2];
    path->waypoint_count = 2;
    path->is_valid = true;
    
    // Calculate path length
    float dx = end[0] - start[0];
    float dy = end[1] - start[1];
    float dz = end[2] - start[2];
    path->total_length = sqrtf(dx*dx + dy*dy + dz*dz);
    
    return path;
}

void pathfinding_destroy_path(Path* path) {
    if (!path) return;
    free(path->waypoints);
    free(path);
}

// === AI WORLD IMPLEMENTATION ===

AIWorld* ai_world_create(void) {
    AIWorld* world = malloc(sizeof(AIWorld));
    if (!world) return NULL;
    
    memset(world, 0, sizeof(AIWorld));
    world->update_interval = 0.016f; // 60 Hz
    
    world->navmesh = navmesh_create();
    
    log_info("AI world created");
    return world;
}

void ai_world_destroy(AIWorld* world) {
    if (!world) return;
    
    for (uint32_t i = 0; i < world->agent_count; i++) {
        ai_agent_destroy(world->agents[i]);
    }
    
    if (world->navmesh) {
        navmesh_destroy(world->navmesh);
    }
    
    free(world);
    log_info("AI world destroyed");
}

void ai_world_update(AIWorld* world, float dt) {
    if (!world) return;
    
    world->accumulated_time += dt;
    if (world->accumulated_time < world->update_interval) return;
    
    world->accumulated_time = 0.0f;
    world->active_agents = 0;
    
    for (uint32_t i = 0; i < world->agent_count; i++) {
        AIAgent* agent = world->agents[i];
        if (agent && agent->is_active) {
            ai_agent_update(agent, dt);
            world->active_agents++;
        }
    }
}

void ai_world_add_agent(AIWorld* world, AIAgent* agent) {
    if (!world || !agent || world->agent_count >= MAX_AGENTS) return;
    world->agents[world->agent_count++] = agent;
}

void ai_world_remove_agent(AIWorld* world, AIAgent* agent) {
    if (!world || !agent) return;
    
    for (uint32_t i = 0; i < world->agent_count; i++) {
        if (world->agents[i] == agent) {
            // Shift remaining agents
            for (uint32_t j = i; j < world->agent_count - 1; j++) {
                world->agents[j] = world->agents[j + 1];
            }
            world->agent_count--;
            break;
        }
    }
}

// === UTILITY FUNCTIONS ===

void ai_vector3_set(float* vec, float x, float y, float z) {
    vec[0] = x; vec[1] = y; vec[2] = z;
}

void ai_vector3_copy(float* dest, const float* src) {
    dest[0] = src[0]; dest[1] = src[1]; dest[2] = src[2];
}

void ai_vector3_add(float* result, const float* a, const float* b) {
    result[0] = a[0] + b[0];
    result[1] = a[1] + b[1];
    result[2] = a[2] + b[2];
}

void ai_vector3_subtract(float* result, const float* a, const float* b) {
    result[0] = a[0] - b[0];
    result[1] = a[1] - b[1];
    result[2] = a[2] - b[2];
}

void ai_vector3_multiply(float* result, const float* vec, float scalar) {
    result[0] = vec[0] * scalar;
    result[1] = vec[1] * scalar;
    result[2] = vec[2] * scalar;
}

float ai_vector3_distance(const float* a, const float* b) {
    float dx = a[0] - b[0];
    float dy = a[1] - b[1];
    float dz = a[2] - b[2];
    return sqrtf(dx*dx + dy*dy + dz*dz);
}

float ai_vector3_length(const float* vec) {
    return sqrtf(vec[0]*vec[0] + vec[1]*vec[1] + vec[2]*vec[2]);
}

void ai_vector3_normalize(float* vec) {
    float length = ai_vector3_length(vec);
    if (length > 1e-6f) {
        ai_vector3_multiply(vec, vec, 1.0f / length);
    }
}

float ai_vector3_dot(const float* a, const float* b) {
    return a[0]*b[0] + a[1]*b[1] + a[2]*b[2];
}

// Stub implementations for remaining functions
BTNode* bt_create_parallel_node(const char* name, bool succeed_on_one, bool fail_on_one) { return NULL; /* TODO */ }
BTNode* bt_create_utility_selector_node(const char* name) { return NULL; /* TODO */ }
void bt_set_blackboard_int(BehaviorTree* tree, const char* key, int value) { /* TODO */ }
void bt_set_blackboard_bool(BehaviorTree* tree, const char* key, bool value) { /* TODO */ }
void bt_set_blackboard_ptr(BehaviorTree* tree, const char* key, void* value) { /* TODO */ }
bool bt_get_blackboard_int(BehaviorTree* tree, const char* key, int* value) { return false; /* TODO */ }
bool bt_get_blackboard_bool(BehaviorTree* tree, const char* key, bool* value) { return false; /* TODO */ }
bool bt_get_blackboard_ptr(BehaviorTree* tree, const char* key, void** value) { return false; /* TODO */ }

void goap_set_world_state_bool(GoapPlanner* planner, const char* key, bool value) { /* TODO */ }
void goap_set_world_state_int(GoapPlanner* planner, const char* key, int value) { /* TODO */ }
void goap_set_world_state_float(GoapPlanner* planner, const char* key, float value) { /* TODO */ }
bool goap_get_world_state_bool(GoapPlanner* planner, const char* key, bool* value) { return false; /* TODO */ }
bool goap_get_world_state_int(GoapPlanner* planner, const char* key, int* value) { return false; /* TODO */ }
bool goap_get_world_state_float(GoapPlanner* planner, const char* key, float* value) { return false; /* TODO */ }

bool navmesh_load_from_file(NavigationMesh* navmesh, const char* filename) { return false; /* TODO */ }
PathfindingRequest* pathfinding_create_request(uint32_t agent_id, const float* start, const float* end, bool can_fly, bool can_swim, float agent_radius) { return NULL; /* TODO */ }
void pathfinding_destroy_request(PathfindingRequest* request) { /* TODO */ }
void pathfinding_update_async_requests(NavigationMesh* navmesh, float dt) { /* TODO */ }

AIAgent* ai_world_find_agent_by_id(AIWorld* world, uint32_t id) { return NULL; /* TODO */ }
void ai_world_set_navigation_mesh(AIWorld* world, NavigationMesh* navmesh) { /* TODO */ }
NavigationMesh* ai_world_get_navigation_mesh(AIWorld* world) { return NULL; /* TODO */ }

bool ai_can_see(AIAgent* agent, AIAgent* target, const float* obstacles) { return false; /* TODO */ }
bool ai_can_hear(AIAgent* agent, AIAgent* target, float max_distance) { return false; /* TODO */ }
float ai_get_threat_level(AIAgent* agent, AIAgent* target) { return 0.0f; /* TODO */ }
bool ai_is_in_range(AIAgent* agent, AIAgent* target, float range) { return false; /* TODO */ }
void ai_look_at(AIAgent* agent, const float* target, float rotation_speed) { /* TODO */ }
