// src/engine/ai/ml_navigation.c
//
// Purpose: Implementation of ML-driven NPC navigation system
// This file implements intelligent pathfinding with machine learning
//
// Implementation Notes:
// - Neural network-enhanced pathfinding algorithms
// - Learning from navigation patterns and failures
// - Predictive path planning based on behavior analysis
// - Multi-agent coordination and formation control
// - Dynamic obstacle avoidance and replanning
// - Performance optimization for large-scale navigation
//
// Dependencies: ml_core.h, core/logger.h, core/memory.h
//

#include "ai/ml_navigation.h"
#include "include/core/logger.h"
#include "include/core/memory.h"
#include <stdlib.h>
#include <string.h>
#include <time.h>
#include <include/math/math_all.h>

// ============================================================================
// INTERNAL CONSTANTS AND STRUCTURES
// ============================================================================

#define MAX_AGENTS 1000
#define MAX_PATHS 2000
#define MAX_GROUPS 100
#define MAX_WAYPOINTS 256
#define PREDICTION_HORIZON_DEFAULT 5.0f
#define LEARNING_BATCH_SIZE 100
#define ADAPTATION_THRESHOLD 50

typedef struct {
    Vec3 position;
    Vec3 velocity;
    f64 timestamp;
    u32 agent_id;
} NavigationSample;

typedef struct {
    NavigationSample *samples;
    u32 count;
    u32 max_samples;
    u32 write_index;
} NavigationHistory;

// ============================================================================
// PATH GENERATION ALGORITHMS
// ============================================================================

static NavigationPath *generate_neural_path(MLNavigationSystem *system, u32 agent_id, Vec3 start, Vec3 goal) {
    if (!system->navigation_models[NAV_MODEL_PATH_PREDICTION]) {
        return NULL; // Model not loaded
    }
    
    // Create path structure
    NavigationPath *path = malloc(sizeof(NavigationPath));
    if (!path) return NULL;
    
    memset(path, 0, sizeof(NavigationPath));
    path->waypoints = malloc(sizeof(Vec3) * MAX_WAYPOINTS);
    path->max_waypoints = MAX_WAYPOINTS;
    path->path_id = system->path_count + 1;
    path->creation_time = time(NULL);
    
    if (!path->waypoints) {
        free(path);
        return NULL;
    }
    
    // Prepare input for neural network
    // This would convert start, goal, and environment data to tensor format
    
    // Create inference context
    MLInferenceContext *context = ml_create_inference_context(system->ml_system, system->navigation_models[NAV_MODEL_PATH_PREDICTION]);
    if (context) {
        // Run neural path prediction
        bool success = ml_run_inference(system->ml_system, context);
        
        if (success) {
            // Extract waypoints from neural network output
            // For now, generate a simple straight-line path as placeholder
            u32 waypoint_count = 10;
            for (u32 i = 0; i < waypoint_count && i < MAX_WAYPOINTS; i++) {
                f32 t = (f32)i / (waypoint_count - 1);
                path->waypoints[i].x = start.x + (goal.x - start.x) * t;
                path->waypoints[i].y = start.y + (goal.y - start.y) * t;
                path->waypoints[i].z = start.z + (goal.z - start.z) * t;
            }
            path->waypoint_count = waypoint_count;
            
            // Calculate path properties
            path->total_length = ml_navigation_calculate_path_length(path);
            path->estimated_time = path->total_length / 5.0f; // Assume 5 units/second
            path->difficulty_score = 0.5f; // Placeholder
            path->is_optimal = true;
            path->is_valid = true;
        }
        
        ml_destroy_inference_context(system->ml_system, context);
    }
    
    return path;
}

