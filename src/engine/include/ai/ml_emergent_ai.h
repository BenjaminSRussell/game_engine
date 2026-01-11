#ifndef ML_EMERGENT_AI_H
#define ML_EMERGENT_AI_H

#include <core/types.h>
#include <stdbool.h>
#include <stddef.h>

// Forward declarations
typedef struct Vec3 Vec3;
typedef struct Vec4 Vec4;

// Neural network types
typedef enum {
    NEURAL_NETWORK_FEEDFORWARD,
    NEURAL_NETWORK_RECURRENT,
    NEURAL_NETWORK_CONVOLUTIONAL,
    NEURAL_NETWORK_LSTM,
    NEURAL_NETWORK_GRU
} NeuralNetworkType;

// Activation functions
typedef enum {
    ACTIVATION_RELU,
    ACTIVATION_SIGMOID,
    ACTIVATION_TANH,
    ACTIVATION_LEAKY_RELU,
    ACTIVATION_SOFTMAX,
    ACTIVATION_LINEAR
} ActivationFunction;

// Learning algorithms
typedef enum {
    LEARNING_GRADIENT_DESCENT,
    LEARNING_ADAM,
    LEARNING_RMSPROP,
    LEARNING_ADA_GRAD,
    LEARNING_EVOLUTIONARY,
    LEARNING_REINFORCEMENT
} LearningAlgorithm;

// Behavior types
typedef enum {
    BEHAVIOR_EXPLORATION,
    BEHAVIOR_SOCIAL,
    BEHAVIOR_SURVIVAL,
    BEHAVIOR_RESOURCE_GATHERING,
    BEHAVIOR_TERRITORY_DEFENSE,
    BEHAVIOR_COOPERATION,
    BEHAVIOR_COMPETITION,
    BEHAVIOR_EMERGENT
} BehaviorType;

// Neural network layer
typedef struct {
    u32 input_size;
    u32 output_size;
    ActivationFunction activation;
    
    float* weights;
    float* biases;
    float* weight_gradients;
    float* bias_gradients;
    
    // Layer-specific data
    float* activation_cache;
    float* delta_cache;
    
    // Convolutional layer data
    u32 filter_width;
    u32 filter_height;
    u32 filter_count;
    u32 stride;
    u32 padding;
    
    // Recurrent layer data
    float* hidden_state;
    float* cell_state;
    float* input_gate_weights;
    float* forget_gate_weights;
    float* output_gate_weights;
    float* candidate_weights;
    
} NeuralLayer;

// Neural network
typedef struct {
    char name[64];
    u32 network_id;
    NeuralNetworkType type;
    
    NeuralLayer* layers;
    u32 layer_count;
    u32 layer_capacity;
    
    // Training data
    float* input_data;
    float* output_data;
    u32 training_samples;
    u32 input_size;
    u32 output_size;
    
    // Learning parameters
    LearningAlgorithm learning_algorithm;
    float learning_rate;
    float momentum;
    float decay_rate;
    u32 batch_size;
    u32 epochs;
    
    // Performance metrics
    float loss;
    float accuracy;
    float validation_loss;
    float validation_accuracy;
    
    // Training state
    bool is_training;
    u32 current_epoch;
    u32 current_batch;
    u64 training_start_time_ms;
    
} NeuralNetwork;

// AI agent
typedef struct {
    char agent_name[64];
    u32 agent_id;
    Vec3 position;
    Vec3 velocity;
    Vec3 acceleration;
    
    // Neural networks for different behaviors
    NeuralNetwork* behavior_network;
    NeuralNetwork* movement_network;
    NeuralNetwork* decision_network;
    
    // Current state
    BehaviorType current_behavior;
    float behavior_strength;
    u64 behavior_start_time_ms;
    
    // Memory and experience
    float* experience_buffer;
    u32 experience_count;
    u32 experience_capacity;
    u32 experience_write_index;
    
    // Sensors
    float vision_range;
    float hearing_range;
    float smell_range;
    float* sensory_input;
    u32 sensory_input_size;
    
    // Internal state
    float hunger;
    float thirst;
    float energy;
    float health;
    float happiness;
    float fear;
    float aggression;
    float curiosity;
    
    // Social data
    u32* nearby_agents;
    u32 nearby_agent_count;
    u32 nearby_agent_capacity;
    float social_cooperation;
    float social_aggression;
    
    // Goals and motivations
    BehaviorType primary_goal;
    float goal_strength;
    Vec3 goal_position;
    
    // Learning parameters
    float exploration_rate;
    float learning_rate;
    float memory_decay_rate;
    
    // Performance tracking
    u32 actions_taken;
    u32 successful_actions;
    u32 failed_actions;
    float success_rate;
    
} AIAgent;

