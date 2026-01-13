#ifndef AI_SYSTEM_H
#define AI_SYSTEM_H

#include <stdint.h>
#include <stdbool.h>

// Forward declarations
typedef struct AIAgent AIAgent;
typedef struct AIWorld AIWorld;
typedef struct BehaviorTree BehaviorTree;
typedef struct GoapPlanner GoapPlanner;
typedef struct NavigationMesh NavigationMesh;
typedef struct PathfindingRequest PathfindingRequest;

// === AI AGENT SYSTEM ===

// Agent types
typedef enum {
    AI_AGENT_NPC,
    AI_AGENT_ENEMY,
    AI_AGENT_COMPANION,
    AI_AGENT_ANIMAL,
    AI_AGENT_VEHICLE
} AIAgentType;

// Agent states
typedef enum {
    AI_STATE_IDLE,
    AI_STATE_PATROL,
    AI_STATE_CHASE,
    AI_STATE_FLEE,
    AI_STATE_ATTACK,
    AI_STATE_INTERACT,
    AI_STATE_SLEEP
} AIAgentState;

// Agent configuration
typedef struct {
    AIAgentType type;
    float perception_radius;
    float attack_range;
    float movement_speed;
    float rotation_speed;
    bool can_fly;
    bool can_swim;
    uint32_t faction_id;
    uint32_t team_id;
} AIAgentConfig;

// Agent creation and management
AIAgent* ai_agent_create(uint32_t id, const AIAgentConfig* config);
void ai_agent_destroy(AIAgent* agent);
void ai_agent_update(AIAgent* agent, float dt);
void ai_agent_set_state(AIAgent* agent, AIAgentState state);
void ai_agent_set_position(AIAgent* agent, float x, float y, float z);
void ai_agent_get_position(AIAgent* agent, float* x, float* y, float* z);
void ai_agent_set_target(AIAgent* agent, AIAgent* target);
AIAgent* ai_agent_get_target(AIAgent* agent);
void ai_agent_set_behavior_tree(AIAgent* agent, BehaviorTree* tree);
void ai_agent_set_goap_planner(AIAgent* agent, GoapPlanner* planner);

// === BEHAVIOR TREE SYSTEM ===

// Node types
typedef enum {
    BT_NODE_ACTION,
    BT_NODE_CONDITION,
    BT_NODE_SELECTOR,
    BT_NODE_SEQUENCE,
    BT_NODE_PARALLEL,
    BT_NODE_DECORATOR,
    BT_NODE_UTILITIES
} BTNodeType;

// Node statuses
typedef enum {
    BT_STATUS_SUCCESS,
    BT_STATUS_FAILURE,
    BT_STATUS_RUNNING
} BTNodeStatus;

// Value types for blackboard
typedef enum {
    BT_VALUE_FLOAT,
    BT_VALUE_INT,
    BT_VALUE_BOOL,
    BT_VALUE_PTR
} BTValueType;

// Behavior tree node
typedef struct BTNode {
    BTNodeType type;
    BTNodeStatus status;
    struct BTNode** children;
    uint32_t child_count;
    uint32_t child_capacity;
    
    // Node-specific data
    union {
        struct {
            BTNodeStatus (*execute)(struct BTNode* node, AIAgent* agent);
            void (*init)(struct BTNode* node, AIAgent* agent);
            void (*cleanup)(struct BTNode* node, AIAgent* agent);
        } action;
        
        struct {
            bool (*check)(AIAgent* agent);
        } condition;
        
        struct {
            float (*score)(AIAgent* agent);
        } utility;
        
        struct {
            float cooldown_time;
            float current_cooldown;
            float timeout_duration;
            float elapsed_time;
            bool invert_condition;
            int max_retries;
            int current_retry;
        } decorator;
    } data;
    
    char name[64];
} BTNode;

// Blackboard for data sharing
typedef struct {
    char key[64];
    BTValueType type;
    union {
        float float_val;
        int int_val;
        bool bool_val;
        void* ptr_val;
    } value;
} BlackboardEntry;

typedef struct {
    BlackboardEntry entries[128];
    uint32_t count;
} Blackboard;

