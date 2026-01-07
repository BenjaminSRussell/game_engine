/*
 * debugging_assertions_renderer_03.h
 *
 * Header file for debugging_assertions_renderer_03 implementation
 * Part of the Advanced 3D Rendering Engine
 */

#ifndef DEBUGGING_ASSERTIONS_RENDERER_03_H
#define DEBUGGING_ASSERTIONS_RENDERER_03_H

#include <stdint.h>
#include <stdbool.h>
#include <stddef.h>

#ifdef __cplusplus
extern "C" {
#endif

/* Forward declarations */
typedef struct debugging_assertions_renderer_03 debugging_assertions_renderer_03_t;
typedef struct debugging_assertions_renderer_03_desc debugging_assertions_renderer_03_desc_t;
typedef struct debugging_assertions_renderer_03_stats debugging_assertions_renderer_03_stats_t;

/* Creation and destruction */
int debugging_assertions_renderer_03_create(debugging_assertions_renderer_03_t** out_ctx, const debugging_assertions_renderer_03_desc_t* desc);
int debugging_assertions_renderer_03_destroy(debugging_assertions_renderer_03_t* ctx);

/* Core operations */
int debugging_assertions_renderer_03_render(debugging_assertions_renderer_03_t* ctx, void* params);
int debugging_assertions_renderer_03_prepare(debugging_assertions_renderer_03_t* ctx, void* params);
int debugging_assertions_renderer_03_bind(debugging_assertions_renderer_03_t* ctx, void* params);
int debugging_assertions_renderer_03_draw(debugging_assertions_renderer_03_t* ctx, void* params);
int debugging_assertions_renderer_03_dispatch(debugging_assertions_renderer_03_t* ctx, void* params);
int debugging_assertions_renderer_03_submit_commands(debugging_assertions_renderer_03_t* ctx, void* params);
int debugging_assertions_renderer_03_build_commands(debugging_assertions_renderer_03_t* ctx, void* params);
int debugging_assertions_renderer_03_sort(debugging_assertions_renderer_03_t* ctx, void* params);
int debugging_assertions_renderer_03_batch(debugging_assertions_renderer_03_t* ctx, void* params);
int debugging_assertions_renderer_03_cull(debugging_assertions_renderer_03_t* ctx, void* params);

/* Utility functions */
int debugging_assertions_renderer_03_get_stats(debugging_assertions_renderer_03_t* ctx);
int debugging_assertions_renderer_03_set_callback(debugging_assertions_renderer_03_t* ctx);
int debugging_assertions_renderer_03_get_memory_usage(debugging_assertions_renderer_03_t* ctx);
int debugging_assertions_renderer_03_optimize(debugging_assertions_renderer_03_t* ctx);
int debugging_assertions_renderer_03_debug_print(debugging_assertions_renderer_03_t* ctx);

/* Module functions */
int debugging_assertions_renderer_03_module_init(void);
int debugging_assertions_renderer_03_module_shutdown(void);

#ifdef __cplusplus
}
#endif

#endif /* DEBUGGING_ASSERTIONS_RENDERER_03_H */
