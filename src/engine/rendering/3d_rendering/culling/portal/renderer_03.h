/*
 * culling_portal_renderer_03.h
 *
 * Header file for culling_portal_renderer_03 implementation
 * Part of the Advanced 3D Rendering Engine
 */

#ifndef CULLING_PORTAL_RENDERER_03_H
#define CULLING_PORTAL_RENDERER_03_H

#include <stdint.h>
#include <stdbool.h>
#include <stddef.h>

#ifdef __cplusplus
extern "C" {
#endif

/* Forward declarations */
typedef struct culling_portal_renderer_03 culling_portal_renderer_03_t;
typedef struct culling_portal_renderer_03_desc culling_portal_renderer_03_desc_t;
typedef struct culling_portal_renderer_03_stats culling_portal_renderer_03_stats_t;

/* Creation and destruction */
int culling_portal_renderer_03_create(culling_portal_renderer_03_t** out_ctx, const culling_portal_renderer_03_desc_t* desc);
int culling_portal_renderer_03_destroy(culling_portal_renderer_03_t* ctx);

/* Core operations */
int culling_portal_renderer_03_render(culling_portal_renderer_03_t* ctx, void* params);
int culling_portal_renderer_03_prepare(culling_portal_renderer_03_t* ctx, void* params);
int culling_portal_renderer_03_bind(culling_portal_renderer_03_t* ctx, void* params);
int culling_portal_renderer_03_draw(culling_portal_renderer_03_t* ctx, void* params);
int culling_portal_renderer_03_dispatch(culling_portal_renderer_03_t* ctx, void* params);
int culling_portal_renderer_03_submit_commands(culling_portal_renderer_03_t* ctx, void* params);
int culling_portal_renderer_03_build_commands(culling_portal_renderer_03_t* ctx, void* params);
int culling_portal_renderer_03_sort(culling_portal_renderer_03_t* ctx, void* params);
int culling_portal_renderer_03_batch(culling_portal_renderer_03_t* ctx, void* params);
int culling_portal_renderer_03_cull(culling_portal_renderer_03_t* ctx, void* params);

/* Utility functions */
int culling_portal_renderer_03_get_stats(culling_portal_renderer_03_t* ctx);
int culling_portal_renderer_03_set_callback(culling_portal_renderer_03_t* ctx);
int culling_portal_renderer_03_get_memory_usage(culling_portal_renderer_03_t* ctx);
int culling_portal_renderer_03_optimize(culling_portal_renderer_03_t* ctx);
int culling_portal_renderer_03_debug_print(culling_portal_renderer_03_t* ctx);

/* Module functions */
int culling_portal_renderer_03_module_init(void);
int culling_portal_renderer_03_module_shutdown(void);

#ifdef __cplusplus
}
#endif

#endif /* CULLING_PORTAL_RENDERER_03_H */
