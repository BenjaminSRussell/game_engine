/*
 * instance_batching.c
 * Instance batch grouping and sorting for efficient rendering
 *
 * Part of the Geometry subsystem
 * Advanced 3D Rendering Engine
 */

#include "instance_batching.h"
#include "../../math/vec3.h"
#include <stdint.h>
#include <stdbool.h>
#include <stddef.h>
#include <string.h>
#include <stdlib.h>

/* ============================================================================
 * CONSTANTS
 * ============================================================================ */

#define BATCH_DEFAULT_CAPACITY 256
#define MAX_INSTANCES_PER_BATCH 1024

/* ============================================================================
 * TYPES
 * ============================================================================ */

typedef struct instance_batch_key {
    uint32_t mesh_id;
    uint32_t material_id;
    uint32_t lod_level;
    uint32_t shader_variant;
} instance_batch_key_t;

typedef struct instance_batch {
    instance_batch_key_t key;
    uint32_t* instance_ids;
    uint32_t count;
    uint32_t capacity;
    float sort_key;  // For depth sorting
} instance_batch_t;

typedef struct geometry_instance_batching_internal {
    uint32_t id;
    uint32_t flags;
    instance_batch_t* batches;
    uint32_t batch_count;
    uint32_t batch_capacity;
    bool initialized;
    bool dirty;
    uint64_t frame_updated;
} geometry_instance_batching_internal_t;

typedef struct geometry_instance_batching_context {
    geometry_instance_batching_internal_t* items;
    uint32_t count;
    uint32_t capacity;
    bool initialized;
} geometry_instance_batching_context_t;

static geometry_instance_batching_context_t g_instance_batching_ctx = {0};

/* ============================================================================
 * PRIVATE HELPER FUNCTIONS
 * ============================================================================ */

static inline bool batch_keys_equal(const instance_batch_key_t* a, const instance_batch_key_t* b) {
    return a->mesh_id == b->mesh_id &&
           a->material_id == b->material_id &&
           a->lod_level == b->lod_level &&
           a->shader_variant == b->shader_variant;
}

static instance_batch_t* find_or_create_batch(
    geometry_instance_batching_internal_t* item,
    const instance_batch_key_t* key
) {
    // Find existing batch
    for (uint32_t i = 0; i < item->batch_count; i++) {
        if (batch_keys_equal(&item->batches[i].key, key)) {
            return &item->batches[i];
        }
    }
    
    // Create new batch
    if (item->batch_count >= item->batch_capacity) {
        uint32_t new_capacity = item->batch_capacity * 2;
        instance_batch_t* new_batches = realloc(item->batches, new_capacity * sizeof(instance_batch_t));
        if (!new_batches) {
            return NULL;
        }
        item->batches = new_batches;
        item->batch_capacity = new_capacity;
    }
    
    instance_batch_t* batch = &item->batches[item->batch_count++];
    batch->key = *key;
    batch->capacity = 64;
    batch->count = 0;
    batch->instance_ids = malloc(batch->capacity * sizeof(uint32_t));
    batch->sort_key = 0.0f;
    
    if (!batch->instance_ids) {
        item->batch_count--;
        return NULL;
    }
    
    return batch;
}

static int compare_batches_opaque(const void* a, const void* b) {
    const instance_batch_t* batch_a = (const instance_batch_t*)a;
    const instance_batch_t* batch_b = (const instance_batch_t*)b;
    
    // Sort opaque: material -> mesh -> shader
    if (batch_a->key.material_id != batch_b->key.material_id) {
        return (int)batch_a->key.material_id - (int)batch_b->key.material_id;
    }
    if (batch_a->key.mesh_id != batch_b->key.mesh_id) {
        return (int)batch_a->key.mesh_id - (int)batch_b->key.mesh_id;
    }
    return (int)batch_a->key.shader_variant - (int)batch_b->key.shader_variant;
}

static int compare_batches_transparent(const void* a, const void* b) {
    const instance_batch_t* batch_a = (const instance_batch_t*)a;
    const instance_batch_t* batch_b = (const instance_batch_t*)b;
    
    // Sort transparent: back-to-front by depth
    if (batch_a->sort_key < batch_b->sort_key) return 1;
    if (batch_a->sort_key > batch_b->sort_key) return -1;
    return 0;
}

/* ============================================================================
 * PRIVATE FUNCTIONS
 * ============================================================================ */

