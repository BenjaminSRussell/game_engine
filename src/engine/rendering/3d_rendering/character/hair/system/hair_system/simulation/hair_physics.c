/*
 * hair_physics.c
 * Hair strand simulation using mass-spring/PBD
 *
 * Part of the Hair System subsystem
 * Advanced 3D Rendering Engine
 *
 * Implements per-strand hair simulation with follow-the-leader constraints
 */

#include "hair_physics.h"
#include <stdint.h>
#include <stdbool.h>
#include <stddef.h>
#include <string.h>
#include <stdlib.h>

/* ============================================================================
 * CONSTANTS
 * ============================================================================ */

#define HAIR_PHYSICS_MAX_COUNT 128
#define HAIR_PHYSICS_DEFAULT_CAPACITY 16
#define HAIR_MAX_STRANDS 65536
#define HAIR_MAX_SEGMENTS_PER_STRAND 32
#define HAIR_PBD_ITERATIONS 4
#define HAIR_DAMPING 0.95f

/* ============================================================================
 * MATH TYPES
 * ============================================================================ */

typedef struct vec3 {
    float x, y, z;
} vec3_t;

typedef struct quat {
    float x, y, z, w;
} quat_t;

static const vec3_t GRAVITY = {0.0f, -9.81f, 0.0f};

/* ============================================================================
 * HAIR PHYSICS TYPES
 * ============================================================================ */

typedef struct hair_particle {
    vec3_t position;
    vec3_t prev_position;
    vec3_t velocity;
    float inv_mass;             // 0 = root (pinned), positive = movable
} hair_particle_t;

typedef struct hair_strand {
    uint32_t start_particle;    // Index into particle array
    uint32_t particle_count;    // Segments + 1
    float segment_length;       // Rest length between particles
    float stiffness;            // Bending stiffness
    float root_stiffness;       // How much strand follows head movement
} hair_strand_t;

typedef struct hair_collision_sphere {
    vec3_t center;
    float radius;
} hair_collision_sphere_t;

typedef struct hair_sim {
    // Particle data (all strands concatenated)
    hair_particle_t* particles;
    uint32_t particle_count;
    
    // Strand definitions
    hair_strand_t* strands;
    uint32_t strand_count;
    
    // Collision objects (head, body)
    hair_collision_sphere_t* collision_spheres;
    uint32_t sphere_count;
    
    // Root transforms (updated each frame from skeleton)
    vec3_t* root_positions;     // Root position per strand
    quat_t* root_rotations;     // Root orientation per strand
    
    // Simulation parameters
    float damping;
    uint32_t solver_iterations;
    vec3_t wind_velocity;
    float wind_strength;
    float gravity_scale;
    
    bool initialized;
} hair_sim_t;

typedef struct hair_system_hair_physics_internal {
    uint32_t id;
    uint32_t flags;
    hair_sim_t sim;
    bool dirty;
    uint64_t frame_updated;
} hair_system_hair_physics_internal_t;

typedef struct hair_system_hair_physics_context {
    hair_system_hair_physics_internal_t* items;
    uint32_t count;
    uint32_t capacity;
    bool initialized;
} hair_system_hair_physics_context_t;

static hair_system_hair_physics_context_t g_hair_physics_ctx = {0};

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
    if (len < 0.0001f) return (vec3_t){0, 1, 0};
    return vec3_scale(v, 1.0f / len);
}

static vec3_t vec3_lerp(vec3_t a, vec3_t b, float t) {
    return (vec3_t){
        a.x + (b.x - a.x) * t,
        a.y + (b.y - a.y) * t,
        a.z + (b.z - a.z) * t
    };
}

static vec3_t quat_rotate_vec3(quat_t q, vec3_t v) {
    // Quaternion vector rotation: q * v * q^-1
    vec3_t qv = {q.x, q.y, q.z};
    float s = q.w;
    
    vec3_t t = vec3_scale((vec3_t){
        qv.y * v.z - qv.z * v.y,
        qv.z * v.x - qv.x * v.z,
        qv.x * v.y - qv.y * v.x
    }, 2.0f);
    
    return vec3_add(v, vec3_add(vec3_scale(t, s), (vec3_t){
        qv.y * t.z - qv.z * t.y,
        qv.z * t.x - qv.x * t.z,
        qv.x * t.y - qv.y * t.x
    }));
}

