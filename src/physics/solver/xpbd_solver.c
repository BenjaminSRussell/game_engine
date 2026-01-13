/**
 * @file xpbd_solver.c
 * @brief Extended Position Based Dynamics (XPBD) solver implementation
 *
 * Implements XPBD for soft body and cloth simulation with support for
 * various constraint types, sub-stepping, and adaptive time stepping.
 */

#include "xpbd_solver.h"
#include <stdlib.h>
#include <string.h>
#include <math.h>
#include <assert.h>

// ========================================
// Constants
// ========================================

#define MAX_PARTICLES 4096
#define MAX_CONSTRAINTS 8192
#define MAX_SUBSTEPS 4
#define MIN_SUBSTEP_TIME 0.001f
#define MAX_SUBSTEP_TIME 0.016f
#define DAMPING_FACTOR 0.99f
#define POSITION_EPSILON 0.0001f

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

static inline float vec3_length_sq(const float *v) {
    return vec3_dot(v, v);
}

static inline float vec3_length(const float *v) {
    return sqrtf(vec3_length_sq(v));
}

static inline void vec3_normalize(float *result, const float *v) {
    float len = vec3_length(v);
    if (len > 0.0001f) {
        vec3_mul(result, v, 1.0f / len);
    } else {
        result[0] = result[1] = result[2] = 0.0f;
    }
}

static inline void vec3_cross(float *result, const float *a, const float *b) {
    result[0] = a[1] * b[2] - a[2] * b[1];
    result[1] = a[2] * b[0] - a[0] * b[2];
    result[2] = a[0] * b[1] - a[1] * b[0];
}

static inline void vec3_copy(float *dest, const float *src) {
    dest[0] = src[0];
    dest[1] = src[1];
    dest[2] = src[2];
}

// ========================================
// Particle Structure
// ========================================

typedef struct XPBDParticle {
    float position[3];            // Current position
    float old_position[3];         // Previous position
    float velocity[3];            // Current velocity
    float force[3];                // Accumulated force
    float mass;                   // Particle mass
    float inv_mass;               // Inverse mass
    float radius;                 // Particle radius
    bool pinned;                  // Fixed in world space
    bool active;                  // Particle participates in simulation
    
} XPBDParticle;

// ========================================
// Constraint Types
// ========================================

typedef enum XPBDConstraintType {
    CONSTRAINT_DISTANCE,          // Distance constraint
    CONSTRAINT_BENDING,           // Bending constraint
    CONSTRAINT_VOLUME,            // Volume preservation
    CONSTRAINT_COLLISION,         // Collision constraint
    CONSTRAINT_ATTACHMENT          // Attachment to rigid body
} XPBDConstraintType;

// ========================================
// Distance Constraint
// ========================================

typedef struct XPBDDistanceConstraint {
    int particle_a;               // First particle index
    int particle_b;               // Second particle index
    float rest_length;            // Rest length
    float stiffness;              // Constraint stiffness
    float lambda;                  // Lagrange multiplier
    bool active;                  // Constraint is active
    
} XPBDDistanceConstraint;

// ========================================
// Bending Constraint
// ========================================

typedef struct XPBDBendingConstraint {
    int particle_a;               // First particle
    int particle_b;               // Second particle
    int particle_c;               // Third particle
    float rest_angle;             // Rest bending angle
    float stiffness;              // Constraint stiffness
    float lambda;                  // Lagrange multiplier
    bool active;                  // Constraint is active
    
} XPBDBendingConstraint;

// ========================================
// Volume Constraint
// ========================================

typedef struct XPBDVolumeConstraint {
    int particles[4];             // Tetrahedron particles
    float rest_volume;            // Rest volume
    float stiffness;              // Constraint stiffness
    float lambda;                  // Lagrange multiplier
    bool active;                  // Constraint is active
    
} XPBDVolumeConstraint;

// ========================================
// Collision Constraint
// ========================================

typedef struct XPBDCollisionConstraint {
    int particle_a;               // Particle index
    float normal[3];              // Collision normal
    float penetration;            // Penetration depth
    float lambda;                  // Lagrange multiplier
    bool active;                  // Constraint is active
    
} XPBDCollisionConstraint;

// ========================================
// Unified Constraint
// ========================================

