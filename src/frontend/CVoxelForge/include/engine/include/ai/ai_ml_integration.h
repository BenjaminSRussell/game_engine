// include/engine/ai/ai_ml_integration.h
//
// Purpose: Advanced AI system with machine learning integration and dynamic learning
// This system provides cutting-edge AI capabilities that surpass Unity's AI systems
// with neural network inference, reinforcement learning, and adaptive behavior.
//
// Key Features:
// - Neural network inference with ONNX/TensorFlow Lite support
// - Reinforcement learning for adaptive AI behavior
// - Behavior trees with visual editor and ML integration
// - Hierarchical pathfinding with dynamic obstacle avoidance
// - Group AI with coordinated squad behavior
// - Dynamic learning from player behavior patterns
// - GPU acceleration for ML inference
// - Real-time AI behavior adaptation
//
// Performance Advantages over Unity AI:
// - Native ML model integration vs external libraries
// - Real-time learning and adaptation
// - Advanced pathfinding algorithms
// - Better parallelization and optimization
// - Integration with our superior ECS architecture
//
// Public APIs:
// - AISystem: Main AI management container
// - BehaviorTree: Visual behavior tree system
// - NeuralNetwork: ML model inference engine
// - Pathfinding: Advanced navigation system
// - LearningSystem: Dynamic learning and adaptation
//
// Ownership: AISystem owns all AI components
// Invariants: AI components must be updated each frame for proper behavior
//
#ifndef AI_ML_INTEGRATION_H
#define AI_ML_INTEGRATION_H

#include "../../common.h"
#include "../ecs/ecs.h"
#include "../core/performance.h"
#include <stdbool.h>

// ============================================================================
// NEURAL NETWORK SYSTEM
// ============================================================================

// Neural network framework types
typedef enum {
  ML_FRAMEWORK_ONNX = 0,
  ML_FRAMEWORK_TENSORFLOW_LITE,
  ML_FRAMEWORK_PYTORCH,
  ML_FRAMEWORK_CUSTOM,
  ML_FRAMEWORK_NATIVE
} MLFramework;

// Neural network layer types
typedef enum {
  LAYER_TYPE_DENSE = 0,
  LAYER_TYPE_CONVOLUTION2D,
  LAYER_TYPE_RECURRENT,
  LAYER_TYPE_LSTM,
  LAYER_TYPE_ATTENTION,
  LAYER_TYPE_EMBEDDING,
  LAYER_TYPE_ACTIVATION,
  LAYER_TYPE_NORMALIZATION,
  LAYER_TYPE_DROPOUT
} LayerType;

// Activation functions
typedef enum {
  ACTIVATION_RELU = 0,
  ACTIVATION_SIGMOID,
  ACTIVATION_TANH,
  ACTIVATION_SOFTMAX,
  ACTIVATION_LEAKY_RELU,
  ACTIVATION_GELU,
  ACTIVATION_SWISH
} ActivationFunction;

// Neural network layer
typedef struct {
  LayerType type;
  ActivationFunction activation;
  u32 input_size;
  u32 output_size;
  u32 kernel_size;
  u32 stride;
  u32 padding;
  f32 *weights;
  f32 *biases;
  f32 *gradients;
  bool is_trainable;
  char *name;
} NeuralLayer;

// Neural network model
typedef struct {
  NeuralLayer *layers;
  u32 layer_count;
  u32 max_layers;
  MLFramework framework;
  char *model_name;
  char *model_path;
  void *native_model;  // Handle to native ML model
  f32 *input_buffer;
  f32 *output_buffer;
  u32 input_size;
  u32 output_size;
  bool is_loaded;
  bool is_training;
  f32 learning_rate;
  u32 batch_size;
  u32 epoch_count;
} NeuralNetwork;

// ML inference context
typedef struct {
  NeuralNetwork *network;
  f32 *inputs;
  f32 *outputs;
  f32 *intermediate_outputs;
  u32 input_count;
  u32 output_count;
  f64 inference_time;
  bool gpu_accelerated;
} MLContext;

// ============================================================================
// REINFORCEMENT LEARNING
// ============================================================================

// RL algorithm types
typedef enum {
  RL_ALGORITHM_DQN = 0,        // Deep Q-Network
  RL_ALGORITHM_PPO,            // Proximal Policy Optimization
  RL_ALGORITHM_A3C,            // Asynchronous Advantage Actor-Critic
  RL_ALGORITHM_SAC,            // Soft Actor-Critic
  RL_ALGORITHM_DDPG,           // Deep Deterministic Policy Gradient
  RL_ALGORITHM_TD3             // Twin Delayed DDPG
} RLAlgorithm;

