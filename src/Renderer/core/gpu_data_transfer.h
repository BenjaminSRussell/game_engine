#ifndef RENDERING_GPU_DATA_TRANSFER_H
#define RENDERING_GPU_DATA_TRANSFER_H

#include <stdint.h>
#include <stdbool.h>
#include "gpu_scene.h"
#include "gpu_types.h"

#ifdef __cplusplus
extern "C" {
#endif

/* ============================================================================
 * GPU DATA TRANSFER
 * ============================================================================
 * Handles uploading CPU instance/material data to GPU buffers using
 * Metal's async transfer and staging buffer patterns
 * ============================================================================ */

typedef struct rendering_gpu_transfer_desc {
    uint32_t staging_buffer_size;
    bool use_async_transfer;
    uint32_t flags;
} rendering_gpu_transfer_desc_t;

typedef struct rendering_gpu_transfer_handle {
    uint32_t id;
} rendering_gpu_transfer_handle_t;

/* Initialization */
int rendering_gpu_transfer_init(void);
void rendering_gpu_transfer_shutdown(void);

/* Lifecycle */
int rendering_gpu_transfer_create(rendering_gpu_transfer_handle_t* out_handle,
                                  const rendering_gpu_transfer_desc_t* desc);
void rendering_gpu_transfer_destroy(rendering_gpu_transfer_handle_t handle);

/* Instance Data Transfer */
int rendering_gpu_transfer_upload_instances(
    rendering_gpu_transfer_handle_t handle,
    rendering_gpu_scene_handle_t scene,
    const GPUInstanceData* instances,
    uint32_t instance_count,
    uint32_t start_offset);

int rendering_gpu_transfer_upload_instances_dirty(
    rendering_gpu_transfer_handle_t handle,
    rendering_gpu_scene_handle_t scene,
    uint32_t start_index,
    uint32_t count);

/* Material Data Transfer */
int rendering_gpu_transfer_upload_materials(
    rendering_gpu_transfer_handle_t handle,
    rendering_gpu_scene_handle_t scene,
    const GPUMaterialData* materials,
    uint32_t material_count,
    uint32_t start_offset);

/* Indirect Args Transfer */
int rendering_gpu_transfer_upload_indirect_args(
    rendering_gpu_transfer_handle_t handle,
    rendering_gpu_scene_handle_t scene,
    const IndirectDrawArgs* args,
    uint32_t arg_count);

/* Culling Results Transfer (GPU->CPU readback) */
int rendering_gpu_transfer_readback_culling_results(
    rendering_gpu_transfer_handle_t handle,
    rendering_gpu_scene_handle_t scene,
    CullingResult* out_results,
    uint32_t* out_count);

/* Synchronization */
int rendering_gpu_transfer_wait_for_transfers(rendering_gpu_transfer_handle_t handle);

#ifdef __cplusplus
}
#endif

#endif /* RENDERING_GPU_DATA_TRANSFER_H */