typedef struct XPBDConstraint {
    XPBDConstraintType type;
    union {
        XPBDDistanceConstraint distance;
        XPBDBendingConstraint bending;
        XPBDVolumeConstraint volume;
        XPBDCollisionConstraint collision;
    } data;
    bool enabled;
    
} XPBDConstraint;

// ========================================
// XPBD Solver Structure
// ========================================

struct XPBDSolver {
    // Particles
    XPBDParticle particles[MAX_PARTICLES];
    int particle_count;
    int particle_capacity;
    
    // Constraints
    XPBDConstraint constraints[MAX_CONSTRAINTS];
    int constraint_count;
    int constraint_capacity;
    
    // Configuration
    XPBDConfig config;
    
    // Simulation state
    float current_time;            // Current simulation time
    float accumulated_time;        // Accumulated time for sub-stepping
    int substep_count;            // Number of substeps
    float substep_dt;             // Substep time step
    
    // Performance metrics
    float solve_time_ms;          // Time spent solving
    int particles_updated;         // Number of particles updated
    int constraints_solved;        // Number of constraints solved
    
};

// ========================================
// Constraint Solving Functions
// ========================================

static void solve_distance_constraint(XPBDParticle *particles, XPBDDistanceConstraint *constraint, float dt) {
    XPBDParticle *p1 = &particles[constraint->particle_a];
    XPBDParticle *p2 = &particles[constraint->particle_b];
    
    if (p1->inv_mass == 0.0f && p2->inv_mass == 0.0f) return;
    
    // Calculate current distance
    float delta[3];
    vec3_sub(delta, p2->position, p1->position);
    float current_length = vec3_length(delta);
    
    if (current_length < 0.0001f) return;
    
    // Calculate constraint function
    float C = current_length - constraint->rest_length;
    
    // Skip if constraint is satisfied
    if (fabsf(C) < POSITION_EPSILON) return;
    
    // Calculate gradient
    float grad_a[3], grad_b[3];
    vec3_normalize(grad_a, delta);
    vec3_mul(grad_b, grad_a, -1.0f);
    
    // Calculate effective mass
    float w1 = p1->inv_mass;
    float w2 = p2->inv_mass;
    float effective_mass = w1 * vec3_dot(grad_a, grad_a) + w2 * vec3_dot(grad_b, grad_b);
    
    if (effective_mass < 0.0001f) return;
    
    // Calculate Lagrange multiplier
    float alpha = 1.0f / (constraint->stiffness * dt * dt);
    float delta_lambda = -(C + alpha * constraint->lambda) / (effective_mass + alpha);
    constraint->lambda += delta_lambda;
    
    // Apply correction
    float correction_a[3], correction_b[3];
    vec3_mul(correction_a, grad_a, delta_lambda * w1);
    vec3_mul(correction_b, grad_b, delta_lambda * w2);
    
    vec3_add(p1->position, p1->position, correction_a);
    vec3_add(p2->position, p2->position, correction_b);
}

static void solve_bending_constraint(XPBDParticle *particles, XPBDBendingConstraint *constraint, float dt) {
    XPBDParticle *p1 = &particles[constraint->particle_a];
    XPBDParticle *p2 = &particles[constraint->particle_b];
    XPBDParticle *p3 = &particles[constraint->particle_c];
    
    if (p1->inv_mass == 0.0f && p2->inv_mass == 0.0f && p3->inv_mass == 0.0f) return;
    
    // Calculate current bending angle
    float v1[3], v2[3];
    vec3_sub(v1, p1->position, p2->position);
    vec3_sub(v2, p3->position, p2->position);
    
    float len1 = vec3_length(v1);
    float len2 = vec3_length(v2);
    
    if (len1 < 0.0001f || len2 < 0.0001f) return;
    
    vec3_normalize(v1, v1);
    vec3_normalize(v2, v2);
    
    float cos_angle = vec3_dot(v1, v2);
    cos_angle = fmaxf(-1.0f, fminf(1.0f, cos_angle));
    float current_angle = acosf(cos_angle);
    
    // Calculate constraint function
    float C = current_angle - constraint->rest_angle;
    
    if (fabsf(C) < 0.001f) return;
    
    // Simplified bending constraint - apply angular correction
    float correction = C * constraint->stiffness * dt * dt;
    
    // Apply correction to particles (simplified)
    float axis[3];
    vec3_cross(axis, v1, v2);
    if (vec3_length_sq(axis) < 0.0001f) return;
    
    vec3_normalize(axis, axis);
    
    // Rotate particles around axis
    for (int i = 0; i < 3; i++) {
        XPBDParticle *particle = (i == 0) ? p1 : (i == 1) ? p2 : p3;
        if (particle->inv_mass == 0.0f) continue;
        
        float r[3];
        vec3_sub(r, particle->position, p2->position);
        
        float rotation[3];
        vec3_cross(rotation, axis, r);
        vec3_mul(rotation, rotation, correction * 0.1f); // Scale down correction
        
        vec3_add(particle->position, particle->position, rotation);
    }
}

