/**
 * Sequential Impulse Constraint Solver
 * 
 * Implements Box2D-style sequential impulse solver for contact constraints.
 * Solves position and velocity constraints iteratively.
 */

#include "constraint_solver.h"
#include <stdlib.h>
#include <string.h>
#include <math.h>
#include <assert.h>

// ========================================
// Constants
// ========================================

#define DEFAULT_MAX_CONSTRAINTS 1024
#define MIN_VELOCITY_THRESHOLD 0.005f
#define MIN_ANGULAR_THRESHOLD 0.005f

// ========================================
// Vector Math Utilities
// ========================================

static inline void vec3_add(float *result, const float *a, const float *b) {
    result[0] = a[0] + b[0];
    result[1] = a[1] + b[1];
    result[2] = a[2] + b[2];
}

static inline void vec3_sub(float *result, const float *a, const float *b) {
    result[0] = a[0] - b[0];
    result[1] = a[1] - b[1];
    result[2] = a[2] - b[2];
}

static inline void vec3_mul(float *result, const float *v, float s) {
    result[0] = v[0] * s;
    result[1] = v[1] * s;
    result[2] = v[2] * s;
}

static inline float vec3_dot(const float *a, const float *b) {
    return a[0] * b[0] + a[1] * b[1] + a[2] * b[2];
}

static inline void vec3_cross(float *result, const float *a, const float *b) {
    result[0] = a[1] * b[2] - a[2] * b[1];
    result[1] = a[2] * b[0] - a[0] * b[2];
    result[2] = a[0] * b[1] - a[1] * b[0];
}

static inline float vec3_length_sq(const float *v) {
    return vec3_dot(v, v);
}

static inline void vec3_normalize(float *result, const float *v) {
    float len_sq = vec3_length_sq(v);
    if (len_sq > 0.0f) {
        float inv_len = 1.0f / sqrtf(len_sq);
        vec3_mul(result, v, inv_len);
    } else {
        result[0] = result[1] = result[2] = 0.0f;
    }
}

// ========================================
// RigidBody Interface (simplified)
// ========================================

// These would normally be defined elsewhere - simplified for this implementation
static void rigidbody_get_velocity(const RigidBody *body, float *velocity) {
    // Simplified - would access actual velocity from rigidbody
    velocity[0] = body->velocity[0];
    velocity[1] = body->velocity[1];
    velocity[2] = body->velocity[2];
}

static void rigidbody_set_velocity(RigidBody *body, const float *velocity) {
    body->velocity[0] = velocity[0];
    body->velocity[1] = velocity[1];
    body->velocity[2] = velocity[2];
}

static void rigidbody_get_angular_velocity(const RigidBody *body, float *angular_velocity) {
    // Simplified - would access actual angular velocity
    angular_velocity[0] = body->angular_velocity[0];
    angular_velocity[1] = body->angular_velocity[1];
    angular_velocity[2] = body->angular_velocity[2];
}

static void rigidbody_set_angular_velocity(RigidBody *body, const float *angular_velocity) {
    body->angular_velocity[0] = angular_velocity[0];
    body->angular_velocity[1] = angular_velocity[1];
    body->angular_velocity[2] = angular_velocity[2];
}

static void rigidbody_get_position(const RigidBody *body, float *position) {
    position[0] = body->position[0];
    position[1] = body->position[1];
    position[2] = body->position[2];
}

static void rigidbody_apply_impulse(RigidBody *body, const float *impulse, const float *contact_point) {
    // Simplified impulse application
    if (body->inv_mass > 0.0f) {
        body->velocity[0] += impulse[0] * body->inv_mass;
        body->velocity[1] += impulse[1] * body->inv_mass;
        body->velocity[2] += impulse[2] * body->inv_mass;
    }
}

// ========================================
// ContactManifold Interface (simplified)
// ========================================

