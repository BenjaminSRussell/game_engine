/**
 * @file physics_solver_system.c
 * @brief Unified Physics Solver System
 *
 * Consolidates XPBD and Sequential Impulse solvers into a single unified
 * physics system with support for soft body, rigid body, and voxel physics.
 */

#include "physics_solver_system.h"
#include "xpbd_solver.h"
#include "sequential_impulse.h"
#include <stdlib.h>
#include <string.h>
#include <math.h>

// ========================================
// Constants
// ========================================

#define MAX_RIGID_BODIES 1024
#define SOLVER_MAX_CONTACTS 512
#define GRAVITY_DEFAULT -9.81f

// ========================================
// Unified Physics Solver Structure
// ========================================

struct physics_solver_system {
    // Solver instances
    XPBDSolver *xpbd_solver;
    SequentialImpulseSolver *impulse_solver;
    
    // Configuration
    solver_type primary_solver;
    bool enable_hybrid_solving;
    
    // Rigid body storage
    RigidBody rigid_bodies[MAX_RIGID_BODIES];
    int rigid_body_count;
    
    // Contact storage
    Contact contacts[SOLVER_MAX_CONTACTS];
    int contact_count;
    
    // World properties
    float gravity[3];
    float time_step;
    
    // Performance metrics
    float solve_time_ms;
    int bodies_updated;
    int contacts_solved;
    int iterations_performed;
    
    // State flags
    bool initialized;
    bool paused;
};

// ========================================
// Forward Declarations
// ========================================

static void update_rigid_bodies(physics_solver_system *system, float dt);
static void detect_collisions(physics_solver_system *system);
static void solve_constraints(physics_solver_system *system, float dt);
static void apply_world_forces(physics_solver_system *system, float dt);

// ========================================
// Creation and Destruction
// ========================================

physics_solver_system* physics_solver_create(const physics_solver_config *config) {
    physics_solver_system *system = (physics_solver_system*)calloc(1, sizeof(physics_solver_system));
    if (!system) return NULL;
    
    // Set default configuration
    if (config) {
        system->primary_solver = config->primary_solver;
        system->enable_hybrid_solving = config->enable_hybrid_solving;
        system->time_step = config->time_step;
    } else {
        system->primary_solver = SOLVER_TYPE_XPBD;
        system->enable_hybrid_solving = false;
        system->time_step = 1.0f / 60.0f;
    }
    
    // Set default gravity
    system->gravity[0] = 0.0f;
    system->gravity[1] = GRAVITY_DEFAULT;
    system->gravity[2] = 0.0f;
    
    // Create solvers based on configuration
    if (system->primary_solver == SOLVER_TYPE_XPBD || system->enable_hybrid_solving) {
        XPBDConfig xpbd_config = xpbd_get_default_config();
        xpbd_config.gravity[0] = system->gravity[0];
        xpbd_config.gravity[1] = system->gravity[1];
        xpbd_config.gravity[2] = system->gravity[2];
        
        system->xpbd_solver = xpbd_create(4096, 8192, &xpbd_config);
        if (!system->xpbd_solver) {
            free(system);
            return NULL;
        }
    }
    
    if (system->primary_solver == SOLVER_TYPE_SEQUENTIAL_IMPULSE || system->enable_hybrid_solving) {
        SequentialImpulseConfig impulse_config = sequential_impulse_get_default_config();
        system->impulse_solver = sequential_impulse_create(&impulse_config);
        if (!system->impulse_solver) {
            if (system->xpbd_solver) {
                xpbd_destroy(system->xpbd_solver);
            }
            free(system);
            return NULL;
        }
    }
    
    system->initialized = true;
    return system;
}

void physics_solver_destroy(physics_solver_system *system) {
    if (!system) return;
    
    if (system->xpbd_solver) {
        xpbd_destroy(system->xpbd_solver);
    }
    
    if (system->impulse_solver) {
        sequential_impulse_destroy(system->impulse_solver);
    }
    
    free(system);
}

