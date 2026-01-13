/*
 * physics_solver_renderer_03.h
 *
 * Header file for physics_solver_renderer_03 implementation
 * Part of the Advanced 3D Rendering Engine
 */

#ifndef PHYSICS_SOLVER_RENDERER_03_H
#define PHYSICS_SOLVER_RENDERER_03_H

#include <stdint.h>
#include <stdbool.h>
#include <stddef.h>

#ifdef __cplusplus
extern "C" {
#endif

/* Forward declarations */
typedef struct physics_solver_renderer_03 physics_solver_renderer_03_t;
typedef struct physics_solver_renderer_03_desc physics_solver_renderer_03_desc_t;
typedef struct physics_solver_renderer_03_stats physics_solver_renderer_03_stats_t;

/* Creation and destruction */
int physics_solver_renderer_03_create(physics_solver_renderer_03_t** out_ctx, const physics_solver_renderer_03_desc_t* desc);
int physics_solver_renderer_03_destroy(physics_solver_renderer_03_t* ctx);

/* Core operations */
int physics_solver_renderer_03_render(physics_solver_renderer_03_t* ctx, void* params);
int physics_solver_renderer_03_prepare(physics_solver_renderer_03_t* ctx, void* params);
int physics_solver_renderer_03_bind(physics_solver_renderer_03_t* ctx, void* params);
int physics_solver_renderer_03_draw(physics_solver_renderer_03_t* ctx, void* params);
int physics_solver_renderer_03_dispatch(physics_solver_renderer_03_t* ctx, void* params);
int physics_solver_renderer_03_submit_commands(physics_solver_renderer_03_t* ctx, void* params);
int physics_solver_renderer_03_build_commands(physics_solver_renderer_03_t* ctx, void* params);
int physics_solver_renderer_03_sort(physics_solver_renderer_03_t* ctx, void* params);
int physics_solver_renderer_03_batch(physics_solver_renderer_03_t* ctx, void* params);
int physics_solver_renderer_03_cull(physics_solver_renderer_03_t* ctx, void* params);

/* Utility functions */
int physics_solver_renderer_03_get_stats(physics_solver_renderer_03_t* ctx);
int physics_solver_renderer_03_set_callback(physics_solver_renderer_03_t* ctx);
int physics_solver_renderer_03_get_memory_usage(physics_solver_renderer_03_t* ctx);
int physics_solver_renderer_03_optimize(physics_solver_renderer_03_t* ctx);
int physics_solver_renderer_03_debug_print(physics_solver_renderer_03_t* ctx);

/* Module functions */
int physics_solver_renderer_03_module_init(void);
int physics_solver_renderer_03_module_shutdown(void);

#ifdef __cplusplus
}
#endif

#endif /* PHYSICS_SOLVER_RENDERER_03_H */
