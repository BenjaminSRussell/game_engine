/*
 * cloth_bones.c
 * Cloth bone simulation
 *
 * Part of the Animation subsystem
 * Advanced 3D Rendering Engine
 *
 * Implementation TODOs:
 * TODO: Implement skeletal animation
 * TODO: Add animation blending
 * TODO: Implement IK solvers
 * TODO: Add morph target support
 * TODO: Implement GPU skinning
 * TODO: Add animation compression
 * TODO: Implement state machine
 * TODO: Add procedural animation
 * TODO: Implement ragdoll physics
 * TODO: Add animation retargeting
 * TODO: Implement cloth bones initialization
 * TODO: Add cloth bones cleanup/shutdown
 * TODO: Implement cloth bones validation
 * TODO: Add cloth bones error handling
 * TODO: Implement cloth bones serialization
 * TODO: Add cloth bones debug output
 * TODO: Implement cloth bones unit tests
 * TODO: Add cloth bones performance counters
 * TODO: Implement cloth bones hot-reload
 * TODO: Add cloth bones thread safety
 * TODO: Implement cloth bones memory pooling
 * TODO: Add cloth bones caching layer
 * TODO: Implement cloth bones async operations
 * TODO: Add cloth bones GPU integration
 * TODO: Implement cloth bones SIMD optimization
 * TODO: Add cloth bones batch processing
 * TODO: Implement cloth bones streaming support
 * TODO: Add cloth bones LOD support
 * TODO: Implement cloth bones culling integration
 * TODO: Add cloth bones render graph node
 */

#include "character/animation/physics_animation/cloth_bones.h"
#include "math/vec3.h"
#include "core/threading/mutex.h"

#include <stdint.h>
#include <stdbool.h>
#include <stddef.h>
#include <string.h>
#include <stdlib.h>
#include <math.h>

/* ============================================================================
 * CONSTANTS
 * ============================================================================ */

#define ANIMATION_CLOTH_BONES_MAX_COUNT 4096
#define ANIMATION_CLOTH_BONES_DEFAULT_CAPACITY 256
#define ANIMATION_CLOTH_BONES_ALIGNMENT 16
#define ANIMATION_CLOTH_BONES_DEFAULT_ITERATIONS 8
#define ANIMATION_CLOTH_BONES_DEFAULT_DRAG 0.05f

/* ============================================================================
 * TYPES
 * ============================================================================ */

typedef struct cloth_particle {
    Vec3 position;
    Vec3 prev_position;
    Vec3 acceleration;
    float mass;
    float inv_mass;
    float radius;
    bool is_fixed;
} cloth_particle_t;

typedef struct cloth_constraint {
    uint32_t particle_a;
    uint32_t particle_b;
    float rest_distance;
    float stiffness;
} cloth_constraint_t;

typedef struct cloth_simulation_data {
    cloth_particle_t* particles;
    uint32_t particle_count;

    cloth_constraint_t* constraints;
    uint32_t constraint_count;

    Vec3 gravity;
    float drag;
    uint32_t solver_iterations;

    Mutex* mutex; // For thread safety
    bool is_simulating;
} cloth_simulation_data_t;

// Configuration structure expected in user_data during creation
typedef struct animation_cloth_bones_config {
    uint32_t particle_count;
    Vec3* initial_positions; // Array of Vec3, can be NULL (will default to line)
    float* masses; // Array of floats (0 for fixed), can be NULL

    uint32_t constraint_count;
    int* constraints_pairs; // Array of pairs [a, b], can be NULL

    Vec3 gravity;
    float drag;
} animation_cloth_bones_config_t;

typedef struct animation_cloth_bones_internal {
    uint32_t id;
    uint32_t flags;
    cloth_simulation_data_t* sim_data; // Replaces generic void* data for type safety
    size_t data_size;
    bool initialized;
    bool dirty;
    uint64_t frame_updated;
} animation_cloth_bones_internal_t;

typedef struct animation_cloth_bones_context {
    animation_cloth_bones_internal_t* items;
    uint32_t count;
    uint32_t capacity;
    void* allocator;
    bool initialized;
} animation_cloth_bones_context_t;

static animation_cloth_bones_context_t g_cloth_bones_ctx = {0};

/* ============================================================================
 * PRIVATE FUNCTIONS
 * ============================================================================ */

