/*
 * cloth_physics.c
 * Cloth physics solver using Position-Based Dynamics (PBD)
 *
 * Part of the Cloth System subsystem
 * Advanced 3D Rendering Engine
 *
 * Implements particle-based cloth simulation with constraints
 */

#include "cloth_physics.h"
#include <stdint.h>
#include <stdbool.h>
#include <stddef.h>
#include <string.h>
#include <stdlib.h>

/* ============================================================================
 * CONSTANTS
 * ============================================================================ */

#define CLOTH_PHYSICS_MAX_COUNT 256
#define CLOTH_PHYSICS_DEFAULT_CAPACITY 32
#define CLOTH_MAX_PARTICLES 65536
#define CLOTH_MAX_CONSTRAINTS 262144
#define CLOTH_PBD_ITERATIONS 8
#define CLOTH_DAMPING 0.99f

/* ============================================================================
 * MATH TYPES
 * ============================================================================ */

typedef struct vec3 {
    float x, y, z;
} vec3_t;

static const vec3_t GRAVITY = {0.0f, -9.81f, 0.0f};

/* ============================================================================
 * CLOTH PHYSICS TYPES
 * ============================================================================ */

typedef enum constraint_type {
    CONSTRAINT_DISTANCE = 0,    // Distance constraint between two particles
    CONSTRAINT_BENDING,         // Bending constraint (4 particles)
    CONSTRAINT_PIN,             // Pin particle to fixed position
    CONSTRAINT_COLLISION        // Collision constraint
} constraint_type_t;

typedef struct distance_constraint {
    uint32_t p0, p1;            // Particle indices
    float rest_length;          // Rest distance
    float stiffness;            // 0-1, higher = stiffer
} distance_constraint_t;

typedef struct bending_constraint {
    uint32_t p0, p1, p2, p3;    // 4 particles forming dihedral angle
    float rest_angle;
    float stiffness;
} bending_constraint_t;

typedef struct pin_constraint {
    uint32_t particle;
    vec3_t position;
} pin_constraint_t;

typedef struct collision_sphere {
    vec3_t center;
    float radius;
} collision_sphere_t;

typedef struct collision_capsule {
    vec3_t p0, p1;
    float radius;
} collision_capsule_t;

typedef struct cloth_sim {
    // Particle data
    vec3_t* positions;
    vec3_t* prev_positions;
    vec3_t* velocities;
    float* inv_masses;          // 0 = pinned, positive = movable
    uint32_t particle_count;
    
    // Constraints
    distance_constraint_t* distance_constraints;
    uint32_t distance_constraint_count;
    
    bending_constraint_t* bending_constraints;
    uint32_t bending_constraint_count;
    
    pin_constraint_t* pin_constraints;
    uint32_t pin_constraint_count;
    
    // Collision objects
    collision_sphere_t* collision_spheres;
    uint32_t sphere_count;
    
    collision_capsule_t* collision_capsules;
    uint32_t capsule_count;
    
    // Simulation parameters
    float damping;
    uint32_t solver_iterations;
    vec3_t wind_velocity;
    float wind_strength;
    
    // State
    bool initialized;
} cloth_sim_t;

typedef struct cloth_system_cloth_physics_internal {
    uint32_t id;
    uint32_t flags;
    cloth_sim_t sim;
    bool dirty;
    uint64_t frame_updated;
} cloth_system_cloth_physics_internal_t;

typedef struct cloth_system_cloth_physics_context {
    cloth_system_cloth_physics_internal_t* items;
    uint32_t count;
    uint32_t capacity;
    bool initialized;
} cloth_system_cloth_physics_context_t;

static cloth_system_cloth_physics_context_t g_cloth_physics_ctx = {0};

/* ============================================================================
 * MATH HELPERS
 * ============================================================================ */

static float sqrtf_approx(float x) {
    if (x <= 0) return 0;
    float guess = x / 2.0f;
    for (int i = 0; i < 8; i++) {
        guess = (guess + x / guess) / 2.0f;
    }
    return guess;
}

static vec3_t vec3_add(vec3_t a, vec3_t b) {
    return (vec3_t){a.x + b.x, a.y + b.y, a.z + b.z};
}

static vec3_t vec3_sub(vec3_t a, vec3_t b) {
    return (vec3_t){a.x - b.x, a.y - b.y, a.z - b.z};
}

static vec3_t vec3_scale(vec3_t v, float s) {
    return (vec3_t){v.x * s, v.y * s, v.z * s};
}

static float vec3_dot(vec3_t a, vec3_t b) {
    return a.x * b.x + a.y * b.y + a.z * b.z;
}

