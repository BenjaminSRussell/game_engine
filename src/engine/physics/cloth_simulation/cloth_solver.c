/**
 * @file cloth_solver.c
 * @brief Cloth constraint solver implementation using Position Based Dynamics (PBD)
 */

#include "cloth_solver.h"
#include <stdlib.h>
#include <string.h>
#include <math.h>
#include <assert.h>

// ========================================
// Constants
// ========================================

#define DEFAULT_MAX_PARTICLES 4096
#define DEFAULT_MAX_CONSTRAINTS 8192
#define MIN_VELOCITY_THRESHOLD 0.001f
#define SPATIAL_HASH_CELL_SIZE 1.0f

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

static inline float vec3_length(const float *v) {
    return sqrtf(vec3_length_sq(v));
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

static inline void vec3_copy(float *dest, const float *src) {
    dest[0] = src[0];
    dest[1] = src[1];
    dest[2] = src[2];
}

// ========================================
// Spatial Hash for Collision Detection
// ========================================

typedef struct SpatialHash {
    int *cell_buckets;       // Bucket indices
    int *particle_ids;       // Particle indices in buckets
    int bucket_count;
    int particles_per_bucket;
    float cell_size;
} SpatialHash;

static int spatial_hash_hash_func(const float *position, float cell_size, int bucket_count) {
    int x = (int)floorf(position[0] / cell_size);
    int y = (int)floorf(position[1] / cell_size);
    int z = (int)floorf(position[2] / cell_size);
    
    // Simple hash function
    return ((x * 73856093) ^ (y * 19349663) ^ (z * 83492791)) & (bucket_count - 1);
}

static SpatialHash* spatial_hash_create(int bucket_count, int particles_per_bucket, float cell_size) {
    SpatialHash *hash = (SpatialHash *)calloc(1, sizeof(SpatialHash));
    if (!hash) return NULL;
    
    hash->cell_buckets = (int *)calloc(bucket_count, sizeof(int));
    hash->particle_ids = (int *)calloc(bucket_count * particles_per_bucket, sizeof(int));
    if (!hash->cell_buckets || !hash->particle_ids) {
        free(hash->cell_buckets);
        free(hash->particle_ids);
        free(hash);
        return NULL;
    }
    
    hash->bucket_count = bucket_count;
    hash->particles_per_bucket = particles_per_bucket;
    hash->cell_size = cell_size;
    
    return hash;
}

static void spatial_hash_destroy(SpatialHash *hash) {
    if (hash) {
        free(hash->cell_buckets);
        free(hash->particle_ids);
        free(hash);
    }
}

// ========================================
// Cloth Solver Structure
// ========================================

struct ClothSolver {
    // Particles
    ClothParticle *particles;
    int particle_count;
    int particle_capacity;
    
    // Constraints
    ClothConstraint *constraints;
    int constraint_count;
    int constraint_capacity;
    
    // Configuration
    ClothConfig config;
    
    // Spatial hashing for collision
    SpatialHash *spatial_hash;
    
    // Temporary storage
    float *temp_positions;
    int *constraint_order;
};

// ========================================
// Constraint Solving Functions
// ========================================

static void solve_distance_constraint(ClothParticle *particles, DistanceConstraint *constraint) {
    ClothParticle *p1 = &particles[constraint->particle_a];
    ClothParticle *p2 = &particles[constraint->particle_b];
    
    if (p1->inv_mass == 0.0f && p2->inv_mass == 0.0f) return;
    
    // Calculate current distance
    float delta[3];
    vec3_sub(delta, p2->position, p1->position);
    float current_length = vec3_length(delta);
    
    if (current_length < 0.0001f) return; // Particles are too close
    
    // Calculate correction
    float rest_length = constraint->rest_length;
    float correction = (current_length - rest_length) / current_length;
    correction *= constraint->stiffness;
    
    // Calculate mass ratio
    float w1 = p1->inv_mass;
    float w2 = p2->inv_mass;
    float total_mass = w1 + w2;
    if (total_mass == 0.0f) return;
    
    float mass_ratio1 = w1 / total_mass;
    float mass_ratio2 = w2 / total_mass;
    
    // Apply correction
    float correction_vec[3];
    vec3_mul(correction_vec, delta, correction);
    
    float p1_correction[3], p2_correction[3];
    vec3_mul(p1_correction, correction_vec, mass_ratio1);
    vec3_mul(p2_correction, correction_vec, -mass_ratio2);
    
    vec3_add(p1->position, p1->position, p1_correction);
    vec3_add(p2->position, p2->position, p2_correction);
}

static void solve_bending_constraint(ClothParticle *particles, BendingConstraint *constraint) {
    // Simplified bending constraint - uses angle between triangles
    ClothParticle *p1 = &particles[constraint->particle_a];
    ClothParticle *p2 = &particles[constraint->particle_b];
    ClothParticle *p3 = &particles[constraint->particle_c];
    ClothParticle *p4 = &particles[constraint->particle_d];
    
    // Calculate normals of the two triangles
    float v1[3], v2[3], v3[3], v4[3];
    vec3_sub(v1, p2->position, p1->position);
    vec3_sub(v2, p3->position, p1->position);
    vec3_sub(v3, p4->position, p2->position);
    vec3_sub(v4, p3->position, p2->position);
    
    float normal1[3], normal2[3];
    vec3_cross(normal1, v1, v2);
    vec3_cross(normal2, v3, v4);
    
    float len1 = vec3_length(normal1);
    float len2 = vec3_length(normal2);
    
    if (len1 < 0.0001f || len2 < 0.0001f) return;
    
    vec3_normalize(normal1, normal1);
    vec3_normalize(normal2, normal2);
    
    // Calculate angle between normals
    float cos_angle = vec3_dot(normal1, normal2);
    cos_angle = fmaxf(-1.0f, fminf(1.0f, cos_angle)); // Clamp
    
    float current_angle = acosf(cos_angle);
    float target_angle = constraint->rest_angle;
    float angle_diff = current_angle - target_angle;
    
    // Apply bending correction (simplified)
    float correction = angle_diff * constraint->stiffness * 0.1f;
    
    // Move particles to satisfy bending constraint
    float center[3];
    vec3_add(center, p1->position, p2->position);
    vec3_add(center, center, p3->position);
    vec3_add(center, center, p4->position);
    vec3_mul(center, center, 0.25f);
    
    float bend_axis[3];
    vec3_cross(bend_axis, normal1, normal2);
    if (vec3_length_sq(bend_axis) < 0.0001f) return;
    vec3_normalize(bend_axis, bend_axis);
    
    // Apply rotational correction to particles
    for (int i = 0; i < 4; i++) {
        ClothParticle *p = (i == 0) ? p1 : (i == 1) ? p2 : (i == 2) ? p3 : p4;
        if (p->inv_mass == 0.0f) continue;
        
        float to_center[3];
        vec3_sub(to_center, p->position, center);
        
        float rotation[3];
        vec3_cross(rotation, bend_axis, to_center);
        vec3_mul(rotation, rotation, correction);
        
        vec3_add(p->position, p->position, rotation);
    }
}

static void solve_collision_constraint(ClothParticle *particles, CollisionConstraint *constraint) {
    ClothParticle *particle = &particles[constraint->particle];
    
    if (particle->inv_mass == 0.0f) return;
    
    // Push particle out of collision
    float penetration = constraint->penetration;
    if (penetration <= 0.0f) return;
    
    float correction[3];
    vec3_mul(correction, constraint->normal, penetration);
    vec3_add(particle->position, particle->position, correction);
    
    // Apply friction to velocity
    float normal_velocity = vec3_dot(particle->velocity, constraint->normal);
    if (normal_velocity < 0.0f) {
        float normal_vel_vec[3];
        vec3_mul(normal_vel_vec, constraint->normal, normal_velocity);
        
        float tangent_vel[3];
        vec3_sub(tangent_vel, particle->velocity, normal_vel_vec);
        
        float tangent_speed = vec3_length(tangent_vel);
        if (tangent_speed > 0.0001f) {
            float friction_impulse = fminf(tangent_speed, constraint->friction * fabsf(normal_velocity));
            float friction_dir[3];
            vec3_normalize(friction_dir, tangent_vel);
            vec3_mul(friction_dir, friction_dir, friction_impulse);
            vec3_sub(particle->velocity, particle->velocity, friction_dir);
        }
    }
}

// ========================================
// Public API Implementation
// ========================================

ClothConfig cloth_solver_get_default_config(void) {
    ClothConfig config = {
        .gravity = {0.0f, -9.81f, 0.0f},
        .damping = 0.01f,
        .air_resistance = 0.001f,
        .solver_iterations = 3,
        .distance_stiffness = 0.8f,
        .bending_stiffness = 0.2f,
        .self_collision = false,
        .world_collision = true,
        .collision_thickness = 0.1f,
        .use_spatial_hash = true,
        .grid_size = 256
    };
    return config;
}

ClothSolver* cloth_solver_create(int max_particles, int max_constraints, const ClothConfig *config) {
    if (max_particles <= 0) max_particles = DEFAULT_MAX_PARTICLES;
    if (max_constraints <= 0) max_constraints = DEFAULT_MAX_CONSTRAINTS;
    
    ClothSolver *solver = (ClothSolver *)calloc(1, sizeof(ClothSolver));
    if (!solver) return NULL;
    
    solver->particles = (ClothParticle *)calloc(max_particles, sizeof(ClothParticle));
    solver->constraints = (ClothConstraint *)calloc(max_constraints, sizeof(ClothConstraint));
    solver->temp_positions = (float *)calloc(max_particles * 3, sizeof(float));
    solver->constraint_order = (int *)malloc(max_constraints * sizeof(int));
    
    if (!solver->particles || !solver->constraints || !solver->temp_positions || !solver->constraint_order) {
        cloth_solver_destroy(solver);
        return NULL;
    }
    
    solver->particle_capacity = max_particles;
    solver->constraint_capacity = max_constraints;
    solver->particle_count = 0;
    solver->constraint_count = 0;
    
    if (config) {
        solver->config = *config;
    } else {
        solver->config = cloth_solver_get_default_config();
    }
    
    // Initialize constraint order
    for (int i = 0; i < max_constraints; i++) {
        solver->constraint_order[i] = i;
    }
    
    // Create spatial hash for collision detection
    if (solver->config.use_spatial_hash) {
        solver->spatial_hash = spatial_hash_create(solver->config.grid_size, 16, SPATIAL_HASH_CELL_SIZE);
        if (!solver->spatial_hash) {
            cloth_solver_destroy(solver);
            return NULL;
        }
    }
    
    return solver;
}

void cloth_solver_destroy(ClothSolver *solver) {
    if (solver) {
        free(solver->particles);
        free(solver->constraints);
        free(solver->temp_positions);
        free(solver->constraint_order);
        spatial_hash_destroy(solver->spatial_hash);
        free(solver);
    }
}

int cloth_solver_add_particle(ClothSolver *solver, const float *position, float mass, bool pinned) {
    if (!solver || solver->particle_count >= solver->particle_capacity) {
        return -1;
    }
    
    ClothParticle *particle = &solver->particles[solver->particle_count];
    
    vec3_copy(particle->position, position);
    vec3_copy(particle->old_position, position);
    particle->velocity[0] = particle->velocity[1] = particle->velocity[2] = 0.0f;
    particle->force[0] = particle->force[1] = particle->force[2] = 0.0f;
    
    particle->mass = mass;
    particle->inv_mass = (mass > 0.0f) ? (1.0f / mass) : 0.0f;
    particle->pinned = pinned;
    particle->collision_enabled = true;
    
    particle->normal[0] = particle->normal[1] = particle->normal[2] = 0.0f;
    particle->penetration = 0.0f;
    
    return solver->particle_count++;
}

void cloth_solver_remove_particle(ClothSolver *solver, int particle_id) {
    if (!solver || particle_id < 0 || particle_id >= solver->particle_count) {
        return;
    }
    
    // Move last particle to removed position
    if (particle_id < solver->particle_count - 1) {
        solver->particles[particle_id] = solver->particles[solver->particle_count - 1];
    }
    
    solver->particle_count--;
    
    // Remove constraints involving this particle (simplified)
    for (int i = 0; i < solver->constraint_count; i++) {
        ClothConstraint *constraint = &solver->constraints[i];
        bool should_remove = false;
        
        switch (constraint->type) {
            case CONSTRAINT_DISTANCE:
                should_remove = (constraint->data.distance.particle_a == particle_id ||
                               constraint->data.distance.particle_b == particle_id);
                break;
            case CONSTRAINT_BENDING:
                should_remove = (constraint->data.bending.particle_a == particle_id ||
                               constraint->data.bending.particle_b == particle_id ||
                               constraint->data.bending.particle_c == particle_id ||
                               constraint->data.bending.particle_d == particle_id);
                break;
            case CONSTRAINT_COLLISION:
                should_remove = (constraint->data.collision.particle == particle_id);
                break;
        }
        
        if (should_remove) {
            cloth_solver_remove_constraint(solver, i);
            i--; // Check same index again
        }
    }
}

ClothParticle* cloth_solver_get_particle(ClothSolver *solver, int particle_id) {
    if (!solver || particle_id < 0 || particle_id >= solver->particle_count) {
        return NULL;
    }
    
    return &solver->particles[particle_id];
}

void cloth_solver_set_particle_position(ClothSolver *solver, int particle_id, const float *position) {
    ClothParticle *particle = cloth_solver_get_particle(solver, particle_id);
    if (particle) {
        vec3_copy(particle->position, position);
        vec3_copy(particle->old_position, position);
    }
}

void cloth_solver_pin_particle(ClothSolver *solver, int particle_id, bool pinned) {
    ClothParticle *particle = cloth_solver_get_particle(solver, particle_id);
    if (particle) {
        particle->pinned = pinned;
    }
}

int cloth_solver_add_distance_constraint(ClothSolver *solver, int particle_a, int particle_b, float stiffness) {
    if (!solver || solver->constraint_count >= solver->constraint_capacity) {
        return -1;
    }
    
    if (particle_a < 0 || particle_a >= solver->particle_count ||
        particle_b < 0 || particle_b >= solver->particle_count) {
        return -1;
    }
    
    ClothConstraint *constraint = &solver->constraints[solver->constraint_count];
    constraint->type = CONSTRAINT_DISTANCE;
    constraint->enabled = true;
    
    DistanceConstraint *distance = &constraint->data.distance;
    distance->particle_a = particle_a;
    distance->particle_b = particle_b;
    distance->stiffness = stiffness;
    
    // Calculate rest length
    ClothParticle *p1 = &solver->particles[particle_a];
    ClothParticle *p2 = &solver->particles[particle_b];
    float delta[3];
    vec3_sub(delta, p2->position, p1->position);
    distance->rest_length = vec3_length(delta);
    
    return solver->constraint_count++;
}

void cloth_solver_remove_constraint(ClothSolver *solver, int constraint_id) {
    if (!solver || constraint_id < 0 || constraint_id >= solver->constraint_count) {
        return;
    }
    
    // Move last constraint to removed position
    if (constraint_id < solver->constraint_count - 1) {
        solver->constraints[constraint_id] = solver->constraints[solver->constraint_count - 1];
    }
    
    solver->constraint_count--;
}

void cloth_solver_create_rect_mesh(ClothSolver *solver, int width, int height, 
                                   float spacing, const float *position, float mass) {
    if (!solver || width <= 0 || height <= 0) return;
    
    float particle_mass = mass / (width * height);
    
    // Create particles
    int start_particle = solver->particle_count;
    for (int y = 0; y < height; y++) {
        for (int x = 0; x < width; x++) {
            float pos[3] = {
                position[0] + x * spacing,
                position[1] + y * spacing,
                position[2]
            };
            cloth_solver_add_particle(solver, pos, particle_mass, false);
        }
    }
    
    // Create distance constraints (structural)
    for (int y = 0; y < height; y++) {
        for (int x = 0; x < width; x++) {
            int current = start_particle + y * width + x;
            
            // Horizontal constraint
            if (x < width - 1) {
                int right = start_particle + y * width + (x + 1);
                cloth_solver_add_distance_constraint(solver, current, right, solver->config.distance_stiffness);
            }
            
            // Vertical constraint
            if (y < height - 1) {
                int bottom = start_particle + (y + 1) * width + x;
                cloth_solver_add_distance_constraint(solver, current, bottom, solver->config.distance_stiffness);
            }
            
            // Diagonal constraints (shear)
            if (x < width - 1 && y < height - 1) {
                int diag_right = start_particle + (y + 1) * width + (x + 1);
                cloth_solver_add_distance_constraint(solver, current, diag_right, solver->config.distance_stiffness * 0.5f);
            }
            
            if (x > 0 && y < height - 1) {
                int diag_left = start_particle + (y + 1) * width + (x - 1);
                cloth_solver_add_distance_constraint(solver, current, diag_left, solver->config.distance_stiffness * 0.5f);
            }
        }
    }
}

void cloth_solver_update(ClothSolver *solver, float dt) {
    if (!solver || dt <= 0.0f) return;
    
    // Apply external forces
    for (int i = 0; i < solver->particle_count; i++) {
        ClothParticle *particle = &solver->particles[i];
        if (particle->pinned) continue;
        
        // Apply gravity
        vec3_add(particle->force, particle->force, solver->config.gravity);
        
        // Apply damping
        vec3_mul(particle->velocity, particle->velocity, (1.0f - solver->config.damping));
        
        // Apply air resistance
        float speed_sq = vec3_length_sq(particle->velocity);
        if (speed_sq > 0.0001f) {
            float drag_force = solver->config.air_resistance * speed_sq;
            float drag_dir[3];
            vec3_normalize(drag_dir, particle->velocity);
            vec3_mul(drag_dir, drag_dir, -drag_force);
            vec3_add(particle->force, particle->force, drag_dir);
        }
    }
    
    // Integrate velocities (semi-implicit Euler)
    for (int i = 0; i < solver->particle_count; i++) {
        ClothParticle *particle = &solver->particles[i];
        if (particle->pinned) continue;
        
        vec3_add(particle->velocity, particle->velocity, 
                particle->force, particle->inv_mass * dt);
    }
    
    // Predict positions
    for (int i = 0; i < solver->particle_count; i++) {
        ClothParticle *particle = &solver->particles[i];
        if (particle->pinned) continue;
        
        vec3_copy(particle->old_position, particle->position);
        vec3_add(particle->position, particle->position, 
                particle->velocity, dt);
    }
    
    // Solve constraints
    for (int iteration = 0; iteration < solver->config.solver_iterations; iteration++) {
        // Solve distance constraints
        for (int i = 0; i < solver->constraint_count; i++) {
            ClothConstraint *constraint = &solver->constraints[i];
            if (!constraint->enabled) continue;
            
            switch (constraint->type) {
                case CONSTRAINT_DISTANCE:
                    solve_distance_constraint(solver->particles, &constraint->data.distance);
                    break;
                case CONSTRAINT_BENDING:
                    solve_bending_constraint(solver->particles, &constraint->data.bending);
                    break;
                case CONSTRAINT_COLLISION:
                    solve_collision_constraint(solver->particles, &constraint->data.collision);
                    break;
            }
        }
        
        // Process collisions
        if (solver->config.world_collision) {
            cloth_solver_process_world_collision(solver);
        }
        
        if (solver->config.self_collision) {
            cloth_solver_process_self_collision(solver);
        }
    }
    
    // Update velocities and clear forces
    for (int i = 0; i < solver->particle_count; i++) {
        ClothParticle *particle = &solver->particles[i];
        if (particle->pinned) {
            particle->velocity[0] = particle->velocity[1] = particle->velocity[2] = 0.0f;
        } else {
            // Calculate new velocity from position change
            vec3_sub(particle->velocity, particle->position, particle->old_position);
            vec3_mul(particle->velocity, particle->velocity, 1.0f / dt);
        }
        
        // Clear forces for next frame
        particle->force[0] = particle->force[1] = particle->force[2] = 0.0f;
    }
}

void cloth_solver_apply_force(ClothSolver *solver, const float *force) {
    if (!solver || !force) return;
    
    for (int i = 0; i < solver->particle_count; i++) {
        ClothParticle *particle = &solver->particles[i];
        if (!particle->pinned) {
            vec3_add(particle->force, particle->force, force);
        }
    }
}

void cloth_solver_apply_particle_force(ClothSolver *solver, int particle_id, const float *force) {
    ClothParticle *particle = cloth_solver_get_particle(solver, particle_id);
    if (particle && !particle->pinned) {
        vec3_add(particle->force, particle->force, force);
    }
}

void cloth_solver_apply_wind(ClothSolver *solver, const float *wind_direction, float strength) {
    if (!solver || !wind_direction) return;
    
    float wind_force[3];
    vec3_normalize(wind_force, wind_direction);
    vec3_mul(wind_force, wind_force, strength);
    
    cloth_solver_apply_force(solver, wind_force);
}

void cloth_solver_process_world_collision(ClothSolver *solver) {
    if (!solver) return;
    
    // Simple ground plane collision (y = 0)
    for (int i = 0; i < solver->particle_count; i++) {
        ClothParticle *particle = &solver->particles[i];
        if (!particle->collision_enabled || particle->pinned) continue;
        
        if (particle->position[1] < 0.0f) {
            // Add collision constraint
            float normal[3] = {0.0f, 1.0f, 0.0f};
            float penetration = -particle->position[1];
            
            cloth_solver_add_world_collision(solver, i, normal, penetration, 0.3f);
        }
    }
}

void cloth_solver_process_self_collision(ClothSolver *solver) {
    // Simplified self-collision - would use spatial hash in full implementation
    if (!solver || !solver->spatial_hash) return;
    
    float thickness = solver->config.collision_thickness;
    
    for (int i = 0; i < solver->particle_count; i++) {
        for (int j = i + 1; j < solver->particle_count; j++) {
            ClothParticle *p1 = &solver->particles[i];
            ClothParticle *p2 = &solver->particles[j];
            
            if (!p1->collision_enabled || !p2->collision_enabled) continue;
            if (p1->pinned && p2->pinned) continue;
            
            float delta[3];
            vec3_sub(delta, p2->position, p1->position);
            float distance_sq = vec3_length_sq(delta);
            float min_distance = thickness * 2.0f;
            
            if (distance_sq < min_distance * min_distance && distance_sq > 0.0001f) {
                float distance = sqrtf(distance_sq);
                float penetration = min_distance - distance;
                
                float normal[3];
                vec3_normalize(normal, delta);
                
                // Separate particles
                float total_mass = p1->inv_mass + p2->inv_mass;
                if (total_mass > 0.0f) {
                    float w1 = p1->inv_mass / total_mass;
                    float w2 = p2->inv_mass / total_mass;
                    
                    float correction[3];
                    vec3_mul(correction, normal, penetration);
                    
                    if (!p1->pinned) {
                        float p1_correction[3];
                        vec3_mul(p1_correction, correction, -w1);
                        vec3_add(p1->position, p1->position, p1_correction);
                    }
                    
                    if (!p2->pinned) {
                        float p2_correction[3];
                        vec3_mul(p2_correction, correction, w2);
                        vec3_add(p2->position, p2->position, p2_correction);
                    }
                }
            }
        }
    }
}

void cloth_solver_add_world_collision(ClothSolver *solver, int particle_id, 
                                     const float *normal, float penetration, float friction) {
    if (!solver || particle_id < 0 || particle_id >= solver->constraint_count) return;
    
    if (solver->constraint_count >= solver->constraint_capacity) return;
    
    ClothConstraint *constraint = &solver->constraints[solver->constraint_count];
    constraint->type = CONSTRAINT_COLLISION;
    constraint->enabled = true;
    
    CollisionConstraint *collision = &constraint->data.collision;
    collision->particle = particle_id;
    vec3_copy(collision->normal, normal);
    collision->penetration = penetration;
    collision->friction = friction;
    
    solver->constraint_count++;
}

void cloth_solver_get_stats(const ClothSolver *solver, int *particle_count, 
                           int *constraint_count, int *collision_count) {
    if (!solver) return;
    
    if (particle_count) *particle_count = solver->particle_count;
    if (constraint_count) *constraint_count = solver->constraint_count;
    
    int collision_constraints = 0;
    for (int i = 0; i < solver->constraint_count; i++) {
        if (solver->constraints[i].type == CONSTRAINT_COLLISION) {
            collision_constraints++;
        }
    }
    if (collision_count) *collision_count = collision_constraints;
}

void cloth_solver_reset(ClothSolver *solver) {
    if (!solver) return;
    
    solver->particle_count = 0;
    solver->constraint_count = 0;
}

bool cloth_solver_validate(const ClothSolver *solver) {
    if (!solver) return false;
    
    // Check particle indices in constraints
    for (int i = 0; i < solver->constraint_count; i++) {
        ClothConstraint *constraint = &solver->constraints[i];
        
        switch (constraint->type) {
            case CONSTRAINT_DISTANCE:
                if (constraint->data.distance.particle_a >= solver->particle_count ||
                    constraint->data.distance.particle_b >= solver->particle_count) {
                    return false;
                }
                break;
            case CONSTRAINT_BENDING:
                if (constraint->data.bending.particle_a >= solver->particle_count ||
                    constraint->data.bending.particle_b >= solver->particle_count ||
                    constraint->data.bending.particle_c >= solver->particle_count ||
                    constraint->data.bending.particle_d >= solver->particle_count) {
                    return false;
                }
                break;
            case CONSTRAINT_COLLISION:
                if (constraint->data.collision.particle >= solver->particle_count) {
                    return false;
                }
                break;
        }
    }
    
    return true;
}
