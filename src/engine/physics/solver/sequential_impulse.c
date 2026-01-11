/**
 * @file sequential_impulse.c
 * @brief Sequential impulse constraint solver implementation
 *
 * Implements the sequential impulse method for solving contact constraints
 * with warm starting, position correction, and friction support.
 */

#include "sequential_impulse.h"
#include <stdlib.h>
#include <string.h>
#include <math.h>
#include <assert.h>

// ========================================
// Constants
// ========================================

#define MAX_CONTACT_POINTS 4
#define MIN_VELOCITY_THRESHOLD 0.01f
#define MIN_PENETRATION_THRESHOLD 0.001f
#define WARM_STARTING_FACTOR 0.8f
#define POSITION_CORRECTION_BAUMGARTE 0.2f

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

static inline void vec3_copy(float *dest, const float *src) {
    dest[0] = src[0];
    dest[1] = src[1];
    dest[2] = src[2];
}

// ========================================
// Contact Point Structure
// ========================================

typedef struct ContactPoint {
    float position[3];            // Contact position in world space
    float normal[3];              // Contact normal (pointing from A to B)
    float penetration;            // Penetration depth
    float normal_impulse;         // Normal impulse magnitude
    float tangent_impulse[2];     // Tangential impulse magnitudes (friction)
    float bias;                   // Position correction bias
    float normal_mass;            // Effective mass for normal direction
    float tangent_mass[2];       // Effective mass for tangential directions
    float r_a[3];                 // Contact point relative to body A center
    float r_b[3];                 // Contact point relative to body B center
    
} ContactPoint;

// ========================================
// Sequential Impulse Solver Structure
// ========================================

struct SequentialImpulseSolver {
    // Configuration
    SequentialImpulseConfig config;
    
    // Contact data
    ContactPoint contacts[MAX_CONTACTS];
    int contact_count;
    
    // Temporary storage
    float temp_velocities_a[MAX_BODIES][6]; // Linear and angular velocities for body A
    float temp_velocities_b[MAX_BODIES][6]; // Linear and angular velocities for body B
    
    // Solver state
    float accumulated_impulse;    // Total accumulated impulse
    int iteration_count;          // Current iteration count
    
    // Performance metrics
    float solve_time_ms;          // Time spent solving (milliseconds)
    int contacts_solved;          // Number of contacts solved
    
};

// ========================================
// Rigid Body Interface Implementation
// ========================================

static void get_body_properties(const RigidBody *body, float *mass, float *inertia, 
                               float *position, float *orientation, 
                               float *linear_velocity, float *angular_velocity) {
    if (!body) return;
    
    // In a real implementation, this would query the rigid body system
    // For now, we'll use placeholder values
    *mass = body->mass;
    *inertia = body->inertia; // Simplified - would be 3x3 tensor
    vec3_copy(position, body->position);
    vec3_copy(orientation, body->orientation);
    vec3_copy(linear_velocity, body->linear_velocity);
    vec3_copy(angular_velocity, body->angular_velocity);
}

static void apply_impulse_to_body(RigidBody *body, const float *impulse, const float *contact_point) {
    if (!body || !impulse || !contact_point) return;
    
    // Apply linear impulse
    if (body->mass > 0.0f) {
        float linear_impulse[3];
        vec3_mul(linear_impulse, impulse, 1.0f / body->mass);
        vec3_add(body->linear_velocity, body->linear_velocity, linear_impulse);
    }
    
    // Apply angular impulse
    float r[3];
    vec3_sub(r, contact_point, body->position);
    
    float angular_impulse[3];
    vec3_cross(angular_impulse, r, impulse);
    
    if (body->inertia > 0.0f) {
        vec3_mul(angular_impulse, angular_impulse, 1.0f / body->inertia);
        vec3_add(body->angular_velocity, body->angular_velocity, angular_impulse);
    }
}

// ========================================
// Contact Point Management
// ========================================

