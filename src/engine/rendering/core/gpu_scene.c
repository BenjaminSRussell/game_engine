/*
 * gpu_scene.c
 * GPU scene buffer management and instance data handling
 *
 * Part of the Rendering subsystem
 * Advanced 3D Rendering Engine
 */

#include "gpu_scene.h"
#include <stdlib.h>
#include <string.h>
#include <stdio.h>

/* ============================================================================
 * CONSTANTS
 * ============================================================================ */

#define GPU_SCENE_DEFAULT_CAPACITY 16
#define INVALID_INSTANCE_INDEX 0xFFFFFFFF

/* ============================================================================
 * TYPES
 * ============================================================================ */

typedef struct gpu_scene_instance_mapping {
    uint32_t instance_id;
    uint32_t gpu_buffer_index;
    bool dirty;
} gpu_scene_instance_mapping_t;

typedef struct rendering_gpu_scene_internal {
    uint32_t id;
    uint32_t flags;

    // Instance data
    GPUInstanceData* instance_data;
    gpu_scene_instance_mapping_t* instance_map;
    uint32_t instance_count;
    uint32_t max_instances;
    uint32_t instance_capacity;

    // Material data
    GPUMaterialData* material_data;
    uint32_t material_count;
    uint32_t max_materials;

    // Indirect draw args
    IndirectDrawArgs* indirect_args;
    uint32_t indirect_arg_count;
    uint32_t max_draw_calls;

    // Culling results
    CullingResult* culling_results;
    uint32_t culling_result_count;

    // GPU Counters
    GPUCounters counters;

    // Dirty tracking
    bool instances_dirty;
    bool materials_dirty;
    uint32_t dirty_instance_start;
    uint32_t dirty_instance_end;

    // Stats
    uint32_t frame_number;
    uint32_t visible_instance_count;

    bool initialized;
} rendering_gpu_scene_internal_t;

typedef struct rendering_gpu_scene_context {
    rendering_gpu_scene_internal_t* items;
    uint32_t count;
    uint32_t capacity;
    bool initialized;
} rendering_gpu_scene_context_t;

static rendering_gpu_scene_context_t g_gpu_scene_ctx = {0};

/* ============================================================================
 * PRIVATE HELPER FUNCTIONS
 * ============================================================================ */

static inline bool gpu_scene_validate(const rendering_gpu_scene_internal_t* scene) {
    return scene != NULL && scene->initialized;
}

static uint32_t find_instance_index(const rendering_gpu_scene_internal_t* scene,
                                    uint32_t instance_id) {
    for (uint32_t i = 0; i < scene->instance_count; i++) {
        if (scene->instance_map[i].instance_id == instance_id) {
            return scene->instance_map[i].gpu_buffer_index;
        }
    }
    return INVALID_INSTANCE_INDEX;
}

static void gpu_scene_cleanup_internal(rendering_gpu_scene_internal_t* scene) {
    if (!scene) return;

    free(scene->instance_data);
    free(scene->instance_map);
    free(scene->material_data);
    free(scene->indirect_args);
    free(scene->culling_results);

    scene->instance_data = NULL;
    scene->instance_map = NULL;
    scene->material_data = NULL;
    scene->indirect_args = NULL;
    scene->culling_results = NULL;

    scene->instance_count = 0;
    scene->material_count = 0;
    scene->indirect_arg_count = 0;
    scene->culling_result_count = 0;
    scene->initialized = false;
}

/* ============================================================================
 * PUBLIC API
 * ============================================================================ */

int rendering_gpu_scene_init(void) {
    if (g_gpu_scene_ctx.initialized) {
        return 0;
    }

    g_gpu_scene_ctx.capacity = GPU_SCENE_DEFAULT_CAPACITY;
    g_gpu_scene_ctx.items = calloc(g_gpu_scene_ctx.capacity,
                                   sizeof(rendering_gpu_scene_internal_t));
    if (!g_gpu_scene_ctx.items) {
        return -1;
    }

    g_gpu_scene_ctx.count = 0;
    g_gpu_scene_ctx.initialized = true;

    return 0;
}