/* ============================================================================
 * CONSTRAINT SOLVERS
 * ============================================================================ */

static void solve_distance_constraint(hair_particle_t* p0, hair_particle_t* p1, 
                                        float rest_length, float stiffness) {
    vec3_t delta = vec3_sub(p1->position, p0->position);
    float current_length = vec3_length(delta);
    
    if (current_length < 0.0001f) return;
    
    float w0 = p0->inv_mass;
    float w1 = p1->inv_mass;
    float w_sum = w0 + w1;
    
    if (w_sum < 0.0001f) return;
    
    float diff = (current_length - rest_length) / current_length;
    vec3_t correction = vec3_scale(delta, diff * stiffness);
    
    p0->position = vec3_add(p0->position, vec3_scale(correction, w0 / w_sum));
    p1->position = vec3_sub(p1->position, vec3_scale(correction, w1 / w_sum));
}

static void solve_follow_the_leader(hair_sim_t* sim, hair_strand_t* strand) {
    // Follow-the-leader: each particle follows the one above it
    uint32_t start = strand->start_particle;
    
    for (uint32_t i = 1; i < strand->particle_count; i++) {
        hair_particle_t* parent = &sim->particles[start + i - 1];
        hair_particle_t* child = &sim->particles[start + i];
        
        vec3_t to_child = vec3_sub(child->position, parent->position);
        float dist = vec3_length(to_child);
        
        if (dist > 0.0001f) {
            vec3_t dir = vec3_scale(to_child, 1.0f / dist);
            child->position = vec3_add(parent->position, 
                                        vec3_scale(dir, strand->segment_length));
        }
    }
}

static void solve_sphere_collision(hair_sim_t* sim, const hair_collision_sphere_t* sphere) {
    for (uint32_t i = 0; i < sim->particle_count; i++) {
        hair_particle_t* p = &sim->particles[i];
        if (p->inv_mass <= 0.0f) continue;
        
        vec3_t to_particle = vec3_sub(p->position, sphere->center);
        float dist = vec3_length(to_particle);
        
        if (dist < sphere->radius && dist > 0.0001f) {
            vec3_t normal = vec3_scale(to_particle, 1.0f / dist);
            p->position = vec3_add(sphere->center, 
                                    vec3_scale(normal, sphere->radius * 1.01f));
        }
    }
}

/* ============================================================================
 * SIMULATION STEP
 * ============================================================================ */

