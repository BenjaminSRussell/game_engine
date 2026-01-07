/*
 * debris_system.c
 * Debris generation and management
 *
 * Part of the Destruction subsystem
 * Advanced 3D Rendering Engine
 *
 * Implements physics debris spawning, pooling, and lifecycle management
 */

#include "debris_system.h"
#include <stdint.h>
#include <stdbool.h>
#include <stddef.h>
#include <string.h>
#include <stdlib.h>

/* ============================================================================
 * CONSTANTS
 * ============================================================================ */

#define DEBRIS_MAX_COUNT 4096
#define DEBRIS_SYSTEM_DEFAULT_CAPACITY 4
#define DEBRIS_POOL_SIZE 1024
#define DEBRIS_BATCH_SIZE 64
#define DEBRIS_LIFETIME_MIN 2.0f
#define DEBRIS_LIFETIME_MAX 5.0f

/* ============================================================================
 * MATH TYPES
 * ============================================================================ */

typedef struct vec3 {
    float x, y, z;
} vec3_t;

typedef struct quat {
    float x, y, z, w;
} quat_t;

/* ============================================================================
 * DEBRIS TYPES
 * ============================================================================ */

typedef struct debris_particle {
    vec3_t position;
    quat_t rotation;
    vec3_t velocity;
    vec3_t angular_velocity;
    vec3_t scale;
    
    float lifetime;
    float max_lifetime;
    bool active;
    
    uint32_t mesh_id;       // Instanced mesh ID
    uint32_t material_id;
} debris_particle_t;

typedef struct destruction_debris_system_internal {
    uint32_t id;
    uint32_t flags;
    
    debris_particle_t particles[DEBRIS_POOL_SIZE];
    uint32_t active_count;
    
    // Physics settings
    float gravity;
    float drag;
    float restitution;
    float friction;
    
    bool initialized;
    bool dirty;
    uint64_t frame_updated;
} destruction_debris_system_internal_t;

typedef struct destruction_debris_system_context {
    destruction_debris_system_internal_t* items;
    uint32_t count;
    uint32_t capacity;
    bool initialized;
} destruction_debris_system_context_t;

static destruction_debris_system_context_t g_debris_ctx = {0};

/* ============================================================================
 * MATH HELPERS
 * ============================================================================ */

static float randf(void) {
    return (float)rand() / (float)RAND_MAX;
}

static float lerpf(float a, float b, float t) {
    return a + (b - a) * t;
}

static vec3_t vec3_add(vec3_t a, vec3_t b) {
    return (vec3_t){a.x + b.x, a.y + b.y, a.z + b.z};
}

static vec3_t vec3_scale(vec3_t v, float s) {
    return (vec3_t){v.x * s, v.y * s, v.z * s};
}

/* ============================================================================
 * PUBLIC API
 * ============================================================================ */

int destruction_debris_system_init(void) {
    if (g_debris_ctx.initialized) {
        return 0;
    }

    g_debris_ctx.capacity = DEBRIS_SYSTEM_DEFAULT_CAPACITY;
    g_debris_ctx.items = calloc(g_debris_ctx.capacity, sizeof(destruction_debris_system_internal_t));
    if (!g_debris_ctx.items) {
        return -1;
    }

    g_debris_ctx.count = 0;
    g_debris_ctx.initialized = true;

    return 0;
}

void destruction_debris_system_shutdown(void) {
    if (!g_debris_ctx.initialized) {
        return;
    }

    free(g_debris_ctx.items);
    g_debris_ctx.items = NULL;
    g_debris_ctx.count = 0;
    g_debris_ctx.capacity = 0;
    g_debris_ctx.initialized = false;
}

int destruction_debris_system_create(destruction_debris_system_handle_t* out_handle, 
                                       const destruction_debris_system_desc_t* desc) {
    if (!out_handle || !desc) {
        return -1;
    }

    if (!g_debris_ctx.initialized) {
        return -2;
    }

    if (g_debris_ctx.count >= g_debris_ctx.capacity) {
        return -3;
    }

    uint32_t index = g_debris_ctx.count++;
    destruction_debris_system_internal_t* item = &g_debris_ctx.items[index];

    item->id = index;
    item->flags = desc->flags;
    item->active_count = 0;
    
    // Default physics
    item->gravity = -9.81f;
    item->drag = 0.5f;
    item->restitution = 0.3f;
    item->friction = 0.6f;
    
    memset(item->particles, 0, sizeof(item->particles));
    
    item->initialized = true;
    item->dirty = true;
    item->frame_updated = 0;

    out_handle->id = index;
    return 0;
}

void destruction_debris_system_destroy(destruction_debris_system_handle_t handle) {
    if (handle.id >= g_debris_ctx.count) {
        return;
    }

    g_debris_ctx.items[handle.id].initialized = false;
}