static bool geometry_instance_batching_validate(const geometry_instance_batching_internal_t* item) {
    if (!item) return false;
    if (!item->initialized) return false;
    return true;
}

static void geometry_instance_batching_cleanup_internal(geometry_instance_batching_internal_t* item) {
    if (!item) return;
    
    if (item->batches) {
        for (uint32_t i = 0; i < item->batch_count; i++) {
            free(item->batches[i].instance_ids);
        }
        free(item->batches);
        item->batches = NULL;
    }
    
    item->batch_count = 0;
    item->batch_capacity = 0;
    item->initialized = false;
}

/* ============================================================================
 * PUBLIC API
 * ============================================================================ */

int geometry_instance_batching_init(void) {
    if (g_instance_batching_ctx.initialized) {
        return 0;
    }
    
    g_instance_batching_ctx.capacity = BATCH_DEFAULT_CAPACITY;
    g_instance_batching_ctx.items = calloc(g_instance_batching_ctx.capacity,
                                           sizeof(geometry_instance_batching_internal_t));
    if (!g_instance_batching_ctx.items) {
        return -1;
    }
    
    g_instance_batching_ctx.count = 0;
    g_instance_batching_ctx.initialized = true;
    
    return 0;
}

void geometry_instance_batching_shutdown(void) {
    if (!g_instance_batching_ctx.initialized) {
        return;
    }
    
    for (uint32_t i = 0; i < g_instance_batching_ctx.count; i++) {
        geometry_instance_batching_cleanup_internal(&g_instance_batching_ctx.items[i]);
    }
    
    free(g_instance_batching_ctx.items);
    g_instance_batching_ctx.items = NULL;
    g_instance_batching_ctx.count = 0;
    g_instance_batching_ctx.capacity = 0;
    g_instance_batching_ctx.initialized = false;
}

int geometry_instance_batching_create(geometry_instance_batching_handle_t* out_handle,
                                       const geometry_instance_batching_desc_t* desc) {
    if (!out_handle || !desc) {
        return -1;
    }
    
    if (!g_instance_batching_ctx.initialized) {
        return -2;
    }
    
    if (g_instance_batching_ctx.count >= g_instance_batching_ctx.capacity) {
        return -3;
    }
    
    uint32_t index = g_instance_batching_ctx.count++;
    geometry_instance_batching_internal_t* item = &g_instance_batching_ctx.items[index];
    
    item->id = index;
    item->flags = desc->flags;
    item->batch_capacity = 32;
    item->batch_count = 0;
    item->batches = malloc(item->batch_capacity * sizeof(instance_batch_t));
    item->initialized = true;
    item->dirty = true;
    item->frame_updated = 0;
    
    if (!item->batches) {
        return -4;
    }
    
    out_handle->id = index;
    return 0;
}

void geometry_instance_batching_destroy(geometry_instance_batching_handle_t handle) {
    if (handle.id >= g_instance_batching_ctx.count) {
        return;
    }
    
    geometry_instance_batching_cleanup_internal(&g_instance_batching_ctx.items[handle.id]);
}

int geometry_instance_batching_add_instance(
    geometry_instance_batching_handle_t handle,
    uint32_t instance_id,
    uint32_t mesh_id,
    uint32_t material_id,
    uint32_t lod_level,
    uint32_t shader_variant,
    float depth
) {
    if (handle.id >= g_instance_batching_ctx.count) {
        return -1;
    }
    
    geometry_instance_batching_internal_t* item = &g_instance_batching_ctx.items[handle.id];
    if (!item->initialized) {
        return -2;
    }
    
    instance_batch_key_t key = {
        .mesh_id = mesh_id,
        .material_id = material_id,
        .lod_level = lod_level,
        .shader_variant = shader_variant
    };
    
    instance_batch_t* batch = find_or_create_batch(item, &key);
    if (!batch) {
        return -3;
    }
    
    // Resize instance list if needed
    if (batch->count >= batch->capacity) {
        uint32_t new_capacity = batch->capacity * 2;
        uint32_t* new_ids = realloc(batch->instance_ids, new_capacity * sizeof(uint32_t));
        if (!new_ids) {
            return -4;
        }
        batch->instance_ids = new_ids;
        batch->capacity = new_capacity;
    }
    
    batch->instance_ids[batch->count++] = instance_id;
    batch->sort_key = depth;  // Use average or max depth for sorting
    item->dirty = true;
    
    return 0;
}