void rendering_gpu_scene_shutdown(void) {
    if (!g_gpu_scene_ctx.initialized) {
        return;
    }

    for (uint32_t i = 0; i < g_gpu_scene_ctx.count; i++) {
        gpu_scene_cleanup_internal(&g_gpu_scene_ctx.items[i]);
    }

    free(g_gpu_scene_ctx.items);
    g_gpu_scene_ctx.items = NULL;
    g_gpu_scene_ctx.count = 0;
    g_gpu_scene_ctx.capacity = 0;
    g_gpu_scene_ctx.initialized = false;
}

int rendering_gpu_scene_create(rendering_gpu_scene_handle_t* out_handle,
                               const rendering_gpu_scene_desc_t* desc) {
    if (!out_handle || !desc) {
        return -1;
    }

    if (!g_gpu_scene_ctx.initialized) {
        return -2;
    }

    if (g_gpu_scene_ctx.count >= g_gpu_scene_ctx.capacity) {
        uint32_t new_capacity = g_gpu_scene_ctx.capacity * 2;
        rendering_gpu_scene_internal_t* new_items = realloc(g_gpu_scene_ctx.items,
                                                             new_capacity * sizeof(rendering_gpu_scene_internal_t));
        if (!new_items) {
            return -3;
        }
        g_gpu_scene_ctx.items = new_items;
        g_gpu_scene_ctx.capacity = new_capacity;
    }

    uint32_t index = g_gpu_scene_ctx.count++;
    rendering_gpu_scene_internal_t* scene = &g_gpu_scene_ctx.items[index];

    // Initialize instance data
    scene->max_instances = desc->max_instances > 0 ? desc->max_instances : 10000;
    scene->instance_data = malloc(scene->max_instances * sizeof(GPUInstanceData));
    scene->instance_map = malloc(scene->max_instances * sizeof(gpu_scene_instance_mapping_t));
    if (!scene->instance_data || !scene->instance_map) {
        gpu_scene_cleanup_internal(scene);
        return -4;
    }

    // Initialize material data
    scene->max_materials = desc->max_materials > 0 ? desc->max_materials : 256;
    scene->material_data = malloc(scene->max_materials * sizeof(GPUMaterialData));
    if (!scene->material_data) {
        gpu_scene_cleanup_internal(scene);
        return -5;
    }

    // Initialize indirect args
    scene->max_draw_calls = desc->max_draw_calls > 0 ? desc->max_draw_calls : 1024;
    scene->indirect_args = malloc(scene->max_draw_calls * sizeof(IndirectDrawArgs));
    if (!scene->indirect_args) {
        gpu_scene_cleanup_internal(scene);
        return -6;
    }

    // Initialize culling results
    scene->culling_results = malloc(scene->max_instances * sizeof(CullingResult));
    if (!scene->culling_results) {
        gpu_scene_cleanup_internal(scene);
        return -7;
    }

    // Initialize metadata
    scene->id = index;
    scene->flags = desc->flags;
    scene->instance_count = 0;
    scene->instance_capacity = scene->max_instances;
    scene->material_count = 0;
    scene->indirect_arg_count = 0;
    scene->culling_result_count = 0;
    scene->instances_dirty = false;
    scene->materials_dirty = false;
    scene->dirty_instance_start = 0;
    scene->dirty_instance_end = 0;
    scene->frame_number = 0;
    scene->visible_instance_count = 0;
    scene->initialized = true;

    // Initialize counters
    memset(&scene->counters, 0, sizeof(GPUCounters));

    out_handle->id = index;
    return 0;
}

void rendering_gpu_scene_destroy(rendering_gpu_scene_handle_t handle) {
    if (handle.id >= g_gpu_scene_ctx.count) {
        return;
    }

    gpu_scene_cleanup_internal(&g_gpu_scene_ctx.items[handle.id]);
}