int destruction_debris_system_spawn(destruction_debris_system_handle_t handle,
                                      uint32_t count, vec3_t position, float radius) {
    if (handle.id >= g_debris_ctx.count) {
        return -1;
    }
    
    destruction_debris_system_internal_t* item = &g_debris_ctx.items[handle.id];
    
    uint32_t spawned = 0;
    for (uint32_t i = 0; i < DEBRIS_POOL_SIZE && spawned < count; i++) {
        if (!item->particles[i].active) {
            // Spawn new particle
            debris_particle_t* p = &item->particles[i];
            p->active = true;
            
            // Random position in sphere
            vec3_t offset = {
                (randf() * 2.0f - 1.0f) * radius,
                (randf() * 2.0f - 1.0f) * radius,
                (randf() * 2.0f - 1.0f) * radius
            };
            p->position = vec3_add(position, offset);
            
            // Random explosion velocity
            p->velocity = vec3_scale(offset, 2.0f + randf() * 5.0f);
            p->velocity.y += 2.0f; // Upwards bias
            
            p->rotation = (quat_t){0, 0, 0, 1};
            
            // Random tumble
            p->angular_velocity = (vec3_t){
                (randf() * 2.0f - 1.0f) * 10.0f,
                (randf() * 2.0f - 1.0f) * 10.0f,
                (randf() * 2.0f - 1.0f) * 10.0f
            };
            
            p->scale = (vec3_t){
                0.1f + randf() * 0.2f,
                0.1f + randf() * 0.2f,
                0.1f + randf() * 0.2f
            };
            
            p->lifetime = 0.0f;
            p->max_lifetime = lerpf(DEBRIS_LIFETIME_MIN, DEBRIS_LIFETIME_MAX, randf());
            
            spawned++;
            item->active_count++;
        }
    }
    
    return spawned;
}

int destruction_debris_system_update(destruction_debris_system_handle_t handle, 
                                       const void* data, size_t size) {
    // Treat data as dt
    if (handle.id >= g_debris_ctx.count) {
        return -1;
    }
    
    float dt = 0.016f;
    if (data && size == sizeof(float)) {
        dt = *(const float*)data;
    }
    
    destruction_debris_system_internal_t* item = &g_debris_ctx.items[handle.id];
    
    for (uint32_t i = 0; i < DEBRIS_POOL_SIZE; i++) {
        debris_particle_t* p = &item->particles[i];
        if (p->active) {
            p->lifetime += dt;
            if (p->lifetime >= p->max_lifetime) {
                p->active = false;
                item->active_count--;
                continue;
            }
            
            // Physics update
            p->velocity.y += item->gravity * dt;
            p->velocity = vec3_scale(p->velocity, 1.0f - item->drag * dt);
            
            p->position = vec3_add(p->position, vec3_scale(p->velocity, dt));
            
            // Rotate
            // Simple angular integration would go here
            
            // Floor collision (y = 0)
            if (p->position.y < 0.0f) {
                p->position.y = 0.0f;
                p->velocity.y = -p->velocity.y * item->restitution;
                p->velocity.x *= (1.0f - item->friction);
                p->velocity.z *= (1.0f - item->friction);
                p->angular_velocity = vec3_scale(p->angular_velocity, 0.5f);
            }
        }
    }

    item->dirty = true;
    return 0;
}

bool destruction_debris_system_is_valid(destruction_debris_system_handle_t handle) {
    if (handle.id >= g_debris_ctx.count) {
        return false;
    }
    return g_debris_ctx.items[handle.id].initialized;
}

int destruction_debris_system_get_info(destruction_debris_system_handle_t handle, 
                                         destruction_debris_system_info_t* out_info) {
    if (!out_info) {
        return -1;
    }

    if (handle.id >= g_debris_ctx.count) {
        return -2;
    }

    const destruction_debris_system_internal_t* item = &g_debris_ctx.items[handle.id];
    out_info->id = item->id;
    out_info->flags = item->flags;
    out_info->initialized = item->initialized;

    return 0;
}

void destruction_debris_system_mark_dirty(destruction_debris_system_handle_t handle) {
    if (handle.id < g_debris_ctx.count) {
        g_debris_ctx.items[handle.id].dirty = true;
    }
}

int destruction_debris_system_process_pending(void) {
    int processed = 0;
    for (uint32_t i = 0; i < g_debris_ctx.count; i++) {
        destruction_debris_system_internal_t* item = &g_debris_ctx.items[i];
        if (item->initialized && item->dirty) {
            item->dirty = false;
            processed++;
        }
    }

    return processed;
}

uint32_t destruction_debris_system_get_count(void) {
    return g_debris_ctx.count;
}

size_t destruction_debris_system_get_memory_usage(void) {
    size_t total = sizeof(g_debris_ctx);
    total += g_debris_ctx.capacity * sizeof(destruction_debris_system_internal_t);
    return total;
}

void destruction_debris_system_debug_print(void) {
    // Debug output
}

/* End of debris_system.c */