static void solve_volume_constraint(XPBDParticle *particles, XPBDVolumeConstraint *constraint, float dt) {
    XPBDParticle *p0 = &particles[constraint->particles[0]];
    XPBDParticle *p1 = &particles[constraint->particles[1]];
    XPBDParticle *p2 = &particles[constraint->particles[2]];
    XPBDParticle *p3 = &particles[constraint->particles[3]];
    
    float total_inv_mass = p0->inv_mass + p1->inv_mass + p2->inv_mass + p3->inv_mass;
    if (total_inv_mass < 0.0001f) return;
    
    // Calculate current volume using tetrahedron formula
    float v1[3], v2[3], v3[3];
    vec3_sub(v1, p1->position, p0->position);
    vec3_sub(v2, p2->position, p0->position);
    vec3_sub(v3, p3->position, p0->position);
    
    float cross_result[3];
    vec3_cross(cross_result, v2, v3);
    float current_volume = fabsf(vec3_dot(v1, cross_result)) / 6.0f;
    
    // Calculate constraint function
    float C = current_volume - constraint->rest_volume;
    
    if (fabsf(C) < 0.001f) return;
    
    // Calculate gradient for each particle
    float grad0[3], grad1[3], grad2[3], grad3[3];
    vec3_cross(grad0, v2, v3);
    vec3_cross(grad1, v3, v1);
    vec3_cross(grad2, v1, v3);
    vec3_cross(grad3, v2, v1);
    
    float scale = 1.0f / 6.0f;
    vec3_mul(grad0, grad0, scale);
    vec3_mul(grad1, grad1, scale);
    vec3_mul(grad2, grad2, scale);
    vec3_mul(grad3, grad3, scale);
    
    // Calculate effective mass
    float effective_mass = p0->inv_mass * vec3_dot(grad0, grad0) +
                         p1->inv_mass * vec3_dot(grad1, grad1) +
                         p2->inv_mass * vec3_dot(grad2, grad2) +
                         p3->inv_mass * vec3_dot(grad3, grad3);
    
    if (effective_mass < 0.0001f) return;
    
    // Calculate Lagrange multiplier
    float alpha = 1.0f / (constraint->stiffness * dt * dt);
    float delta_lambda = -(C + alpha * constraint->lambda) / (effective_mass + alpha);
    constraint->lambda += delta_lambda;
    
    // Apply corrections
    if (p0->inv_mass > 0.0f) {
        float correction[3];
        vec3_mul(correction, grad0, delta_lambda * p0->inv_mass);
        vec3_add(p0->position, p0->position, correction);
    }
    
    if (p1->inv_mass > 0.0f) {
        float correction[3];
        vec3_mul(correction, grad1, delta_lambda * p1->inv_mass);
        vec3_add(p1->position, p1->position, correction);
    }
    
    if (p2->inv_mass > 0.0f) {
        float correction[3];
        vec3_mul(correction, grad2, delta_lambda * p2->inv_mass);
        vec3_add(p2->position, p2->position, correction);
    }
    
    if (p3->inv_mass > 0.0f) {
        float correction[3];
        vec3_mul(correction, grad3, delta_lambda * p3->inv_mass);
        vec3_add(p3->position, p3->position, correction);
    }
}