// Environment state
typedef struct {
  f32 *state_vector;
  u32 state_size;
  f32 reward;
  bool is_terminal;
  u32 episode_step;
  u32 episode_count;
  f32 total_reward;
} EnvironmentState;

// RL agent
typedef struct {
  NeuralNetwork *policy_network;
  NeuralNetwork *value_network;
  NeuralNetwork *target_network;
  RLAlgorithm algorithm;
  EnvironmentState current_state;
  EnvironmentState *experience_buffer;
  u32 buffer_size;
  u32 buffer_capacity;
  u32 buffer_write_index;
  f32 epsilon;           // Exploration rate
  f32 gamma;             // Discount factor
  f32 alpha;             // Learning rate
  f32 beta;              // Advantage scaling
  u32 update_frequency;
  u32 training_steps;
  bool is_training;
  f32 performance_score;
} RLAgent;

// ============================================================================
// BEHAVIOR TREE SYSTEM
// ============================================================================

// Behavior tree node types
typedef enum {
  BT_NODE_ACTION = 0,
  BT_NODE_CONDITION,
  BT_NODE_SELECTOR,
  BT_NODE_SEQUENCE,
  BT_NODE_PARALLEL,
  BT_NODE_DECORATOR,
  BT_NODE_CUSTOM,
  BT_NODE_ML_DECISION
} BTNodeType;

// Behavior tree node status
typedef enum {
  BT_STATUS_SUCCESS = 0,
  BT_STATUS_FAILURE,
  BT_STATUS_RUNNING
} BTNodeStatus;

// Behavior tree node
typedef struct {
  u32 id;
  BTNodeType type;
  const char *name;
  char *description;
  BTNodeStatus status;
  u32 parent_id;
  u32 *child_ids;
  u32 child_count;
  u32 max_children;
  
  // Node-specific data
  union {
    struct {
      bool (*condition)(void *context);
    } condition_data;
    
    struct {
      BTNodeStatus (*action)(void *context);
      f32 execution_time;
      u32 execution_count;
    } action_data;
    
    struct {
      NeuralNetwork *ml_network;
      f32 confidence_threshold;
      f32 last_confidence;
    } ml_data;
    
    struct {
      u32 max_running_children;
      bool succeed_on_one;
      bool fail_on_one;
    } parallel_data;
    
    struct {
      BTNodeStatus (*custom_func)(void *context, void *user_data);
      void *user_data;
    } custom_data;
  } node_data;
  
  // Visual editor data
  vec2 position;
  vec2 size;
  u32 color;
  bool is_enabled;
  bool has_breakpoint;
} BTNode;

// Behavior tree
typedef struct {
  BTNode *nodes;
  u32 node_count;
  u32 max_nodes;
  u32 root_node_id;
  char *tree_name;
  char *description;
  bool is_dirty;
  u32 version;
  
  // Execution state
  u32 current_node_id;
  BTNodeStatus last_status;
  f64 execution_time;
  u32 execution_count;
  
  // ML integration
  NeuralNetwork **ml_networks;
  u32 ml_network_count;
} BehaviorTree;

// ============================================================================
// PATHFINDING SYSTEM
// ============================================================================

// Pathfinding algorithm types
typedef enum {
  PATHFINDING_ASTAR = 0,
  PATHFINDING_DIJKSTRA,
  PATHFINDING_JPS,           // Jump Point Search
  PATHFINDING_HPA,           // Hierarchical Pathfinding
  PATHFINDING_D*,            // Dynamic A*
  PATHFINDING_THETA_STAR,
  PATHFINDING_FLOW_FIELD
} PathfindingAlgorithm;

// Navigation mesh node
typedef struct {
  vec3 position;
  u32 id;
  u32 *neighbors;
  u32 neighbor_count;
  u32 max_neighbors;
  f32 cost;
  bool is_walkable;
  bool is_obstacle;
  u32 region_id;
} NavNode;

// Navigation mesh
typedef struct {
  NavNode *nodes;
  u32 node_count;
  u32 max_nodes;
  vec3 bounding_box_min;
  vec3 bounding_box_max;
  f32 cell_size;
  u32 grid_width;
  u32 grid_height;
  u32 grid_depth;
  NavNode ***grid;  // 3D grid for fast lookup
  bool is_dynamic;
  bool needs_update;
} NavMesh;