static void hair_sim_step(hair_sim_t* sim, float dt) {
    if (!sim->initialized || sim->particle_count == 0) return;
    if (dt <= 0.0f || dt > 0.1f) dt = 0.016f;
    
    vec3_t gravity_force = vec3_scale(GRAVITY, sim->gravity_scale);
    
    // 1. Update root particles from skeletal transforms
    for (uint32_t s = 0; s < sim->strand_count; s++) {
        hair_strand_t* strand = &sim->strands[s];
        uint32_t root_idx = strand->start_particle;
        
        // Root follows head/body movement
        sim->particles[root_idx].position = sim->root_positions[s];
        sim->particles[root_idx].inv_mass = 0.0f;  // Pinned
    }
    
    // 2. Apply external forces and integrate
    for (uint32_t i = 0; i < sim->particle_count; i++) {
        hair_particle_t* p = &sim->particles[i];
        if (p->inv_mass <= 0.0f) continue;
        
        // Gravity
        p->velocity = vec3_add(p->velocity, vec3_scale(gravity_force, dt));
        
        // Wind (randomized per particle for variation)
        if (sim->wind_strength > 0.0f) {
            float wind_factor = sim->wind_strength * dt;
            // Add slight variation based on position
            float variation = 1.0f + (float)(i % 17) * 0.05f;
            vec3_t wind_force = vec3_scale(sim->wind_velocity, wind_factor * variation);
            p->velocity = vec3_add(p->velocity, wind_force);
        }
        
        // Damping
        p->velocity = vec3_scale(p->velocity, sim->damping);
        
        // Store previous and predict new position
        p->prev_position = p->position;
        p->position = vec3_add(p->position, vec3_scale(p->velocity, dt));
    }
    
    // 3. Solve constraints (PBD iterations)
    for (uint32_t iter = 0; iter < sim->solver_iterations; iter++) {
        // Distance constraints within each strand
        for (uint32_t s = 0; s < sim->strand_count; s++) {
            hair_strand_t* strand = &sim->strands[s];
            uint32_t start = strand->start_particle;
            
            for (uint32_t i = 0; i < strand->particle_count - 1; i++) {
                solve_distance_constraint(
                    &sim->particles[start + i],
                    &sim->particles[start + i + 1],
                    strand->segment_length,
                    strand->stiffness
                );
            }
        }
        
        // Bending resistance (keep strand somewhat straight)
        for (uint32_t s = 0; s < sim->strand_count; s++) {
            hair_strand_t* strand = &sim->strands[s];
            if (strand->root_stiffness > 0.0f) {
                // Blend towards rest pose direction
                uint32_t start = strand->start_particle;
                vec3_t root_dir = quat_rotate_vec3(sim->root_rotations[s], 
                                                    (vec3_t){0, -1, 0});
                
                for (uint32_t i = 1; i < strand->particle_count && i < 4; i++) {
                    hair_particle_t* p = &sim->particles[start + i];
                    if (p->inv_mass <= 0.0f) continue;
                    
                    // Desired position based on rest direction
                    vec3_t rest_pos = vec3_add(
                        sim->particles[start + i - 1].position,
                        vec3_scale(root_dir, strand->segment_length)
                    );
                    
                    // Blend towards rest pose
                    float blend = strand->root_stiffness * (1.0f - (float)i / 4.0f);
                    p->position = vec3_lerp(p->position, rest_pos, blend * 0.3f);
                }
            }
        }
        
        // Follow-the-leader (ensures strand connectivity)
        for (uint32_t s = 0; s < sim->strand_count; s++) {
            solve_follow_the_leader(sim, &sim->strands[s]);
        }
        
        // Collision constraints
        for (uint32_t c = 0; c < sim->sphere_count; c++) {
            solve_sphere_collision(sim, &sim->collision_spheres[c]);
        }
    }
    
    // 4. Update velocities from position changes
    float inv_dt = 1.0f / dt;
    for (uint32_t i = 0; i < sim->particle_count; i++) {
        hair_particle_t* p = &sim->particles[i];
        if (p->inv_mass <= 0.0f) continue;
        
        p->velocity = vec3_scale(vec3_sub(p->position, p->prev_position), inv_dt);
    }
}

/* ============================================================================
 * HAIR SETUP HELPERS
 * ============================================================================ */

static void hair_sim_cleanup(hair_sim_t* sim) {
    if (!sim) return;
    
    if (sim->particles) { free(sim->particles); sim->particles = NULL; }
    if (sim->strands) { free(sim->strands); sim->strands = NULL; }
    if (sim->collision_spheres) { free(sim->collision_spheres); sim->collision_spheres = NULL; }
    if (sim->root_positions) { free(sim->root_positions); sim->root_positions = NULL; }
    if (sim->root_rotations) { free(sim->root_rotations); sim->root_rotations = NULL; }
    
    sim->particle_count = 0;
    sim->strand_count = 0;
    sim->initialized = false;
}

