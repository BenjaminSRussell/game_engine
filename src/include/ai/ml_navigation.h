// include/engine/ai/ml_navigation.h
//
// Purpose: ML-driven NPC navigation system for intelligent pathfinding
// This system uses machine learning to predict optimal paths and adapt to dynamic environments
//
// Key Features:
// - Neural network-based pathfinding with dynamic obstacle avoidance
// - Learning from player and NPC navigation patterns
// - Predictive path planning for anticipated player movement
// - Adaptive navigation based on environmental changes
// - Multi-agent coordination for group navigation
// - Real-time path optimization and replanning
//
// Performance Targets:
// - <2ms path prediction time for complex environments
// - <10MB memory usage per navigation model
// - Support for 1000+ concurrent navigating agents
// - <5% path failure rate in dynamic environments
//
// Ownership: MLNavigationSystem owns all navigation models and path data
// Invariants: Navigation maps must be loaded, agent states must be valid
//

#ifndef ML_NAVIGATION_H
#define ML_NAVIGATION_H

#include "include/common.h"
#include "include/ai/ml/ml_core.h"
#include "math/vec3.h"
#include <stdbool.h>
#include <stdint.h>

// ============================================================================
// NAVIGATION MODELS
// ============================================================================

typedef enum {
    NAV_MODEL_PATH_PREDICTION = 0,    // Neural path prediction model
    NAV_MODEL_OBSTACLE_AVOIDANCE,     // Obstacle avoidance network
    NAV_MODEL_PLAYER_BEHAVIOR,        // Player behavior prediction
    NAV_MODEL_GROUP_COORDINATION,     // Multi-agent coordination
    NAV_MODEL_TERRAIN_ANALYSIS,       // Terrain difficulty assessment
    NAV_MODEL_CUSTOM                  // Custom navigation model
} NavigationModel;

typedef enum {
    NAV_ALGORITHM_NEURAL_ASTAR = 0,   // Neural-enhanced A*
    NAV_ALGORITHM_RL_NAVIGATION,      // Reinforcement learning navigation
    NAV_ALGORITHM_HYBRID,             // Hybrid traditional + ML
    NAV_ALGORITHM_PREDICTIVE,         // Predictive pathfinding
    NAV_ALGORITHM_COLLABORATIVE       // Multi-agent collaborative
} NavigationAlgorithm;

// ============================================================================
// NAVIGATION ENVIRONMENT
// ============================================================================

typedef struct {
    f32 width, height, depth;         // Environment dimensions
    u32 resolution_x, resolution_y;   // Grid resolution
    void *terrain_data;               // Height map and terrain types
    void *static_obstacles;          // Static obstacle map
    void *dynamic_obstacles;         // Dynamic obstacle positions
    f32 cell_size;                   // Size of each navigation cell
    bool is_3d_environment;          // True for 3D navigation
    u32 navigation_layers;           // Number of navigation layers (ground, air, water)
} NavigationEnvironment;

// ============================================================================
// PATH REPRESENTATION
// ============================================================================

typedef struct {
    Vec3 *waypoints;                 // Path waypoints
    u32 waypoint_count;             // Number of waypoints
    u32 max_waypoints;              // Maximum waypoints allocated
    f32 total_length;               // Total path length
    f32 estimated_time;             // Estimated travel time
    f32 difficulty_score;           // Path difficulty rating
    bool is_optimal;                // Whether path is optimal
    bool is_valid;                  // Whether path is currently valid
    u64 creation_time;              // When path was created
    u32 path_id;                    // Unique path identifier
} NavigationPath;

// ============================================================================
// NAVIGATION AGENT
// ============================================================================

typedef struct {
    u32 agent_id;                    // Unique agent identifier
    Vec3 current_position;           // Current world position
    Vec3 target_position;           // Target destination
    Vec3 velocity;                  // Current movement velocity
    f32 max_speed;                  // Maximum movement speed
    f32 acceleration;               // Acceleration rate
    f32 turning_radius;             // Minimum turning radius
    NavigationPath *current_path;    // Currently following path
    u32 current_waypoint;           // Current waypoint index
    bool is_navigating;             // Currently navigating
    bool path_recalculation_needed; // Needs path recalculation
    f64 last_path_update;           // Time of last path update
    u32 failed_path_attempts;      // Number of failed path attempts
    void *agent_specific_data;      // Agent-specific navigation data
} NavigationAgent;

// ============================================================================
// PREDICTIVE NAVIGATION
// ============================================================================