static void solve_collision_constraint(XPBDParticle *particles, XPBDCollisionConstraint *constraint, float dt) {
    XPBDParticle *particle = &particles[constraint->particle_a];
    
    if (particle->inv_mass == 0.0f) return;
    
    // Calculate constraint function
    float C = vec3_dot(particle->position, constraint->normal) + constraint->penetration;
    
    if (C < 0.0f) return; // No penetration
    
    // Calculate gradient
    float grad[3];
    vec3_copy(grad, constraint->normal);
    
    // Calculate effective mass
    float effective_mass = particle->inv_mass * vec3_dot(grad, grad);
    if (effective_mass < 0.0001f) return;
    
    // Calculate Lagrange multiplier
    float alpha = 1.0f / (1000.0f * dt * dt); // High stiffness for collision
    float delta_lambda = -(C + alpha * constraint->lambda) / (effective_mass + alpha);
    constraint->lambda += delta_lambda;
    
    // Apply correction
    float correction[3];
    vec3_mul(correction, grad, delta_lambda * particle->inv_mass);
    vec3_add(particle->position, particle->position, correction);
}

// ========================================
// Public API Implementation
// ========================================

XPBDConfig xpbd_get_default_config(void) {
    XPBDConfig config = {
        .gravity = {0.0f, -9.81f, 0.0f},
        .damping = 0.99f,
        .max_substeps = 4,
        .min_substep_time = 0.001f,
        .max_substep_time = 0.016f,
        .position_iterations = 3,
        .velocity_iterations = 1,
        .enable_collision = true,
        .enable_volume_preservation = true,
        .enable_adaptive_substepping = true
    };
    return config;
}

XPBDSolver* xpbd_create(int max_particles, int max_constraints, const XPBDConfig *config) {
    if (max_particles <= 0) max_particles = MAX_PARTICLES;
    if (max_constraints <= 0) max_constraints = MAX_CONSTRAINTS;
    
    XPBDSolver *solver = (XPBDSolver *)calloc(1, sizeof(XPBDSolver));
    if (!solver) return NULL;
    
    solver->particle_capacity = max_particles;
    solver->constraint_capacity = max_constraints;
    solver->particle_count = 0;
    solver->constraint_count = 0;
    
    if (config) {
        solver->config = *config;
    } else {
        solver->config = xpbd_get_default_config();
    }
    
    solver->current_time = 0.0f;
    solver->accumulated_time = 0.0f;
    solver->substep_count = 0;
    solver->substep_dt = solver->config.min_substep_time;
    
    return solver;
}

void xpbd_destroy(XPBDSolver *solver) {
    if (solver) {
        free(solver);
    }
}

int xpbd_add_particle(XPBDSolver *solver, const float *position, float mass, bool pinned) {
    if (!solver || solver->particle_count >= solver->particle_capacity) {
        return -1;
    }
    
    XPBDParticle *particle = &solver->particles[solver->particle_count++];
    
    vec3_copy(particle->position, position);
    vec3_copy(particle->old_position, position);
    particle->velocity[0] = particle->velocity[1] = particle->velocity[2] = 0.0f;
    particle->force[0] = particle->force[1] = particle->force[2] = 0.0f;
    
    particle->mass = mass;
    particle->inv_mass = mass > 0.0f ? 1.0f / mass : 0.0f;
    particle->radius = 0.1f;
    particle->pinned = pinned;
    particle->active = true;
    
    return solver->particle_count - 1;
}

int xpbd_add_distance_constraint(XPBDSolver *solver, int particle_a, int particle_b, 
                                float stiffness) {
    if (!solver || solver->constraint_count >= solver->constraint_capacity) {
        return -1;
    }
    
    if (particle_a >= solver->particle_count || particle_b >= solver->particle_count) {
        return -1;
    }
    
    XPBDConstraint *constraint = &solver->constraints[solver->constraint_count++];
    constraint->type = CONSTRAINT_DISTANCE;
    constraint->enabled = true;
    
    XPBDDistanceConstraint *distance = &constraint->data.distance;
    distance->particle_a = particle_a;
    distance->particle_b = particle_b;
    
    // Calculate rest length
    XPBDParticle *p1 = &solver->particles[particle_a];
    XPBDParticle *p2 = &solver->particles[particle_b];
    float delta[3];
    vec3_sub(delta, p2->position, p1->position);
    distance->rest_length = vec3_length(delta);
    
    distance->stiffness = stiffness;
    distance->lambda = 0.0f;
    distance->active = true;
    
    return solver->constraint_count - 1;
}