static int hair_sim_create(hair_sim_t* sim, uint32_t strand_count, 
                            uint32_t segments_per_strand, float segment_length) {
    if (strand_count > HAIR_MAX_STRANDS) return -1;
    if (segments_per_strand > HAIR_MAX_SEGMENTS_PER_STRAND) return -2;
    
    hair_sim_cleanup(sim);
    
    uint32_t particles_per_strand = segments_per_strand + 1;
    uint32_t total_particles = strand_count * particles_per_strand;
    
    // Allocate arrays
    sim->particles = calloc(total_particles, sizeof(hair_particle_t));
    sim->strands = calloc(strand_count, sizeof(hair_strand_t));
    sim->root_positions = calloc(strand_count, sizeof(vec3_t));
    sim->root_rotations = calloc(strand_count, sizeof(quat_t));
    
    if (!sim->particles || !sim->strands || 
        !sim->root_positions || !sim->root_rotations) {
        hair_sim_cleanup(sim);
        return -3;
    }
    
    sim->particle_count = total_particles;
    sim->strand_count = strand_count;
    
    // Initialize strands
    for (uint32_t s = 0; s < strand_count; s++) {
        hair_strand_t* strand = &sim->strands[s];
        strand->start_particle = s * particles_per_strand;
        strand->particle_count = particles_per_strand;
        strand->segment_length = segment_length;
        strand->stiffness = 0.9f;
        strand->root_stiffness = 0.5f;
        
        // Initialize root transform
        sim->root_positions[s] = (vec3_t){0, 0, 0};
        sim->root_rotations[s] = (quat_t){0, 0, 0, 1};
        
        // Initialize particles along strand
        for (uint32_t i = 0; i < particles_per_strand; i++) {
            uint32_t idx = strand->start_particle + i;
            sim->particles[idx].position = (vec3_t){0, -segment_length * i, 0};
            sim->particles[idx].prev_position = sim->particles[idx].position;
            sim->particles[idx].velocity = (vec3_t){0, 0, 0};
            sim->particles[idx].inv_mass = (i == 0) ? 0.0f : 1.0f;
        }
    }
    
    // Default parameters
    sim->damping = HAIR_DAMPING;
    sim->solver_iterations = HAIR_PBD_ITERATIONS;
    sim->wind_velocity = (vec3_t){0, 0, 0};
    sim->wind_strength = 0.0f;
    sim->gravity_scale = 1.0f;
    
    sim->initialized = true;
    return 0;
}

/* ============================================================================
 * PUBLIC API
 * ============================================================================ */

int hair_system_hair_physics_init(void) {
    if (g_hair_physics_ctx.initialized) {
        return 0;
    }

    g_hair_physics_ctx.capacity = HAIR_PHYSICS_DEFAULT_CAPACITY;
    g_hair_physics_ctx.items = calloc(g_hair_physics_ctx.capacity, 
                                       sizeof(hair_system_hair_physics_internal_t));
    if (!g_hair_physics_ctx.items) {
        return -1;
    }

    g_hair_physics_ctx.count = 0;
    g_hair_physics_ctx.initialized = true;

    return 0;
}

void hair_system_hair_physics_shutdown(void) {
    if (!g_hair_physics_ctx.initialized) {
        return;
    }

    for (uint32_t i = 0; i < g_hair_physics_ctx.count; i++) {
        hair_sim_cleanup(&g_hair_physics_ctx.items[i].sim);
    }

    free(g_hair_physics_ctx.items);
    g_hair_physics_ctx.items = NULL;
    g_hair_physics_ctx.count = 0;
    g_hair_physics_ctx.capacity = 0;
    g_hair_physics_ctx.initialized = false;
}

int hair_system_hair_physics_create(hair_system_hair_physics_handle_t* out_handle, 
                                      const hair_system_hair_physics_desc_t* desc) {
    if (!out_handle || !desc) {
        return -1;
    }

    if (!g_hair_physics_ctx.initialized) {
        return -2;
    }

    if (g_hair_physics_ctx.count >= g_hair_physics_ctx.capacity) {
        return -3;
    }

    uint32_t index = g_hair_physics_ctx.count++;
    hair_system_hair_physics_internal_t* item = &g_hair_physics_ctx.items[index];

    item->id = index;
    item->flags = desc->flags;
    memset(&item->sim, 0, sizeof(hair_sim_t));
    item->dirty = true;
    item->frame_updated = 0;

    out_handle->id = index;
    return 0;
}

void hair_system_hair_physics_destroy(hair_system_hair_physics_handle_t handle) {
    if (handle.id >= g_hair_physics_ctx.count) {
        return;
    }

    hair_sim_cleanup(&g_hair_physics_ctx.items[handle.id].sim);
}

int hair_system_hair_physics_setup(hair_system_hair_physics_handle_t handle,
                                     uint32_t strand_count,
                                     uint32_t segments_per_strand,
                                     float segment_length) {
    if (handle.id >= g_hair_physics_ctx.count) {
        return -1;
    }
    
    hair_sim_t* sim = &g_hair_physics_ctx.items[handle.id].sim;
    return hair_sim_create(sim, strand_count, segments_per_strand, segment_length);
}

