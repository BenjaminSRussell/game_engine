/*
 * particle_simulation.c
 * CPU particle simulation
 *
 * Part of the Effects subsystem
 * Advanced 3D Rendering Engine
 *
 * Implementation TODOs:
 * TODO: Implement GPU particle system
 * TODO: Add particle collision
 * TODO: Implement ribbon/trail rendering
 * TODO: Add VFX graph system
 * TODO: Implement decal rendering
 * TODO: Add weather effects
 * TODO: Implement particle sorting
 * TODO: Add particle LOD
 * TODO: Implement force fields
 * TODO: Add particle events
 * TODO: Implement particle simulation initialization
 * TODO: Add particle simulation cleanup/shutdown
 * TODO: Implement particle simulation validation
 * TODO: Add particle simulation error handling
 * TODO: Implement particle simulation serialization
 * TODO: Add particle simulation debug output
 * TODO: Implement particle simulation unit tests
 * TODO: Add particle simulation performance counters
 * TODO: Implement particle simulation hot-reload
 * TODO: Add particle simulation thread safety
 * TODO: Implement particle simulation memory pooling
 * TODO: Add particle simulation caching layer
 * TODO: Implement particle simulation async operations
 * TODO: Add particle simulation GPU integration
 * TODO: Implement particle simulation SIMD optimization
 * TODO: Add particle simulation batch processing
 * TODO: Implement particle simulation streaming support
 * TODO: Add particle simulation LOD support
 * TODO: Implement particle simulation culling integration
 * TODO: Add particle simulation render graph node
 */

#include "effects/particles/particle_simulation.h"
#include "include/math/vec3.h"
#include "include/math/vec4.h"
#include <stdint.h>
#include <stdbool.h>
#include <stddef.h>
#include <string.h>
#include <stdlib.h>

/* ============================================================================
 * CONSTANTS
 * ============================================================================ */

#define EFFECTS_PARTICLE_SIMULATION_MAX_COUNT 4096
#define EFFECTS_PARTICLE_SIMULATION_DEFAULT_CAPACITY 256
#define EFFECTS_PARTICLE_SIMULATION_ALIGNMENT 16

/* ============================================================================
 * TYPES
 * ============================================================================ */

typedef struct effects_particle_simulation_internal {
    uint32_t id;
    uint32_t flags;
    particle_t* particles;
    uint32_t active_count;
    uint32_t max_particles;
    bool initialized;
    bool dirty;
    bool enable_gpu;
    // For simple CPU pool management, we just track active count and swap-remove
} effects_particle_simulation_internal_t;

typedef struct effects_particle_simulation_context {
    effects_particle_simulation_internal_t* items;
    uint32_t count;
    uint32_t capacity;
    void* allocator;
    bool initialized;
} effects_particle_simulation_context_t;

static effects_particle_simulation_context_t g_particle_simulation_ctx = {0};

/* ============================================================================
 * PRIVATE FUNCTIONS
 * ============================================================================ */

static float random_float(float min, float max) {
    return min + (float)rand() / (float)RAND_MAX * (max - min);
}

static void random_vec3(vec3_t* v, float min, float max) {
    v->x = random_float(min, max);
    v->y = random_float(min, max);
    v->z = random_float(min, max);
}

/* ============================================================================
 * PUBLIC API
 * ============================================================================ */

int effects_particle_simulation_init(void) {
    if (g_particle_simulation_ctx.initialized) {
        return 0;
    }

    g_particle_simulation_ctx.capacity = EFFECTS_PARTICLE_SIMULATION_DEFAULT_CAPACITY;
    g_particle_simulation_ctx.items = calloc(g_particle_simulation_ctx.capacity, sizeof(effects_particle_simulation_internal_t));
    if (!g_particle_simulation_ctx.items) {
        return -1;
    }

    g_particle_simulation_ctx.count = 0;
    g_particle_simulation_ctx.initialized = true;

    return 0;
}

void effects_particle_simulation_shutdown(void) {
    if (!g_particle_simulation_ctx.initialized) {
        return;
    }

    for (uint32_t i = 0; i < g_particle_simulation_ctx.count; i++) {
        effects_particle_simulation_internal_t* item = &g_particle_simulation_ctx.items[i];
        if (item->particles) {
            free(item->particles);
            item->particles = NULL;
        }
        item->initialized = false;
    }

    free(g_particle_simulation_ctx.items);
    g_particle_simulation_ctx.items = NULL;
    g_particle_simulation_ctx.count = 0;
    g_particle_simulation_ctx.capacity = 0;
    g_particle_simulation_ctx.initialized = false;
}

int effects_particle_simulation_create(effects_particle_simulation_handle_t* out_handle, const effects_particle_simulation_desc_t* desc) {
    if (!out_handle || !desc) {
        return -1;
    }

    if (!g_particle_simulation_ctx.initialized) {
        return -2;
    }

    if (g_particle_simulation_ctx.count >= g_particle_simulation_ctx.capacity) {
        return -3;
    }

    uint32_t index = g_particle_simulation_ctx.count++;
    effects_particle_simulation_internal_t* item = &g_particle_simulation_ctx.items[index];

    item->id = index;
    item->flags = desc->flags;
    item->max_particles = desc->max_particles > 0 ? desc->max_particles : 1000;
    item->enable_gpu = desc->enable_gpu_simulation;
    item->particles = calloc(item->max_particles, sizeof(particle_t));
    if (!item->particles) {
        return -4;
    }
    
    item->active_count = 0;
    item->initialized = true;
    item->dirty = true;

    out_handle->id = index;
    return 0;
}