int rendering_gpu_scene_add_instance(rendering_gpu_scene_handle_t handle,
                                     uint32_t instance_id,
                                     const GPUInstanceData* instance_data) {
    if (!instance_data || handle.id >= g_gpu_scene_ctx.count) {
        return -1;
    }

    rendering_gpu_scene_internal_t* scene = &g_gpu_scene_ctx.items[handle.id];
    if (!gpu_scene_validate(scene)) {
        return -2;
    }

    if (scene->instance_count >= scene->max_instances) {
        return -3;  // Capacity exceeded
    }

    // Check if instance already exists
    if (find_instance_index(scene, instance_id) != INVALID_INSTANCE_INDEX) {
        return -4;  // Instance already exists
    }

    uint32_t gpu_idx = scene->instance_count;
    scene->instance_data[gpu_idx] = *instance_data;
    scene->instance_map[gpu_idx].instance_id = instance_id;
    scene->instance_map[gpu_idx].gpu_buffer_index = gpu_idx;
    scene->instance_map[gpu_idx].dirty = true;
    scene->instance_count++;

    scene->instances_dirty = true;
    scene->dirty_instance_start = 0;
    scene->dirty_instance_end = scene->instance_count;

    return 0;
}

int rendering_gpu_scene_update_instance(rendering_gpu_scene_handle_t handle,
                                        uint32_t instance_id,
                                        const GPUInstanceData* instance_data) {
    if (!instance_data || handle.id >= g_gpu_scene_ctx.count) {
        return -1;
    }

    rendering_gpu_scene_internal_t* scene = &g_gpu_scene_ctx.items[handle.id];
    if (!gpu_scene_validate(scene)) {
        return -2;
    }

    uint32_t gpu_idx = find_instance_index(scene, instance_id);
    if (gpu_idx == INVALID_INSTANCE_INDEX) {
        return -3;  // Instance not found
    }

    scene->instance_data[gpu_idx] = *instance_data;
    scene->instance_map[gpu_idx].dirty = true;

    // Update dirty range
    if (gpu_idx < scene->dirty_instance_start) {
        scene->dirty_instance_start = gpu_idx;
    }
    if (gpu_idx >= scene->dirty_instance_end) {
        scene->dirty_instance_end = gpu_idx + 1;
    }

    scene->instances_dirty = true;

    return 0;
}

int rendering_gpu_scene_remove_instance(rendering_gpu_scene_handle_t handle,
                                        uint32_t instance_id) {
    if (handle.id >= g_gpu_scene_ctx.count) {
        return -1;
    }

    rendering_gpu_scene_internal_t* scene = &g_gpu_scene_ctx.items[handle.id];
    if (!gpu_scene_validate(scene)) {
        return -2;
    }

    uint32_t gpu_idx = find_instance_index(scene, instance_id);
    if (gpu_idx == INVALID_INSTANCE_INDEX) {
        return -3;  // Instance not found
    }

    // Move last instance to this position
    if (gpu_idx < scene->instance_count - 1) {
        scene->instance_data[gpu_idx] = scene->instance_data[scene->instance_count - 1];
        scene->instance_map[gpu_idx] = scene->instance_map[scene->instance_count - 1];
        scene->instance_map[gpu_idx].gpu_buffer_index = gpu_idx;
    }

    scene->instance_count--;
    scene->instances_dirty = true;

    return 0;
}

int rendering_gpu_scene_set_instances(rendering_gpu_scene_handle_t handle,
                                      const GPUInstanceData* instances,
                                      uint32_t count) {
    if (!instances || handle.id >= g_gpu_scene_ctx.count) {
        return -1;
    }

    rendering_gpu_scene_internal_t* scene = &g_gpu_scene_ctx.items[handle.id];
    if (!gpu_scene_validate(scene)) {
        return -2;
    }

    if (count > scene->max_instances) {
        return -3;  // Capacity exceeded
    }

    memcpy(scene->instance_data, instances, count * sizeof(GPUInstanceData));
    scene->instance_count = count;

    // Initialize mapping
    for (uint32_t i = 0; i < count; i++) {
        scene->instance_map[i].instance_id = i;
        scene->instance_map[i].gpu_buffer_index = i;
        scene->instance_map[i].dirty = true;
    }

    scene->instances_dirty = true;
    scene->dirty_instance_start = 0;
    scene->dirty_instance_end = count;

    return 0;
}