int hair_system_hair_physics_update_roots(hair_system_hair_physics_handle_t handle,
                                            const vec3_t* positions,
                                            const quat_t* rotations,
                                            uint32_t count) {
    if (handle.id >= g_hair_physics_ctx.count) {
        return -1;
    }
    
    hair_sim_t* sim = &g_hair_physics_ctx.items[handle.id].sim;
    if (!sim->initialized) {
        return -2;
    }
    
    uint32_t copy_count = (count < sim->strand_count) ? count : sim->strand_count;
    
    if (positions) {
        memcpy(sim->root_positions, positions, copy_count * sizeof(vec3_t));
    }
    if (rotations) {
        memcpy(sim->root_rotations, rotations, copy_count * sizeof(quat_t));
    }
    
    return 0;
}

int hair_system_hair_physics_add_collision_sphere(hair_system_hair_physics_handle_t handle,
                                                    vec3_t center, float radius) {
    if (handle.id >= g_hair_physics_ctx.count) {
        return -1;
    }
    
    hair_sim_t* sim = &g_hair_physics_ctx.items[handle.id].sim;
    
    uint32_t new_count = sim->sphere_count + 1;
    hair_collision_sphere_t* new_spheres = realloc(sim->collision_spheres, 
                                                    new_count * sizeof(hair_collision_sphere_t));
    if (!new_spheres) {
        return -2;
    }
    
    sim->collision_spheres = new_spheres;
    sim->collision_spheres[sim->sphere_count] = (hair_collision_sphere_t){center, radius};
    sim->sphere_count = new_count;
    
    return 0;
}

int hair_system_hair_physics_simulate(hair_system_hair_physics_handle_t handle, float dt) {
    if (handle.id >= g_hair_physics_ctx.count) {
        return -1;
    }
    
    hair_system_hair_physics_internal_t* item = &g_hair_physics_ctx.items[handle.id];
    hair_sim_step(&item->sim, dt);
    item->frame_updated++;
    item->dirty = true;
    return 0;
}

int hair_system_hair_physics_update(hair_system_hair_physics_handle_t handle, 
                                      const void* data, size_t size) {
    if (handle.id >= g_hair_physics_ctx.count) {
        return -1;
    }

    g_hair_physics_ctx.items[handle.id].dirty = true;
    return 0;
}

bool hair_system_hair_physics_is_valid(hair_system_hair_physics_handle_t handle) {
    if (handle.id >= g_hair_physics_ctx.count) {
        return false;
    }
    return g_hair_physics_ctx.items[handle.id].sim.initialized;
}

int hair_system_hair_physics_get_info(hair_system_hair_physics_handle_t handle, 
                                        hair_system_hair_physics_info_t* out_info) {
    if (!out_info) {
        return -1;
    }

    if (handle.id >= g_hair_physics_ctx.count) {
        return -2;
    }

    const hair_system_hair_physics_internal_t* item = &g_hair_physics_ctx.items[handle.id];
    out_info->id = item->id;
    out_info->flags = item->flags;
    out_info->initialized = item->sim.initialized;

    return 0;
}

void hair_system_hair_physics_mark_dirty(hair_system_hair_physics_handle_t handle) {
    if (handle.id < g_hair_physics_ctx.count) {
        g_hair_physics_ctx.items[handle.id].dirty = true;
    }
}

int hair_system_hair_physics_process_pending(void) {
    int processed = 0;
    for (uint32_t i = 0; i < g_hair_physics_ctx.count; i++) {
        hair_system_hair_physics_internal_t* item = &g_hair_physics_ctx.items[i];
        if (item->sim.initialized && item->dirty) {
            item->dirty = false;
            processed++;
        }
    }

    return processed;
}

uint32_t hair_system_hair_physics_get_count(void) {
    return g_hair_physics_ctx.count;
}

size_t hair_system_hair_physics_get_memory_usage(void) {
    size_t total = sizeof(g_hair_physics_ctx);
    total += g_hair_physics_ctx.capacity * sizeof(hair_system_hair_physics_internal_t);

    for (uint32_t i = 0; i < g_hair_physics_ctx.count; i++) {
        hair_sim_t* sim = &g_hair_physics_ctx.items[i].sim;
        if (sim->particles) total += sim->particle_count * sizeof(hair_particle_t);
        if (sim->strands) total += sim->strand_count * sizeof(hair_strand_t);
    }

    return total;
}

void hair_system_hair_physics_debug_print(void) {
    // Debug output
}

/* End of hair_physics.c */