static void compute_contact_properties(ContactPoint *contact, const RigidBody *body_a, 
                                     const RigidBody *body_b) {
    if (!contact || !body_a || !body_b) return;
    
    // Compute relative positions
    vec3_sub(contact->r_a, contact->position, body_a->position);
    vec3_sub(contact->r_b, contact->position, body_b->position);
    
    // Compute effective masses
    float inv_mass_a = body_a->mass > 0.0f ? 1.0f / body_a->mass : 0.0f;
    float inv_mass_b = body_b->mass > 0.0f ? 1.0f / body_b->mass : 0.0f;
    
    // Normal effective mass
    float rn_a = vec3_dot(contact->r_a, contact->normal);
    float rn_b = vec3_dot(contact->r_b, contact->normal);
    
    float k_normal = inv_mass_a + inv_mass_b;
    k_normal += (rn_a * rn_a) / (body_a->inertia + 0.001f); // Add small epsilon
    k_normal += (rn_b * rn_b) / (body_b->inertia + 0.001f);
    
    contact->normal_mass = k_normal > 0.0f ? 1.0f / k_normal : 0.0f;
    
    // Tangent effective masses
    float tangent1[3], tangent2[3];
    
    // Create tangent vectors perpendicular to normal
    if (fabsf(contact->normal[0]) > 0.5f) {
        tangent1[0] = -contact->normal[1];
        tangent1[1] = contact->normal[0];
        tangent1[2] = 0.0f;
    } else {
        tangent1[0] = 0.0f;
        tangent1[1] = -contact->normal[2];
        tangent1[2] = contact->normal[1];
    }
    
    vec3_normalize(tangent1, tangent1);
    vec3_cross(tangent2, contact->normal, tangent1);
    
    float rt1_a = vec3_dot(contact->r_a, tangent1);
    float rt1_b = vec3_dot(contact->r_b, tangent1);
    float rt2_a = vec3_dot(contact->r_a, tangent2);
    float rt2_b = vec3_dot(contact->r_b, tangent2);
    
    float k_tangent1 = inv_mass_a + inv_mass_b;
    k_tangent1 += (rt1_a * rt1_a) / (body_a->inertia + 0.001f);
    k_tangent1 += (rt1_b * rt1_b) / (body_b->inertia + 0.001f);
    
    float k_tangent2 = inv_mass_a + inv_mass_b;
    k_tangent2 += (rt2_a * rt2_a) / (body_a->inertia + 0.001f);
    k_tangent2 += (rt2_b * rt2_b) / (body_b->inertia + 0.001f);
    
    contact->tangent_mass[0] = k_tangent1 > 0.0f ? 1.0f / k_tangent1 : 0.0f;
    contact->tangent_mass[1] = k_tangent2 > 0.0f ? 1.0f / k_tangent2 : 0.0f;
    
    // Compute position correction bias (Baumgarte stabilization)
    contact->bias = POSITION_CORRECTION_BAUMGARTE * contact->penetration;
}

static void compute_relative_velocity(ContactPoint *contact, const RigidBody *body_a, 
                                     const RigidBody *body_b, float *relative_velocity) {
    if (!contact || !body_a || !body_b || !relative_velocity) return;
    
    // Linear velocities
    float v_a[3], v_b[3];
    vec3_copy(v_a, body_a->linear_velocity);
    vec3_copy(v_b, body_b->linear_velocity);
    
    // Angular velocities contribution
    float omega_a_cross_r[3], omega_b_cross_r[3];
    vec3_cross(omega_a_cross_r, body_a->angular_velocity, contact->r_a);
    vec3_cross(omega_b_cross_r, body_b->angular_velocity, contact->r_b);
    
    // Relative velocity at contact point
    vec3_sub(relative_velocity, v_b, v_a);
    vec3_add(relative_velocity, relative_velocity, omega_b_cross_r);
    vec3_sub(relative_velocity, relative_velocity, omega_a_cross_r);
}

// ========================================
// Constraint Solving
// ========================================

static void solve_normal_impulse(ContactPoint *contact, RigidBody *body_a, RigidBody *body_b) {
    if (!contact || !body_a || !body_b) return;
    
    // Compute relative velocity
    float relative_velocity[3];
    compute_relative_velocity(contact, body_a, body_b, relative_velocity);
    
    // Normal velocity component
    float normal_velocity = vec3_dot(relative_velocity, contact->normal);
    
    // Don't resolve if velocities are separating
    if (normal_velocity > 0.0f) return;
    
    // Compute restitution
    float restitution = (body_a->restitution + body_b->restitution) * 0.5f;
    
    // Compute impulse magnitude
    float impulse_magnitude = -(1.0f + restitution) * normal_velocity + contact->bias;
    impulse_magnitude *= contact->normal_mass;
    
    // Clamp impulse
    float new_impulse = fmaxf(contact->normal_impulse + impulse_magnitude, 0.0f);
    impulse_magnitude = new_impulse - contact->normal_impulse;
    contact->normal_impulse = new_impulse;
    
    // Apply impulse
    if (fabsf(impulse_magnitude) > MIN_VELOCITY_THRESHOLD) {
        float impulse[3];
        vec3_mul(impulse, contact->normal, impulse_magnitude);
        
        apply_impulse_to_body(body_a, impulse, contact->position);
        vec3_mul(impulse, impulse, -1.0f);
        apply_impulse_to_body(body_b, impulse, contact->position);
    }
}

