/*
 * postprocessing_ssr_renderer_03.h
 *
 * Header file for postprocessing_ssr_renderer_03 implementation
 * Part of the Advanced 3D Rendering Engine
 */

#ifndef POSTPROCESSING_SSR_RENDERER_03_H
#define POSTPROCESSING_SSR_RENDERER_03_H

#include <stdint.h>
#include <stdbool.h>
#include <stddef.h>

#ifdef __cplusplus
extern "C" {
#endif

/* Forward declarations */
typedef struct postprocessing_ssr_renderer_03 postprocessing_ssr_renderer_03_t;
typedef struct postprocessing_ssr_renderer_03_desc postprocessing_ssr_renderer_03_desc_t;
typedef struct postprocessing_ssr_renderer_03_stats postprocessing_ssr_renderer_03_stats_t;

/* Creation and destruction */
int postprocessing_ssr_renderer_03_create(postprocessing_ssr_renderer_03_t** out_ctx, const postprocessing_ssr_renderer_03_desc_t* desc);
int postprocessing_ssr_renderer_03_destroy(postprocessing_ssr_renderer_03_t* ctx);

/* Core operations */
int postprocessing_ssr_renderer_03_render(postprocessing_ssr_renderer_03_t* ctx, void* params);
int postprocessing_ssr_renderer_03_prepare(postprocessing_ssr_renderer_03_t* ctx, void* params);
int postprocessing_ssr_renderer_03_bind(postprocessing_ssr_renderer_03_t* ctx, void* params);
int postprocessing_ssr_renderer_03_draw(postprocessing_ssr_renderer_03_t* ctx, void* params);
int postprocessing_ssr_renderer_03_dispatch(postprocessing_ssr_renderer_03_t* ctx, void* params);
int postprocessing_ssr_renderer_03_submit_commands(postprocessing_ssr_renderer_03_t* ctx, void* params);
int postprocessing_ssr_renderer_03_build_commands(postprocessing_ssr_renderer_03_t* ctx, void* params);
int postprocessing_ssr_renderer_03_sort(postprocessing_ssr_renderer_03_t* ctx, void* params);
int postprocessing_ssr_renderer_03_batch(postprocessing_ssr_renderer_03_t* ctx, void* params);
int postprocessing_ssr_renderer_03_cull(postprocessing_ssr_renderer_03_t* ctx, void* params);

/* Utility functions */
int postprocessing_ssr_renderer_03_get_stats(postprocessing_ssr_renderer_03_t* ctx);
int postprocessing_ssr_renderer_03_set_callback(postprocessing_ssr_renderer_03_t* ctx);
int postprocessing_ssr_renderer_03_get_memory_usage(postprocessing_ssr_renderer_03_t* ctx);
int postprocessing_ssr_renderer_03_optimize(postprocessing_ssr_renderer_03_t* ctx);
int postprocessing_ssr_renderer_03_debug_print(postprocessing_ssr_renderer_03_t* ctx);

/* Module functions */
int postprocessing_ssr_renderer_03_module_init(void);
int postprocessing_ssr_renderer_03_module_shutdown(void);

#ifdef __cplusplus
}
#endif

#endif /* POSTPROCESSING_SSR_RENDERER_03_H */