static float vec3_length(vec3_t v) {
    return sqrtf_approx(vec3_dot(v, v));
}

static vec3_t vec3_normalize(vec3_t v) {
    float len = vec3_length(v);
    if (len < 0.0001f) return (vec3_t){0, 0, 0};
    return vec3_scale(v, 1.0f / len);
}

/* ============================================================================
 * CONSTRAINT SOLVERS
 * ============================================================================ */

static void solve_distance_constraint(cloth_sim_t* sim, const distance_constraint_t* c) {
    vec3_t* p0 = &sim->positions[c->p0];
    vec3_t* p1 = &sim->positions[c->p1];
    float w0 = sim->inv_masses[c->p0];
    float w1 = sim->inv_masses[c->p1];
    
    float w_sum = w0 + w1;
    if (w_sum < 0.0001f) return;  // Both pinned
    
    vec3_t delta = vec3_sub(*p1, *p0);
    float current_length = vec3_length(delta);
    
    if (current_length < 0.0001f) return;
    
    float diff = (current_length - c->rest_length) / current_length;
    vec3_t correction = vec3_scale(delta, diff * c->stiffness);
    
    *p0 = vec3_add(*p0, vec3_scale(correction, w0 / w_sum));
    *p1 = vec3_sub(*p1, vec3_scale(correction, w1 / w_sum));
}

static void solve_pin_constraint(cloth_sim_t* sim, const pin_constraint_t* c) {
    sim->positions[c->particle] = c->position;
}

static void solve_sphere_collision(cloth_sim_t* sim, const collision_sphere_t* sphere) {
    for (uint32_t i = 0; i < sim->particle_count; i++) {
        if (sim->inv_masses[i] <= 0.0f) continue;  // Skip pinned
        
        vec3_t to_particle = vec3_sub(sim->positions[i], sphere->center);
        float dist = vec3_length(to_particle);
        
        if (dist < sphere->radius) {
            // Push particle outside sphere
            vec3_t normal = vec3_normalize(to_particle);
            sim->positions[i] = vec3_add(sphere->center, 
                                          vec3_scale(normal, sphere->radius));
        }
    }
}

static void solve_capsule_collision(cloth_sim_t* sim, const collision_capsule_t* capsule) {
    vec3_t axis = vec3_sub(capsule->p1, capsule->p0);
    float axis_length = vec3_length(axis);
    if (axis_length < 0.0001f) return;
    
    vec3_t axis_norm = vec3_scale(axis, 1.0f / axis_length);
    
    for (uint32_t i = 0; i < sim->particle_count; i++) {
        if (sim->inv_masses[i] <= 0.0f) continue;
        
        vec3_t to_particle = vec3_sub(sim->positions[i], capsule->p0);
        float t = vec3_dot(to_particle, axis_norm);
        
        // Clamp to capsule segment
        if (t < 0.0f) t = 0.0f;
        if (t > axis_length) t = axis_length;
        
        vec3_t closest = vec3_add(capsule->p0, vec3_scale(axis_norm, t));
        vec3_t to_closest = vec3_sub(sim->positions[i], closest);
        float dist = vec3_length(to_closest);
        
        if (dist < capsule->radius) {
            vec3_t normal = vec3_normalize(to_closest);
            sim->positions[i] = vec3_add(closest, 
                                          vec3_scale(normal, capsule->radius));
        }
    }
}

/* ============================================================================
 * SIMULATION STEP
 * ============================================================================ */