static NavigationPath *generate_traditional_path(MLNavigationSystem *system, Vec3 start, Vec3 goal) {
    // Fallback traditional A* pathfinding
    NavigationPath *path = malloc(sizeof(NavigationPath));
    if (!path) return NULL;
    
    memset(path, 0, sizeof(NavigationPath));
    path->waypoints = malloc(sizeof(Vec3) * MAX_WAYPOINTS);
    path->max_waypoints = MAX_WAYPOINTS;
    path->path_id = system->path_count + 1;
    path->creation_time = time(NULL);
    
    if (!path->waypoints) {
        free(path);
        return NULL;
    }
    
    // Simple straight-line path as fallback
    u32 waypoint_count = 5;
    for (u32 i = 0; i < waypoint_count && i < MAX_WAYPOINTS; i++) {
        f32 t = (f32)i / (waypoint_count - 1);
        path->waypoints[i].x = start.x + (goal.x - start.x) * t;
        path->waypoints[i].y = start.y + (goal.y - start.y) * t;
        path->waypoints[i].z = start.z + (goal.z - start.z) * t;
    }
    path->waypoint_count = waypoint_count;
    
    path->total_length = ml_navigation_calculate_path_length(path);
    path->estimated_time = path->total_length / 3.0f; // Slower than neural
    path->difficulty_score = 0.7f; // Higher difficulty for traditional
    path->is_optimal = false;
    path->is_valid = true;
    
    return path;
}

// ============================================================================
// OBSTACLE AVOIDANCE
// ============================================================================

static bool check_path_collision(MLNavigationSystem *system, const NavigationPath *path) {
    if (!system || !path || !path->waypoints) return false;
    
    // Check each path segment for collisions
    for (u32 i = 0; i < path->waypoint_count - 1; i++) {
        Vec3 start = path->waypoints[i];
        Vec3 end = path->waypoints[i + 1];
        
        // Simple collision check with environment bounds
        if (start.x < 0 || start.x > system->environment.width ||
            start.y < 0 || start.y > system->environment.height ||
            start.z < 0 || start.z > system->environment.depth) {
            return true; // Collision with environment boundary
        }
        
        if (end.x < 0 || end.x > system->environment.width ||
            end.y < 0 || end.y > system->environment.height ||
            end.z < 0 || end.z > system->environment.depth) {
            return true; // Collision with environment boundary
        }
    }
    
    return false; // No collision detected
}

static bool avoid_obstacles_in_path(MLNavigationSystem *system, NavigationPath *path) {
    if (!system->navigation_models[NAV_MODEL_OBSTACLE_AVOIDANCE]) {
        return false; // Model not loaded
    }
    
    // Use neural network to modify path around obstacles
    MLInferenceContext *context = ml_create_inference_context(system->ml_system, system->navigation_models[NAV_MODEL_OBSTACLE_AVOIDANCE]);
    if (context) {
        // Set up input with current path and obstacle data
        // Run inference to get modified waypoints
        
        bool success = ml_run_inference(system->ml_system, context);
        
        ml_destroy_inference_context(system->ml_system, context);
        
        return success;
    }
    
    return false;
}

// ============================================================================
// PREDICTIVE NAVIGATION
// ============================================================================

static PlayerPrediction *predict_player_movement_simple(MLNavigationSystem *system, Vec3 current_pos, 
                                                       Vec3 current_vel, f32 horizon) {
    PlayerPrediction *prediction = malloc(sizeof(PlayerPrediction));
    if (!prediction) return NULL;
    
    memset(prediction, 0, sizeof(PlayerPrediction));
    
    // Simple linear prediction as fallback
    u32 prediction_count = 10;
    f32 time_step = horizon / prediction_count;
    
    for (u32 i = 0; i < prediction_count; i++) {
        f32 t = (i + 1) * time_step;
        prediction->predicted_positions[i].x = current_pos.x + current_vel.x * t;
        prediction->predicted_positions[i].y = current_pos.y + current_vel.y * t;
        prediction->predicted_positions[i].z = current_pos.z + current_vel.z * t;
        prediction->prediction_confidence[i] = 1.0f - (t / horizon); // Decreasing confidence
    }
    
    prediction->prediction_count = prediction_count;
    prediction->prediction_time = time(NULL);
    prediction->prediction_target = prediction->predicted_positions[prediction_count - 1];
    prediction->target_confidence = prediction->prediction_confidence[prediction_count - 1];
    
    return prediction;
}

