/*
 * lod_gpu_selection.h
 * GPU-driven LOD selection using compute shaders
 *
 * Part of the Geometry subsystem
 * Advanced 3D Rendering Engine
 */

#ifndef GEOMETRY_LOD_GPU_SELECTION_H
#define GEOMETRY_LOD_GPU_SELECTION_H

#include <stdint.h>
#include <stdbool.h>
#include <stddef.h>
#include "geometry/lod/lod_selector.h"

#ifdef __cplusplus
extern "C" {
#endif

/* ============================================================================
 * TYPES
 * ============================================================================ */

// Per-instance data for GPU LOD selection
typedef struct lod_instance_data {
    float position[3];              // World position
    float radius;                   // Bounding sphere radius
    uint32_t chain_id;              // LOD chain ID
    float last_distance;            // Last frame distance
    uint32_t current_lod;           // Current LOD level
    uint32_t _padding;              // Align to 16 bytes
} lod_instance_data_t;

// GPU LOD selection result
typedef struct lod_gpu_result {
    uint32_t selected_lod;
    float distance;
    float screen_coverage;
    uint32_t _padding;
} lod_gpu_result_t;

// GPU selection configuration
typedef struct lod_gpu_config {
    bool enable_gpu_selection;
    bool use_async_readback;
    uint32_t max_instances_per_dispatch;
    uint32_t readback_latency_frames;  // Frames to wait for readback
} lod_gpu_config_t;

/* ============================================================================
 * API
 * ============================================================================ */

/* Initialization */
int lod_gpu_selection_init(const lod_gpu_config_t* config);
void lod_gpu_selection_shutdown(void);

/* Instance Management */
int lod_gpu_add_instance(const lod_instance_data_t* instance);
void lod_gpu_remove_instance(uint32_t instance_id);
void lod_gpu_update_instance(uint32_t instance_id, const lod_instance_data_t* instance);

/* Selection */
int lod_gpu_select_lods(
    const lod_camera_t* camera,
    uint32_t instance_count,
    lod_gpu_result_t* results
);

/* Results */
int lod_gpu_get_results(lod_gpu_result_t* results, uint32_t max_count);
bool lod_gpu_results_ready(void);

/* Fallback */
void lod_gpu_set_fallback_enabled(bool enabled);
bool lod_gpu_is_supported(void);

#ifdef __cplusplus
}
#endif

#endif /* GEOMETRY_LOD_GPU_SELECTION_H */