static void cloth_sim_step(cloth_sim_t* sim, float dt) {
    if (!sim->initialized || sim->particle_count == 0) return;
    if (dt <= 0.0f || dt > 0.1f) dt = 0.016f;  // Clamp timestep
    
    // 1. Apply external forces (gravity, wind)
    for (uint32_t i = 0; i < sim->particle_count; i++) {
        if (sim->inv_masses[i] <= 0.0f) continue;  // Skip pinned
        
        // Gravity
        sim->velocities[i] = vec3_add(sim->velocities[i], 
                                       vec3_scale(GRAVITY, dt));
        
        // Wind (simple aerodynamic drag)
        if (sim->wind_strength > 0.0f) {
            vec3_t wind_force = vec3_scale(sim->wind_velocity, 
                                           sim->wind_strength * dt);
            sim->velocities[i] = vec3_add(sim->velocities[i], wind_force);
        }
        
        // Damping
        sim->velocities[i] = vec3_scale(sim->velocities[i], sim->damping);
        
        // Store previous position for velocity update
        sim->prev_positions[i] = sim->positions[i];
        
        // Predict new position
        sim->positions[i] = vec3_add(sim->positions[i], 
                                      vec3_scale(sim->velocities[i], dt));
    }
    
    // 2. Solve constraints iteratively (PBD)
    for (uint32_t iter = 0; iter < sim->solver_iterations; iter++) {
        // Distance constraints
        for (uint32_t c = 0; c < sim->distance_constraint_count; c++) {
            solve_distance_constraint(sim, &sim->distance_constraints[c]);
        }
        
        // Pin constraints (pinned particles)
        for (uint32_t c = 0; c < sim->pin_constraint_count; c++) {
            solve_pin_constraint(sim, &sim->pin_constraints[c]);
        }
        
        // Collision constraints
        for (uint32_t s = 0; s < sim->sphere_count; s++) {
            solve_sphere_collision(sim, &sim->collision_spheres[s]);
        }
        for (uint32_t c = 0; c < sim->capsule_count; c++) {
            solve_capsule_collision(sim, &sim->collision_capsules[c]);
        }
    }
    
    // 3. Update velocities from position change
    float inv_dt = 1.0f / dt;
    for (uint32_t i = 0; i < sim->particle_count; i++) {
        if (sim->inv_masses[i] <= 0.0f) continue;
        
        sim->velocities[i] = vec3_scale(
            vec3_sub(sim->positions[i], sim->prev_positions[i]), 
            inv_dt
        );
    }
}

/* ============================================================================
 * CLOTH SETUP HELPERS
 * ============================================================================ */

static void cloth_sim_cleanup(cloth_sim_t* sim) {
    if (!sim) return;
    
    if (sim->positions) { free(sim->positions); sim->positions = NULL; }
    if (sim->prev_positions) { free(sim->prev_positions); sim->prev_positions = NULL; }
    if (sim->velocities) { free(sim->velocities); sim->velocities = NULL; }
    if (sim->inv_masses) { free(sim->inv_masses); sim->inv_masses = NULL; }
    if (sim->distance_constraints) { free(sim->distance_constraints); sim->distance_constraints = NULL; }
    if (sim->bending_constraints) { free(sim->bending_constraints); sim->bending_constraints = NULL; }
    if (sim->pin_constraints) { free(sim->pin_constraints); sim->pin_constraints = NULL; }
    if (sim->collision_spheres) { free(sim->collision_spheres); sim->collision_spheres = NULL; }
    if (sim->collision_capsules) { free(sim->collision_capsules); sim->collision_capsules = NULL; }
    
    sim->particle_count = 0;
    sim->initialized = false;
}

static int cloth_sim_create_grid(cloth_sim_t* sim, uint32_t width, uint32_t height, 
                                   float spacing, vec3_t origin) {
    uint32_t particle_count = width * height;
    if (particle_count > CLOTH_MAX_PARTICLES) return -1;
    
    cloth_sim_cleanup(sim);
    
    // Allocate particle arrays
    sim->positions = calloc(particle_count, sizeof(vec3_t));
    sim->prev_positions = calloc(particle_count, sizeof(vec3_t));
    sim->velocities = calloc(particle_count, sizeof(vec3_t));
    sim->inv_masses = calloc(particle_count, sizeof(float));
    
    if (!sim->positions || !sim->prev_positions || 
        !sim->velocities || !sim->inv_masses) {
        cloth_sim_cleanup(sim);
        return -2;
    }
    
    // Initialize particles in grid
    for (uint32_t y = 0; y < height; y++) {
        for (uint32_t x = 0; x < width; x++) {
            uint32_t idx = y * width + x;
            sim->positions[idx] = (vec3_t){
                origin.x + x * spacing,
                origin.y,
                origin.z + y * spacing
            };
            sim->prev_positions[idx] = sim->positions[idx];
            sim->velocities[idx] = (vec3_t){0, 0, 0};
            sim->inv_masses[idx] = 1.0f;  // All movable
        }
    }
    
    sim->particle_count = particle_count;
    
    // Create distance constraints (structural + shear)
    uint32_t max_constraints = width * height * 4;  // Rough estimate
    sim->distance_constraints = calloc(max_constraints, sizeof(distance_constraint_t));
    if (!sim->distance_constraints) {
        cloth_sim_cleanup(sim);
        return -3;
    }
    
    uint32_t constraint_idx = 0;
    float diag_spacing = spacing * 1.41421356f;  // sqrt(2)
    
    for (uint32_t y = 0; y < height; y++) {
        for (uint32_t x = 0; x < width; x++) {
            uint32_t idx = y * width + x;
            
            // Horizontal structural
            if (x < width - 1) {
                sim->distance_constraints[constraint_idx++] = (distance_constraint_t){
                    .p0 = idx, .p1 = idx + 1,
                    .rest_length = spacing, .stiffness = 1.0f
                };
            }
            
            // Vertical structural
            if (y < height - 1) {
                sim->distance_constraints[constraint_idx++] = (distance_constraint_t){
                    .p0 = idx, .p1 = idx + width,
                    .rest_length = spacing, .stiffness = 1.0f
                };
            }
            
            // Diagonal shear (bottom-right)
            if (x < width - 1 && y < height - 1) {
                sim->distance_constraints[constraint_idx++] = (distance_constraint_t){
                    .p0 = idx, .p1 = idx + width + 1,
                    .rest_length = diag_spacing, .stiffness = 0.8f
                };
            }
            
            // Diagonal shear (bottom-left)
            if (x > 0 && y < height - 1) {
                sim->distance_constraints[constraint_idx++] = (distance_constraint_t){
                    .p0 = idx, .p1 = idx + width - 1,
                    .rest_length = diag_spacing, .stiffness = 0.8f
                };
            }
        }
    }
    
    sim->distance_constraint_count = constraint_idx;
    
    // Default simulation parameters
    sim->damping = CLOTH_DAMPING;
    sim->solver_iterations = CLOTH_PBD_ITERATIONS;
    sim->wind_velocity = (vec3_t){0, 0, 0};
    sim->wind_strength = 0.0f;
    
    sim->initialized = true;
    return 0;
}