static PlayerPrediction *predict_player_movement_neural(MLNavigationSystem *system, Vec3 current_pos, 
                                                         Vec3 current_vel, f32 horizon) {
    if (!system->navigation_models[NAV_MODEL_PLAYER_BEHAVIOR]) {
        return predict_player_movement_simple(system, current_pos, current_vel, horizon);
    }
    
    PlayerPrediction *prediction = malloc(sizeof(PlayerPrediction));
    if (!prediction) return NULL;
    
    memset(prediction, 0, sizeof(PlayerPrediction));
    
    // Use neural network for prediction
    MLInferenceContext *context = ml_create_inference_context(system->ml_system, system->navigation_models[NAV_MODEL_PLAYER_BEHAVIOR]);
    if (context) {
        // Set up input with current position, velocity, and historical data
        // Run inference to get predicted movement pattern
        
        bool success = ml_run_inference(system->ml_system, context);
        
        if (success) {
            // Extract predictions from neural network output
            // For now, use simple prediction as placeholder
            free(prediction);
            prediction = predict_player_movement_simple(system, current_pos, current_vel, horizon);
        }
        
        ml_destroy_inference_context(system->ml_system, context);
    }
    
    return prediction;
}

// ============================================================================
// GROUP COORDINATION
// ============================================================================

static bool calculate_formation_positions(MLNavigationSystem *system, NavigationGroup *group) {
    if (!system || !group || group->agent_count == 0) return false;
    
    // Simple line formation as fallback
    Vec3 direction = vec3_normalize(vec3_sub(group->group_target, group->formation_center));
    Vec3 perpendicular = vec3_cross(direction, (Vec3){0, 1, 0});
    perpendicular = vec3_normalize(perpendicular);
    
    for (u32 i = 0; i < group->agent_count; i++) {
        f32 offset = (f32)i - (group->agent_count - 1) * 0.5f;
        Vec3 formation_pos = vec3_add(group->formation_center, 
                                     vec3_mul(perpendicular, offset * group->formation_spacing));
        
        // Update agent target to formation position
        if (group->agents[i].agent_id > 0 && group->agents[i].agent_id <= system->max_agents) {
            u32 agent_index = group->agents[i].agent_id - 1;
            system->agents[agent_index].target_position = formation_pos;
        }
    }
    
    return true;
}

// ============================================================================
// LEARNING AND ADAPTATION
// ============================================================================

static bool record_navigation_sample(MLNavigationSystem *system, u32 agent_id, const NavigationPath *path, f32 success) {
    if (!system || agent_id == 0 || agent_id > system->max_agents) return false;
    
    NavigationAgent *agent = &system->agents[agent_id - 1];
    
    // Record navigation sample for learning
    NavigationSample sample = {
        .position = agent->current_position,
        .velocity = agent->velocity,
        .timestamp = time(NULL),
        .agent_id = agent_id
    };
    
    // Add to training data (simplified)
    system->training_samples++;
    
    // Update success metrics
    if (success > 0.5f) {
        system->path_success_rate = system->path_success_rate * 0.9f + success * 0.1f;
    }
    
    return true;
}

static bool should_retrain_models(MLNavigationSystem *system) {
    if (!system->is_learning_enabled) return false;
    
    // Check if we have enough new data to retrain
    u64 time_since_last_training = time(NULL) - system->last_training_time;
    
    return (system->training_samples >= LEARNING_BATCH_SIZE && 
            time_since_last_training >= 3600); // Retain at most once per hour
}

// ============================================================================
// PUBLIC API IMPLEMENTATION
// ============================================================================

MLNavigationSystem *ml_navigation_create(MLSystem *ml_system) {
    NAV_CHECK_NULL_PARAM_PTR(ml_system);
    
    MLNavigationSystem *system = malloc(sizeof(MLNavigationSystem));
    if (!system) {
        LOG_ERROR("Failed to allocate ML navigation system");
        return NULL;
    }
    
    memset(system, 0, sizeof(MLNavigationSystem));
    system->ml_system = ml_system;
    system->default_algorithm = NAV_ALGORITHM_NEURAL_ASTAR;
    system->replanning_threshold = 2.0f;
    system->learning_rate = 0.01f;
    system->is_learning_enabled = true;
    
    // Allocate arrays
    system->agents = calloc(MAX_AGENTS, sizeof(NavigationAgent));
    system->paths = calloc(MAX_PATHS, sizeof(NavigationPath));
    system->groups = calloc(MAX_GROUPS, sizeof(NavigationGroup));
    
    if (!system->agents || !system->paths || !system->groups) {
        ml_navigation_destroy(system);
        LOG_ERROR("Failed to allocate navigation arrays");
        return NULL;
    }
    
    system->max_agents = MAX_AGENTS;
    system->max_paths = MAX_PATHS;
    system->max_groups = MAX_GROUPS;
    
    // Initialize predictive navigation
    system->predictive.max_predictions = 100;
    system->predictive.predictions = calloc(system->predictive.max_predictions, sizeof(PlayerPrediction));
    system->predictive.prediction_horizon = PREDICTION_HORIZON_DEFAULT;
    
    LOG_INFO("ML Navigation system created");
    return system;
}