// Behavior tree
struct BehaviorTree {
    BTNode* root;
    Blackboard blackboard;
    bool is_active;
    float update_interval;
    float accumulated_time;
};

// Behavior tree creation and management
BehaviorTree* bt_create(void);
void bt_destroy(BehaviorTree* tree);
void bt_update(BehaviorTree* tree, AIAgent* agent, float dt);
BTNode* bt_create_action_node(const char* name, BTNodeStatus (*execute)(BTNode* node, AIAgent* agent));
BTNode* bt_create_condition_node(const char* name, bool (*check)(AIAgent* agent));
BTNode* bt_create_selector_node(const char* name);
BTNode* bt_create_sequence_node(const char* name);
BTNode* bt_create_parallel_node(const char* name, bool succeed_on_one, bool fail_on_one);
BTNode* bt_create_utility_selector_node(const char* name);
void bt_add_child(BTNode* parent, BTNode* child);
void bt_set_blackboard_float(BehaviorTree* tree, const char* key, float value);
void bt_set_blackboard_int(BehaviorTree* tree, const char* key, int value);
void bt_set_blackboard_bool(BehaviorTree* tree, const char* key, bool value);
void bt_set_blackboard_ptr(BehaviorTree* tree, const char* key, void* value);
bool bt_get_blackboard_float(BehaviorTree* tree, const char* key, float* value);
bool bt_get_blackboard_int(BehaviorTree* tree, const char* key, int* value);
bool bt_get_blackboard_bool(BehaviorTree* tree, const char* key, bool* value);
bool bt_get_blackboard_ptr(BehaviorTree* tree, const char* key, void** value);

// === GOAP SYSTEM ===

// GOAP state value types
typedef enum {
    GOAP_STATE_BOOL,
    GOAP_STATE_INT,
    GOAP_STATE_FLOAT
} GoapStateType;

// GOAP state key
typedef struct {
    char name[64];
    GoapStateType type;
    union {
        bool bool_val;
        int int_val;
        float float_val;
    } value;
} GoapStateKey;

// GOAP world state
typedef struct {
    GoapStateKey keys[32];
    uint32_t key_count;
} GoapWorldState;

// GOAP action
typedef struct {
    char name[64];
    float cost;
    GoapWorldState preconditions;
    GoapWorldState effects;
    bool (*can_execute)(AIAgent* agent);
    void (*execute)(AIAgent* agent);
    float duration;
    float cooldown;
} GoapAction;

// GOAP goal
typedef struct {
    char name[64];
    GoapWorldState desired_state;
    float priority;
    bool (*is_valid)(AIAgent* agent);
} GoapGoal;

// GOAP planner
struct GoapPlanner {
    GoapAction* actions[64];
    uint32_t action_count;
    GoapGoal* goals[16];
    uint32_t goal_count;
    
    // Planning state
    GoapWorldState current_state;
    GoapAction* current_plan[32];
    uint32_t plan_length;
    uint32_t current_action_index;
    float plan_timer;
    
    // Search data
    struct {
        GoapWorldState* states;
        float* g_scores;
        float* f_scores;
        GoapAction** came_from;
        bool* closed_set;
        bool* open_set;
        uint32_t capacity;
    } search_data;
};

// GOAP creation and management
GoapPlanner* goap_create_planner(void);
void goap_destroy_planner(GoapPlanner* planner);
void goap_add_action(GoapPlanner* planner, GoapAction* action);
void goap_add_goal(GoapPlanner* planner, GoapGoal* goal);
bool goap_plan(GoapPlanner* planner, AIAgent* agent);
void goap_execute_plan(GoapPlanner* planner, AIAgent* agent, float dt);
void goap_set_world_state_bool(GoapPlanner* planner, const char* key, bool value);
void goap_set_world_state_int(GoapPlanner* planner, const char* key, int value);
void goap_set_world_state_float(GoapPlanner* planner, const char* key, float value);
bool goap_get_world_state_bool(GoapPlanner* planner, const char* key, bool* value);
bool goap_get_world_state_int(GoapPlanner* planner, const char* key, int* value);
bool goap_get_world_state_float(GoapPlanner* planner, const char* key, float* value);

