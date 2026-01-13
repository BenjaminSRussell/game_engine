#ifndef MOTION_DATABASE_H
#define MOTION_DATABASE_H

// Include math/math.h first to ensure vector types are defined correctly
// before core/types.h can interfere or cause redefinition issues
#include "math/math.h"
#include "core/types.h"
#include <stdbool.h>

// Constants
#define MOTION_MATCHING_TRAJECTORY_POINTS 5

typedef struct {
    Vec3 velocity;
    Vec3 angular_velocity;
    // Trajectory positions relative to root (future)
    Vec3 trajectory_pos[MOTION_MATCHING_TRAJECTORY_POINTS];
    // Trajectory directions relative to root (future)
    Vec3 trajectory_dir[MOTION_MATCHING_TRAJECTORY_POINTS];
    // Foot positions relative to root
    Vec3 left_foot_pos;
    Vec3 right_foot_pos;
    // Foot velocities
    Vec3 left_foot_vel;
    Vec3 right_foot_vel;
} MotionFeatures;

typedef struct {
    u32 animation_id;
    float time;
    MotionFeatures features;

    // Index in the global pose array if needed for retrieval
    u32 frame_index;
} MotionDbPose;

typedef struct MotionDatabase MotionDatabase;

// API

// Create a new motion database with the specified capacity
MotionDatabase* motion_database_create(u32 capacity);

// Destroy the motion database and free resources
void motion_database_destroy(MotionDatabase* db);

// Add a pose to the database. Returns true on success.
bool motion_database_add_pose(MotionDatabase* db, const MotionDbPose* pose);

// Search for the best match given a query feature vector
// Returns index of the best pose in the database, or -1 if failed
// out_cost: if not NULL, will be filled with the cost of the best match
int motion_database_search(MotionDatabase* db, const MotionFeatures* query, float* out_cost);

// Populate features from raw data (helper)
void motion_database_extract_features(MotionFeatures* out_features,
                                    Vec3 velocity,
                                    Vec3* trajectory_pos,
                                    Vec3* trajectory_dir,
                                    Vec3 left_foot_pos, Vec3 right_foot_pos,
                                    Vec3 left_foot_vel, Vec3 right_foot_vel);

// Get a pose from the database by index
const MotionDbPose* motion_database_get_pose(MotionDatabase* db, u32 index);

// Get the number of poses in the database
u32 motion_database_get_pose_count(MotionDatabase* db);

#endif // MOTION_DATABASE_H
