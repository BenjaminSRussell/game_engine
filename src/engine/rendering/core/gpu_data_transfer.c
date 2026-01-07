/*
 * gpu_data_transfer.c
 * GPU data transfer and buffer synchronization
 *
 * Part of the Rendering subsystem
 * Advanced 3D Rendering Engine
 */

#include "gpu_data_transfer.h"
#include <stdlib.h>
#include <string.h>
#include <stdio.h>

/* ============================================================================
 * CONSTANTS
 * ============================================================================ */

#define GPU_TRANSFER_DEFAULT_CAPACITY 8
#define DEFAULT_STAGING_BUFFER_SIZE (64 * 1024 * 1024)  // 64MB

/* ============================================================================
 * TYPES
 * ============================================================================ */

typedef struct rendering_gpu_transfer_internal {
    uint32_t id;
    uint32_t flags;

    // Staging buffer for CPU->GPU transfers
    void* staging_buffer;
    size_t staging_buffer_size;
    size_t staging_offset;

    // Transfer tracking
    uint32_t pending_transfers;
    bool use_async_transfer;

    bool initialized;
} rendering_gpu_transfer_internal_t;

typedef struct rendering_gpu_transfer_context {
    rendering_gpu_transfer_internal_t* items;
    uint32_t count;
    uint32_t capacity;
    bool initialized;
} rendering_gpu_transfer_context_t;

static rendering_gpu_transfer_context_t g_gpu_transfer_ctx = {0};

/* ============================================================================
 * PRIVATE HELPER FUNCTIONS
 * ============================================================================ */

static inline bool gpu_transfer_validate(const rendering_gpu_transfer_internal_t* transfer) {
    return transfer != NULL && transfer->initialized;
}

static void gpu_transfer_cleanup_internal(rendering_gpu_transfer_internal_t* transfer) {
    if (!transfer) return;

    if (transfer->staging_buffer) {
        free(transfer->staging_buffer);
        transfer->staging_buffer = NULL;
    }

    transfer->staging_buffer_size = 0;
    transfer->staging_offset = 0;
    transfer->pending_transfers = 0;
    transfer->initialized = false;
}

/* ============================================================================
 * PUBLIC API
 * ============================================================================ */

int rendering_gpu_transfer_init(void) {
    if (g_gpu_transfer_ctx.initialized) {
        return 0;
    }

    g_gpu_transfer_ctx.capacity = GPU_TRANSFER_DEFAULT_CAPACITY;
    g_gpu_transfer_ctx.items = calloc(g_gpu_transfer_ctx.capacity,
                                      sizeof(rendering_gpu_transfer_internal_t));
    if (!g_gpu_transfer_ctx.items) {
        return -1;
    }

    g_gpu_transfer_ctx.count = 0;
    g_gpu_transfer_ctx.initialized = true;

    return 0;
}

void rendering_gpu_transfer_shutdown(void) {
    if (!g_gpu_transfer_ctx.initialized) {
        return;
    }

    for (uint32_t i = 0; i < g_gpu_transfer_ctx.count; i++) {
        gpu_transfer_cleanup_internal(&g_gpu_transfer_ctx.items[i]);
    }

    free(g_gpu_transfer_ctx.items);
    g_gpu_transfer_ctx.items = NULL;
    g_gpu_transfer_ctx.count = 0;
    g_gpu_transfer_ctx.capacity = 0;
    g_gpu_transfer_ctx.initialized = false;
}

int rendering_gpu_transfer_create(rendering_gpu_transfer_handle_t* out_handle,
                                  const rendering_gpu_transfer_desc_t* desc) {
    if (!out_handle) {
        return -1;
    }

    if (!g_gpu_transfer_ctx.initialized) {
        return -2;
    }

    if (g_gpu_transfer_ctx.count >= g_gpu_transfer_ctx.capacity) {
        uint32_t new_capacity = g_gpu_transfer_ctx.capacity * 2;
        rendering_gpu_transfer_internal_t* new_items = realloc(g_gpu_transfer_ctx.items,
                                                                new_capacity * sizeof(rendering_gpu_transfer_internal_t));
        if (!new_items) {
            return -3;
        }
        g_gpu_transfer_ctx.items = new_items;
        g_gpu_transfer_ctx.capacity = new_capacity;
    }

    uint32_t index = g_gpu_transfer_ctx.count++;
    rendering_gpu_transfer_internal_t* transfer = &g_gpu_transfer_ctx.items[index];

    // Determine staging buffer size
    size_t staging_size = DEFAULT_STAGING_BUFFER_SIZE;
    if (desc && desc->staging_buffer_size > 0) {
        staging_size = desc->staging_buffer_size;
    }

    // Allocate staging buffer
    transfer->staging_buffer = malloc(staging_size);
    if (!transfer->staging_buffer) {
        return -4;
    }

    transfer->id = index;
    transfer->flags = desc ? desc->flags : 0;
    transfer->staging_buffer_size = staging_size;
    transfer->staging_offset = 0;
    transfer->pending_transfers = 0;
    transfer->use_async_transfer = desc ? desc->use_async_transfer : true;
    transfer->initialized = true;

    out_handle->id = index;
    return 0;
}

void rendering_gpu_transfer_destroy(rendering_gpu_transfer_handle_t handle) {
    if (handle.id >= g_gpu_transfer_ctx.count) {
        return;
    }

    gpu_transfer_cleanup_internal(&g_gpu_transfer_ctx.items[handle.id]);
}

