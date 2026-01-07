/*
 * gpu_particles.c
 * GPU particle compute
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
 * TODO: Implement gpu particles initialization
 * TODO: Add gpu particles cleanup/shutdown
 * TODO: Implement gpu particles validation
 * TODO: Add gpu particles error handling
 * TODO: Implement gpu particles serialization
 * TODO: Add gpu particles debug output
 * TODO: Implement gpu particles unit tests
 * TODO: Add gpu particles performance counters
 * TODO: Implement gpu particles hot-reload
 * TODO: Add gpu particles thread safety
 * TODO: Implement gpu particles memory pooling
 * TODO: Add gpu particles caching layer
 * TODO: Implement gpu particles async operations
 * TODO: Add gpu particles GPU integration
 * TODO: Implement gpu particles SIMD optimization
 * TODO: Add gpu particles batch processing
 * TODO: Implement gpu particles streaming support
 * TODO: Add gpu particles LOD support
 * TODO: Implement gpu particles culling integration
 * TODO: Add gpu particles render graph node
 */

#include "effects/particles/gpu_particles.h"
#include <stdint.h>
#include <stdbool.h>
#include <stddef.h>
#include <string.h>
#include <stdlib.h>

/* ============================================================================
 * CONSTANTS
 * ============================================================================ */

#define EFFECTS_GPU_PARTICLES_MAX_COUNT 4096
#define EFFECTS_GPU_PARTICLES_DEFAULT_CAPACITY 256
#define EFFECTS_GPU_PARTICLES_ALIGNMENT 16

/* ============================================================================
 * TYPES
 * ============================================================================ */

typedef struct effects_gpu_particles_internal {
    uint32_t id;
    uint32_t flags;
    effects_particle_buffer_handle_t buffer_handle;
    bool initialized;
    bool dirty;
    uint64_t frame_updated;
} effects_gpu_particles_internal_t;

typedef struct effects_gpu_particles_context {
    effects_gpu_particles_internal_t* items;
    uint32_t count;
    uint32_t capacity;
    void* allocator;
    bool initialized;
} effects_gpu_particles_context_t;

static effects_gpu_particles_context_t g_gpu_particles_ctx = {0};

/* ============================================================================
 * PRIVATE FUNCTIONS
 * ============================================================================ */

static bool effects_gpu_particles_validate(const effects_gpu_particles_internal_t* item) {
    if (!item) return false;
    if (!item->initialized) return false;
    return true;
}

static void effects_gpu_particles_cleanup_internal(effects_gpu_particles_internal_t* item) {
    if (!item) return;
    
    if (effects_particle_buffer_is_valid(item->buffer_handle)) {
        effects_particle_buffer_destroy(item->buffer_handle);
    }
    
    item->initialized = false;
}

/* ============================================================================
 * PUBLIC API
 * ============================================================================ */

int effects_gpu_particles_init(void) {
    if (g_gpu_particles_ctx.initialized) {
        return 0; // Already initialized
    }
    
    // Initialize subsystems
    effects_particle_buffer_init();

    g_gpu_particles_ctx.capacity = EFFECTS_GPU_PARTICLES_DEFAULT_CAPACITY;
    g_gpu_particles_ctx.items = calloc(g_gpu_particles_ctx.capacity, sizeof(effects_gpu_particles_internal_t));
    if (!g_gpu_particles_ctx.items) {
        return -1;
    }

    g_gpu_particles_ctx.count = 0;
    g_gpu_particles_ctx.initialized = true;

    return 0;
}

void effects_gpu_particles_shutdown(void) {
    if (!g_gpu_particles_ctx.initialized) {
        return;
    }

    for (uint32_t i = 0; i < g_gpu_particles_ctx.count; i++) {
        effects_gpu_particles_cleanup_internal(&g_gpu_particles_ctx.items[i]);
    }
    
    // Shutdown subsystems
    effects_particle_buffer_shutdown();

    free(g_gpu_particles_ctx.items);
    g_gpu_particles_ctx.items = NULL;
    g_gpu_particles_ctx.count = 0;
    g_gpu_particles_ctx.capacity = 0;
    g_gpu_particles_ctx.initialized = false;
}