// Emergent behavior system
typedef struct {
    char system_name[64];
    u32 system_id;
    
    // AI agents
    AIAgent* agents;
    u32 agent_count;
    u32 agent_capacity;
    
    // Neural networks
    NeuralNetwork* networks;
    u32 network_count;
    u32 network_capacity;
    
    // Environment data
    float* environment_grid;
    u32 grid_width;
    u32 grid_height;
    u32 grid_depth;
    float cell_size;
    
    // Resources in environment
    struct {
        Vec3 position;
        float amount;
        u32 resource_type;
    }* resources;
    u32 resource_count;
    u32 resource_capacity;
    
    // Emergent patterns
    struct {
        BehaviorType behavior;
        u32 agent_count;
        float pattern_strength;
        Vec3 center_position;
        float radius;
        u64 discovery_time_ms;
    }* emergent_patterns;
    u32 pattern_count;
    u32 pattern_capacity;
    
    // System parameters
    float time_scale;
    float interaction_range;
    float learning_rate_modifier;
    float mutation_rate;
    float selection_pressure;
    
    // Evolution parameters
    u32 generation;
    u32 population_size;
    float fitness_threshold;
    bool enable_evolution;
    
    // Performance tracking
    u64 last_update_time_ms;
    u32 updates_per_second;
    u64 total_updates;
    float average_fitness;
    float diversity_index;
    
    // Callbacks
    void (*on_agent_created)(AIAgent* agent);
    void (*on_agent_destroyed)(AIAgent* agent);
    void (*on_behavior_changed)(AIAgent* agent, BehaviorType old_behavior, BehaviorType new_behavior);
    void (*on_pattern_discovered)(const char* pattern_name, float strength);
    void (*on_evolution_completed)(u32 generation, float average_fitness);
    
    void* user_data;
    
} EmergentAISystem;

// MARK: - Neural Network Operations

bool neural_network_init(NeuralNetwork* network, const char* name, NeuralNetworkType type, u32 input_size, u32 output_size);
void neural_network_shutdown(NeuralNetwork* network);

bool neural_network_add_layer(NeuralNetwork* network, u32 size, ActivationFunction activation);
bool neural_network_add_convolutional_layer(NeuralNetwork* network, u32 filter_count, u32 filter_width, u32 filter_height, u32 stride, u32 padding);
bool neural_network_add_recurrent_layer(NeuralNetwork* network, u32 hidden_size);

float* neural_network_forward(NeuralNetwork* network, const float* input);
float neural_network_backward(NeuralNetwork* network, const float* input, const float* target, float learning_rate);
bool neural_network_train(NeuralNetwork* network, const float* inputs, const float* targets, u32 sample_count, u32 epochs, float learning_rate);

bool neural_network_save(NeuralNetwork* network, const char* filename);
bool neural_network_load(NeuralNetwork* network, const char* filename);

// MARK: - AI Agent Operations

bool ai_agent_init(AIAgent* agent, const char* name, u32 input_size, u32 output_size);
void ai_agent_shutdown(AIAgent* agent);

bool ai_agent_update_sensors(AIAgent* agent, const EmergentAISystem* system);
bool ai_agent_update_internal_state(AIAgent* agent, float delta_time);
bool ai_agent_make_decision(AIAgent* agent, const EmergentAISystem* system);
bool ai_agent_execute_action(AIAgent* agent, const EmergentAISystem* system);

bool ai_agent_add_experience(AIAgent* agent, const float* state, const float* action, const float* reward, const float* next_state);
bool ai_agent_learn_from_experience(AIAgent* agent, u32 batch_size, float learning_rate);

bool ai_agent_set_position(AIAgent* agent, const Vec3* position);
bool ai_agent_set_behavior(AIAgent* agent, BehaviorType behavior, float strength);

// MARK: - Emergent AI System Management

bool emergent_ai_init(EmergentAISystem* system, const char* name, u32 max_agents, u32 max_networks, u32 grid_width, u32 grid_height, u32 grid_depth);
void emergent_ai_shutdown(EmergentAISystem* system);

AIAgent* emergent_ai_create_agent(EmergentAISystem* system, const char* name, u32 input_size, u32 output_size);
bool emergent_ai_destroy_agent(EmergentAISystem* system, AIAgent* agent);
AIAgent* emergent_ai_get_agent(EmergentAISystem* system, const char* name);
AIAgent* emergent_ai_get_agent_by_id(EmergentAISystem* system, u32 agent_id);

NeuralNetwork* emergent_ai_create_network(EmergentAISystem* system, const char* name, NeuralNetworkType type, u32 input_size, u32 output_size);
bool emergent_ai_destroy_network(EmergentAISystem* system, NeuralNetwork* network);