int rendering_gpu_transfer_upload_instances(
    rendering_gpu_transfer_handle_t handle,
    rendering_gpu_scene_handle_t scene,
    const GPUInstanceData* instances,
    uint32_t instance_count,
    uint32_t start_offset) {

    if (!instances || instance_count == 0 || handle.id >= g_gpu_transfer_ctx.count) {
        return -1;
    }

    rendering_gpu_transfer_internal_t* transfer = &g_gpu_transfer_ctx.items[handle.id];
    if (!gpu_transfer_validate(transfer)) {
        return -2;
    }

    // Validate scene handle
    if (!rendering_gpu_scene_is_valid(scene)) {
        return -3;
    }

    size_t data_size = instance_count * sizeof(GPUInstanceData);

    // Check if data fits in staging buffer
    if (data_size > transfer->staging_buffer_size) {
        return -4;  // Data too large for staging buffer
    }

    // Copy to staging buffer
    memcpy(transfer->staging_buffer, instances, data_size);

    // TODO: Implement actual GPU upload using Metal backend
    // This would be:
    // 1. Get GPU scene's instance buffer
    // 2. Use blit encoder to copy from staging buffer to GPU buffer
    // 3. Or use direct copy if using shared memory buffers
    // 4. Track as pending transfer if async

    transfer->pending_transfers++;

    return 0;
}

int rendering_gpu_transfer_upload_instances_dirty(
    rendering_gpu_transfer_handle_t handle,
    rendering_gpu_scene_handle_t scene,
    uint32_t start_index,
    uint32_t count) {

    if (count == 0 || handle.id >= g_gpu_transfer_ctx.count) {
        return -1;
    }

    rendering_gpu_transfer_internal_t* transfer = &g_gpu_transfer_ctx.items[handle.id];
    if (!gpu_transfer_validate(transfer)) {
        return -2;
    }

    if (!rendering_gpu_scene_is_valid(scene)) {
        return -3;
    }

    // Get instance data from GPU scene
    const GPUInstanceData* instance_data = (const GPUInstanceData*)
        rendering_gpu_scene_get_instance_buffer(scene);

    if (!instance_data) {
        return -4;
    }

    // Upload dirty region
    return rendering_gpu_transfer_upload_instances(
        handle, scene,
        &instance_data[start_index],
        count,
        start_index);
}

int rendering_gpu_transfer_upload_materials(
    rendering_gpu_transfer_handle_t handle,
    rendering_gpu_scene_handle_t scene,
    const GPUMaterialData* materials,
    uint32_t material_count,
    uint32_t start_offset) {

    if (!materials || material_count == 0 || handle.id >= g_gpu_transfer_ctx.count) {
        return -1;
    }

    rendering_gpu_transfer_internal_t* transfer = &g_gpu_transfer_ctx.items[handle.id];
    if (!gpu_transfer_validate(transfer)) {
        return -2;
    }

    if (!rendering_gpu_scene_is_valid(scene)) {
        return -3;
    }

    size_t data_size = material_count * sizeof(GPUMaterialData);

    if (data_size > transfer->staging_buffer_size) {
        return -4;  // Data too large
    }

    // Copy to staging buffer
    memcpy(transfer->staging_buffer, materials, data_size);

    // TODO: Implement actual GPU upload
    transfer->pending_transfers++;

    return 0;
}

int rendering_gpu_transfer_upload_indirect_args(
    rendering_gpu_transfer_handle_t handle,
    rendering_gpu_scene_handle_t scene,
    const IndirectDrawArgs* args,
    uint32_t arg_count) {

    if (!args || arg_count == 0 || handle.id >= g_gpu_transfer_ctx.count) {
        return -1;
    }

    rendering_gpu_transfer_internal_t* transfer = &g_gpu_transfer_ctx.items[handle.id];
    if (!gpu_transfer_validate(transfer)) {
        return -2;
    }

    if (!rendering_gpu_scene_is_valid(scene)) {
        return -3;
    }

    size_t data_size = arg_count * sizeof(IndirectDrawArgs);

    if (data_size > transfer->staging_buffer_size) {
        return -4;  // Data too large
    }

    // Copy to staging buffer
    memcpy(transfer->staging_buffer, args, data_size);

    // TODO: Implement actual GPU upload
    transfer->pending_transfers++;

    return 0;
}

int rendering_gpu_transfer_readback_culling_results(
    rendering_gpu_transfer_handle_t handle,
    rendering_gpu_scene_handle_t scene,
    CullingResult* out_results,
    uint32_t* out_count) {

    if (!out_results || !out_count || handle.id >= g_gpu_transfer_ctx.count) {
        return -1;
    }

    rendering_gpu_transfer_internal_t* transfer = &g_gpu_transfer_ctx.items[handle.id];
    if (!gpu_transfer_validate(transfer)) {
        return -2;
    }

    if (!rendering_gpu_scene_is_valid(scene)) {
        return -3;
    }

    // Get culling results buffer from scene
    const CullingResult* culling_data = (const CullingResult*)
        rendering_gpu_scene_get_culling_buffer(scene);

    if (!culling_data) {
        return -4;
    }

    // TODO: Implement actual GPU readback/synchronization
    // For now, just return that no results are ready
    *out_count = 0;

    return 0;
}

int rendering_gpu_transfer_wait_for_transfers(rendering_gpu_transfer_handle_t handle) {
    if (handle.id >= g_gpu_transfer_ctx.count) {
        return -1;
    }

    rendering_gpu_transfer_internal_t* transfer = &g_gpu_transfer_ctx.items[handle.id];
    if (!gpu_transfer_validate(transfer)) {
        return -2;
    }

    // TODO: Implement actual GPU synchronization
    // This would wait for all pending transfers to complete

    transfer->pending_transfers = 0;

    return 0;
}

/* End of gpu_data_transfer.c */