// Simplified contact manifold access
static void contact_manifold_get_point(const ContactManifold *manifold, int index, 
                                       float *point, float *normal, float *penetration) {
    // Simplified - would access actual manifold data
    if (index == 0) {
        point[0] = manifold->points[0].x;
        point[1] = manifold->points[0].y;
        point[2] = manifold->points[0].z;
        normal[0] = manifold->normal.x;
        normal[1] = manifold->normal.y;
        normal[2] = manifold->normal.z;
        *penetration = manifold->penetration;
    }
}

// ========================================
// Constraint Implementation
// ========================================

static void constraint_initialize(ContactConstraint *constraint, 
                                 RigidBody *body_a, RigidBody *body_b,
                                 const float *normal, const float *point,
                                 float penetration, float restitution,
                                 float static_friction, float kinetic_friction) {
    constraint->body_a = body_a;
    constraint->body_b = body_b;
    
    memcpy(constraint->normal, normal, sizeof(float) * 3);
    memcpy(constraint->point, point, sizeof(float) * 3);
    constraint->penetration = penetration;
    
    constraint->restitution = restitution;
    constraint->static_friction = static_friction;
    constraint->kinetic_friction = kinetic_friction;
    
    constraint->normal_impulse = 0.0f;
    constraint->tangent_impulse[0] = 0.0f;
    constraint->tangent_impulse[1] = 0.0f;
    
    constraint->enabled = true;
    constraint->persistent = false;
}

static void constraint_compute_effective_mass(ContactConstraint *constraint) {
    // Simplified effective mass calculation
    // In full implementation, would consider rotational inertia and contact point
    
    float inv_mass_a = constraint->body_a ? constraint->body_a->inv_mass : 0.0f;
    float inv_mass_b = constraint->body_b ? constraint->body_b->inv_mass : 0.0f;
    
    constraint->normal_mass = 1.0f / (inv_mass_a + inv_mass_b);
    constraint->tangent_mass[0] = constraint->normal_mass;
    constraint->tangent_mass[1] = constraint->normal_mass;
}

static void constraint_compute_relative_velocity(ContactConstraint *constraint) {
    float vel_a[3], vel_b[3];
    
    if (constraint->body_a) {
        rigidbody_get_velocity(constraint->body_a, vel_a);
    } else {
        vel_a[0] = vel_a[1] = vel_a[2] = 0.0f;
    }
    
    if (constraint->body_b) {
        rigidbody_get_velocity(constraint->body_b, vel_b);
    } else {
        vel_b[0] = vel_b[1] = vel_b[2] = 0.0f;
    }
    
    vec3_sub(constraint->relative_velocity, vel_a, vel_b);
    constraint->normal_velocity = vec3_dot(constraint->relative_velocity, constraint->normal);
    
    // Calculate tangent directions (simplified - would use orthonormal basis)
    float tangent1[3] = {1.0f, 0.0f, 0.0f};
    if (fabsf(constraint->normal[0]) > 0.9f) {
        tangent1[0] = 0.0f;
        tangent1[1] = 1.0f;
    }
    
    float temp[3];
    vec3_cross(temp, constraint->normal, tangent1);
    vec3_normalize(tangent1, temp);
    
    float tangent2[3];
    vec3_cross(tangent2, constraint->normal, tangent1);
    
    constraint->tangent_velocity[0] = vec3_dot(constraint->relative_velocity, tangent1);
    constraint->tangent_velocity[1] = vec3_dot(constraint->relative_velocity, tangent2);
}

// ========================================
// Public API Implementation
// ========================================

SolverConfig constraint_solver_get_default_config(void) {
    SolverConfig config = {
        .velocity_iterations = 8,
        .position_iterations = 3,
        .warm_starting = true,
        .position_correction = true,
        .baumgarte_factor = 0.2f,
        .slop = 0.005f,
        .max_linear_correction = 0.2f,
        .max_angular_correction = 0.2f
    };
    return config;
}