/* ============================================================================
 * PUBLIC API
 * ============================================================================ */

int cloth_system_cloth_physics_init(void) {
    if (g_cloth_physics_ctx.initialized) {
        return 0;
    }

    g_cloth_physics_ctx.capacity = CLOTH_PHYSICS_DEFAULT_CAPACITY;
    g_cloth_physics_ctx.items = calloc(g_cloth_physics_ctx.capacity, 
                                        sizeof(cloth_system_cloth_physics_internal_t));
    if (!g_cloth_physics_ctx.items) {
        return -1;
    }

    g_cloth_physics_ctx.count = 0;
    g_cloth_physics_ctx.initialized = true;

    return 0;
}

void cloth_system_cloth_physics_shutdown(void) {
    if (!g_cloth_physics_ctx.initialized) {
        return;
    }

    for (uint32_t i = 0; i < g_cloth_physics_ctx.count; i++) {
        cloth_sim_cleanup(&g_cloth_physics_ctx.items[i].sim);
    }

    free(g_cloth_physics_ctx.items);
    g_cloth_physics_ctx.items = NULL;
    g_cloth_physics_ctx.count = 0;
    g_cloth_physics_ctx.capacity = 0;
    g_cloth_physics_ctx.initialized = false;
}

int cloth_system_cloth_physics_create(cloth_system_cloth_physics_handle_t* out_handle, 
                                        const cloth_system_cloth_physics_desc_t* desc) {
    if (!out_handle || !desc) {
        return -1;
    }

    if (!g_cloth_physics_ctx.initialized) {
        return -2;
    }

    if (g_cloth_physics_ctx.count >= g_cloth_physics_ctx.capacity) {
        return -3;
    }

    uint32_t index = g_cloth_physics_ctx.count++;
    cloth_system_cloth_physics_internal_t* item = &g_cloth_physics_ctx.items[index];

    item->id = index;
    item->flags = desc->flags;
    memset(&item->sim, 0, sizeof(cloth_sim_t));
    item->dirty = true;
    item->frame_updated = 0;

    out_handle->id = index;
    return 0;
}

void cloth_system_cloth_physics_destroy(cloth_system_cloth_physics_handle_t handle) {
    if (handle.id >= g_cloth_physics_ctx.count) {
        return;
    }

    cloth_sim_cleanup(&g_cloth_physics_ctx.items[handle.id].sim);
}

int cloth_system_cloth_physics_setup_grid(cloth_system_cloth_physics_handle_t handle,
                                            uint32_t width, uint32_t height,
                                            float spacing) {
    if (handle.id >= g_cloth_physics_ctx.count) {
        return -1;
    }
    
    cloth_sim_t* sim = &g_cloth_physics_ctx.items[handle.id].sim;
    vec3_t origin = {0, 0, 0};
    return cloth_sim_create_grid(sim, width, height, spacing, origin);
}