// === PATHFINDING SYSTEM ===

// Pathfinding node
typedef struct {
    float x, y, z;
    uint32_t connections[16];
    uint32_t connection_count;
    bool is_walkable;
    uint32_t area_id;
} PathNode;

// Pathfinding result
typedef struct {
    float* waypoints;
    uint32_t waypoint_count;
    uint32_t waypoint_capacity;
    float total_length;
    bool is_valid;
} Path;

// Navigation mesh
struct NavigationMesh {
    PathNode* nodes;
    uint32_t node_count;
    uint32_t node_capacity;
    
    // Areas for different movement types
    uint32_t* area_ids;
    uint32_t area_count;
    
    // Spatial optimization
    struct {
        float min_x, min_y, min_z;
        float max_x, max_y, max_z;
        uint32_t grid_size_x, grid_size_z;
        uint32_t** grid;
    } spatial_grid;
};

// Pathfinding request
struct PathfindingRequest {
    uint32_t agent_id;
    float start[3];
    float end[3];
    bool can_fly;
    bool can_swim;
    float agent_radius;
    Path* result;
    bool is_completed;
    void (*callback)(PathfindingRequest* request);
};

// Pathfinding functions
NavigationMesh* navmesh_create(void);
void navmesh_destroy(NavigationMesh* navmesh);
bool navmesh_load_from_file(NavigationMesh* navmesh, const char* filename);
bool navmesh_add_node(NavigationMesh* navmesh, float x, float y, float z, bool is_walkable);
bool navmesh_add_connection(NavigationMesh* navmesh, uint32_t from_node, uint32_t to_node);
Path* pathfinding_find_path(NavigationMesh* navmesh, const float* start, const float* end, bool can_fly, bool can_swim, float agent_radius);
void pathfinding_destroy_path(Path* path);
PathfindingRequest* pathfinding_create_request(uint32_t agent_id, const float* start, const float* end, bool can_fly, bool can_swim, float agent_radius);
void pathfinding_destroy_request(PathfindingRequest* request);
void pathfinding_update_async_requests(NavigationMesh* navmesh, float dt);

// === AI WORLD SYSTEM ===

// AI world for managing all agents
struct AIWorld {
    AIAgent* agents[1024];
    uint32_t agent_count;
    NavigationMesh* navmesh;
    float update_interval;
    float accumulated_time;
    
    // Performance stats
    uint32_t active_agents;
    uint32_t behavior_tree_updates;
    uint32_t goap_plans_generated;
    uint32_t paths_calculated;
};

// AI world management
AIWorld* ai_world_create(void);
void ai_world_destroy(AIWorld* world);
void ai_world_update(AIWorld* world, float dt);
void ai_world_add_agent(AIWorld* world, AIAgent* agent);
void ai_world_remove_agent(AIWorld* world, AIAgent* agent);
AIAgent* ai_world_find_agent_by_id(AIWorld* world, uint32_t id);
void ai_world_set_navigation_mesh(AIWorld* world, NavigationMesh* navmesh);
NavigationMesh* ai_world_get_navigation_mesh(AIWorld* world);

// === UTILITY FUNCTIONS ===

// Vector math for AI
void ai_vector3_set(float* vec, float x, float y, float z);
void ai_vector3_copy(float* dest, const float* src);
void ai_vector3_add(float* result, const float* a, const float* b);
void ai_vector3_subtract(float* result, const float* a, const float* b);
void ai_vector3_multiply(float* result, const float* vec, float scalar);
float ai_vector3_distance(const float* a, const float* b);
float ai_vector3_length(const float* vec);
void ai_vector3_normalize(float* vec);
float ai_vector3_dot(const float* a, const float* b);

// AI utilities
bool ai_can_see(AIAgent* agent, AIAgent* target, const float* obstacles);
bool ai_can_hear(AIAgent* agent, AIAgent* target, float max_distance);
float ai_get_threat_level(AIAgent* agent, AIAgent* target);
bool ai_is_in_range(AIAgent* agent, AIAgent* target, float range);
void ai_look_at(AIAgent* agent, const float* target, float rotation_speed);

#endif // AI_SYSTEM_H