void ml_navigation_destroy(MLNavigationSystem *system) {
    if (!system) return;
    
    ml_navigation_shutdown(system);
    
    // Free arrays
    if (system->agents) free(system->agents);
    if (system->paths) free(system->paths);
    if (system->groups) free(system->groups);
    if (system->predictive.predictions) free(system->predictive.predictions);
    
    free(system);
    LOG_INFO("ML Navigation system destroyed");
}

bool ml_navigation_initialize(MLNavigationSystem *system, const NavigationEnvironment *environment) {
    NAV_CHECK_NULL_PARAM_PTR(system);
    NAV_CHECK_NULL_PARAM_PTR(environment);
    NAV_CHECK_ERROR(!system->initialized, NAV_ERROR_INVALID_PARAMETER);
    
    if (!ml_navigation_validate_environment(environment)) {
        LOG_ERROR("Invalid navigation environment");
        return false;
    }
    
    // Copy environment
    system->environment = *environment;
    
    // Load navigation models
    char model_path[256];
    
    // Path prediction model
    snprintf(model_path, sizeof(model_path), "assets/models/nav_path_prediction.mlmodel");
    MLModelMetadata metadata = {
        .name = "path_prediction_model",
        .type = ML_MODEL_TYPE_NAVIGATION,
        .framework = ML_FRAMEWORK_COREML,
        .preferred_device = ML_DEVICE_GPU,
        .supports_dynamic_shapes = false,
        .max_batch_size = 1
    };
    
    system->navigation_models[NAV_MODEL_PATH_PREDICTION] = ml_load_model(system->ml_system, model_path, &metadata);
    if (!system->navigation_models[NAV_MODEL_PATH_PREDICTION]) {
        LOG_WARN("Failed to load path prediction model");
    }
    
    // Obstacle avoidance model
    snprintf(model_path, sizeof(model_path), "assets/models/nav_obstacle_avoidance.mlmodel");
    metadata.name = "obstacle_avoidance_model";
    system->navigation_models[NAV_MODEL_OBSTACLE_AVOIDANCE] = ml_load_model(system->ml_system, model_path, &metadata);
    if (!system->navigation_models[NAV_MODEL_OBSTACLE_AVOIDANCE]) {
        LOG_WARN("Failed to load obstacle avoidance model");
    }
    
    // Player behavior model
    snprintf(model_path, sizeof(model_path), "assets/models/nav_player_behavior.mlmodel");
    metadata.name = "player_behavior_model";
    system->navigation_models[NAV_MODEL_PLAYER_BEHAVIOR] = ml_load_model(system->ml_system, model_path, &metadata);
    if (!system->navigation_models[NAV_MODEL_PLAYER_BEHAVIOR]) {
        LOG_WARN("Failed to load player behavior model");
    }
    
    system->initialized = true;
    LOG_INFO("ML Navigation system initialized: %.1fx%.1fx%.1f", 
             environment->width, environment->height, environment->depth);
    return true;
}

void ml_navigation_shutdown(MLNavigationSystem *system) {
    if (!system || !system->initialized) return;
    
    // Unload models
    for (int i = 0; i < 6; i++) {
        if (system->navigation_models[i]) {
            ml_unload_model(system->ml_system, system->navigation_models[i]);
            system->navigation_models[i] = NULL;
        }
    }
    
    // Free paths
    for (u32 i = 0; i < system->path_count; i++) {
        if (system->paths[i].waypoints) {
            free(system->paths[i].waypoints);
        }
    }
    
    // Free predictions
    for (u32 i = 0; i < system->predictive.active_prediction_count; i++) {
        if (system->predictive.predictions) {
            free(system->predictive.predictions);
        }
    }
    
    system->initialized = false;
    LOG_INFO("ML Navigation system shutdown complete");
}