typedef struct {
    Vec3 predicted_positions[10];    // Predicted future positions
    f32 prediction_confidence[10];   // Confidence for each prediction
    u32 prediction_count;           // Number of predictions
    f64 prediction_time;            // When predictions were made
    Vec3 prediction_target;         // Predicted target location
    f32 target_confidence;          // Confidence in target prediction
} PlayerPrediction;

typedef struct {
    PlayerPrediction *predictions;   // Player movement predictions
    u32 max_predictions;           // Maximum predictions stored
    u32 active_prediction_count;    // Currently active predictions
    f32 prediction_horizon;         // How far ahead to predict (seconds)
    f32 prediction_accuracy;        // Historical prediction accuracy
    bool is_prediction_enabled;     // Whether prediction is active
} PredictiveNavigation;

// ============================================================================
// GROUP COORDINATION
// ============================================================================

typedef struct {
    u32 group_id;                   // Group identifier
    NavigationAgent *agents;         // Agents in the group
    u32 agent_count;                // Number of agents in group
    Vec3 group_target;              // Group destination
    Vec3 formation_center;          // Center of formation
    f32 formation_spacing;          // Spacing between agents
    bool is_formation_maintained;   // Whether formation is maintained
    u32 coordination_strategy;       // Coordination strategy type
    void *coordination_data;        // Strategy-specific data
} NavigationGroup;

// ============================================================================
// ML NAVIGATION SYSTEM
// ============================================================================

typedef struct {
    // ML system integration
    MLSystem *ml_system;
    void *navigation_models[6];      // Navigation model instances
    
    // Environment
    NavigationEnvironment environment;
    
    // Agents and paths
    NavigationAgent *agents;
    u32 agent_count;
    u32 max_agents;
    
    NavigationPath *paths;
    u32 path_count;
    u32 max_paths;
    
    // Groups
    NavigationGroup *groups;
    u32 group_count;
    u32 max_groups;
    
    // Predictive navigation
    PredictiveNavigation predictive;
    
    // Learning and adaptation
    void *training_data;            // Collected training data
    u32 training_samples;           // Number of training samples
    f32 learning_rate;              // Learning rate for adaptation
    bool is_learning_enabled;       // Whether learning is enabled
    u64 last_training_time;         // Last time model was trained
    
    // Performance monitoring
    f64 total_navigation_time;
    u64 total_paths_generated;
    f32 average_navigation_time;
    f32 path_success_rate;
    f32 prediction_accuracy;
    
    // Configuration
    NavigationAlgorithm default_algorithm;
    bool enable_prediction;
    bool enable_group_coordination;
    bool enable_dynamic_replanning;
    f32 replanning_threshold;       // When to trigger replanning
    
    bool initialized;
} MLNavigationSystem;

// ============================================================================
// PUBLIC API - SYSTEM MANAGEMENT
// ============================================================================

// System creation and management
MLNavigationSystem *ml_navigation_create(MLSystem *ml_system);
void ml_navigation_destroy(MLNavigationSystem *system);
bool ml_navigation_initialize(MLNavigationSystem *system, const NavigationEnvironment *environment);
void ml_navigation_shutdown(MLNavigationSystem *system);

// Configuration
void ml_navigation_set_algorithm(MLNavigationSystem *system, NavigationAlgorithm algorithm);
void ml_navigation_enable_prediction(MLNavigationSystem *system, bool enable);
void ml_navigation_enable_group_coordination(MLNavigationSystem *system, bool enable);
void ml_navigation_set_replanning_threshold(MLNavigationSystem *system, f32 threshold);

// ============================================================================
// PUBLIC API - ENVIRONMENT MANAGEMENT
// ============================================================================

// Environment setup
bool ml_navigation_load_terrain(MLNavigationSystem *system, const char *terrain_file);
bool ml_navigation_add_static_obstacle(MLNavigationSystem *system, Vec3 position, Vec3 size);
bool ml_navigation_update_dynamic_obstacles(MLNavigationSystem *system, const Vec3 *obstacles, u32 count);
bool ml_navigation_update_environment(MLNavigationSystem *system);

// Environment analysis
f32 ml_navigation_get_terrain_difficulty(MLNavigationSystem *system, Vec3 position);
bool ml_navigation_is_position_walkable(MLNavigationSystem *system, Vec3 position);
bool ml_navigation_has_line_of_sight(MLNavigationSystem *system, Vec3 start, Vec3 end);

// ============================================================================
// PUBLIC API - AGENT MANAGEMENT
// ============================================================================