int xpbd_add_bending_constraint(XPBDSolver *solver, int particle_a, int particle_b, 
                               int particle_c, float stiffness) {
    if (!solver || solver->constraint_count >= solver->constraint_capacity) {
        return -1;
    }
    
    XPBDConstraint *constraint = &solver->constraints[solver->constraint_count++];
    constraint->type = CONSTRAINT_BENDING;
    constraint->enabled = true;
    
    XPBDBendingConstraint *bending = &constraint->data.bending;
    bending->particle_a = particle_a;
    bending->particle_b = particle_b;
    bending->particle_c = particle_c;
    bending->stiffness = stiffness;
    bending->lambda = 0.0f;
    bending->active = true;
    
    // Calculate rest angle
    XPBDParticle *p1 = &solver->particles[particle_a];
    XPBDParticle *p2 = &solver->particles[particle_b];
    XPBDParticle *p3 = &solver->particles[particle_c];
    
    float v1[3], v2[3];
    vec3_sub(v1, p1->position, p2->position);
    vec3_sub(v2, p3->position, p2->position);
    
    float len1 = vec3_length(v1);
    float len2 = vec3_length(v2);
    
    if (len1 > 0.0001f && len2 > 0.0001f) {
        vec3_normalize(v1, v1);
        vec3_normalize(v2, v2);
        float cos_angle = vec3_dot(v1, v2);
        bending->rest_angle = acosf(fmaxf(-1.0f, fminf(1.0f, cos_angle)));
    } else {
        bending->rest_angle = 0.0f;
    }
    
    return solver->constraint_count - 1;
}

void xpbd_update(XPBDSolver *solver, float dt) {
    if (!solver || dt <= 0.0f) return;
    
    solver->solve_time_ms = 0.0f;
    solver->particles_updated = 0;
    solver->constraints_solved = 0;
    
    // Adaptive sub-stepping
    float remaining_time = dt;
    solver->substep_count = 0;
    
    while (remaining_time > 0.0f) {
        // Calculate substep time
        float substep_dt = fminf(remaining_time, solver->config.max_substep_time);
        
        if (solver->config.enable_adaptive_substepping) {
            // Adaptive substep based on maximum velocity
            float max_velocity = 0.0f;
            for (int i = 0; i < solver->particle_count; i++) {
                XPBDParticle *particle = &solver->particles[i];
                if (!particle->active) continue;
                
                float speed = vec3_length(particle->velocity);
                max_velocity = fmaxf(max_velocity, speed);
            }
            
            if (max_velocity > 0.0f) {
                float adaptive_dt = 0.5f / max_velocity; // CFL condition
                substep_dt = fminf(substep_dt, fmaxf(adaptive_dt, solver->config.min_substep_time));
            }
        }
        
        // Store old positions
        for (int i = 0; i < solver->particle_count; i++) {
            XPBDParticle *particle = &solver->particles[i];
            if (!particle->active) continue;
            
            vec3_copy(particle->old_position, particle->position);
        }
        
        // Apply forces
        for (int i = 0; i < solver->particle_count; i++) {
            XPBDParticle *particle = &solver->particles[i];
            if (!particle->active || particle->pinned) continue;
            
            // Apply gravity
            vec3_add(particle->force, particle->force, solver->config.gravity);
            
            // Apply damping
            vec3_mul(particle->velocity, particle->velocity, solver->config.damping);
        }
        
        // Integrate velocities
        for (int i = 0; i < solver->particle_count; i++) {
            XPBDParticle *particle = &solver->particles[i];
            if (!particle->active || particle->pinned) continue;
            
            if (particle->inv_mass > 0.0f) {
                float acceleration[3];
                vec3_mul(acceleration, particle->force, particle->inv_mass);
                vec3_mul(acceleration, acceleration, substep_dt);
                vec3_add(particle->velocity, particle->velocity, acceleration);
            }
        }
        
        // Predict positions
        for (int i = 0; i < solver->particle_count; i++) {
            XPBDParticle *particle = &solver->particles[i];
            if (!particle->active || particle->pinned) continue;
            
            float velocity_step[3];
            vec3_mul(velocity_step, particle->velocity, substep_dt);
            vec3_add(particle->position, particle->position, velocity_step);
        }
        
        // Solve constraints
        for (int iteration = 0; iteration < solver->config.position_iterations; iteration++) {
            for (int i = 0; i < solver->constraint_count; i++) {
                XPBDConstraint *constraint = &solver->constraints[i];
                if (!constraint->enabled) continue;
                
                switch (constraint->type) {
                    case CONSTRAINT_DISTANCE:
                        solve_distance_constraint(solver->particles, &constraint->data.distance, substep_dt);
                        break;
                    case CONSTRAINT_BENDING:
                        solve_bending_constraint(solver->particles, &constraint->data.bending, substep_dt);
                        break;
                    case CONSTRAINT_VOLUME:
                        solve_volume_constraint(solver->particles, &constraint->data.volume, substep_dt);
                        break;
                    case CONSTRAINT_COLLISION:
                        solve_collision_constraint(solver->particles, &constraint->data.collision, substep_dt);
                        break;
                    default:
                        break;
                }
                solver->constraints_solved++;
            }
        }
        
        // Update velocities from position changes
        for (int i = 0; i < solver->particle_count; i++) {
            XPBDParticle *particle = &solver->particles[i];
            if (!particle->active || particle->pinned) continue;
            
            float delta_pos[3];
            vec3_sub(delta_pos, particle->position, particle->old_position);
            vec3_mul(particle->velocity, delta_pos, 1.0f / substep_dt);
        }
        
        // Clear forces
        for (int i = 0; i < solver->particle_count; i++) {
            XPBDParticle *particle = &solver->particles[i];
            particle->force[0] = particle->force[1] = particle->force[2] = 0.0f;
        }
        
        remaining_time -= substep_dt;
        solver->substep_count++;
        solver->particles_updated = solver->particle_count;
    }
    
    solver->current_time += dt;
}