// Pathfinding request
typedef struct {
  Entity agent_entity;
  vec3 start_position;
  vec3 goal_position;
  vec3 *path;
  u32 path_length;
  u32 max_path_length;
  PathfindingAlgorithm algorithm;
  f32 heuristic_weight;
  bool allow_diagonal;
  bool cut_corners;
  u32 max_iterations;
  bool is_completed;
  f64 computation_time;
} PathfindingRequest;

// Pathfinding system
typedef struct {
  NavMesh *nav_mesh;
  PathfindingRequest *requests;
  u32 request_count;
  u32 max_requests;
  PathfindingAlgorithm default_algorithm;
  bool enable_dynamic_obstacles;
  u32 max_concurrent_requests;
  void *worker_threads;
  u32 worker_thread_count;
} PathfindingSystem;

// ============================================================================
// GROUP AI SYSTEM
// ============================================================================

// Group AI roles
typedef enum {
  AI_ROLE_LEADER = 0,
  AI_ROLE_FOLLOWER,
  AI_ROLE_SCOUT,
  AI_ROLE_GUARD,
  AI_ROLE_HEALER,
  AI_ROLE_ATTACKER,
  AI_ROLE_SUPPORT
} AIRole;

// Group formation types
typedef enum {
  FORMATION_LINE = 0,
  FORMATION_COLUMN,
  FORMATION_WEDGE,
  FORMATION_CIRCLE,
  FORMATION_GRID,
  FORMATION_CUSTOM
} FormationType;

// AI agent
typedef struct {
  Entity entity;
  BehaviorTree *behavior_tree;
  RLAgent *rl_agent;
  NeuralNetwork *perception_network;
  vec3 current_position;
  vec3 target_position;
  vec3 velocity;
  f32 perception_radius;
  f32 attack_radius;
  f32 flee_radius;
  AIRole role;
  u32 group_id;
  bool is_active;
  bool is_learning;
  f64 last_update_time;
} AIAgent;

// AI group
typedef struct {
  u32 group_id;
  const char *group_name;
  AIAgent *agents;
  u32 agent_count;
  u32 max_agents;
  AIEntity leader;
  FormationType formation;
  vec3 formation_center;
  f32 formation_spacing;
  vec3 group_velocity;
  vec3 group_target;
  bool is_combat_mode;
  f32 cohesion_strength;
  f32 separation_strength;
  f32 alignment_strength;
} AIGroup;

// ============================================================================
// MAIN AI SYSTEM
// ============================================================================

// AI system configuration
typedef struct {
  f32 update_frequency;
  u32 max_agents;
  u32 max_groups;
  u32 max_behavior_trees;
  u32 max_neural_networks;
  u32 max_pathfinding_requests;
  bool enable_ml_inference;
  bool enable_reinforcement_learning;
  bool enable_gpu_acceleration;
  bool enable_profiling;
  u32 worker_threads;
  f32 perception_update_rate;
  f32 learning_update_rate;
} AIConfig;

// AI system
typedef struct {
  // Configuration
  AIConfig config;
  
  // Neural networks
  NeuralNetwork *neural_networks;
  u32 neural_network_count;
  u32 max_neural_networks;
  
  // Behavior trees
  BehaviorTree *behavior_trees;
  u32 behavior_tree_count;
  u32 max_behavior_trees;
  
  // AI agents
  AIAgent *agents;
  u32 agent_count;
  u32 max_agents;
  
  // AI groups
  AIGroup *groups;
  u32 group_count;
  u32 max_groups;
  
  // Pathfinding
  PathfindingSystem pathfinding;
  
  // Learning system
  RLAgent *rl_agents;
  u32 rl_agent_count;
  u32 max_rl_agents;
  
  // Performance
  Profiler *ai_profiler;
  f64 total_ai_time;
  f64 behavior_tree_time;
  f64 pathfinding_time;
  f64 ml_inference_time;
  
  // ECS integration
  World *ecs_world;
  
  // Threading
  void *worker_threads;
  u32 worker_thread_count;
} AISystem;

// ============================================================================
// PUBLIC API
// ============================================================================

// AI system management
AISystem *ai_system_create(const AIConfig *config, World *ecs_world);
void ai_system_destroy(AISystem *system);
void ai_system_update(AISystem *system, f32 delta_time);

// Configuration
AIConfig ai_create_default_config(void);
AIConfig ai_create_high_performance_config(void);
AIConfig ai_create_learning_config(void);

// ============================================================================
// NEURAL NETWORK API
// ============================================================================