ConstraintSolver* constraint_solver_create(int max_constraints, const SolverConfig *config) {
    if (max_constraints <= 0) max_constraints = DEFAULT_MAX_CONSTRAINTS;
    
    ConstraintSolver *solver = (ConstraintSolver *)calloc(1, sizeof(ConstraintSolver));
    if (!solver) return NULL;
    
    solver->constraints = (ContactConstraint *)calloc(max_constraints, sizeof(ContactConstraint));
    if (!solver->constraints) {
        free(solver);
        return NULL;
    }
    
    solver->constraint_capacity = max_constraints;
    solver->constraint_count = 0;
    
    if (config) {
        solver->config = *config;
    } else {
        solver->config = constraint_solver_get_default_config();
    }
    
    return solver;
}

void constraint_solver_destroy(ConstraintSolver *solver) {
    if (solver) {
        free(solver->constraints);
        free(solver);
    }
}

void constraint_solver_clear(ConstraintSolver *solver) {
    if (solver) {
        solver->constraint_count = 0;
    }
}

int constraint_solver_add_contact(ConstraintSolver *solver, 
                                 const ContactManifold *manifold,
                                 RigidBody *body_a, RigidBody *body_b) {
    if (!solver || !manifold || solver->constraint_count >= solver->constraint_capacity) {
        return -1;
    }
    
    ContactConstraint *constraint = &solver->constraints[solver->constraint_count];
    
    float point[3], normal[3], penetration;
    contact_manifold_get_point(manifold, 0, point, normal, &penetration);
    
    // Get material properties from bodies (simplified)
    float restitution = 0.3f;
    float static_friction = 0.5f;
    float kinetic_friction = 0.3f;
    
    if (body_a) {
        restitution = body_a->restitution;
        static_friction = body_a->friction;
    }
    if (body_b) {
        restitution = (restitution + body_b->restitution) * 0.5f;
        static_friction = (static_friction + body_b->friction) * 0.5f;
        kinetic_friction = static_friction * 0.6f; // Simplified
    }
    
    constraint_initialize(constraint, body_a, body_b, normal, point, 
                        penetration, restitution, static_friction, kinetic_friction);
    
    constraint_compute_effective_mass(constraint);
    
    return solver->constraint_count++;
}

void constraint_solver_warm_start(ConstraintSolver *solver) {
    if (!solver || !solver->config.warm_starting) return;
    
    for (int i = 0; i < solver->constraint_count; i++) {
        ContactConstraint *constraint = &solver->constraints[i];
        if (!constraint->enabled || !constraint->persistent) continue;
        
        // Apply accumulated impulses from previous frame
        float impulse[3];
        vec3_mul(impulse, constraint->normal, constraint->normal_impulse);
        
        if (constraint->body_a) {
            rigidbody_apply_impulse(constraint->body_a, impulse, constraint->point);
        }
        if (constraint->body_b) {
            float neg_impulse[3];
            vec3_mul(neg_impulse, impulse, -1.0f);
            rigidbody_apply_impulse(constraint->body_b, neg_impulse, constraint->point);
        }
    }
}

