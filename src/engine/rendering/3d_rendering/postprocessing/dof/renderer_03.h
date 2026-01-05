/*
 * postprocessing_dof_renderer_03.h
 *
 * Header file for postprocessing_dof_renderer_03 implementation
 * Part of the Advanced 3D Rendering Engine
 */

#ifndef POSTPROCESSING_DOF_RENDERER_03_H
#define POSTPROCESSING_DOF_RENDERER_03_H

#include <stdint.h>
#include <stdbool.h>
#include <stddef.h>

#ifdef __cplusplus
extern "C" {
#endif

/* Forward declarations */
typedef struct postprocessing_dof_renderer_03 postprocessing_dof_renderer_03_t;
typedef struct postprocessing_dof_renderer_03_desc postprocessing_dof_renderer_03_desc_t;
typedef struct postprocessing_dof_renderer_03_stats postprocessing_dof_renderer_03_stats_t;

/* Creation and destruction */
int postprocessing_dof_renderer_03_create(postprocessing_dof_renderer_03_t** out_ctx, const postprocessing_dof_renderer_03_desc_t* desc);
int postprocessing_dof_renderer_03_destroy(postprocessing_dof_renderer_03_t* ctx);

/* Core operations */
int postprocessing_dof_renderer_03_render(postprocessing_dof_renderer_03_t* ctx, void* params);
int postprocessing_dof_renderer_03_prepare(postprocessing_dof_renderer_03_t* ctx, void* params);
int postprocessing_dof_renderer_03_bind(postprocessing_dof_renderer_03_t* ctx, void* params);
int postprocessing_dof_renderer_03_draw(postprocessing_dof_renderer_03_t* ctx, void* params);
int postprocessing_dof_renderer_03_dispatch(postprocessing_dof_renderer_03_t* ctx, void* params);
int postprocessing_dof_renderer_03_submit_commands(postprocessing_dof_renderer_03_t* ctx, void* params);
int postprocessing_dof_renderer_03_build_commands(postprocessing_dof_renderer_03_t* ctx, void* params);
int postprocessing_dof_renderer_03_sort(postprocessing_dof_renderer_03_t* ctx, void* params);
int postprocessing_dof_renderer_03_batch(postprocessing_dof_renderer_03_t* ctx, void* params);
int postprocessing_dof_renderer_03_cull(postprocessing_dof_renderer_03_t* ctx, void* params);

/* Utility functions */
int postprocessing_dof_renderer_03_get_stats(postprocessing_dof_renderer_03_t* ctx);
int postprocessing_dof_renderer_03_set_callback(postprocessing_dof_renderer_03_t* ctx);
int postprocessing_dof_renderer_03_get_memory_usage(postprocessing_dof_renderer_03_t* ctx);
int postprocessing_dof_renderer_03_optimize(postprocessing_dof_renderer_03_t* ctx);
int postprocessing_dof_renderer_03_debug_print(postprocessing_dof_renderer_03_t* ctx);

/* Module functions */
int postprocessing_dof_renderer_03_module_init(void);
int postprocessing_dof_renderer_03_module_shutdown(void);

#ifdef __cplusplus
}
#endif

#endif /* POSTPROCESSING_DOF_RENDERER_03_H */