static void solve_friction_impulse(ContactPoint *contact, RigidBody *body_a, RigidBody *body_b) {
    if (!contact || !body_a || !body_b) return;
    
    // Compute relative velocity
    float relative_velocity[3];
    compute_relative_velocity(contact, body_a, body_b, relative_velocity);
    
    // Create tangent vectors
    float tangent1[3], tangent2[3];
    
    if (fabsf(contact->normal[0]) > 0.5f) {
        tangent1[0] = -contact->normal[1];
        tangent1[1] = contact->normal[0];
        tangent1[2] = 0.0f;
    } else {
        tangent1[0] = 0.0f;
        tangent1[1] = -contact->normal[2];
        tangent1[2] = contact->normal[1];
    }
    
    vec3_normalize(tangent1, tangent1);
    vec3_cross(tangent2, contact->normal, tangent1);
    
    // Compute friction coefficients
    float static_friction = sqrtf(body_a->static_friction * body_b->static_friction);
    float kinetic_friction = sqrtf(body_a->kinetic_friction * body_b->kinetic_friction);
    
    // Solve tangential impulses
    for (int i = 0; i < 2; i++) {
        float *tangent = (i == 0) ? tangent1 : tangent2;
        float tangent_velocity = vec3_dot(relative_velocity, tangent);
        
        // Compute impulse magnitude
        float impulse_magnitude = -tangent_velocity * contact->tangent_mass[i];
        
        // Clamp to friction cone
        float max_friction = static_friction * contact->normal_impulse;
        float new_impulse = fmaxf(fminf(contact->tangent_impulse[i] + impulse_magnitude, max_friction), -max_friction);
        impulse_magnitude = new_impulse - contact->tangent_impulse[i];
        contact->tangent_impulse[i] = new_impulse;
        
        // Apply impulse
        if (fabsf(impulse_magnitude) > MIN_VELOCITY_THRESHOLD) {
            float impulse[3];
            vec3_mul(impulse, tangent, impulse_magnitude);
            
            apply_impulse_to_body(body_a, impulse, contact->position);
            vec3_mul(impulse, impulse, -1.0f);
            apply_impulse_to_body(body_b, impulse, contact->position);
        }
    }
}

// ========================================
// Public API Implementation
// ========================================

SequentialImpulseSolver* sequential_impulse_create(const SequentialImpulseConfig *config) {
    SequentialImpulseSolver *solver = (SequentialImpulseSolver *)calloc(1, sizeof(SequentialImpulseSolver));
    if (!solver) return NULL;
    
    if (config) {
        solver->config = *config;
    } else {
        // Default configuration
        solver->config.max_iterations = 10;
        solver->config.position_correction = true;
        solver->config.warm_starting = true;
        solver->config.friction_enabled = true;
        solver->config.restitution_enabled = true;
        solver->config.min_velocity_threshold = MIN_VELOCITY_THRESHOLD;
        solver->config.min_penetration_threshold = MIN_PENETRATION_THRESHOLD;
    }
    
    solver->contact_count = 0;
    solver->accumulated_impulse = 0.0f;
    solver->iteration_count = 0;
    
    return solver;
}

void sequential_impulse_destroy(SequentialImpulseSolver *solver) {
    if (solver) {
        free(solver);
    }
}

void sequential_impulse_reset(SequentialImpulseSolver *solver) {
    if (!solver) return;
    
    solver->contact_count = 0;
    solver->accumulated_impulse = 0.0f;
    solver->iteration_count = 0;
}

