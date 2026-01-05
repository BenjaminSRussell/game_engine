/*
 * lighting_voxel_gi_renderer_03.h
 *
 * Header file for lighting_voxel_gi_renderer_03 implementation
 * Part of the Advanced 3D Rendering Engine
 */

#ifndef LIGHTING_VOXEL_GI_RENDERER_03_H
#define LIGHTING_VOXEL_GI_RENDERER_03_H

#include <stdint.h>
#include <stdbool.h>
#include <stddef.h>

#ifdef __cplusplus
extern "C" {
#endif

/* Forward declarations */
typedef struct lighting_voxel_gi_renderer_03 lighting_voxel_gi_renderer_03_t;
typedef struct lighting_voxel_gi_renderer_03_desc lighting_voxel_gi_renderer_03_desc_t;
typedef struct lighting_voxel_gi_renderer_03_stats lighting_voxel_gi_renderer_03_stats_t;

/* Creation and destruction */
int lighting_voxel_gi_renderer_03_create(lighting_voxel_gi_renderer_03_t** out_ctx, const lighting_voxel_gi_renderer_03_desc_t* desc);
int lighting_voxel_gi_renderer_03_destroy(lighting_voxel_gi_renderer_03_t* ctx);

/* Core operations */
int lighting_voxel_gi_renderer_03_render(lighting_voxel_gi_renderer_03_t* ctx, void* params);
int lighting_voxel_gi_renderer_03_prepare(lighting_voxel_gi_renderer_03_t* ctx, void* params);
int lighting_voxel_gi_renderer_03_bind(lighting_voxel_gi_renderer_03_t* ctx, void* params);
int lighting_voxel_gi_renderer_03_draw(lighting_voxel_gi_renderer_03_t* ctx, void* params);
int lighting_voxel_gi_renderer_03_dispatch(lighting_voxel_gi_renderer_03_t* ctx, void* params);
int lighting_voxel_gi_renderer_03_submit_commands(lighting_voxel_gi_renderer_03_t* ctx, void* params);
int lighting_voxel_gi_renderer_03_build_commands(lighting_voxel_gi_renderer_03_t* ctx, void* params);
int lighting_voxel_gi_renderer_03_sort(lighting_voxel_gi_renderer_03_t* ctx, void* params);
int lighting_voxel_gi_renderer_03_batch(lighting_voxel_gi_renderer_03_t* ctx, void* params);
int lighting_voxel_gi_renderer_03_cull(lighting_voxel_gi_renderer_03_t* ctx, void* params);

/* Utility functions */
int lighting_voxel_gi_renderer_03_get_stats(lighting_voxel_gi_renderer_03_t* ctx);
int lighting_voxel_gi_renderer_03_set_callback(lighting_voxel_gi_renderer_03_t* ctx);
int lighting_voxel_gi_renderer_03_get_memory_usage(lighting_voxel_gi_renderer_03_t* ctx);
int lighting_voxel_gi_renderer_03_optimize(lighting_voxel_gi_renderer_03_t* ctx);
int lighting_voxel_gi_renderer_03_debug_print(lighting_voxel_gi_renderer_03_t* ctx);

/* Module functions */
int lighting_voxel_gi_renderer_03_module_init(void);
int lighting_voxel_gi_renderer_03_module_shutdown(void);

#ifdef __cplusplus
}
#endif

#endif /* LIGHTING_VOXEL_GI_RENDERER_03_H */