// MARK: - Environment and Resources

bool emergent_ai_update_environment(EmergentAISystem* system, float delta_time);
bool emergent_ai_add_resource(EmergentAISystem* system, const Vec3* position, float amount, u32 resource_type);
bool emergent_ai_remove_resource(EmergentAISystem* system, const Vec3* position, float amount);
float emergent_ai_get_resource_at_position(EmergentAISystem* system, const Vec3* position, u32 resource_type);

bool emergent_ai_set_environment_cell(EmergentAISystem* system, u32 x, u32 y, u32 z, float value);
float emergent_ai_get_environment_cell(EmergentAISystem* system, u32 x, u32 y, u32 z);

// MARK: - Emergent Behavior Detection

bool emergent_ai_detect_patterns(EmergentAISystem* system);
bool emergent_ai_analyze_agent_clusters(EmergentAISystem* system);
bool emergent_ai_identify_social_structures(EmergentAISystem* system);
bool emergent_ai_track_behavior_evolution(EmergentAISystem* system);

bool emergent_ai_add_pattern(EmergentAISystem* system, BehaviorType behavior, u32 agent_count, float strength, const Vec3* center, float radius);
bool emergent_ai_remove_pattern(EmergentAISystem* system, u32 pattern_index);

// MARK: - Evolution and Learning

bool emergent_ai_evolve_population(EmergentAISystem* system);
bool emergent_ai_select_parents(EmergentAISystem* system, AIAgent** parents, u32 parent_count);
bool emergent_ai_crossover_agents(EmergentAISystem* system, AIAgent* parent1, AIAgent* parent2, AIAgent* child);
bool emergent_ai_mutate_agent(EmergentAISystem* system, AIAgent* agent, float mutation_rate);

bool emergent_ai_calculate_fitness(EIAgent* agent, float* fitness);
bool emergent_ai_rank_agents_by_fitness(EmergentAISystem* system);

// MARK: - Social Interactions

bool emergent_ai_process_social_interactions(EmergentAISystem* system);
bool emergent_ai_agent_interaction(AIAgent* agent1, AIAgent* agent2, float interaction_strength);
bool emergent_ai_form_groups(EmergentAISystem* system);
bool emergent_ai_compete_for_resources(EmergentAISystem* system);

// MARK: - Utility Functions

void emergent_ai_update(EmergentAISystem* system, float delta_time);
void emergent_ai_render_debug_info(EmergentAISystem* system);
void emergent_ai_export_agent_data(EmergentAISystem* system, const char* filename);
void emergent_ai_export_patterns(EmergentAISystem* system, const char* filename);

void emergent_ai_get_statistics(EmergentAISystem* system, u32* total_agents, u32* active_patterns, 
                              float* average_fitness, float* diversity_index);
void emergent_ai_print_statistics(EmergentAISystem* system);

// MARK: - Configuration

void emergent_ai_set_evolution_parameters(EmergentAISystem* system, float mutation_rate, float selection_pressure, bool enable_evolution);
void emergent_ai_set_learning_parameters(EmergentAISystem* system, float learning_rate_modifier, float exploration_rate);
void emergent_ai_set_environment_parameters(EmergentAISystem* system, float time_scale, float interaction_range);

void emergent_ai_set_callbacks(EmergentAISystem* system,
                              void (*on_agent_created)(AIAgent*),
                              void (*on_agent_destroyed)(AIAgent*),
                              void (*on_behavior_changed)(AIAgent*, BehaviorType, BehaviorType),
                              void (*on_pattern_discovered)(const char*, float),
                              void (*on_evolution_completed)(u32, float));

// MARK: - Debug Macros

#define EMERGENT_AI_ENABLED 1

#if EMERGENT_AI_ENABLED
    #define EMERGENT_AI_CREATE_AGENT(system, name, inputs, outputs) emergent_ai_create_agent(system, name, inputs, outputs)
    #define EMERGENT_AI_UPDATE(system, delta_time) emergent_ai_update(system, delta_time)
    #define EMERGENT_AI_DETECT_PATTERNS(system) emergent_ai_detect_patterns(system)
    #define EMERGENT_AI_EVOLVE(system) emergent_ai_evolve_population(system)
#else
    #define EMERGENT_AI_CREATE_AGENT(system, name, inputs, outputs) NULL
    #define EMERGENT_AI_UPDATE(system, delta_time)
    #define EMERGENT_AI_DETECT_PATTERNS(system) false
    #define EMERGENT_AI_EVOLVE(system) false
#endif

// Global emergent AI system instance
extern EmergentAISystem* g_emergent_ai_system;

#endif // ML_EMERGENT_AI_H