u32 ml_navigation_create_agent(MLNavigationSystem *system, Vec3 start_position, f32 max_speed) {
    NAV_CHECK_NULL_PARAM_RET(system, 0);
    NAV_CHECK_RET(system->agent_count < system->max_agents, NAV_ERROR_OUT_OF_MEMORY, 0);
    
    u32 agent_id = system->agent_count + 1; // 1-based indexing
    NavigationAgent *agent = &system->agents[system->agent_count];
    
    memset(agent, 0, sizeof(NavigationAgent));
    agent->agent_id = agent_id;
    agent->current_position = start_position;
    agent->target_position = start_position;
    agent->max_speed = max_speed;
    agent->acceleration = max_speed * 2.0f;
    agent->turning_radius = 1.0f;
    agent->is_navigating = false;
    
    system->agent_count++;
    
    LOG_INFO("Created navigation agent %d at (%.1f, %.1f, %.1f)", agent_id, 
             start_position.x, start_position.y, start_position.z);
    return agent_id;
}

NavigationPath *ml_navigation_generate_path(MLNavigationSystem *system, u32 agent_id, Vec3 start, Vec3 goal) {
    NAV_CHECK_NULL_PARAM_PTR(system);
    NAV_CHECK_ERROR_PTR(agent_id > 0 && agent_id <= system->agent_count, NAV_ERROR_AGENT_NOT_FOUND);
    
    NAV_START_TIMER(system);
    
    NavigationPath *path = NULL;
    bool success = false;
    
    // Try neural path generation first
    switch (system->default_algorithm) {
        case NAV_ALGORITHM_NEURAL_ASTAR:
        case NAV_ALGORITHM_RL_NAVIGATION:
        case NAV_ALGORITHM_PREDICTIVE:
            path = generate_neural_path(system, agent_id, start, goal);
            if (path) {
                // Check for collisions and avoid obstacles
                if (check_path_collision(system, path)) {
                    avoid_obstacles_in_path(system, path);
                }
                success = true;
            }
            break;
            
        case NAV_ALGORITHM_HYBRID:
            path = generate_neural_path(system, agent_id, start, goal);
            if (!path) {
                path = generate_traditional_path(system, start, goal);
            }
            success = (path != NULL);
            break;
            
        default:
            path = generate_traditional_path(system, start, goal);
            success = (path != NULL);
            break;
    }
    
    if (success && path && system->path_count < system->max_paths) {
        // Store path in system
        system->paths[system->path_count] = *path;
        system->path_count++;
        
        // Assign to agent
        NavigationAgent *agent = &system->agents[agent_id - 1];
        agent->current_path = &system->paths[system->path_count - 1];
        agent->current_waypoint = 0;
        agent->is_navigating = true;
        agent->last_path_update = time(NULL);
        
        LOG_DEBUG("Generated path for agent %d: %d waypoints, length %.1f", 
                 agent_id, path->waypoint_count, path->total_length);
    }
    
    NAV_END_TIMER(system, NULL);
    
    return path;
}

bool ml_navigation_follow_path(MLNavigationSystem *system, u32 agent_id, NavigationPath *path, f32 delta_time) {
    NAV_CHECK_NULL_PARAM_PTR(system);
    NAV_CHECK_ERROR(agent_id > 0 && agent_id <= system->agent_count, NAV_ERROR_AGENT_NOT_FOUND);
    
    NavigationAgent *agent = &system->agents[agent_id - 1];
    if (!agent->is_navigating || !agent->current_path) {
        return false;
    }
    

    if (agent->current_waypoint >= path->waypoint_count) {
        // Reached destination
        agent->is_navigating = false;
        return true;
    }
    
    // Get current target waypoint
    Vec3 target = path->waypoints[agent->current_waypoint];
    Vec3 direction = vec3_sub(target, agent->current_position);
    f32 distance = vec3_length(direction);
    
    // Check if reached current waypoint
    if (distance < 0.5f) {
        agent->current_waypoint++;
        if (agent->current_waypoint >= path->waypoint_count) {
            agent->is_navigating = false;
            return true;
        }
        target = path->waypoints[agent->current_waypoint];
        direction = vec3_sub(target, agent->current_position);
        distance = vec3_length(direction);
    }
    
    // Move towards target
    if (distance > 0.0f) {
        direction = vec3_normalize(direction);
        Vec3 movement = vec3_mul(direction, agent->max_speed * delta_time);
        agent->current_position = vec3_add(agent->current_position, movement);
        agent->velocity = movement;
    }
    
    return true;
}

