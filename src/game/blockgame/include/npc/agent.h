// include/npc/agent.h
//
// Purpose: Defines a comprehensive Non-Player Character (NPC) agent system,
// encompassing agent types, states, behavior tree structures, and the `AgentComponent`
// to hold all data pertinent to an individual agent's AI, movement, combat, inventory,
// work activities, pathfinding, and personality traits. It also provides the `AgentManager`
// for centralized management and parallel processing of all agents.
//
// Public APIs:
// - `AgentType`: Enumeration categorizing different kinds of agents (e.g., Villager, Guard, Enemy).
// - `AgentState`: Enumeration defining the various behavioral states an agent can be in
//   (e.g., Idle, Wandering, Attacking, Working).
// - `BTNodeType`: Enumeration for different types of nodes in a Behavior Tree (Sequence, Selector, Condition, Action).
// - `BTNode`: Structure representing a node in a behavior tree, with function pointers for
//   conditions and actions, and children nodes for tree traversal.
// - `AgentComponent`: A detailed structure for an individual agent, including:
//   - `type`, `state`, `entity_id`: Basic identification and current state.
//   - `behavior_tree`: Pointer to the agent's behavior tree root.
//   - `target_position`, `target_entity`, `awareness_radius`, `detection_radius`: AI-related fields.
//   - `velocity`, `speed`, `max_speed`, `acceleration`: Movement parameters.
//   - `health`, `max_health`, `damage`, `attack_range`, `attack_cooldown`: Combat statistics.
//   - `inventory`: Simple array for agent inventory.
//   - `work_position`, `work_timer`, `is_working`: Work/activity related fields.
//   - `path`, `path_length`, `current_path_index`: Pathfinding data.
//   - `hunger`, `thirst`, `energy`, `happiness`: Personality/needs metrics.
// - `AgentManager`: Structure managing a collection of `AgentComponent` instances,
//   leveraging a `ThreadPool` for concurrent AI updates.
// - `agent_manager_init`: Initializes the `AgentManager` with a specified capacity and thread pool.
// - `agent_manager_free`: Frees resources held by the `AgentManager`.
// - `agent_create`: Spawns a new agent with a given type and position.
// - `agent_manager_update`: Updates all agents, including their behaviors, movement, and combat.
// - `agent_update_behavior`, `agent_update_movement`, `agent_update_combat`, `agent_update_pathfinding`:
//   Core functions for updating individual agent aspects.
// - `bt_node_create`, `bt_node_add_child`, `bt_node_execute`: API for constructing and executing behavior trees.
// - `agent_find_path`: Initiates pathfinding for an agent.
//
// Ownership: The `AgentManager` owns the collection of `AgentComponent` instances.
// `AgentComponent` instances manage their internal data, including potentially dynamically
// allocated `path` and `behavior_tree` structures.
//
// Invariants:
// - An `AgentManager` must be initialized before creating or updating agents.
// - `entity_id` within `AgentComponent` must be a valid ECS entity ID.
// - `ECSWorld`, `PhysicsWorld`, and `ThreadPool` instances must be valid and initialized when passed.
// - Behavior tree nodes must be correctly structured to avoid infinite loops or errors.
//
#ifndef AGENT_H
#define AGENT_H

#include "../game_common.h"
#include <ecs/ecs.h>
#include <math/vec3.h>
#include <math/quat.h>
#include <physics/physics.h>
#include "../thread/job.h"

// Agent types
typedef enum {
    AGENT_TYPE_VILLAGER,
    AGENT_TYPE_GUARD,
    AGENT_TYPE_MERCHANT,
    AGENT_TYPE_ENEMY,
    AGENT_TYPE_ANIMAL,
    AGENT_TYPE_COUNT
} AgentType;

// Agent state
typedef enum {
    AGENT_STATE_IDLE,
    AGENT_STATE_WANDERING,
    AGENT_STATE_PATROLLING,
    AGENT_STATE_FOLLOWING,
    AGENT_STATE_FLEEING,
    AGENT_STATE_ATTACKING,
    AGENT_STATE_WORKING,
    AGENT_STATE_SLEEPING
} AgentState;

// Agent behavior tree node types
typedef enum {
    BT_NODE_SEQUENCE,
    BT_NODE_SELECTOR,
    BT_NODE_CONDITION,
    BT_NODE_ACTION
} BTNodeType;

// Behavior tree node
typedef struct BTNode {
    BTNodeType type;
    bool (*condition)(void *agent, void *context);
    void (*action)(void *agent, void *context);
    struct BTNode *children;
    u32 child_count;
} BTNode;

// Agent component
typedef struct {
    AgentType type;
    AgentState state;
    EntityID entity_id;
    
    // AI
    BTNode *behavior_tree;
    Vec3 target_position;
    EntityID target_entity;
    f32 awareness_radius;
    f32 detection_radius;
    
    // Movement
    Vec3 velocity;
    f32 speed;
    f32 max_speed;
    f32 acceleration;
    
    // Combat
    f32 health;
    f32 max_health;
    f32 damage;
    f32 attack_range;
    f32 attack_cooldown;
    f32 last_attack_time;
    
    // Inventory (for merchants, workers)
    u32 inventory[16];
    u32 inventory_count;
    
    // Work/activity
    Vec3 work_position;
    f32 work_timer;
    bool is_working;
    
    // Pathfinding
    Vec3 *path;
    u32 path_length;
    u32 current_path_index;
    
    // Personality/needs
    f32 hunger;
    f32 thirst;
    f32 energy;
    f32 happiness;
} AgentComponent;

// Agent manager
typedef struct {
    AgentComponent *agents;
    u32 count;
    u32 capacity;
    ThreadPool *thread_pool; // For parallel AI processing
} AgentManager;

// Initialize agent system
void agent_manager_init(AgentManager *manager, u32 capacity, ThreadPool *thread_pool);
void agent_manager_free(AgentManager *manager);

// Create agent
EntityID agent_create(AgentManager *manager, ECSWorld *ecs, AgentType type, Vec3 position);

// Update agents (parallel)
void agent_manager_update(AgentManager *manager, ECSWorld *ecs, PhysicsWorld *physics, f32 delta_time);

// Agent behavior functions
void agent_update_behavior(AgentComponent *agent, ECSWorld *ecs, PhysicsWorld *physics, f32 delta_time);
void agent_update_movement(AgentComponent *agent, PhysicsWorld *physics, f32 delta_time);
void agent_update_combat(AgentComponent *agent, ECSWorld *ecs, f32 delta_time);
void agent_update_pathfinding(AgentComponent *agent, Vec3 target);

// Behavior tree functions
BTNode *bt_node_create(BTNodeType type);
void bt_node_add_child(BTNode *parent, BTNode *child);
bool bt_node_execute(BTNode *node, void *agent, void *context);

// Pathfinding
bool agent_find_path(AgentComponent *agent, Vec3 start, Vec3 end, Vec3 **path, u32 *path_length);

#endif // AGENT_H