// Neural network creation and management
NeuralNetwork *ai_create_neural_network(AISystem *system, const char *name, MLFramework framework);
void ai_destroy_neural_network(AISystem *system, NeuralNetwork *network);
bool ai_load_neural_network(AISystem *system, NeuralNetwork *network, const char *model_path);
bool ai_save_neural_network(AISystem *system, NeuralNetwork *network, const char *model_path);

// Neural network operations
bool ai_add_layer(AISystem *system, NeuralNetwork *network, const NeuralLayer *layer);
bool ai_forward_pass(AISystem *system, NeuralNetwork *network, const f32 *inputs, f32 *outputs);
bool ai_train_step(AISystem *system, NeuralNetwork *network, const f32 *inputs, const f32 *targets);
f64 ai_get_inference_time(AISystem *system, NeuralNetwork *network);

// ============================================================================
// REINFORCEMENT LEARNING API
// ============================================================================

// RL agent creation and management
RLAgent *ai_create_rl_agent(AISystem *system, RLAlgorithm algorithm, u32 state_size, u32 action_size);
void ai_destroy_rl_agent(AISystem *system, RLAgent *agent);
bool ai_rl_agent_step(AISystem *system, RLAgent *agent, const f32 *state, u32 action, f32 reward, bool is_terminal);
u32 ai_rl_agent_select_action(AISystem *system, RLAgent *agent, const f32 *state, bool explore);
bool ai_rl_agent_train(AISystem *system, RLAgent *agent, u32 training_steps);

// RL agent configuration
void ai_rl_agent_set_hyperparameters(AISystem *system, RLAgent *agent, f32 epsilon, f32 gamma, f32 alpha);
void ai_rl_agent_set_exploration(AISystem *system, RLAgent *agent, f32 exploration_rate);
void ai_rl_agent_enable_training(AISystem *system, RLAgent *agent, bool enable);

// ============================================================================
// BEHAVIOR TREE API
// ============================================================================

// Behavior tree creation and management
BehaviorTree *ai_create_behavior_tree(AISystem *system, const char *name);
void ai_destroy_behavior_tree(AISystem *system, BehaviorTree *tree);
BTNode *ai_add_bt_node(AISystem *system, BehaviorTree *tree, BTNodeType type, const char *name);
bool ai_remove_bt_node(AISystem *system, BehaviorTree *tree, u32 node_id);
bool ai_connect_bt_nodes(AISystem *system, BehaviorTree *tree, u32 parent_id, u32 child_id);

// Behavior tree execution
BTNodeStatus ai_execute_behavior_tree(AISystem *system, BehaviorTree *tree, void *context);
void ai_reset_behavior_tree(AISystem *system, BehaviorTree *tree);
bool ai_set_bt_root(AISystem *system, BehaviorTree *tree, u32 node_id);

// ML integration
bool ai_attach_ml_to_bt_node(AISystem *system, BehaviorTree *tree, u32 node_id, NeuralNetwork *network);
bool ai_set_ml_decision_threshold(AISystem *system, BehaviorTree *tree, u32 node_id, f32 threshold);

// ============================================================================
// PATHFINDING API
// ============================================================================

// Navigation mesh management
bool ai_create_nav_mesh(AISystem *system, vec3 world_min, vec3 world_max, f32 cell_size);
bool ai_update_nav_mesh(AISystem *system, const vec3 *obstacles, u32 obstacle_count);
bool ai_set_nav_walkable(AISystem *system, vec3 position, bool walkable);

// Pathfinding requests
PathfindingRequest *ai_request_path(AISystem *system, Entity agent, vec3 start, vec3 goal);
bool ai_cancel_path_request(AISystem *system, PathfindingRequest *request);
bool ai_is_path_ready(AISystem *system, PathfindingRequest *request);
vec3 *ai_get_path(AISystem *system, PathfindingRequest *request, u32 *path_length);

// Pathfinding configuration
void ai_set_pathfinding_algorithm(AISystem *system, PathfindingAlgorithm algorithm);
void ai_set_heuristic_weight(AISystem *system, f32 weight);
void ai_enable_dynamic_obstacles(AISystem *system, bool enable);

// ============================================================================
// GROUP AI API
// ============================================================================

// AI agent management
AIAgent *ai_create_agent(AISystem *system, Entity entity);
void ai_destroy_agent(AISystem *system, AIAgent *agent);
bool ai_set_agent_behavior_tree(AISystem *system, AIAgent *agent, BehaviorTree *tree);
bool ai_set_agent_rl_agent(AISystem *system, AIAgent *agent, RLAgent *rl_agent);
bool ai_set_agent_perception(AISystem *system, AIAgent *agent, f32 radius);