// ========================================
// Rigid Body Management
// ========================================

int physics_solver_add_rigid_body(physics_solver_system *system, const RigidBody *body) {
    if (!system || !body || system->rigid_body_count >= MAX_RIGID_BODIES) {
        return -1;
    }
    
    int body_id = system->rigid_body_count++;
    memcpy(&system->rigid_bodies[body_id], body, sizeof(RigidBody));
    
    // If using XPBD solver, create corresponding particles
    if (system->xpbd_solver && !body->is_static) {
        xpbd_add_particle(system->xpbd_solver, body->position, body->mass, false);
    }
    
    return body_id;
}

void physics_solver_remove_rigid_body(physics_solver_system *system, int body_id) {
    if (!system || body_id < 0 || body_id >= system->rigid_body_count) {
        return;
    }
    
    // Remove from array by shifting
    for (int i = body_id; i < system->rigid_body_count - 1; i++) {
        system->rigid_bodies[i] = system->rigid_bodies[i + 1];
    }
    
    system->rigid_body_count--;
}

RigidBody* physics_solver_get_rigid_body(physics_solver_system *system, int body_id) {
    if (!system || body_id < 0 || body_id >= system->rigid_body_count) {
        return NULL;
    }
    
    return &system->rigid_bodies[body_id];
}

// ========================================
// Contact Management
// ========================================

int physics_solver_add_contact(physics_solver_system *system, const Contact *contact) {
    if (!system || !contact || system->contact_count >= SOLVER_MAX_CONTACTS) {
        return -1;
    }
    
    int contact_id = system->contact_count++;
    memcpy(&system->contacts[contact_id], contact, sizeof(Contact));
    
    // Add to sequential impulse solver if available
    if (system->impulse_solver) {
        sequential_impulse_add_contact(system->impulse_solver, contact);
    }
    
    return contact_id;
}

void physics_solver_clear_contacts(physics_solver_system *system) {
    if (!system) return;
    
    system->contact_count = 0;
    
    if (system->impulse_solver) {
        sequential_impulse_reset(system->impulse_solver);
    }
}

// ========================================
// Simulation Interface
// ========================================

void physics_solver_step(physics_solver_system *system, float dt) {
    if (!system || !system->initialized || system->paused) {
        return;
    }
    
    float solve_start_time = 0.0f; // Would use high-resolution timer in real implementation
    
    // Reset performance metrics
    system->solve_time_ms = 0.0f;
    system->bodies_updated = 0;
    system->contacts_solved = 0;
    system->iterations_performed = 0;
    
    // Apply world forces (gravity, etc.)
    apply_world_forces(system, dt);
    
    // Update rigid bodies
    update_rigid_bodies(system, dt);
    
    // Detect collisions
    detect_collisions(system);
    
    // Solve constraints
    solve_constraints(system, dt);
    
    // Update performance metrics
    system->solve_time_ms = 0.0f; // Would calculate actual time
}

void physics_solver_update(physics_solver_system *system, float dt) {
    if (!system) return;
    
    // Use fixed time step with sub-stepping
    float accumulated_time = dt;
    
    while (accumulated_time >= system->time_step) {
        physics_solver_step(system, system->time_step);
        accumulated_time -= system->time_step;
    }
    
    // Handle remaining time
    if (accumulated_time > 0.0f) {
        physics_solver_step(system, accumulated_time);
    }
}

// ========================================
// Configuration Management
// ========================================

void physics_solver_set_gravity(physics_solver_system *system, const float *gravity) {
    if (!system || !gravity) return;
    
    memcpy(system->gravity, gravity, sizeof(float) * 3);
    
    // Update solver gravity
    if (system->xpbd_solver) {
        xpbd_set_gravity(system->xpbd_solver, gravity);
    }
}

void physics_solver_set_time_step(physics_solver_system *system, float time_step) {
    if (!system || time_step <= 0.0f) return;
    
    system->time_step = time_step;
}