void effects_particle_simulation_destroy(effects_particle_simulation_handle_t handle) {
    if (handle.id >= g_particle_simulation_ctx.count) {
        return;
    }

    effects_particle_simulation_internal_t* item = &g_particle_simulation_ctx.items[handle.id];
    if (item->particles) {
        free(item->particles);
        item->particles = NULL;
    }
    item->initialized = false;
}

int effects_particle_simulation_update(effects_particle_simulation_handle_t handle, float dt) {
    if (handle.id >= g_particle_simulation_ctx.count) {
        return -1;
    }

    effects_particle_simulation_internal_t* item = &g_particle_simulation_ctx.items[handle.id];
    if (!item->initialized) {
        return -2;
    }
    
    // CPU Update
    uint32_t active = item->active_count;
    for (uint32_t i = 0; i < active; ) {
        particle_t* p = &item->particles[i];
        
        p->age += dt;
        if (p->age >= p->lifetime) {
            // Kill particle: swap with last active
            *p = item->particles[active - 1];
            active--;
            continue;
        }
        
        // Physics
        p->velocity.y += -9.8f * dt; // Simple gravity
        
        p->position.x += p->velocity.x * dt;
        p->position.y += p->velocity.y * dt;
        p->position.z += p->velocity.z * dt;
        
        p->rotation += p->rotation_speed * dt;
        
        i++;
    }
    item->active_count = active;

    return 0;
}

int effects_particle_simulation_spawn(effects_particle_simulation_handle_t handle, const emitter_params_t* emitter, float dt) {
    if (handle.id >= g_particle_simulation_ctx.count) return -1;
    effects_particle_simulation_internal_t* item = &g_particle_simulation_ctx.items[handle.id];
    
    // Calculate how many particles to spawn
    float spawn_count = emitter->spawn_rate * dt;
    uint32_t count = (uint32_t)spawn_count;
    if (random_float(0, 1) < (spawn_count - count)) {
        count++;
    }
    
    for (uint32_t i = 0; i < count; i++) {
        if (item->active_count >= item->max_particles) break;
        
        particle_t* p = &item->particles[item->active_count++];
        
        // Initialize particle from emitter params
        // Position validation based on shape
        p->position = emitter->position; // Simplification, need shape logic
        if (emitter->shape == EMITTER_SHAPE_SPHERE) {
             vec3_t offset;
             random_vec3(&offset, -emitter->shape_radius, emitter->shape_radius);
             p->position.x += offset.x; 
             p->position.y += offset.y; 
             p->position.z += offset.z; 
        }
        
        p->velocity.x = random_float(emitter->initial_speed.min, emitter->initial_speed.max); // Random dir
        p->velocity.y = random_float(emitter->initial_speed.min, emitter->initial_speed.max);
        p->velocity.z = random_float(emitter->initial_speed.min, emitter->initial_speed.max);
        
        p->color = emitter->start_color;
        p->lifetime = random_float(emitter->initial_lifetime.min, emitter->initial_lifetime.max);
        p->age = 0;
        p->size = random_float(emitter->initial_size.min, emitter->initial_size.max);
        p->rotation = random_float(emitter->initial_rotation.min, emitter->initial_rotation.max);
        p->rotation_speed = random_float(emitter->initial_rotation_speed.min, emitter->initial_rotation_speed.max);
        p->scale.x = p->scale.y = p->scale.z = 1.0f;
    }
    
    return 0;
}

bool effects_particle_simulation_is_valid(effects_particle_simulation_handle_t handle) {
    if (handle.id >= g_particle_simulation_ctx.count) return false;
    return g_particle_simulation_ctx.items[handle.id].initialized;
}

int effects_particle_simulation_get_info(effects_particle_simulation_handle_t handle, effects_particle_simulation_info_t* out_info) {
    if (!out_info) return -1;
    if (handle.id >= g_particle_simulation_ctx.count) return -2;

    const effects_particle_simulation_internal_t* item = &g_particle_simulation_ctx.items[handle.id];
    out_info->id = item->id;
    out_info->active_particle_count = item->active_count;
    out_info->max_particles = item->max_particles;
    out_info->using_gpu = item->enable_gpu;
    out_info->initialized = item->initialized;

    return 0;
}

void effects_particle_simulation_mark_dirty(effects_particle_simulation_handle_t handle) {
    if (handle.id < g_particle_simulation_ctx.count) {
        g_particle_simulation_ctx.items[handle.id].dirty = true;
    }
}

int effects_particle_simulation_process_pending(void) {
    return 0;
}

const particle_t* effects_particle_simulation_get_particles(effects_particle_simulation_handle_t handle, uint32_t* out_count) {
    if (handle.id >= g_particle_simulation_ctx.count) {
        if (out_count) *out_count = 0;
        return NULL;
    }
    const effects_particle_simulation_internal_t* item = &g_particle_simulation_ctx.items[handle.id];
    if (out_count) *out_count = item->active_count;
    return item->particles;
}

uint32_t effects_particle_simulation_get_count(void) {
    return g_particle_simulation_ctx.count;
}

size_t effects_particle_simulation_get_memory_usage(void) {
    size_t total = sizeof(g_particle_simulation_ctx);
    total += g_particle_simulation_ctx.capacity * sizeof(effects_particle_simulation_internal_t);
    for (uint32_t i = 0; i < g_particle_simulation_ctx.count; i++) {
        if (g_particle_simulation_ctx.items[i].particles) {
            total += g_particle_simulation_ctx.items[i].max_particles * sizeof(particle_t);
        }
    }
    return total;
}

void effects_particle_simulation_debug_print(void) {
    // TODO: Implement debug output
    // Debug printing implementation
}

/* End of particle_simulation.c */
