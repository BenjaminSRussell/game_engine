/*
 * postprocessing_ssr_builder_05.h
 *
 * Header file for postprocessing_ssr_builder_05 implementation
 * Part of the Advanced 3D Rendering Engine
 */

#ifndef POSTPROCESSING_SSR_BUILDER_05_H
#define POSTPROCESSING_SSR_BUILDER_05_H

#include <stdint.h>
#include <stdbool.h>
#include <stddef.h>

#ifdef __cplusplus
extern "C" {
#endif

/* Forward declarations */
typedef struct postprocessing_ssr_builder_05 postprocessing_ssr_builder_05_t;
typedef struct postprocessing_ssr_builder_05_desc postprocessing_ssr_builder_05_desc_t;
typedef struct postprocessing_ssr_builder_05_stats postprocessing_ssr_builder_05_stats_t;

/* Creation and destruction */
int postprocessing_ssr_builder_05_create(postprocessing_ssr_builder_05_t** out_ctx, const postprocessing_ssr_builder_05_desc_t* desc);
int postprocessing_ssr_builder_05_destroy(postprocessing_ssr_builder_05_t* ctx);

/* Core operations */
int postprocessing_ssr_builder_05_begin(postprocessing_ssr_builder_05_t* ctx, void* params);
int postprocessing_ssr_builder_05_end(postprocessing_ssr_builder_05_t* ctx, void* params);
int postprocessing_ssr_builder_05_add(postprocessing_ssr_builder_05_t* ctx, void* params);
int postprocessing_ssr_builder_05_remove(postprocessing_ssr_builder_05_t* ctx, void* params);
int postprocessing_ssr_builder_05_modify(postprocessing_ssr_builder_05_t* ctx, void* params);
int postprocessing_ssr_builder_05_finalize(postprocessing_ssr_builder_05_t* ctx, void* params);
int postprocessing_ssr_builder_05_validate(postprocessing_ssr_builder_05_t* ctx, void* params);
int postprocessing_ssr_builder_05_optimize(postprocessing_ssr_builder_05_t* ctx, void* params);
int postprocessing_ssr_builder_05_compile(postprocessing_ssr_builder_05_t* ctx, void* params);
int postprocessing_ssr_builder_05_link(postprocessing_ssr_builder_05_t* ctx, void* params);

/* Utility functions */
int postprocessing_ssr_builder_05_get_stats(postprocessing_ssr_builder_05_t* ctx);
int postprocessing_ssr_builder_05_set_callback(postprocessing_ssr_builder_05_t* ctx);
int postprocessing_ssr_builder_05_get_memory_usage(postprocessing_ssr_builder_05_t* ctx);
int postprocessing_ssr_builder_05_optimize(postprocessing_ssr_builder_05_t* ctx);
int postprocessing_ssr_builder_05_debug_print(postprocessing_ssr_builder_05_t* ctx);

/* Module functions */
int postprocessing_ssr_builder_05_module_init(void);
int postprocessing_ssr_builder_05_module_shutdown(void);

#ifdef __cplusplus
}
#endif

#endif /* POSTPROCESSING_SSR_BUILDER_05_H */