int geometry_instance_batching_clear(geometry_instance_batching_handle_t handle) {
    if (handle.id >= g_instance_batching_ctx.count) {
        return -1;
    }
    
    geometry_instance_batching_internal_t* item = &g_instance_batching_ctx.items[handle.id];
    if (!item->initialized) {
        return -2;
    }
    
    for (uint32_t i = 0; i < item->batch_count; i++) {
        item->batches[i].count = 0;
    }
    
    item->dirty = true;
    return 0;
}

int geometry_instance_batching_sort(geometry_instance_batching_handle_t handle, bool transparent) {
    if (handle.id >= g_instance_batching_ctx.count) {
        return -1;
    }
    
    geometry_instance_batching_internal_t* item = &g_instance_batching_ctx.items[handle.id];
    if (!item->initialized) {
        return -2;
    }
    
    if (transparent) {
        qsort(item->batches, item->batch_count, sizeof(instance_batch_t), compare_batches_transparent);
    } else {
        qsort(item->batches, item->batch_count, sizeof(instance_batch_t), compare_batches_opaque);
    }
    
    return 0;
}

int geometry_instance_batching_update(geometry_instance_batching_handle_t handle,
                                       const void* data, size_t size) {
    if (handle.id >= g_instance_batching_ctx.count) {
        return -1;
    }
    
    geometry_instance_batching_internal_t* item = &g_instance_batching_ctx.items[handle.id];
    if (!item->initialized) {
        return -2;
    }
    
    item->dirty = true;
    return 0;
}

bool geometry_instance_batching_is_valid(geometry_instance_batching_handle_t handle) {
    if (handle.id >= g_instance_batching_ctx.count) {
        return false;
    }
    return g_instance_batching_ctx.items[handle.id].initialized;
}

int geometry_instance_batching_get_info(geometry_instance_batching_handle_t handle,
                                         geometry_instance_batching_info_t* out_info) {
    if (!out_info) {
        return -1;
    }
    
    if (handle.id >= g_instance_batching_ctx.count) {
        return -2;
    }
    
    const geometry_instance_batching_internal_t* item = &g_instance_batching_ctx.items[handle.id];
    out_info->id = item->id;
    out_info->flags = item->flags;
    out_info->initialized = item->initialized;
    out_info->batch_count = item->batch_count;
    
    return 0;
}

void geometry_instance_batching_mark_dirty(geometry_instance_batching_handle_t handle) {
    if (handle.id < g_instance_batching_ctx.count) {
        g_instance_batching_ctx.items[handle.id].dirty = true;
    }
}

int geometry_instance_batching_process_pending(void) {
    int processed = 0;
    
    for (uint32_t i = 0; i < g_instance_batching_ctx.count; i++) {
        geometry_instance_batching_internal_t* item = &g_instance_batching_ctx.items[i];
        if (item->initialized && item->dirty) {
            item->dirty = false;
            processed++;
        }
    }
    
    return processed;
}

uint32_t geometry_instance_batching_get_count(void) {
    return g_instance_batching_ctx.count;
}

size_t geometry_instance_batching_get_memory_usage(void) {
    size_t total = sizeof(g_instance_batching_ctx);
    total += g_instance_batching_ctx.capacity * sizeof(geometry_instance_batching_internal_t);
    
    for (uint32_t i = 0; i < g_instance_batching_ctx.count; i++) {
        const geometry_instance_batching_internal_t* item = &g_instance_batching_ctx.items[i];
        total += item->batch_capacity * sizeof(instance_batch_t);
        for (uint32_t j = 0; j < item->batch_count; j++) {
            total += item->batches[j].capacity * sizeof(uint32_t);
        }
    }
    
    return total;
}

void geometry_instance_batching_debug_print(void) {
    printf("[Instance Batching] Total batchers: %u\n", g_instance_batching_ctx.count);
    
    for (uint32_t i = 0; i < g_instance_batching_ctx.count; i++) {
        const geometry_instance_batching_internal_t* item = &g_instance_batching_ctx.items[i];
        if (item->initialized) {
            uint32_t total_instances = 0;
            for (uint32_t j = 0; j < item->batch_count; j++) {
                total_instances += item->batches[j].count;
            }
            printf("  Batcher %u: %u batches, %u total instances\n",
                   i, item->batch_count, total_instances);
        }
    }
}

/* End of instance_batching.c */
