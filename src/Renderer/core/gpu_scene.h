#ifndef RENDERING_GPU_SCENE_H
#define RENDERING_GPU_SCENE_H

#include <stdint.h>
#include <stdbool.h>
#include <stddef.h>
#include "gpu_types.h"

#ifdef __cplusplus
extern "C" {
#endif

/* ============================================================================
 * TYPES
 * ============================================================================ */

typedef struct rendering_gpu_scene_handle {
    uint32_t id;
} rendering_gpu_scene_handle_t;

typedef struct rendering_gpu_scene_desc {
    uint32_t max_instances;
    uint32_t max_materials;
    uint32_t max_draw_calls;
    uint32_t flags;
} rendering_gpu_scene_desc_t;

typedef struct rendering_gpu_scene_stats {
    uint32_t total_instances;
    uint32_t visible_instances;
    uint32_t draw_call_count;
    uint32_t batch_count;
    size_t gpu_memory_used;
    uint32_t frame_number;
} rendering_gpu_scene_stats_t;

/* ============================================================================
 * API
 * ============================================================================ */

/* Initialization */
int rendering_gpu_scene_init(void);
void rendering_gpu_scene_shutdown(void);

/* Lifecycle */
int rendering_gpu_scene_create(rendering_gpu_scene_handle_t* out_handle,
                               const rendering_gpu_scene_desc_t* desc);
void rendering_gpu_scene_destroy(rendering_gpu_scene_handle_t handle);

/* Instance Management */
int rendering_gpu_scene_add_instance(rendering_gpu_scene_handle_t handle,
                                     uint32_t instance_id,
                                     const GPUInstanceData* instance_data);

int rendering_gpu_scene_update_instance(rendering_gpu_scene_handle_t handle,
                                        uint32_t instance_id,
                                        const GPUInstanceData* instance_data);

int rendering_gpu_scene_remove_instance(rendering_gpu_scene_handle_t handle,
                                        uint32_t instance_id);

/* Batch Management */
int rendering_gpu_scene_set_instances(rendering_gpu_scene_handle_t handle,
                                      const GPUInstanceData* instances,
                                      uint32_t count);

/* Material Management */
int rendering_gpu_scene_set_material(rendering_gpu_scene_handle_t handle,
                                     uint32_t material_id,
                                     const GPUMaterialData* material_data);

int rendering_gpu_scene_set_materials(rendering_gpu_scene_handle_t handle,
                                      const GPUMaterialData* materials,
                                      uint32_t count);

/* GPU Buffer Access */
void* rendering_gpu_scene_get_instance_buffer(rendering_gpu_scene_handle_t handle);
void* rendering_gpu_scene_get_material_buffer(rendering_gpu_scene_handle_t handle);
void* rendering_gpu_scene_get_indirect_buffer(rendering_gpu_scene_handle_t handle);
void* rendering_gpu_scene_get_culling_buffer(rendering_gpu_scene_handle_t handle);

/* Dirty Tracking */
void rendering_gpu_scene_mark_instances_dirty(rendering_gpu_scene_handle_t handle,
                                              uint32_t start_index,
                                              uint32_t count);

void rendering_gpu_scene_mark_materials_dirty(rendering_gpu_scene_handle_t handle);

/* GPU Upload */
int rendering_gpu_scene_upload_dirty_data(rendering_gpu_scene_handle_t handle);

/* Statistics */
int rendering_gpu_scene_get_stats(rendering_gpu_scene_handle_t handle,
                                  rendering_gpu_scene_stats_t* out_stats);

bool rendering_gpu_scene_is_valid(rendering_gpu_scene_handle_t handle);

/* Memory */
size_t rendering_gpu_scene_get_memory_usage(void);

#ifdef __cplusplus
}
#endif

#endif /* RENDERING_GPU_SCENE_H */
