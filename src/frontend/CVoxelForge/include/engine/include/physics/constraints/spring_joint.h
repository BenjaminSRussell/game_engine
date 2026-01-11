#ifndef SPRING_JOINT_H
#define SPRING_JOINT_H

#include "physics/constraint_solver.h"
#include "math/vec3.h"
#include "math/quat.h"
#include <stdbool.h>

#ifdef __cplusplus
extern "C" {
#endif

// Spring Joint Context
typedef struct {
    SpringConstraint constraint;
    
    // Runtime data
    Vec3 world_anchor_a;
    Vec3 world_anchor_b;
    Vec3 world_direction;
    float current_length;
    float current_velocity;
    
    // Spring state
    float accumulated_impulse;
    float previous_lambda;
    
    // Warm starting data
    bool warm_started;
    
    // Breaking detection
    float current_force;
    bool broken;
} SpringJointContext;

// Spring Joint Functions
SpringJointContext* spring_create(EntityID body_a, EntityID body_b,
                                 const Vec3* anchor_a, const Vec3* anchor_b,
                                 float rest_length, float stiffness, float damping);
void spring_destroy(SpringJointContext* spring);

// Constraint solving
void spring_prepare(SpringJointContext* spring, const RigidBody* body_a, const RigidBody* body_b);
void spring_solve_velocity(SpringJointContext* spring, RigidBody* body_a, RigidBody* body_b, float dt);
void spring_solve_position(SpringJointContext* spring, RigidBody* body_a, RigidBody* body_b, float dt);

// Spring parameters
void spring_set_stiffness(SpringJointContext* spring, float stiffness);
void spring_set_damping(SpringJointContext* spring, float damping);
void spring_set_rest_length(SpringJointContext* spring, float rest_length);
void spring_set_max_force(SpringJointContext* spring, float max_force);

// Utility functions
float spring_get_current_length(const SpringJointContext* spring);
float spring_get_current_force(const SpringJointContext* spring);
bool spring_is_broken(const SpringJointContext* spring);

// Warm starting
void spring_warm_start(SpringJointContext* spring);
void spring_save_impulse(SpringJointContext* spring);

// Debug visualization
void spring_debug_draw(const SpringJointContext* spring);

#ifdef __cplusplus
}
#endif

#endif // SPRING_JOINT_H
