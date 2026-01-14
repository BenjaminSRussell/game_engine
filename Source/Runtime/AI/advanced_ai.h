// Advanced AI System
// Provides comprehensive artificial intelligence capabilities

#ifndef ADVANCED_AI_H
#define ADVANCED_AI_H

#include <stdbool.h>
#include <stdint.h>
#include <float.h>

#ifdef __cplusplus
extern "C" {
#endif

// Forward declarations
typedef struct AIWorld AIWorld;
typedef struct AIAgent AIAgent;
typedef struct AIState AIState;
typedef struct AIBehavior AIBehavior;

// AI agent types
typedef enum {
    AI_AGENT_TYPE_PASSIVE,
    AI_AGENT_TYPE_NEUTRAL,
    AI_AGENT_TYPE_HOSTILE,
    AI_AGENT_TYPE_FRIENDLY,
    AI_AGENT_TYPE_PLAYER_CONTROLLED
} AIAgentType;

// AI behavior states
typedef enum {
    AI_STATE_IDLE,
    AI_STATE_PATROL,
    AI_STATE_CHASE,
    AI_STATE_FLEE,
    AI_STATE_ATTACK,
    AI_STATE_SEARCH,
    AI_STATE_INTERACT,
    AI_STATE_SLEEP
} AIStateType;

// AI perception types
typedef enum {
    AI_PERCEPTION_VISUAL,
    AI_PERCEPTION_AUDIO,
    AI_PERCEPTION_SMELL,
    AI_PERCEPTION_RADAR
} AIPerceptionType;

// Vector3 for AI calculations
typedef struct {
    float x, y, z;
} AIVector3;

// AI perception data
typedef struct {
    AIPerceptionType type;
    AIVector3 position;
    AIVector3 direction;
    float strength;
    float range;
    uint64_t sourceId;
    float timestamp;
} AIPerception;

// AI memory for storing past events
typedef struct {
    AIVector3 position;
    float timestamp;
    float importance;
    uint32_t eventType;
    void* data;
} AIMemory;

// AI pathfinding node
typedef struct {
    AIVector3 position;
    float gCost; // Cost from start
    float hCost; // Heuristic cost to goal
    float fCost; // Total cost
    struct PathNode* parent;
    bool isWalkable;
} PathNode;

// AI navigation mesh
typedef struct {
    AIVector3* vertices;
    uint32_t vertexCount;
    uint32_t* indices;
    uint32_t indexCount;
    AIVector3* normals;
    uint32_t normalCount;
} AINavigationMesh;

// AI behavior tree node
typedef enum {
    AI_NODE_SELECTOR,
    AI_NODE_SEQUENCE,
    AI_NODE_PARALLEL,
    AI_NODE_CONDITION,
    AI_NODE_ACTION
} AIBehaviorNodeType;

typedef struct AIBehaviorNode {
    AIBehaviorNodeType type;
    bool (*condition)(AIAgent* agent);
    bool (*action)(AIAgent* agent, float deltaTime);
    struct AIBehaviorNode** children;
    uint32_t childCount;
    bool isActive;
} AIBehaviorNode;

// AI agent structure
struct AIAgent {
    uint64_t id;
    AIAgentType type;
    AIStateType currentState;
    AIStateType previousState;
    
    // Position and movement
    AIVector3 position;
    AIVector3 velocity;
    AIVector3 acceleration;
    AIVector3 targetPosition;
    float maxSpeed;
    float maxForce;
    
    // Perception
    AIPerception* perceptions;
    uint32_t perceptionCount;
    uint32_t maxPerceptions;
    float visualRange;
    float audioRange;
    float fieldOfView;
    
    // Memory
    AIMemory* memories;
    uint32_t memoryCount;
    uint32_t maxMemories;
    float memoryDecayTime;
    
    // Pathfinding
    PathNode* currentPath;
    uint32_t pathLength;
    uint32_t currentPathIndex;
    bool hasPath;
    
    // Behavior tree
    AIBehaviorNode* behaviorTree;
    AIBehaviorNode* currentBehavior;
    
    // State timers
    float stateTimer;
    float decisionTimer;
    float decisionInterval;
    
    // Health and status
    float health;
    float maxHealth;
    bool isAlive;
    bool isActive;
    
    // User data
    void* userData;
};

// AI world for managing all agents
struct AIWorld {
    AIAgent* agents;
    uint32_t agentCount;
    uint32_t maxAgents;
    
    AINavigationMesh* navMesh;
    
    // Global AI settings
    float globalTimeScale;
    bool enablePathfinding;
    bool enablePerception;
    bool enableBehaviorTrees;
    
    // Spatial partitioning for optimization
    void* spatialGrid;
    float gridSize;
};

// Function declarations
AIWorld* ai_world_create(uint32_t maxAgents);
void ai_world_destroy(AIWorld* world);
void ai_world_update(AIWorld* world, float deltaTime);

// Agent management
uint64_t ai_agent_create(AIWorld* world, AIAgentType type, const AIVector3* position);
void ai_agent_destroy(AIWorld* world, uint64_t agentId);
AIAgent* ai_agent_get(AIWorld* world, uint64_t agentId);
void ai_agent_set_position(AIWorld* world, uint64_t agentId, const AIVector3* position);
void ai_agent_set_target(AIWorld* world, uint64_t agentId, const AIVector3* target);

// State management
void ai_agent_set_state(AIAgent* agent, AIStateType newState);
void ai_agent_update_state(AIAgent* agent, float deltaTime);

// Perception system
void ai_agent_add_perception(AIAgent* agent, const AIPerception* perception);
void ai_agent_update_perceptions(AIAgent* agent, float deltaTime);
bool ai_agent_can_see(AIAgent* agent, const AIVector3* target, float maxDistance);
bool ai_agent_can_hear(AIAgent* agent, const AIVector3* soundSource, float volume);

// Memory system
void ai_agent_add_memory(AIAgent* agent, const AIMemory* memory);
void ai_agent_update_memories(AIAgent* agent, float deltaTime);
AIMemory* ai_agent_get_relevant_memory(AIAgent* agent, const AIVector3* position, float maxDistance);

// Pathfinding
bool ai_agent_find_path(AIAgent* agent, const AIVector3* start, const AIVector3* goal);
void ai_agent_follow_path(AIAgent* agent, float deltaTime);
void ai_agent_clear_path(AIAgent* agent);
bool ai_agent_has_reached_target(AIAgent* agent, float tolerance);

// Behavior trees
AIBehaviorNode* ai_behavior_create_selector(AIBehaviorNode** children, uint32_t childCount);
AIBehaviorNode* ai_behavior_create_sequence(AIBehaviorNode** children, uint32_t childCount);
AIBehaviorNode* ai_behavior_create_condition(bool (*condition)(AIAgent* agent));
AIBehaviorNode* ai_behavior_create_action(bool (*action)(AIAgent* agent, float deltaTime));
void ai_behavior_destroy(AIBehaviorNode* node);
bool ai_behavior_execute(AIBehaviorNode* node, AIAgent* agent, float deltaTime);

// Navigation mesh
AINavigationMesh* ai_navmesh_create(const AIVector3* vertices, uint32_t vertexCount, const uint32_t* indices, uint32_t indexCount);
void ai_navmesh_destroy(AINavigationMesh* navMesh);
bool ai_navmesh_find_path(AINavigationMesh* navMesh, const AIVector3* start, const AIVector3* goal, PathNode** path, uint32_t* pathLength);

// Utility functions
AIVector3 ai_vector3_make(float x, float y, float z);
AIVector3 ai_vector3_add(const AIVector3* a, const AIVector3* b);
AIVector3 ai_vector3_subtract(const AIVector3* a, const AIVector3* b);
AIVector3 ai_vector3_multiply(const AIVector3* v, float scalar);
float ai_vector3_distance(const AIVector3* a, const AIVector3* b);
float ai_vector3_length(const AIVector3* v);
AIVector3 ai_vector3_normalize(const AIVector3* v);
float ai_vector3_dot(const AIVector3* a, const AIVector3* b);

// Built-in behaviors
bool ai_behavior_idle(AIAgent* agent, float deltaTime);
bool ai_behavior_patrol(AIAgent* agent, float deltaTime);
bool ai_behavior_chase(AIAgent* agent, float deltaTime);
bool ai_behavior_flee(AIAgent* agent, float deltaTime);
bool ai_behavior_attack(AIAgent* agent, float deltaTime);
bool ai_behavior_search(AIAgent* agent, float deltaTime);

// Decision making
AIStateType ai_agent_decide_state(AIAgent* agent);
void ai_agent_evaluate_threats(AIAgent* agent);
void ai_agent_evaluate_opportunities(AIAgent* agent);

#ifdef __cplusplus
}
#endif

#endif // ADVANCED_AI_H
