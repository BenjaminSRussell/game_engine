/*
 * bvh_metal_culling.h
 * Metal compute culling integration
 *
 * Part of the Geometry subsystem
 * Advanced 3D Rendering Engine
 */

#ifndef GEOMETRY_BVH_METAL_CULLING_H
#define GEOMETRY_BVH_METAL_CULLING_H

#include <stdint.h>
#include <stdbool.h>
#include "../../core/math/math/vec4.h"

#ifdef __cplusplus
extern "C" {
#endif

/* ============================================================================
 * TYPES
 * ============================================================================ */

typedef struct bvh_metal_context bvh_metal_context_t;

typedef struct bvh_metal_stats {
    uint64_t gpu_cull_time_ns;
    uint32_t objects_processed;
    uint32_t objects_visible;
} bvh_metal_stats_t;

/* ============================================================================
 * API
 * ============================================================================ */

/* Initialize Metal culling subsystem */
bvh_metal_context_t* bvh_metal_init(void* metal_device);

/* Shutdown and free resources */
void bvh_metal_shutdown(bvh_metal_context_t* ctx);

/* Upload object data to GPU */
/* data is array of ObjectInstance structs matching .metal definition */
void bvh_metal_upload_objects(bvh_metal_context_t* ctx, const void* data, uint32_t count);

/* Dispatch culling compute shader */
/* Returns ID of buffer containing indirect draw commands */
void bvh_metal_dispatch_cull(
    bvh_metal_context_t* ctx,
    const vec4_t frustum_planes[6],
    void* out_visible_count_buffer_handle,  /* id<MTLBuffer> */
    void* out_indirect_command_buffer_handle /* id<MTLBuffer> */
);

/* Get performance statistics */
void bvh_metal_get_stats(bvh_metal_context_t* ctx, bvh_metal_stats_t* out_stats);

#ifdef __cplusplus
}
#endif

#endif /* GEOMETRY_BVH_METAL_CULLING_H */