int cloth_system_cloth_physics_pin_particles(cloth_system_cloth_physics_handle_t handle,
                                               const uint32_t* particle_indices,
                                               const vec3_t* positions,
                                               uint32_t count) {
    if (handle.id >= g_cloth_physics_ctx.count) {
        return -1;
    }
    
    cloth_sim_t* sim = &g_cloth_physics_ctx.items[handle.id].sim;
    if (!sim->initialized) {
        return -2;
    }
    
    // Allocate pin constraints
    if (sim->pin_constraints) {
        free(sim->pin_constraints);
    }
    sim->pin_constraints = calloc(count, sizeof(pin_constraint_t));
    if (!sim->pin_constraints) {
        return -3;
    }
    
    for (uint32_t i = 0; i < count; i++) {
        uint32_t idx = particle_indices[i];
        if (idx >= sim->particle_count) continue;
        
        sim->inv_masses[idx] = 0.0f;  // Mark as pinned
        sim->pin_constraints[i] = (pin_constraint_t){
            .particle = idx,
            .position = positions ? positions[i] : sim->positions[idx]
        };
    }
    sim->pin_constraint_count = count;
    
    return 0;
}

int cloth_system_cloth_physics_set_wind(cloth_system_cloth_physics_handle_t handle,
                                          vec3_t direction, float strength) {
    if (handle.id >= g_cloth_physics_ctx.count) {
        return -1;
    }
    
    cloth_sim_t* sim = &g_cloth_physics_ctx.items[handle.id].sim;
    sim->wind_velocity = direction;
    sim->wind_strength = strength;
    return 0;
}

int cloth_system_cloth_physics_simulate(cloth_system_cloth_physics_handle_t handle, float dt) {
    if (handle.id >= g_cloth_physics_ctx.count) {
        return -1;
    }
    
    cloth_system_cloth_physics_internal_t* item = &g_cloth_physics_ctx.items[handle.id];
    cloth_sim_step(&item->sim, dt);
    item->frame_updated++;
    item->dirty = true;
    return 0;
}

int cloth_system_cloth_physics_update(cloth_system_cloth_physics_handle_t handle, 
                                        const void* data, size_t size) {
    if (handle.id >= g_cloth_physics_ctx.count) {
        return -1;
    }

    g_cloth_physics_ctx.items[handle.id].dirty = true;
    return 0;
}

bool cloth_system_cloth_physics_is_valid(cloth_system_cloth_physics_handle_t handle) {
    if (handle.id >= g_cloth_physics_ctx.count) {
        return false;
    }
    return g_cloth_physics_ctx.items[handle.id].sim.initialized;
}

int cloth_system_cloth_physics_get_info(cloth_system_cloth_physics_handle_t handle, 
                                          cloth_system_cloth_physics_info_t* out_info) {
    if (!out_info) {
        return -1;
    }

    if (handle.id >= g_cloth_physics_ctx.count) {
        return -2;
    }

    const cloth_system_cloth_physics_internal_t* item = &g_cloth_physics_ctx.items[handle.id];
    out_info->id = item->id;
    out_info->flags = item->flags;
    out_info->initialized = item->sim.initialized;

    return 0;
}

void cloth_system_cloth_physics_mark_dirty(cloth_system_cloth_physics_handle_t handle) {
    if (handle.id < g_cloth_physics_ctx.count) {
        g_cloth_physics_ctx.items[handle.id].dirty = true;
    }
}

int cloth_system_cloth_physics_process_pending(void) {
    int processed = 0;
    for (uint32_t i = 0; i < g_cloth_physics_ctx.count; i++) {
        cloth_system_cloth_physics_internal_t* item = &g_cloth_physics_ctx.items[i];
        if (item->sim.initialized && item->dirty) {
            item->dirty = false;
            processed++;
        }
    }

    return processed;
}

uint32_t cloth_system_cloth_physics_get_count(void) {
    return g_cloth_physics_ctx.count;
}

size_t cloth_system_cloth_physics_get_memory_usage(void) {
    size_t total = sizeof(g_cloth_physics_ctx);
    total += g_cloth_physics_ctx.capacity * sizeof(cloth_system_cloth_physics_internal_t);

    for (uint32_t i = 0; i < g_cloth_physics_ctx.count; i++) {
        cloth_sim_t* sim = &g_cloth_physics_ctx.items[i].sim;
        if (sim->positions) total += sim->particle_count * sizeof(vec3_t) * 3;
        if (sim->inv_masses) total += sim->particle_count * sizeof(float);
        if (sim->distance_constraints) total += sim->distance_constraint_count * sizeof(distance_constraint_t);
    }

    return total;
}

void cloth_system_cloth_physics_debug_print(void) {
    // Debug output
}

/* End of cloth_physics.c */