int effects_gpu_particles_create(effects_gpu_particles_handle_t* out_handle, const effects_gpu_particles_desc_t* desc) {
    if (!out_handle || !desc) {
        return -1;
    }

    if (!g_gpu_particles_ctx.initialized) {
        return -2;
    }

    if (g_gpu_particles_ctx.count >= g_gpu_particles_ctx.capacity) {
        return -3;
    }

    uint32_t index = g_gpu_particles_ctx.count++;
    effects_gpu_particles_internal_t* item = &g_gpu_particles_ctx.items[index];

    item->id = index;
    item->flags = desc->flags;
    item->initialized = true;
    item->dirty = true;
    item->frame_updated = 0;
    
    // Create particle buffer
    effects_particle_buffer_desc_t buffer_desc = {
        .max_particles = desc->max_particles,
        .flags = 0, // Default flags
        .user_data = NULL
    };
    if (effects_particle_buffer_create(&item->buffer_handle, &buffer_desc) != 0) {
        // Handle error: clean up and return
        item->initialized = false;
        return -4;
    }

    out_handle->id = index;
    return 0;
}

void effects_gpu_particles_destroy(effects_gpu_particles_handle_t handle) {
    if (handle.id >= g_gpu_particles_ctx.count) {
        return;
    }

    effects_gpu_particles_cleanup_internal(&g_gpu_particles_ctx.items[handle.id]);
}

int effects_gpu_particles_update(effects_gpu_particles_handle_t handle, const void* data, size_t size) {
    if (handle.id >= g_gpu_particles_ctx.count) {
        return -1;
    }

    effects_gpu_particles_internal_t* item = &g_gpu_particles_ctx.items[handle.id];
    if (!item->initialized) {
        return -2;
    }

    // This is where we would trigger compute shaders
    // dispatch_spawn(...);
    // dispatch_update(...);

    item->dirty = true;
    return 0;
}

bool effects_gpu_particles_is_valid(effects_gpu_particles_handle_t handle) {
    if (handle.id >= g_gpu_particles_ctx.count) {
        return false;
    }
    return g_gpu_particles_ctx.items[handle.id].initialized;
}

int effects_gpu_particles_get_info(effects_gpu_particles_handle_t handle, effects_gpu_particles_info_t* out_info) {
    if (!out_info) {
        return -1;
    }

    if (handle.id >= g_gpu_particles_ctx.count) {
        return -2;
    }

    const effects_gpu_particles_internal_t* item = &g_gpu_particles_ctx.items[handle.id];
    out_info->id = item->id;
    // Retrieve info from buffer
    effects_particle_buffer_info_t buffer_info;
    if (effects_particle_buffer_get_info(item->buffer_handle, &buffer_info) == 0) {
        out_info->max_particles = buffer_info.max_particles;
    } else {
        out_info->max_particles = 0;
    }
    out_info->active_particles = 0; // Need atomic counter readback
    out_info->initialized = item->initialized;

    return 0;
}

void effects_gpu_particles_mark_dirty(effects_gpu_particles_handle_t handle) {
    if (handle.id < g_gpu_particles_ctx.count) {
        g_gpu_particles_ctx.items[handle.id].dirty = true;
    }
}

int effects_gpu_particles_process_pending(void) {
    int processed = 0;
    for (uint32_t i = 0; i < g_gpu_particles_ctx.count; i++) {
        effects_gpu_particles_internal_t* item = &g_gpu_particles_ctx.items[i];
        if (item->initialized && item->dirty) {
            // Process item
            item->dirty = false;
            processed++;
        }
    }

    return processed;
}

uint32_t effects_gpu_particles_get_count(void) {
    return g_gpu_particles_ctx.count;
}

size_t effects_gpu_particles_get_memory_usage(void) {
    size_t total = sizeof(g_gpu_particles_ctx);
    total += g_gpu_particles_ctx.capacity * sizeof(effects_gpu_particles_internal_t);
    // Add buffer memory usage
    total += effects_particle_buffer_get_memory_usage();
    return total;
}

void effects_gpu_particles_debug_print(void) {
    // TODO: Implement debug output
    // Debug printing implementation
}

/* End of gpu_particles.c */