static bool animation_cloth_bones_validate(const animation_cloth_bones_internal_t* item) {
    if (!item) return false;
    if (!item->initialized) return false;
    if (!item->sim_data) return false;
    if (item->sim_data->particle_count > 0 && !item->sim_data->particles) return false;
    return true;
}

static void animation_cloth_bones_cleanup_internal(animation_cloth_bones_internal_t* item) {
    if (!item) return;

    if (item->sim_data) {
        // Acquire mutex before destruction if it was initialized
        // In this case we assume we own the object and no one else is using it
        if (item->sim_data->mutex) {
            mutex_destroy(item->sim_data->mutex);
        }

        if (item->sim_data->particles) {
            free(item->sim_data->particles);
        }
        if (item->sim_data->constraints) {
            free(item->sim_data->constraints);
        }
        free(item->sim_data);
        item->sim_data = NULL;
    }
    item->initialized = false;
}

/* ============================================================================
 * PUBLIC API
 * ============================================================================ */

int animation_cloth_bones_init(void) {
    if (g_cloth_bones_ctx.initialized) {
        return 0; // Already initialized
    }

    g_cloth_bones_ctx.capacity = ANIMATION_CLOTH_BONES_DEFAULT_CAPACITY;
    g_cloth_bones_ctx.items = calloc(g_cloth_bones_ctx.capacity, sizeof(animation_cloth_bones_internal_t));
    if (!g_cloth_bones_ctx.items) {
        return -1;
    }

    g_cloth_bones_ctx.count = 0;
    g_cloth_bones_ctx.initialized = true;

    return 0;
}

void animation_cloth_bones_shutdown(void) {
    if (!g_cloth_bones_ctx.initialized) {
        return;
    }

    for (uint32_t i = 0; i < g_cloth_bones_ctx.count; i++) {
        animation_cloth_bones_cleanup_internal(&g_cloth_bones_ctx.items[i]);
    }

    free(g_cloth_bones_ctx.items);
    g_cloth_bones_ctx.items = NULL;
    g_cloth_bones_ctx.count = 0;
    g_cloth_bones_ctx.capacity = 0;
    g_cloth_bones_ctx.initialized = false;
}