void physics_solver_pause(physics_solver_system *system, bool paused) {
    if (!system) return;
    
    system->paused = paused;
}

void physics_solver_reset(physics_solver_system *system) {
    if (!system) return;
    
    system->rigid_body_count = 0;
    system->contact_count = 0;
    
    if (system->xpbd_solver) {
        xpbd_reset(system->xpbd_solver);
    }
    
    if (system->impulse_solver) {
        sequential_impulse_reset(system->impulse_solver);
    }
}

// ========================================
// Statistics and Debugging
// ========================================

void physics_solver_get_stats(const physics_solver_system *system, physics_solver_stats *stats) {
    if (!system || !stats) return;
    
    stats->rigid_body_count = system->rigid_body_count;
    stats->contact_count = system->contact_count;
    stats->solve_time_ms = system->solve_time_ms;
    stats->bodies_updated = system->bodies_updated;
    stats->contacts_solved = system->contacts_solved;
    stats->iterations_performed = system->iterations_performed;
    
    // Get solver-specific stats
    if (system->xpbd_solver) {
        int particle_count, constraint_count, substep_count;
        float solve_time;
        xpbd_get_stats(system->xpbd_solver, &particle_count, &constraint_count, 
                      &substep_count, &solve_time);
        stats->xpbd_particle_count = particle_count;
        stats->xpbd_constraint_count = constraint_count;
        stats->xpbd_substep_count = substep_count;
    }
    
    if (system->impulse_solver) {
        int contact_count, iteration_count;
        float total_impulse;
        sequential_impulse_get_stats(system->impulse_solver, &contact_count, 
                                    &iteration_count, &total_impulse);
        stats->impulse_iteration_count = iteration_count;
        stats->impulse_total_impulse = total_impulse;
    }
}

bool physics_solver_validate(const physics_solver_system *system) {
    if (!system) return false;
    
    if (!system->initialized) return false;
    
    // Validate solver states
    if (system->xpbd_solver && !xpbd_validate(system->xpbd_solver)) {
        return false;
    }
    
    if (system->impulse_solver && !sequential_impulse_validate(system->impulse_solver)) {
        return false;
    }
    
    return true;
}

// ========================================
// Internal Helper Functions
// ========================================

static void apply_world_forces(physics_solver_system *system, float dt) {
    if (!system) return;
    
    // Apply gravity to all dynamic rigid bodies
    for (int i = 0; i < system->rigid_body_count; i++) {
        RigidBody *body = &system->rigid_bodies[i];
        if (!body->is_static && body->mass > 0.0f) {
            // Apply gravity as force (F = ma)
            float gravity_force[3];
            gravity_force[0] = system->gravity[0] * body->mass;
            gravity_force[1] = system->gravity[1] * body->mass;
            gravity_force[2] = system->gravity[2] * body->mass;
            
            // Convert to acceleration and update velocity
            float acceleration[3];
            acceleration[0] = gravity_force[0] / body->mass;
            acceleration[1] = gravity_force[1] / body->mass;
            acceleration[2] = gravity_force[2] / body->mass;
            
            body->linear_velocity[0] += acceleration[0] * dt;
            body->linear_velocity[1] += acceleration[1] * dt;
            body->linear_velocity[2] += acceleration[2] * dt;
        }
    }
}

static void update_rigid_bodies(physics_solver_system *system, float dt) {
    if (!system) return;
    
    for (int i = 0; i < system->rigid_body_count; i++) {
        RigidBody *body = &system->rigid_bodies[i];
        if (!body->is_static) {
            // Update position
            body->position[0] += body->linear_velocity[0] * dt;
            body->position[1] += body->linear_velocity[1] * dt;
            body->position[2] += body->linear_velocity[2] * dt;
            
            // Update orientation (simplified - would use quaternion integration)
            body->orientation[0] += body->angular_velocity[0] * dt;
            body->orientation[1] += body->angular_velocity[1] * dt;
            body->orientation[2] += body->angular_velocity[2] * dt;
            body->orientation[3] += body->angular_velocity[2] * dt; // Simplified
            
            system->bodies_updated++;
        }
    }
}