void constraint_solver_solve_velocities(ConstraintSolver *solver, float dt) {
    if (!solver) return;
    
    for (int iteration = 0; iteration < solver->config.velocity_iterations; iteration++) {
        for (int i = 0; i < solver->constraint_count; i++) {
            ContactConstraint *constraint = &solver->constraints[i];
            if (!constraint->enabled) continue;
            
            constraint_compute_relative_velocity(constraint);
            
            // Normal impulse
            float vn = constraint->normal_velocity;
            float impulse = -constraint->normal_mass * (vn + constraint->restitution * vn);
            
            float new_impulse = fmaxf(constraint->normal_impulse + impulse, 0.0f);
            impulse = new_impulse - constraint->normal_impulse;
            constraint->normal_impulse = new_impulse;
            
            // Apply normal impulse
            float normal_impulse_vec[3];
            vec3_mul(normal_impulse_vec, constraint->normal, impulse);
            
            if (constraint->body_a) {
                rigidbody_apply_impulse(constraint->body_a, normal_impulse_vec, constraint->point);
            }
            if (constraint->body_b) {
                float neg_impulse[3];
                vec3_mul(neg_impulse, normal_impulse_vec, -1.0f);
                rigidbody_apply_impulse(constraint->body_b, neg_impulse, constraint->point);
            }
            
            // Friction impulse (simplified)
            for (int j = 0; j < 2; j++) {
                float vt = constraint->tangent_velocity[j];
                float friction_impulse = -constraint->tangent_mass[j] * vt;
                
                float max_friction = constraint->static_friction * constraint->normal_impulse;
                float new_friction_impulse = fmaxf(-max_friction, fminf(friction_impulse, max_friction));
                friction_impulse = new_friction_impulse - constraint->tangent_impulse[j];
                constraint->tangent_impulse[j] = new_friction_impulse;
                
                // Apply friction impulse (simplified - would use proper tangent directions)
                float friction_impulse_vec[3];
                vec3_mul(friction_impulse_vec, constraint->normal, friction_impulse * 0.1f); // Simplified
                
                if (constraint->body_a) {
                    rigidbody_apply_impulse(constraint->body_a, friction_impulse_vec, constraint->point);
                }
                if (constraint->body_b) {
                    float neg_impulse[3];
                    vec3_mul(neg_impulse, friction_impulse_vec, -1.0f);
                    rigidbody_apply_impulse(constraint->body_b, neg_impulse, constraint->point);
                }
            }
        }
    }
}

void constraint_solver_solve_positions(ConstraintSolver *solver) {
    if (!solver || !solver->config.position_correction) return;
    
    for (int iteration = 0; iteration < solver->config.position_iterations; iteration++) {
        for (int i = 0; i < solver->constraint_count; i++) {
            ContactConstraint *constraint = &solver->constraints[i];
            if (!constraint->enabled) continue;
            
            float penetration = constraint->penetration - solver->config.slop;
            if (penetration <= 0.0f) continue;
            
            float correction = fmaxf(-solver->config.max_linear_correction, 
                                    fminf(penetration * solver->config.baumgarte_factor, 
                                          solver->config.max_linear_correction));
            
            float position_impulse[3];
            vec3_mul(position_impulse, constraint->normal, correction);
            
            // Apply position correction
            if (constraint->body_a && constraint->body_a->inv_mass > 0.0f) {
                constraint->body_a->position[0] += position_impulse[0] * constraint->body_a->inv_mass;
                constraint->body_a->position[1] += position_impulse[1] * constraint->body_a->inv_mass;
                constraint->body_a->position[2] += position_impulse[2] * constraint->body_a->inv_mass;
            }
            
            if (constraint->body_b && constraint->body_b->inv_mass > 0.0f) {
                constraint->body_b->position[0] -= position_impulse[0] * constraint->body_b->inv_mass;
                constraint->body_b->position[1] -= position_impulse[1] * constraint->body_b->inv_mass;
                constraint->body_b->position[2] -= position_impulse[2] * constraint->body_b->inv_mass;
            }
        }
    }
}

void constraint_solver_cull(ConstraintSolver *solver) {
    if (!solver) return;
    
    int write_index = 0;
    for (int read_index = 0; read_index < solver->constraint_count; read_index++) {
        ContactConstraint *constraint = &solver->constraints[read_index];
        
        // Keep constraint if enabled and has significant penetration
        if (constraint->enabled && constraint->penetration > solver->config.slop) {
            if (write_index != read_index) {
                solver->constraints[write_index] = *constraint;
            }
            write_index++;
        }
    }
    
    solver->constraint_count = write_index;
}

void constraint_solver_set_config(ConstraintSolver *solver, const SolverConfig *config) {
    if (solver && config) {
        solver->config = *config;
    }
}

void constraint_solver_get_stats(const ConstraintSolver *solver, 
                                int *active_constraints, 
                                int *total_iterations) {
    if (!solver) return;
    
    int active = 0;
    for (int i = 0; i < solver->constraint_count; i++) {
        if (solver->constraints[i].enabled) {
            active++;
        }
    }
    
    if (active_constraints) *active_constraints = active;
    if (total_iterations) {
        *total_iterations = solver->config.velocity_iterations + solver->config.position_iterations;
    }
}