int sequential_impulse_add_contact(SequentialImpulseSolver *solver, const Contact *contact) {
    if (!solver || !contact || solver->contact_count >= MAX_CONTACTS) {
        return -1;
    }
    
    ContactPoint *cp = &solver->contacts[solver->contact_count++];
    
    vec3_copy(cp->position, contact->position);
    vec3_copy(cp->normal, contact->normal);
    cp->penetration = contact->penetration;
    
    cp->normal_impulse = 0.0f;
    cp->tangent_impulse[0] = cp->tangent_impulse[1] = 0.0f;
    cp->bias = 0.0f;
    
    // Warm starting
    if (solver->config.warm_starting && contact->persistent_id >= 0) {
        // In a real implementation, this would look up previous impulse values
        cp->normal_impulse = contact->previous_normal_impulse * WARM_STARTING_FACTOR;
        cp->tangent_impulse[0] = contact->previous_tangent_impulse[0] * WARM_STARTING_FACTOR;
        cp->tangent_impulse[1] = contact->previous_tangent_impulse[1] * WARM_STARTING_FACTOR;
    }
    
    return solver->contact_count - 1;
}

void sequential_impulse_solve(SequentialImpulseSolver *solver, RigidBody *body_a, RigidBody *body_b, float dt) {
    if (!solver || !body_a || !body_b) return;
    
    solver->iteration_count = 0;
    solver->accumulated_impulse = 0.0f;
    
    // Precompute contact properties
    for (int i = 0; i < solver->contact_count; i++) {
        compute_contact_properties(&solver->contacts[i], body_a, body_b);
    }
    
    // Solve constraints iteratively
    for (int iteration = 0; iteration < solver->config.max_iterations; iteration++) {
        solver->iteration_count++;
        
        // Solve normal impulses
        for (int i = 0; i < solver->contact_count; i++) {
            ContactPoint *contact = &solver->contacts[i];
            
            // Skip if penetration is too small
            if (contact->penetration < solver->config.min_penetration_threshold) {
                continue;
            }
            
            solve_normal_impulse(contact, body_a, body_b);
            solver->accumulated_impulse += fabsf(contact->normal_impulse);
        }
        
        // Solve friction impulses
        if (solver->config.friction_enabled) {
            for (int i = 0; i < solver->contact_count; i++) {
                ContactPoint *contact = &solver->contacts[i];
                solve_friction_impulse(contact, body_a, body_b);
            }
        }
        
        // Position correction
        if (solver->config.position_correction && iteration == solver->config.max_iterations - 1) {
            for (int i = 0; i < solver->contact_count; i++) {
                ContactPoint *contact = &solver->contacts[i];
                
                if (contact->penetration > solver->config.min_penetration_threshold) {
                    float correction[3];
                    vec3_mul(correction, contact->normal, contact->bias * dt);
                    
                    // Apply position correction
                    float inv_mass_a = body_a->mass > 0.0f ? 1.0f / body_a->mass : 0.0f;
                    float inv_mass_b = body_b->mass > 0.0f ? 1.0f / body_b->mass : 0.0f;
                    
                    float total_mass = inv_mass_a + inv_mass_b;
                    if (total_mass > 0.0f) {
                        float mass_ratio_a = inv_mass_a / total_mass;
                        float mass_ratio_b = inv_mass_b / total_mass;
                        
                        float correction_a[3], correction_b[3];
                        vec3_mul(correction_a, correction, mass_ratio_a);
                        vec3_mul(correction_b, correction, -mass_ratio_b);
                        
                        vec3_add(body_a->position, body_a->position, correction_a);
                        vec3_add(body_b->position, body_b->position, correction_b);
                    }
                }
            }
        }
    }
}

void sequential_impulse_get_stats(const SequentialImpulseSolver *solver, 
                                 int *contact_count, int *iteration_count, 
                                 float *total_impulse) {
    if (!solver) return;
    
    if (contact_count) *contact_count = solver->contact_count;
    if (iteration_count) *iteration_count = solver->iteration_count;
    if (total_impulse) *total_impulse = solver->accumulated_impulse;
}

void sequential_impulse_set_config(SequentialImpulseSolver *solver, 
                                 const SequentialImpulseConfig *config) {
    if (solver && config) {
        solver->config = *config;
    }
}

SequentialImpulseConfig sequential_impulse_get_default_config(void) {
    SequentialImpulseConfig config = {
        .max_iterations = 10,
        .position_correction = true,
        .warm_starting = true,
        .friction_enabled = true,
        .restitution_enabled = true,
        .min_velocity_threshold = MIN_VELOCITY_THRESHOLD,
        .min_penetration_threshold = MIN_PENETRATION_THRESHOLD
    };
    return config;
}
