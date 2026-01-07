/**
 * =================================================================================================
 *                          VEHICLE SUSPENSION SYSTEM
 * =================================================================================================
 */

#ifndef SUSPENSION_H
#define SUSPENSION_H

#include <stdbool.h>

typedef struct RigidBody RigidBody;
typedef struct PhysicsWorld PhysicsWorld;

// Suspension configuration
typedef struct {
    // Spring properties
    float rest_length;          // Uncompressed length (m)
    float spring_stiffness;     // N/m (typical: 10000-50000)
    float damping_coefficient;  // Ns/m (typical: 1000-5000)
    
    // Limits
    float max_compression;      // Maximum compression distance
    float max_extension;        // Maximum extension distance
    
    // Anti-squat / Anti-dive
    float anti_squat_factor;    // 0.0 = none, 1.0 = full
    float anti_dive_factor;     // 0.0 = none, 1.0 = full
    
    // Camber
    float camber_angle;         // Wheel tilt (radians)
    float camber_rate;          // Change per meter of compression
    
    // Attachment points
    float wheel_attach_local[3];  // Local position on body
    float ray_direction[3];       // Usually (0, -1, 0)
} SuspensionConfig;

// Runtime suspension state
typedef struct {
    float current_length;
    float compression;          // 0.0 = rest, 1.0 = max compressed
    float compression_velocity;
    
    bool in_contact;
    float contact_point[3];
    float contact_normal[3];
    
    float force_magnitude;
    float current_camber;
} SuspensionState;

// Suspension wheel
typedef struct {
    SuspensionConfig config;
    SuspensionState state;
    
    RigidBody *attached_body;
    PhysicsWorld *world;
} Suspension;

// API
Suspension *suspension_create(RigidBody *body, PhysicsWorld *world, const SuspensionConfig *config);
void suspension_destroy(Suspension *susp);
void suspension_update(Suspension *susp, float dt);
void suspension_apply_forces(Suspension *susp);

// Queries
float suspension_get_compression(const Suspension *susp);
bool suspension_is_grounded(const Suspension *susp);
float suspension_get_force(const Suspension *susp);

#endif // SUSPENSION_H