int animation_cloth_bones_create(animation_cloth_bones_handle_t* out_handle, const animation_cloth_bones_desc_t* desc) {
    // TODO: Implement cloth bones serialization

    if (!out_handle || !desc) {
        return -1;
    }

    if (!g_cloth_bones_ctx.initialized) {
        return -2;
    }

    if (g_cloth_bones_ctx.count >= g_cloth_bones_ctx.capacity) {
        // TODO: Expand capacity
        return -3;
    }

    uint32_t index = g_cloth_bones_ctx.count++;
    animation_cloth_bones_internal_t* item = &g_cloth_bones_ctx.items[index];

    item->id = index;
    item->flags = desc->flags;
    item->initialized = true;
    item->dirty = true;
    item->frame_updated = 0;

    // Allocate simulation data
    item->sim_data = calloc(1, sizeof(cloth_simulation_data_t));
    if (!item->sim_data) {
        item->initialized = false;
        g_cloth_bones_ctx.count--;
        return -4;
    }

    // Initialize mutex
    item->sim_data->mutex = mutex_create(false, "ClothBonesMutex");
    if (!item->sim_data->mutex) {
        free(item->sim_data);
        item->initialized = false;
        g_cloth_bones_ctx.count--;
        return -5;
    }

    // Parse config from user_data or use defaults
    animation_cloth_bones_config_t* config = (animation_cloth_bones_config_t*)desc->user_data;

    if (config) {
        item->sim_data->particle_count = config->particle_count;
        item->sim_data->constraint_count = config->constraint_count;
        item->sim_data->gravity = config->gravity;
        item->sim_data->drag = config->drag;
    } else {
        // Defaults
        item->sim_data->particle_count = 5;
        item->sim_data->constraint_count = 4;
        item->sim_data->gravity = vec3_create(0.0f, -9.81f, 0.0f);
        item->sim_data->drag = ANIMATION_CLOTH_BONES_DEFAULT_DRAG;
    }

    item->sim_data->solver_iterations = ANIMATION_CLOTH_BONES_DEFAULT_ITERATIONS;
    item->sim_data->is_simulating = true;

    // Allocate particles
    if (item->sim_data->particle_count > 0) {
        item->sim_data->particles = calloc(item->sim_data->particle_count, sizeof(cloth_particle_t));
        if (!item->sim_data->particles) {
            mutex_destroy(item->sim_data->mutex);
            free(item->sim_data);
            item->initialized = false;
            g_cloth_bones_ctx.count--;
            return -4;
        }

        // Initialize particles
        for (uint32_t i = 0; i < item->sim_data->particle_count; i++) {
            cloth_particle_t* p = &item->sim_data->particles[i];

            if (config && config->initial_positions) {
                p->position = config->initial_positions[i];
                p->prev_position = config->initial_positions[i];
            } else {
                // Default: vertical line
                p->position = vec3_create(0.0f, -((float)i * 0.5f), 0.0f);
                p->prev_position = p->position;
            }

            p->acceleration = vec3_zero();

            if (config && config->masses) {
                p->mass = config->masses[i];
            } else {
                p->mass = (i == 0) ? 0.0f : 1.0f; // Top fixed by default
            }

            if (p->mass <= 0.0001f) {
                p->inv_mass = 0.0f;
                p->is_fixed = true;
            } else {
                p->inv_mass = 1.0f / p->mass;
                p->is_fixed = false;
            }
            p->radius = 0.1f;
        }
    }

    // Allocate constraints
    if (item->sim_data->constraint_count > 0) {
        item->sim_data->constraints = calloc(item->sim_data->constraint_count, sizeof(cloth_constraint_t));
        if (!item->sim_data->constraints) {
            free(item->sim_data->particles);
            mutex_destroy(item->sim_data->mutex);
            free(item->sim_data);
            item->initialized = false;
            g_cloth_bones_ctx.count--;
            return -4;
        }

        // Initialize constraints
        for (uint32_t i = 0; i < item->sim_data->constraint_count; i++) {
            cloth_constraint_t* c = &item->sim_data->constraints[i];

            if (config && config->constraints_pairs) {
                c->particle_a = config->constraints_pairs[i * 2];
                c->particle_b = config->constraints_pairs[i * 2 + 1];
            } else {
                // Default: linear chain
                c->particle_a = i;
                c->particle_b = i + 1;
            }

            // Validate indices
            if (c->particle_a < item->sim_data->particle_count && c->particle_b < item->sim_data->particle_count) {
                float dist = vec3_distance(item->sim_data->particles[c->particle_a].position,
                                          item->sim_data->particles[c->particle_b].position);
                c->rest_distance = dist;
            } else {
                c->rest_distance = 0.0f;
            }

            c->stiffness = 1.0f;
        }
    }

    item->data_size = sizeof(cloth_simulation_data_t) +
                      item->sim_data->particle_count * sizeof(cloth_particle_t) +
                      item->sim_data->constraint_count * sizeof(cloth_constraint_t);

    out_handle->id = index;
    return 0;
}

void animation_cloth_bones_destroy(animation_cloth_bones_handle_t handle) {
    // TODO: Add cloth bones performance counters

    if (handle.id >= g_cloth_bones_ctx.count) {
        return;
    }

    animation_cloth_bones_cleanup_internal(&g_cloth_bones_ctx.items[handle.id]);
}