void xpbd_apply_force(XPBDSolver *solver, int particle_id, const float *force) {
    if (!solver || particle_id < 0 || particle_id >= solver->particle_count || !force) {
        return;
    }
    
    XPBDParticle *particle = &solver->particles[particle_id];
    if (particle->active && !particle->pinned) {
        vec3_add(particle->force, particle->force, force);
    }
}

void xpbd_set_particle_position(XPBDSolver *solver, int particle_id, const float *position) {
    if (!solver || particle_id < 0 || particle_id >= solver->particle_count || !position) {
        return;
    }
    
    XPBDParticle *particle = &solver->particles[particle_id];
    vec3_copy(particle->position, position);
    vec3_copy(particle->old_position, position);
}

void xpbd_pin_particle(XPBDSolver *solver, int particle_id, bool pinned) {
    if (!solver || particle_id < 0 || particle_id >= solver->particle_count) {
        return;
    }
    
    solver->particles[particle_id].pinned = pinned;
}

void xpbd_get_stats(const XPBDSolver *solver, int *particle_count, int *constraint_count, 
                    int *substep_count, float *solve_time) {
    if (!solver) return;
    
    if (particle_count) *particle_count = solver->particle_count;
    if (constraint_count) *constraint_count = solver->constraint_count;
    if (substep_count) *substep_count = solver->substep_count;
    if (solve_time) *solve_time = solver->solve_time_ms;
}

void xpbd_reset(XPBDSolver *solver) {
    if (!solver) return;
    
    solver->particle_count = 0;
    solver->constraint_count = 0;
    solver->current_time = 0.0f;
    solver->accumulated_time = 0.0f;
    solver->substep_count = 0;
    solver->substep_dt = solver->config.min_substep_time;
    
    solver->solve_time_ms = 0.0f;
    solver->particles_updated = 0;
    solver->constraints_solved = 0;
}

void xpbd_set_gravity(XPBDSolver *solver, const float *gravity) {
    if (!solver || !gravity) return;
    
    solver->config.gravity[0] = gravity[0];
    solver->config.gravity[1] = gravity[1];
    solver->config.gravity[2] = gravity[2];
}

bool xpbd_validate(const XPBDSolver *solver) {
    if (!solver) return false;
    
    if (solver->particle_count < 0 || solver->particle_count > MAX_PARTICLES) {
        return false;
    }
    
    if (solver->constraint_count < 0 || solver->constraint_count > MAX_CONSTRAINTS) {
        return false;
    }
    
    return true;
}