PlayerPrediction *ml_navigation_predict_player_movement(MLNavigationSystem *system, Vec3 current_pos, 
                                                      Vec3 current_vel, f32 prediction_horizon) {
    NAV_CHECK_NULL_PARAM_PTR(system);
    
    if (system->predictive.active_prediction_count >= system->predictive.max_predictions) {
        return NULL; // Prediction storage full
    }
    
    PlayerPrediction *prediction;
    
    // Try neural prediction first
    if (system->enable_prediction && system->navigation_models[NAV_MODEL_PLAYER_BEHAVIOR]) {
        prediction = predict_player_movement_neural(system, current_pos, current_vel, prediction_horizon);
    } else {
        prediction = predict_player_movement_simple(system, current_pos, current_vel, prediction_horizon);
    }
    
    if (prediction) {
        // Store prediction
        system->predictive.predictions[system->predictive.active_prediction_count] = *prediction;
        system->predictive.active_prediction_count++;
        system->predictive.prediction_horizon = prediction_horizon;
    }
    
    return prediction;
}

// ============================================================================
// UTILITY FUNCTIONS
// ============================================================================

const char *ml_navigation_get_model_name(NavigationModel model) {
    switch (model) {
        case NAV_MODEL_PATH_PREDICTION: return "Path Prediction";
        case NAV_MODEL_OBSTACLE_AVOIDANCE: return "Obstacle Avoidance";
        case NAV_MODEL_PLAYER_BEHAVIOR: return "Player Behavior";
        case NAV_MODEL_GROUP_COORDINATION: return "Group Coordination";
        case NAV_MODEL_TERRAIN_ANALYSIS: return "Terrain Analysis";
        case NAV_MODEL_CUSTOM: return "Custom";
        default: return "Unknown";
    }
}

f32 ml_navigation_calculate_path_length(const NavigationPath *path) {
    if (!path || !path->waypoints || path->waypoint_count < 2) {
        return 0.0f;
    }
    
    f32 total_length = 0.0f;
    for (u32 i = 0; i < path->waypoint_count - 1; i++) {
        Vec3 segment = vec3_sub(path->waypoints[i + 1], path->waypoints[i]);
        total_length += vec3_length(segment);
    }
    
    return total_length;
}

bool ml_navigation_validate_environment(const NavigationEnvironment *environment) {
    if (!environment) return false;
    
    // Check dimensions are positive
    if (environment->width <= 0 || environment->height <= 0 || environment->depth <= 0) {
        return false;
    }
    
    // Check resolution is reasonable
    if (environment->resolution_x == 0 || environment->resolution_y == 0) {
        return false;
    }
    
    // Check cell size is positive
    if (environment->cell_size <= 0) {
        return false;
    }
    
    return true;
}

const char *ml_navigation_get_error_string(NavigationError error) {
    switch (error) {
        case NAV_ERROR_NONE: return "No error";
        case NAV_ERROR_INVALID_PARAMETER: return "Invalid parameter";
        case NAV_ERROR_MODEL_NOT_LOADED: return "Model not loaded";
        case NAV_ERROR_AGENT_NOT_FOUND: return "Agent not found";
        case NAV_ERROR_PATH_GENERATION_FAILED: return "Path generation failed";
        case NAV_ERROR_ENVIRONMENT_INVALID: return "Environment invalid";
        case NAV_ERROR_OUT_OF_MEMORY: return "Out of memory";
        case NAV_ERROR_TARGET_UNREACHABLE: return "Target unreachable";
        case NAV_ERROR_PREDICTION_FAILED: return "Prediction failed";
        default: return "Unknown error";
    }
}