static void detect_collisions(physics_solver_system *system) {
    if (!system) return;
    
    // Simple broad-phase collision detection
    for (int i = 0; i < system->rigid_body_count; i++) {
        for (int j = i + 1; j < system->rigid_body_count; j++) {
            RigidBody *body_a = &system->rigid_bodies[i];
            RigidBody *body_b = &system->rigid_bodies[j];
            
            // Skip if both are static
            if (body_a->is_static && body_b->is_static) continue;
            
            // Simple sphere-sphere collision check (simplified)
            float dx = body_a->position[0] - body_b->position[0];
            float dy = body_a->position[1] - body_b->position[1];
            float dz = body_a->position[2] - body_b->position[2];
            
            float distance_sq = dx*dx + dy*dy + dz*dz;
            float min_distance = 1.0f; // Simplified - would use actual bounds
            
            if (distance_sq < min_distance * min_distance) {
                // Create contact
                Contact contact = {0};
                contact.position[0] = (body_a->position[0] + body_b->position[0]) * 0.5f;
                contact.position[1] = (body_a->position[1] + body_b->position[1]) * 0.5f;
                contact.position[2] = (body_a->position[2] + body_b->position[2]) * 0.5f;
                
                float distance = sqrtf(distance_sq);
                if (distance > 0.0001f) {
                    contact.normal[0] = dx / distance;
                    contact.normal[1] = dy / distance;
                    contact.normal[2] = dz / distance;
                } else {
                    contact.normal[1] = 1.0f; // Default up
                }
                
                contact.penetration = min_distance - distance;
                contact.valid = true;
                
                physics_solver_add_contact(system, &contact);
            }
        }
    }
}

static void solve_constraints(physics_solver_system *system, float dt) {
    if (!system) return;
    
    // Use primary solver for constraint resolution
    switch (system->primary_solver) {
        case SOLVER_TYPE_XPBD:
            if (system->xpbd_solver) {
                xpbd_update(system->xpbd_solver, dt);
                system->iterations_performed = 3; // Default XPBD iterations
            }
            break;
            
        case SOLVER_TYPE_SEQUENTIAL_IMPULSE:
            if (system->impulse_solver && system->contact_count > 0) {
                // Solve contacts for each body pair
                for (int i = 0; i < system->rigid_body_count; i++) {
                    for (int j = i + 1; j < system->rigid_body_count; j++) {
                        sequential_impulse_solve(system->impulse_solver, 
                                              &system->rigid_bodies[i], 
                                              &system->rigid_bodies[j], dt);
                    }
                }
                system->iterations_performed = 10; // Default sequential impulse iterations
            }
            break;
            
        case SOLVER_TYPE_HYBRID:
            // Use XPBD for soft bodies and sequential impulse for rigid contacts
            if (system->xpbd_solver) {
                xpbd_update(system->xpbd_solver, dt);
            }
            if (system->impulse_solver && system->contact_count > 0) {
                for (int i = 0; i < system->rigid_body_count; i++) {
                    for (int j = i + 1; j < system->rigid_body_count; j++) {
                        sequential_impulse_solve(system->impulse_solver, 
                                              &system->rigid_bodies[i], 
                                              &system->rigid_bodies[j], dt);
                    }
                }
            }
            system->iterations_performed = 13; // Combined iterations
            break;
    }
    
    system->contacts_solved = system->contact_count;
}

// ========================================
// Utility Functions
// ========================================

physics_solver_config physics_solver_get_default_config(void) {
    physics_solver_config config = {
        .primary_solver = SOLVER_TYPE_XPBD,
        .enable_hybrid_solving = false,
        .time_step = 1.0f / 60.0f,
        .gravity = {0.0f, GRAVITY_DEFAULT, 0.0f},
        .max_rigid_bodies = MAX_RIGID_BODIES,
        .max_contacts = SOLVER_MAX_CONTACTS
    };
    return config;
}