// Agent creation and management
u32 ml_navigation_create_agent(MLNavigationSystem *system, Vec3 start_position, f32 max_speed);
bool ml_navigation_destroy_agent(MLNavigationSystem *system, u32 agent_id);
bool ml_navigation_set_agent_target(MLNavigationSystem *system, u32 agent_id, Vec3 target);
bool ml_navigation_update_agent_position(MLNavigationSystem *system, u32 agent_id, Vec3 position);

// Agent state
NavigationAgent *ml_navigation_get_agent(MLNavigationSystem *system, u32 agent_id);
bool ml_navigation_is_agent_navigating(MLNavigationSystem *system, u32 agent_id);
f32 ml_navigation_get_agent_progress(MLNavigationSystem *system, u32 agent_id);

// ============================================================================
// PUBLIC API - PATH GENERATION
// ============================================================================

// Main path generation
NavigationPath *ml_navigation_generate_path(MLNavigationSystem *system, u32 agent_id, Vec3 start, Vec3 goal);
bool ml_navigation_generate_path_async(MLNavigationSystem *system, u32 agent_id, Vec3 start, Vec3 goal,
                                      void (*callback)(NavigationPath *path, void *user_data), void *user_data);

// Path validation and optimization
bool ml_navigation_validate_path(MLNavigationSystem *system, NavigationPath *path);
bool ml_navigation_optimize_path(MLNavigationSystem *system, NavigationPath *path);
bool ml_navigation_smooth_path(MLNavigationSystem *system, NavigationPath *path);

// Path following
bool ml_navigation_follow_path(MLNavigationSystem *system, u32 agent_id, NavigationPath *path, f32 delta_time);
Vec3 ml_navigation_get_next_waypoint(MLNavigationSystem *system, u32 agent_id);
bool ml_navigation_reached_destination(MLNavigationSystem *system, u32 agent_id);

// ============================================================================
// PUBLIC API - PREDICTIVE NAVIGATION
// ============================================================================

// Player prediction
PlayerPrediction *ml_navigation_predict_player_movement(MLNavigationSystem *system, Vec3 current_pos, 
                                                      Vec3 current_vel, f32 prediction_horizon);
bool ml_navigation_update_prediction(MLNavigationSystem *system, u32 prediction_id, Vec3 actual_position);
f32 ml_navigation_get_prediction_accuracy(MLNavigationSystem *system);

// Predictive pathfinding
NavigationPath *ml_navigation_generate_predictive_path(MLNavigationSystem *system, u32 agent_id, 
                                                      Vec3 start, PlayerPrediction *target_prediction);

// ============================================================================
// PUBLIC API - GROUP COORDINATION
// ============================================================================

// Group management
u32 ml_navigation_create_group(MLNavigationSystem *system);
bool ml_navigation_add_agent_to_group(MLNavigationSystem *system, u32 group_id, u32 agent_id);
bool ml_navigation_remove_agent_from_group(MLNavigationSystem *system, u32 group_id, u32 agent_id);
bool ml_navigation_set_group_target(MLNavigationSystem *system, u32 group_id, Vec3 target);

// Formation control
bool ml_navigation_set_formation(MLNavigationSystem *system, u32 group_id, u32 formation_type, f32 spacing);
bool ml_navigation_maintain_formation(MLNavigationSystem *system, u32 group_id, f32 delta_time);
Vec3 ml_navigation_get_formation_position(MLNavigationSystem *system, u32 group_id, u32 agent_id);

// ============================================================================
// PUBLIC API - LEARNING AND ADAPTATION
// ============================================================================

// Learning system
bool ml_navigation_record_navigation_data(MLNavigationSystem *system, u32 agent_id, 
                                           const NavigationPath *path, f32 success_metric);
bool ml_navigation_train_models(MLNavigationSystem *system);
bool ml_navigation_save_training_data(MLNavigationSystem *system, const char *filename);
bool ml_navigation_load_training_data(MLNavigationSystem *system, const char *filename);

// Adaptation
bool ml_navigation_adapt_to_environment(MLNavigationSystem *system);
bool ml_navigation_adapt_to_player_behavior(MLNavigationSystem *system, u32 player_id);
f32 ml_navigation_get_adaptation_progress(MLNavigationSystem *system);

// ============================================================================
// PUBLIC API - PERFORMANCE MONITORING
// ============================================================================

// Performance statistics
typedef struct {
    f64 average_path_generation_time_ms;
    f32 path_success_rate;
    f32 prediction_accuracy;
    u64 total_paths_generated;
    u64 active_agents;
    f32 average_path_length;
    f32 environment_coverage;
    f32 memory_usage_mb;
} NavigationPerformanceStats;

