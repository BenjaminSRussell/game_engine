/**
 * =================================================================================================
 *                          MOTION MATCHING ANIMATION SYSTEM
 * =================================================================================================
 * 
 * PURPOSE: Data-driven animation using pose search instead of state machines
 * APPROACH: Build feature vectors from game state, find nearest pose in database
 */

#include <math.h>
#include <stdlib.h>
#include <string.h>
#include <float.h>

#define MAX_POSES 10000
#define FEATURE_DIM 32
#define SEARCH_WINDOW 60  // Search forward 1 second at 60fps

// =================================================================================================
// POSE DATABASE
// =================================================================================================

typedef struct {
    // Skeletal pose data
    float joint_positions[32][3];  // 32 joints max
    float joint_rotations[32][4];  // Quaternions
    float root_position[3];
    float root_rotation[4];
    
    // Feature vector for search
    float features[FEATURE_DIM];
    
    //Metadata
    uint32_t animation_id;
    uint32_t frame_index;
    float timestamp;
} Pose;

typedef struct {
    Pose* poses;
    uint32_t pose_count;
    uint32_t pose_capacity;
    
    // Current playback state
    uint32_t current_pose_idx;
    float blend_time;
    
    // Search costs
    float* feature_weights;
} MotionDatabase;

// =================================================================================================
// FEATURE EXTRACTION
// =================================================================================================

static void extract_pose_features(Pose* pose, float trajectory[6][3], float velocity[3]) {
    // Feature vector components:
    // - Current root velocity (3D)
    // - Future trajectory positions (6 samples * 3D = 18D)
    // - Foot positions relative to root (2 feet * 3D = 6D)
    // - Body facing direction (3D)
    // Total: 3 + 18 + 6 + 3 = 30D (rounded to 32 for alignment)
    
    uint32_t idx = 0;
    
    // Root velocity
    pose->features[idx++] = velocity[0];
    pose->features[idx++] = velocity[1];
    pose->features[idx++] = velocity[2];
    
    // Trajectory samples
    for (int i = 0; i < 6; i++) {
        pose->features[idx++] = trajectory[i][0];
        pose->features[idx++] = trajectory[i][1];
        pose->features[idx++] = trajectory[i][2];
    }
    
    // Left foot position (joint 14)
    pose->features[idx++] = pose->joint_positions[14][0] - pose->root_position[0];
    pose->features[idx++] = pose->joint_positions[14][1] - pose->root_position[1];
    pose->features[idx++] = pose->joint_positions[14][2] - pose->root_position[2];
    
    // Right foot position (joint 15)
    pose->features[idx++] = pose->joint_positions[15][0] - pose->root_position[0];
    pose->features[idx++] = pose->joint_positions[15][1] - pose->root_position[1];
    pose->features[idx++] = pose->joint_positions[15][2] - pose->root_position[2];
    
    // Facing direction (from root rotation)
    // Simplified: just use forward vector from quaternion
    float qw = pose->root_rotation[3];
    float qy = pose->root_rotation[1];
    float forward[3] = {
        2.0f * (pose->root_rotation[0] * pose->root_rotation[2] + qw * qy),
        2.0f * (qy * pose->root_rotation[2] - qw * pose->root_rotation[0]),
        1.0f - 2.0f * (pose->root_rotation[0] * pose->root_rotation[0] + qy * qy)
    };
    
    pose->features[idx++] = forward[0];
    pose->features[idx++] = forward[1];
    pose->features[idx++] = forward[2];
}

// =================================================================================================
// INITIALIZATION
// =================================================================================================

MotionDatabase* motion_database_create(uint32_t capacity) {
    MotionDatabase* db = (MotionDatabase*)calloc(1, sizeof(MotionDatabase));
    
    db->pose_capacity = capacity;
    db->poses = (Pose*)calloc(capacity, sizeof(Pose));
    
    // Feature weights (can be tuned)
    db->feature_weights = (float*)malloc(FEATURE_DIM * sizeof(float));
    for (int i = 0; i < FEATURE_DIM; i++) {
        db->feature_weights[i] = 1.0f;
    }
    
    // Give higher weight to trajectory (indices 3-20)
    for (int i = 3; i < 21; i++) {
        db->feature_weights[i] = 2.0f;
    }
    
    return db;
}

void motion_database_destroy(MotionDatabase* db) {
    if (db) {
        free(db->poses);
        free(db->feature_weights);
        free(db);
    }
}

// =================================================================================================
// POSE SEARCH
// =================================================================================================

static float compute_pose_cost(const Pose* pose, const float query_features[FEATURE_DIM], 
                               const float* weights) {
    float cost = 0.0f;
    
    for (int i = 0; i < FEATURE_DIM; i++) {
        float diff = pose->features[i] - query_features[i];
        cost += weights[i] * diff * diff;
    }
    
    return cost;
}

uint32_t motion_database_search(MotionDatabase* db, float query_features[FEATURE_DIM]) {
    float best_cost = FLT_MAX;
    uint32_t best_idx = 0;
    
    // Search within temporal window from current pose
    uint32_t search_start = db->current_pose_idx;
    uint32_t search_end = (db->current_pose_idx + SEARCH_WINDOW) % db->pose_count;
    
    for (uint32_t i = search_start; i != search_end; i = (i + 1) % db->pose_count) {
        float cost = compute_pose_cost(&db->poses[i], query_features, db->feature_weights);
        
        if (cost < best_cost) {
            best_cost = cost;
            best_idx = i;
        }
    }
    
    return best_idx;
}

// =================================================================================================
// PLAYBACK & BLENDING
// =================================================================================================

void motion_database_update(MotionDatabase* db, float query_features[FEATURE_DIM], float dt) {
    // Find best matching pose
    uint32_t target_idx = motion_database_search(db, query_features);
    
    // Blend towards target pose
    db->blend_time += dt;
    float blend_alpha = fminf(db->blend_time / 0.1f, 1.0f); // 100ms blend
    
    if (blend_alpha >= 1.0f) {
        db->current_pose_idx = target_idx;
        db->blend_time = 0.0f;
    }
}

Pose* motion_database_get_current_pose(MotionDatabase* db) {
    return &db->poses[db->current_pose_idx];
}

// =================================================================================================
// HELPER: Load from Motion Capture Data (Stub)
// =================================================================================================

void motion_database_load_animation(MotionDatabase* db, const char* filepath) {
    // In production, would parse BVH/FBX/GLB files
    // For now, procedurally generate some idle/walk/run poses
    
    (void)filepath; // Stub
    
    // Example: Generate simple walk cycle
    for (uint32_t i = 0; i < 100; i++) {
        if (db->pose_count >= db->pose_capacity) break;
        
        Pose* pose = &db->poses[db->pose_count++];
        float t = (float)i / 100.0f;
        
        // Sinusoidal leg motion
        pose->joint_positions[14][1] = sinf(t * 6.28f) * 0.3f; // Left leg
        pose->joint_positions[15][1] = sinf(t * 6.28f + 3.14f) * 0.3f; // Right leg
        
        // Extract features
        float traj[6][3] = {0};
        float vel[3] = {1.0f, 0.0f, 0.0f};
        extract_pose_features(pose, traj, vel);
    }
}