int animation_cloth_bones_update(animation_cloth_bones_handle_t handle, const void* data, size_t size) {
    // TODO: Implement cloth bones memory pooling
    // TODO: Add cloth bones caching layer
    // TODO: Implement cloth bones async operations

    if (handle.id >= g_cloth_bones_ctx.count) {
        return -1;
    }

    animation_cloth_bones_internal_t* item = &g_cloth_bones_ctx.items[handle.id];
    if (!animation_cloth_bones_validate(item)) {
        return -2;
    }

    // TODO: Add cloth bones GPU integration
    // TODO: Implement cloth bones SIMD optimization

    cloth_simulation_data_t* sim = item->sim_data;

    // Thread safety
    mutex_lock(sim->mutex);

    // Extract delta time from data (assuming data contains float dt)
    float dt = 0.016f; // Default 60fps
    if (data && size >= sizeof(float)) {
        dt = *(const float*)data;
    }

    // Clamp dt to avoid explosions
    if (dt > 0.1f) dt = 0.1f;

    // 1. Apply forces (Gravity + Drag)
    for (uint32_t i = 0; i < sim->particle_count; i++) {
        cloth_particle_t* p = &sim->particles[i];
        if (p->is_fixed) continue;

        // Gravity
        Vec3 force = vec3_mul(sim->gravity, p->mass);

        // Drag
        Vec3 velocity = vec3_sub(p->position, p->prev_position);
        Vec3 drag_force = vec3_mul(velocity, -sim->drag);

        force = vec3_add(force, drag_force);

        p->acceleration = vec3_add(p->acceleration, vec3_mul(force, p->inv_mass));
    }

    // 2. Verlet Integration
    for (uint32_t i = 0; i < sim->particle_count; i++) {
        cloth_particle_t* p = &sim->particles[i];
        if (p->is_fixed) continue;

        Vec3 temp_pos = p->position;

        // pos = pos + (pos - prev_pos) + acc * dt * dt
        Vec3 velocity = vec3_sub(p->position, p->prev_position);
        Vec3 delta = vec3_add(velocity, vec3_mul(p->acceleration, dt * dt));

        p->position = vec3_add(p->position, delta);
        p->prev_position = temp_pos;

        // Reset acceleration
        p->acceleration = vec3_zero();
    }

    // 3. Solve Constraints
    for (uint32_t iter = 0; iter < sim->solver_iterations; iter++) {
        for (uint32_t i = 0; i < sim->constraint_count; i++) {
            cloth_constraint_t* c = &sim->constraints[i];

            if (c->particle_a >= sim->particle_count || c->particle_b >= sim->particle_count) continue;

            cloth_particle_t* p1 = &sim->particles[c->particle_a];
            cloth_particle_t* p2 = &sim->particles[c->particle_b];

            Vec3 delta = vec3_sub(p2->position, p1->position);
            float dist = vec3_length(delta);

            if (dist > 0.0001f) {
                float diff = (dist - c->rest_distance) / dist;
                Vec3 correction = vec3_mul(delta, 0.5f * diff * c->stiffness);

                if (!p1->is_fixed) {
                    p1->position = vec3_add(p1->position, correction);
                }
                if (!p2->is_fixed) {
                    p2->position = vec3_sub(p2->position, correction);
                }
            }
        }
    }

    item->dirty = true;
    item->frame_updated++;

    mutex_unlock(sim->mutex);

    return 0;
}

bool animation_cloth_bones_is_valid(animation_cloth_bones_handle_t handle) {
    // TODO: Add cloth bones batch processing

    if (handle.id >= g_cloth_bones_ctx.count) {
        return false;
    }
    return g_cloth_bones_ctx.items[handle.id].initialized;
}

int animation_cloth_bones_get_info(animation_cloth_bones_handle_t handle, animation_cloth_bones_info_t* out_info) {
    // TODO: Implement cloth bones streaming support
    // TODO: Add cloth bones LOD support

    if (!out_info) {
        return -1;
    }

    if (handle.id >= g_cloth_bones_ctx.count) {
        return -2;
    }

    const animation_cloth_bones_internal_t* item = &g_cloth_bones_ctx.items[handle.id];
    out_info->id = item->id;
    out_info->flags = item->flags;
    out_info->initialized = item->initialized;

    return 0;
}

void animation_cloth_bones_mark_dirty(animation_cloth_bones_handle_t handle) {
    // TODO: Implement cloth bones culling integration

    if (handle.id < g_cloth_bones_ctx.count) {
        g_cloth_bones_ctx.items[handle.id].dirty = true;
    }
}

int animation_cloth_bones_process_pending(void) {
    // TODO: Add cloth bones render graph node

    int processed = 0;
    for (uint32_t i = 0; i < g_cloth_bones_ctx.count; i++) {
        animation_cloth_bones_internal_t* item = &g_cloth_bones_ctx.items[i];
        if (item->initialized && item->dirty) {
            // Process item
            item->dirty = false;
            processed++;
        }
    }

    return processed;
}

uint32_t animation_cloth_bones_get_count(void) {
    return g_cloth_bones_ctx.count;
}

size_t animation_cloth_bones_get_memory_usage(void) {
    // TODO: Implement memory tracking (detailed)

    size_t total = sizeof(g_cloth_bones_ctx);
    total += g_cloth_bones_ctx.capacity * sizeof(animation_cloth_bones_internal_t);

    for (uint32_t i = 0; i < g_cloth_bones_ctx.count; i++) {
        if (g_cloth_bones_ctx.items[i].initialized) {
            total += g_cloth_bones_ctx.items[i].data_size;
        }
    }

    return total;
}

void animation_cloth_bones_debug_print(void) {
    // TODO: Implement debug output
}

/* End of cloth_bones.c */