int rendering_gpu_scene_set_material(rendering_gpu_scene_handle_t handle,
                                     uint32_t material_id,
                                     const GPUMaterialData* material_data) {
    if (!material_data || handle.id >= g_gpu_scene_ctx.count) {
        return -1;
    }

    rendering_gpu_scene_internal_t* scene = &g_gpu_scene_ctx.items[handle.id];
    if (!gpu_scene_validate(scene)) {
        return -2;
    }

    if (material_id >= scene->max_materials) {
        return -3;  // Material ID out of range
    }

    scene->material_data[material_id] = *material_data;

    // Update material count if needed
    if (material_id >= scene->material_count) {
        scene->material_count = material_id + 1;
    }

    scene->materials_dirty = true;

    return 0;
}

int rendering_gpu_scene_set_materials(rendering_gpu_scene_handle_t handle,
                                      const GPUMaterialData* materials,
                                      uint32_t count) {
    if (!materials || handle.id >= g_gpu_scene_ctx.count) {
        return -1;
    }

    rendering_gpu_scene_internal_t* scene = &g_gpu_scene_ctx.items[handle.id];
    if (!gpu_scene_validate(scene)) {
        return -2;
    }

    if (count > scene->max_materials) {
        return -3;  // Capacity exceeded
    }

    memcpy(scene->material_data, materials, count * sizeof(GPUMaterialData));
    scene->material_count = count;
    scene->materials_dirty = true;

    return 0;
}

void* rendering_gpu_scene_get_instance_buffer(rendering_gpu_scene_handle_t handle) {
    if (handle.id >= g_gpu_scene_ctx.count) {
        return NULL;
    }

    const rendering_gpu_scene_internal_t* scene = &g_gpu_scene_ctx.items[handle.id];
    if (!gpu_scene_validate(scene)) {
        return NULL;
    }

    return (void*)scene->instance_data;
}

void* rendering_gpu_scene_get_material_buffer(rendering_gpu_scene_handle_t handle) {
    if (handle.id >= g_gpu_scene_ctx.count) {
        return NULL;
    }

    const rendering_gpu_scene_internal_t* scene = &g_gpu_scene_ctx.items[handle.id];
    if (!gpu_scene_validate(scene)) {
        return NULL;
    }

    return (void*)scene->material_data;
}

void* rendering_gpu_scene_get_indirect_buffer(rendering_gpu_scene_handle_t handle) {
    if (handle.id >= g_gpu_scene_ctx.count) {
        return NULL;
    }

    const rendering_gpu_scene_internal_t* scene = &g_gpu_scene_ctx.items[handle.id];
    if (!gpu_scene_validate(scene)) {
        return NULL;
    }

    return (void*)scene->indirect_args;
}

void* rendering_gpu_scene_get_culling_buffer(rendering_gpu_scene_handle_t handle) {
    if (handle.id >= g_gpu_scene_ctx.count) {
        return NULL;
    }

    const rendering_gpu_scene_internal_t* scene = &g_gpu_scene_ctx.items[handle.id];
    if (!gpu_scene_validate(scene)) {
        return NULL;
    }

    return (void*)scene->culling_results;
}

