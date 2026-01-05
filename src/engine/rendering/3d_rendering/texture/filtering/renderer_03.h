/*
 * texture_filtering_renderer_03.h
 *
 * Header file for texture_filtering_renderer_03 implementation
 * Part of the Advanced 3D Rendering Engine
 */

#ifndef TEXTURE_FILTERING_RENDERER_03_H
#define TEXTURE_FILTERING_RENDERER_03_H

#include <stdint.h>
#include <stdbool.h>
#include <stddef.h>

#ifdef __cplusplus
extern "C" {
#endif

/* Forward declarations */
typedef struct texture_filtering_renderer_03 texture_filtering_renderer_03_t;
typedef struct texture_filtering_renderer_03_desc texture_filtering_renderer_03_desc_t;
typedef struct texture_filtering_renderer_03_stats texture_filtering_renderer_03_stats_t;

/* Creation and destruction */
int texture_filtering_renderer_03_create(texture_filtering_renderer_03_t** out_ctx, const texture_filtering_renderer_03_desc_t* desc);
int texture_filtering_renderer_03_destroy(texture_filtering_renderer_03_t* ctx);

/* Core operations */
int texture_filtering_renderer_03_render(texture_filtering_renderer_03_t* ctx, void* params);
int texture_filtering_renderer_03_prepare(texture_filtering_renderer_03_t* ctx, void* params);
int texture_filtering_renderer_03_bind(texture_filtering_renderer_03_t* ctx, void* params);
int texture_filtering_renderer_03_draw(texture_filtering_renderer_03_t* ctx, void* params);
int texture_filtering_renderer_03_dispatch(texture_filtering_renderer_03_t* ctx, void* params);
int texture_filtering_renderer_03_submit_commands(texture_filtering_renderer_03_t* ctx, void* params);
int texture_filtering_renderer_03_build_commands(texture_filtering_renderer_03_t* ctx, void* params);
int texture_filtering_renderer_03_sort(texture_filtering_renderer_03_t* ctx, void* params);
int texture_filtering_renderer_03_batch(texture_filtering_renderer_03_t* ctx, void* params);
int texture_filtering_renderer_03_cull(texture_filtering_renderer_03_t* ctx, void* params);

/* Utility functions */
int texture_filtering_renderer_03_get_stats(texture_filtering_renderer_03_t* ctx);
int texture_filtering_renderer_03_set_callback(texture_filtering_renderer_03_t* ctx);
int texture_filtering_renderer_03_get_memory_usage(texture_filtering_renderer_03_t* ctx);
int texture_filtering_renderer_03_optimize(texture_filtering_renderer_03_t* ctx);
int texture_filtering_renderer_03_debug_print(texture_filtering_renderer_03_t* ctx);

/* Module functions */
int texture_filtering_renderer_03_module_init(void);
int texture_filtering_renderer_03_module_shutdown(void);

#ifdef __cplusplus
}
#endif

#endif /* TEXTURE_FILTERING_RENDERER_03_H */