NavigationPerformanceStats *ml_navigation_get_performance_stats(MLNavigationSystem *system);
void ml_navigation_reset_performance_stats(MLNavigationSystem *system);
void ml_navigation_print_performance_report(MLNavigationSystem *system);

// Quality metrics
f32 ml_navigation_calculate_path_efficiency(MLNavigationSystem *system, const NavigationPath *path);
f32 ml_navigation_measure_agent_productivity(MLNavigationSystem *system, u32 agent_id);
bool ml_navigation_is_system_healthy(MLNavigationSystem *system);

// ============================================================================
// PUBLIC API - UTILITY FUNCTIONS
// ============================================================================

// Model information
const char *ml_navigation_get_model_name(NavigationModel model);
const char *ml_navigation_get_algorithm_name(NavigationAlgorithm algorithm);

// Utility functions
bool ml_navigation_validate_environment(const NavigationEnvironment *environment);
// bool ml_navigation_validate_path(const NavigationPath *path); // Duplicate removed
f32 ml_navigation_calculate_path_length(const NavigationPath *path);
Vec3 ml_navigation_get_path_direction(const NavigationPath *path, u32 waypoint_index);

// ============================================================================
// ERROR HANDLING
// ============================================================================

typedef enum {
    NAV_ERROR_NONE = 0,
    NAV_ERROR_INVALID_PARAMETER,
    NAV_ERROR_MODEL_NOT_LOADED,
    NAV_ERROR_AGENT_NOT_FOUND,
    NAV_ERROR_PATH_GENERATION_FAILED,
    NAV_ERROR_ENVIRONMENT_INVALID,
    NAV_ERROR_OUT_OF_MEMORY,
    NAV_ERROR_TARGET_UNREACHABLE,
    NAV_ERROR_PREDICTION_FAILED
} NavigationError;

const char *ml_navigation_get_error_string(NavigationError error);

// ============================================================================
// UTILITY MACROS
// ============================================================================

// Configuration creation macros
#define NAV_CREATE_ENVIRONMENT_3D(w, h, d, res) \
    (NavigationEnvironment){ \
        .width = (w), .height = (h), .depth = (d), \
        .resolution_x = (res), .resolution_y = (res), \
        .cell_size = 1.0f, \
        .is_3d_environment = true, \
        .navigation_layers = 3 \
    }

#define NAV_CREATE_AGENT_DEFAULT(pos, speed) \
    (NavigationAgent){ \
        .current_position = (pos), \
        .max_speed = (speed), \
        .acceleration = (speed) * 2.0f, \
        .turning_radius = 1.0f, \
        .is_navigating = false \
    }

// Error checking macros
#define NAV_CHECK_ERROR(condition, error) \
    do { \
        if (!(condition)) { \
            LOG_ERROR("ML Navigation Error: %s", ml_navigation_get_error_string(error)); \
            return error; \
        } \
    } while(0)

#define NAV_CHECK_RET(condition, error, retval) \
    do { \
        if (!(condition)) { \
            LOG_ERROR("ML Navigation Error: %s", ml_navigation_get_error_string(error)); \
            return retval; \
        } \
    } while(0)

#define NAV_CHECK_NULL_PARAM(param) \
    NAV_CHECK_ERROR((param) != NULL, NAV_ERROR_INVALID_PARAMETER)

#define NAV_CHECK_NULL_PARAM_RET(param, retval) \
    NAV_CHECK_RET((param) != NULL, NAV_ERROR_INVALID_PARAMETER, retval)

#define NAV_CHECK_NULL_PARAM_PTR(param) \
    NAV_CHECK_NULL_PARAM_RET(param, NULL)

#define NAV_CHECK_ERROR_PTR(condition, error) \
    NAV_CHECK_RET(condition, error, NULL)

// Performance macros
#include <time.h>
#define NAV_START_TIMER(system) \
    clock_t start_time = clock()

#define NAV_END_TIMER(system, path_time_ptr) \
    do { \
        clock_t end_time = clock(); \
        f64 duration = ((f64)(end_time - start_time)) / CLOCKS_PER_SEC; \
        if ((path_time_ptr) != NULL) *((f64*)(path_time_ptr)) = duration; \
        system->total_navigation_time += duration; \
        system->total_paths_generated++; \
        system->average_navigation_time = system->total_navigation_time / system->total_paths_generated; \
    } while(0)

#endif // ML_NAVIGATION_H