void rendering_gpu_scene_mark_instances_dirty(rendering_gpu_scene_handle_t handle,
                                              uint32_t start_index,
                                              uint32_t count) {
    if (handle.id >= g_gpu_scene_ctx.count) {
        return;
    }

    rendering_gpu_scene_internal_t* scene = &g_gpu_scene_ctx.items[handle.id];
    if (!gpu_scene_validate(scene)) {
        return;
    }

    if (start_index + count > scene->instance_count) {
        return;  // Out of range
    }

    for (uint32_t i = 0; i < count; i++) {
        scene->instance_map[start_index + i].dirty = true;
    }

    scene->instances_dirty = true;
    if (start_index < scene->dirty_instance_start) {
        scene->dirty_instance_start = start_index;
    }
    if (start_index + count > scene->dirty_instance_end) {
        scene->dirty_instance_end = start_index + count;
    }
}

void rendering_gpu_scene_mark_materials_dirty(rendering_gpu_scene_handle_t handle) {
    if (handle.id >= g_gpu_scene_ctx.count) {
        return;
    }

    rendering_gpu_scene_internal_t* scene = &g_gpu_scene_ctx.items[handle.id];
    if (gpu_scene_validate(scene)) {
        scene->materials_dirty = true;
    }
}

int rendering_gpu_scene_upload_dirty_data(rendering_gpu_scene_handle_t handle) {
    if (handle.id >= g_gpu_scene_ctx.count) {
        return -1;
    }

    rendering_gpu_scene_internal_t* scene = &g_gpu_scene_ctx.items[handle.id];
    if (!gpu_scene_validate(scene)) {
        return -2;
    }

    // TODO: Implement actual GPU upload using Metal backend
    // This would use metal_buffer_update() or async transfer
    // For now, just clear the dirty flags

    scene->instances_dirty = false;
    scene->materials_dirty = false;

    for (uint32_t i = 0; i < scene->instance_count; i++) {
        scene->instance_map[i].dirty = false;
    }

    return 0;
}

int rendering_gpu_scene_get_stats(rendering_gpu_scene_handle_t handle,
                                  rendering_gpu_scene_stats_t* out_stats) {
    if (!out_stats || handle.id >= g_gpu_scene_ctx.count) {
        return -1;
    }

    const rendering_gpu_scene_internal_t* scene = &g_gpu_scene_ctx.items[handle.id];
    if (!gpu_scene_validate(scene)) {
        return -2;
    }

    out_stats->total_instances = scene->instance_count;
    out_stats->visible_instances = scene->visible_instance_count;
    out_stats->draw_call_count = scene->indirect_arg_count;
    out_stats->batch_count = 0;  // TODO: Calculate from indirect args
    out_stats->frame_number = scene->frame_number;

    // Calculate memory usage
    size_t mem = (scene->instance_count * sizeof(GPUInstanceData)) +
                 (scene->material_count * sizeof(GPUMaterialData)) +
                 (scene->indirect_arg_count * sizeof(IndirectDrawArgs)) +
                 (scene->instance_count * sizeof(CullingResult));
    out_stats->gpu_memory_used = mem;

    return 0;
}

bool rendering_gpu_scene_is_valid(rendering_gpu_scene_handle_t handle) {
    if (handle.id >= g_gpu_scene_ctx.count) {
        return false;
    }

    return g_gpu_scene_ctx.items[handle.id].initialized;
}

size_t rendering_gpu_scene_get_memory_usage(void) {
    size_t total = sizeof(g_gpu_scene_ctx);
    total += g_gpu_scene_ctx.capacity * sizeof(rendering_gpu_scene_internal_t);

    for (uint32_t i = 0; i < g_gpu_scene_ctx.count; i++) {
        const rendering_gpu_scene_internal_t* scene = &g_gpu_scene_ctx.items[i];
        if (scene->initialized) {
            total += scene->instance_count * sizeof(GPUInstanceData);
            total += scene->instance_count * sizeof(gpu_scene_instance_mapping_t);
            total += scene->material_count * sizeof(GPUMaterialData);
            total += scene->indirect_arg_count * sizeof(IndirectDrawArgs);
            total += scene->instance_count * sizeof(CullingResult);
        }
    }

    return total;
}

/* End of gpu_scene.c */
