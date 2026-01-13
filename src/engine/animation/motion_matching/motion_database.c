#include "animation/motion_matching/motion_database.h"
#include "core/logger.h"
#include <stdlib.h>
#include <string.h>
#include <float.h>
#include <math.h>

// Weights for cost function (tunable)
#define WEIGHT_VELOCITY 1.0f
#define WEIGHT_ANGULAR_VELOCITY 1.0f
#define WEIGHT_TRAJECTORY_POS 2.0f
#define WEIGHT_TRAJECTORY_DIR 1.5f
#define WEIGHT_FOOT_POS 1.5f
#define WEIGHT_FOOT_VEL 1.0f

struct MotionDatabase {
    MotionDbPose* poses;
    u32 pose_count;
    u32 pose_capacity;

    // Configurable weights
    float weight_velocity;
    float weight_angular_velocity;
    float weight_trajectory_pos;
    float weight_trajectory_dir;
    float weight_foot_pos;
    float weight_foot_vel;
};

MotionDatabase* motion_database_create(u32 capacity) {
    if (capacity == 0) {
        LOG_ERROR(LOG_CAT_ANIMATION, "Cannot create motion database with 0 capacity");
        return NULL;
    }

    MotionDatabase* db = (MotionDatabase*)calloc(1, sizeof(MotionDatabase));
    if (!db) {
        LOG_ERROR(LOG_CAT_ANIMATION, "Failed to allocate MotionDatabase");
        return NULL;
    }

    db->poses = (MotionDbPose*)calloc(capacity, sizeof(MotionDbPose));
    if (!db->poses) {
        LOG_ERROR(LOG_CAT_ANIMATION, "Failed to allocate poses array for MotionDatabase");
        free(db);
        return NULL;
    }

    db->pose_capacity = capacity;
    db->pose_count = 0;

    // Set default weights
    db->weight_velocity = WEIGHT_VELOCITY;
    db->weight_angular_velocity = WEIGHT_ANGULAR_VELOCITY;
    db->weight_trajectory_pos = WEIGHT_TRAJECTORY_POS;
    db->weight_trajectory_dir = WEIGHT_TRAJECTORY_DIR;
    db->weight_foot_pos = WEIGHT_FOOT_POS;
    db->weight_foot_vel = WEIGHT_FOOT_VEL;

    LOG_INFO(LOG_CAT_ANIMATION, "Created MotionDatabase with capacity %u", capacity);
    return db;
}

void motion_database_destroy(MotionDatabase* db) {
    if (db) {
        if (db->poses) {
            free(db->poses);
        }
        free(db);
        LOG_INFO(LOG_CAT_ANIMATION, "Destroyed MotionDatabase");
    }
}

bool motion_database_add_pose(MotionDatabase* db, const MotionDbPose* pose) {
    if (!db || !pose) return false;

    if (db->pose_count >= db->pose_capacity) {
        LOG_WARN(LOG_CAT_ANIMATION, "MotionDatabase full (%u/%u), cannot add pose", db->pose_count, db->pose_capacity);
        return false;
    }

    db->poses[db->pose_count] = *pose;
    db->pose_count++;
    return true;
}

static float compute_cost(const MotionFeatures* a, const MotionFeatures* b, const MotionDatabase* db) {
    float cost = 0.0f;

    // Velocity cost
    cost += vec3_distance_sq(a->velocity, b->velocity) * db->weight_velocity;

    // Angular velocity cost (assuming it's a vector, e.g. axis-angle or just Euler rates)
    // Using distance squared on the vector components
    cost += vec3_distance_sq(a->angular_velocity, b->angular_velocity) * db->weight_angular_velocity;

    // Trajectory cost
    for (int i = 0; i < MOTION_MATCHING_TRAJECTORY_POINTS; i++) {
        cost += vec3_distance_sq(a->trajectory_pos[i], b->trajectory_pos[i]) * db->weight_trajectory_pos;
        cost += vec3_distance_sq(a->trajectory_dir[i], b->trajectory_dir[i]) * db->weight_trajectory_dir;
    }

    // Foot cost
    cost += vec3_distance_sq(a->left_foot_pos, b->left_foot_pos) * db->weight_foot_pos;
    cost += vec3_distance_sq(a->right_foot_pos, b->right_foot_pos) * db->weight_foot_pos;
    cost += vec3_distance_sq(a->left_foot_vel, b->left_foot_vel) * db->weight_foot_vel;
    cost += vec3_distance_sq(a->right_foot_vel, b->right_foot_vel) * db->weight_foot_vel;

    return cost;
}

int motion_database_search(MotionDatabase* db, const MotionFeatures* query, float* out_cost) {
    if (!db || !query || db->pose_count == 0) {
        return -1;
    }

    float best_cost = FLT_MAX;
    int best_index = -1;

    // Brute force search
    // Optimization TODO: Use spatial partitioning (KD-Tree / VP-Tree) or PCA dimensionality reduction
    for (u32 i = 0; i < db->pose_count; i++) {
        float cost = compute_cost(&db->poses[i].features, query, db);

        if (cost < best_cost) {
            best_cost = cost;
            best_index = (int)i;
        }
    }

    if (out_cost) {
        *out_cost = best_cost;
    }

    return best_index;
}

void motion_database_extract_features(MotionFeatures* out_features,
                                    Vec3 velocity,
                                    Vec3* trajectory_pos,
                                    Vec3* trajectory_dir,
                                    Vec3 left_foot_pos, Vec3 right_foot_pos,
                                    Vec3 left_foot_vel, Vec3 right_foot_vel) {
    if (!out_features) return;

    out_features->velocity = velocity;

    // Calculate angular velocity estimate if not provided (stub for now, assumed caller handles it)
    out_features->angular_velocity = vec3_zero(); // TODO: Implement if needed, or pass as arg

    if (trajectory_pos) {
        for (int i = 0; i < MOTION_MATCHING_TRAJECTORY_POINTS; i++) {
            out_features->trajectory_pos[i] = trajectory_pos[i];
        }
    } else {
        memset(out_features->trajectory_pos, 0, sizeof(out_features->trajectory_pos));
    }

    if (trajectory_dir) {
        for (int i = 0; i < MOTION_MATCHING_TRAJECTORY_POINTS; i++) {
            out_features->trajectory_dir[i] = trajectory_dir[i];
        }
    } else {
         memset(out_features->trajectory_dir, 0, sizeof(out_features->trajectory_dir));
    }

    out_features->left_foot_pos = left_foot_pos;
    out_features->right_foot_pos = right_foot_pos;
    out_features->left_foot_vel = left_foot_vel;
    out_features->right_foot_vel = right_foot_vel;
}

const MotionDbPose* motion_database_get_pose(MotionDatabase* db, u32 index) {
    if (!db || index >= db->pose_count) return NULL;
    return &db->poses[index];
}

u32 motion_database_get_pose_count(MotionDatabase* db) {
    return db ? db->pose_count : 0;
}