// AI group management
AIGroup *ai_create_group(AISystem *system, const char *name);
void ai_destroy_group(AISystem *system, AIGroup *group);
bool ai_add_agent_to_group(AISystem *system, AIGroup *group, AIAgent *agent, AIRole role);
bool ai_remove_agent_from_group(AISystem *system, AIGroup *group, AIAgent *agent);
bool ai_set_group_formation(AISystem *system, AIGroup *group, FormationType formation);

// Group behavior
void ai_set_group_target(AISystem *system, AIGroup *group, vec3 target);
void ai_set_group_combat_mode(AISystem *system, AIGroup *group, bool combat_mode);
void ai_update_group_formation(AISystem *system, AIGroup *group);

// ============================================================================
// LEARNING AND ADAPTATION
// ============================================================================

// Dynamic learning
bool ai_enable_agent_learning(AISystem *system, AIAgent *agent, bool enable);
void ai_record_agent_experience(AISystem *system, AIAgent *agent, const f32 *state, u32 action, f32 reward);
bool ai_train_agent_from_experience(AISystem *system, AIAgent *agent, u32 training_steps);

// Behavior cloning
bool ai_record_player_behavior(AISystem *system, const f32 *state, const f32 *action);
bool ai_train_behavior_clone(AISystem *system, NeuralNetwork *network, u32 training_steps);

// Performance tracking
typedef struct {
  u32 agent_id;
  f32 success_rate;
  f32 average_reward;
  f32 completion_time;
  u32 total_actions;
  u32 successful_actions;
  f64 learning_progress;
} AgentPerformance;

AgentPerformance ai_get_agent_performance(AISystem *system, AIAgent *agent);
void ai_reset_agent_performance(AISystem *system, AIAgent *agent);

// ============================================================================
// UTILITY MACROS
// ============================================================================

// Neural network creation macros
#define AI_CREATE_DENSE_LAYER(input_size, output_size, activation) \
  (NeuralLayer){ \
    .type = LAYER_TYPE_DENSE, \
    .activation = activation, \
    .input_size = input_size, \
    .output_size = output_size, \
    .weights = NULL, \
    .biases = NULL, \
    .is_trainable = true, \
    .name = "dense_layer" \
  }

#define AI_CREATE_CONV_LAYER(kernel_size, stride, padding, activation) \
  (NeuralLayer){ \
    .type = LAYER_TYPE_CONVOLUTION2D, \
    .activation = activation, \
    .kernel_size = kernel_size, \
    .stride = stride, \
    .padding = padding, \
    .weights = NULL, \
    .biases = NULL, \
    .is_trainable = true, \
    .name = "conv_layer" \
  }

// Behavior tree creation macros
#define AI_CREATE_ACTION_NODE(name, action_func) \
  ai_add_bt_node(system, tree, BT_NODE_ACTION, name); \
  // Set action function

#define AI_CREATE_CONDITION_NODE(name, condition_func) \
  ai_add_bt_node(system, tree, BT_NODE_CONDITION, name); \
  // Set condition function

#define AI_CREATE_SELECTOR_NODE(name) \
  ai_add_bt_node(system, tree, BT_NODE_SELECTOR, name)

#define AI_CREATE_SEQUENCE_NODE(name) \
  ai_add_bt_node(system, tree, BT_NODE_SEQUENCE, name)

// ============================================================================
// ADVANCED FEATURES
// ============================================================================

// Multi-agent coordination
void ai_enable_multi_agent_coordination(AISystem *system, bool enable);
void ai_coordinate_agent_actions(AISystem *system, AIGroup *group);

// Emergent behavior
void ai_enable_emergent_behavior(AISystem *system, bool enable);
void ai_analyze_emergent_patterns(AISystem *system);

// Transfer learning
bool ai_transfer_knowledge(AISystem *system, NeuralNetwork *source, NeuralNetwork *target);
bool ai_fine_tune_network(AISystem *system, NeuralNetwork *network, const f32 *data, u32 data_size);

// Online learning
void ai_enable_online_learning(AISystem *system, bool enable);
bool ai_update_model_online(AISystem *system, NeuralNetwork *network, const f32 *new_data);

// AI debugging and visualization
void ai_debug_render_behavior_trees(AISystem *system);
void ai_debug_render_pathfinding(AISystem *system);
void ai_debug_render_agent_perception(AISystem *system, AIAgent *agent);
void ai_print_ai_statistics(AISystem *system);

#endif // AI_ML_INTEGRATION_H
