/*
 * physics_rigid_renderer_03.h
 *
 * Header file for physics_rigid_renderer_03 implementation
 * Part of the Advanced 3D Rendering Engine
 */

#ifndef PHYSICS_RIGID_RENDERER_03_H
#define PHYSICS_RIGID_RENDERER_03_H

#include <stdint.h>
#include <stdbool.h>
#include <stddef.h>

#ifdef __cplusplus
extern "C" {
#endif

/* Forward declarations */
typedef struct physics_rigid_renderer_03 physics_rigid_renderer_03_t;
typedef struct physics_rigid_renderer_03_desc physics_rigid_renderer_03_desc_t;
typedef struct physics_rigid_renderer_03_stats physics_rigid_renderer_03_stats_t;

/* Creation and destruction */
int physics_rigid_renderer_03_create(physics_rigid_renderer_03_t** out_ctx, const physics_rigid_renderer_03_desc_t* desc);
int physics_rigid_renderer_03_destroy(physics_rigid_renderer_03_t* ctx);

/* Core operations */
int physics_rigid_renderer_03_render(physics_rigid_renderer_03_t* ctx, void* params);
int physics_rigid_renderer_03_prepare(physics_rigid_renderer_03_t* ctx, void* params);
int physics_rigid_renderer_03_bind(physics_rigid_renderer_03_t* ctx, void* params);
int physics_rigid_renderer_03_draw(physics_rigid_renderer_03_t* ctx, void* params);
int physics_rigid_renderer_03_dispatch(physics_rigid_renderer_03_t* ctx, void* params);
int physics_rigid_renderer_03_submit_commands(physics_rigid_renderer_03_t* ctx, void* params);
int physics_rigid_renderer_03_build_commands(physics_rigid_renderer_03_t* ctx, void* params);
int physics_rigid_renderer_03_sort(physics_rigid_renderer_03_t* ctx, void* params);
int physics_rigid_renderer_03_batch(physics_rigid_renderer_03_t* ctx, void* params);
int physics_rigid_renderer_03_cull(physics_rigid_renderer_03_t* ctx, void* params);

/* Utility functions */
int physics_rigid_renderer_03_get_stats(physics_rigid_renderer_03_t* ctx);
int physics_rigid_renderer_03_set_callback(physics_rigid_renderer_03_t* ctx);
int physics_rigid_renderer_03_get_memory_usage(physics_rigid_renderer_03_t* ctx);
int physics_rigid_renderer_03_optimize(physics_rigid_renderer_03_t* ctx);
int physics_rigid_renderer_03_debug_print(physics_rigid_renderer_03_t* ctx);

/* Module functions */
int physics_rigid_renderer_03_module_init(void);
int physics_rigid_renderer_03_module_shutdown(void);

#ifdef __cplusplus
}
#endif

#endif /* PHYSICS_RIGID_RENDERER_03_H */
