/*
 * postprocessing_ssr_manager_01.h
 *
 * Header file for postprocessing_ssr_manager_01 implementation
 * Part of the Advanced 3D Rendering Engine
 */

#ifndef POSTPROCESSING_SSR_MANAGER_01_H
#define POSTPROCESSING_SSR_MANAGER_01_H

#include <stdint.h>
#include <stdbool.h>
#include <stddef.h>

#ifdef __cplusplus
extern "C" {
#endif

/* Forward declarations */
typedef struct postprocessing_ssr_manager_01 postprocessing_ssr_manager_01_t;
typedef struct postprocessing_ssr_manager_01_desc postprocessing_ssr_manager_01_desc_t;
typedef struct postprocessing_ssr_manager_01_stats postprocessing_ssr_manager_01_stats_t;

/* Creation and destruction */
int postprocessing_ssr_manager_01_create(postprocessing_ssr_manager_01_t** out_ctx, const postprocessing_ssr_manager_01_desc_t* desc);
int postprocessing_ssr_manager_01_destroy(postprocessing_ssr_manager_01_t* ctx);

/* Core operations */
int postprocessing_ssr_manager_01_init(postprocessing_ssr_manager_01_t* ctx, void* params);
int postprocessing_ssr_manager_01_shutdown(postprocessing_ssr_manager_01_t* ctx, void* params);
int postprocessing_ssr_manager_01_update(postprocessing_ssr_manager_01_t* ctx, void* params);
int postprocessing_ssr_manager_01_create(postprocessing_ssr_manager_01_t* ctx, void* params);
int postprocessing_ssr_manager_01_destroy(postprocessing_ssr_manager_01_t* ctx, void* params);
int postprocessing_ssr_manager_01_get(postprocessing_ssr_manager_01_t* ctx, void* params);
int postprocessing_ssr_manager_01_set(postprocessing_ssr_manager_01_t* ctx, void* params);
int postprocessing_ssr_manager_01_reset(postprocessing_ssr_manager_01_t* ctx, void* params);
int postprocessing_ssr_manager_01_validate(postprocessing_ssr_manager_01_t* ctx, void* params);
int postprocessing_ssr_manager_01_flush(postprocessing_ssr_manager_01_t* ctx, void* params);

/* Utility functions */
int postprocessing_ssr_manager_01_get_stats(postprocessing_ssr_manager_01_t* ctx);
int postprocessing_ssr_manager_01_set_callback(postprocessing_ssr_manager_01_t* ctx);
int postprocessing_ssr_manager_01_get_memory_usage(postprocessing_ssr_manager_01_t* ctx);
int postprocessing_ssr_manager_01_optimize(postprocessing_ssr_manager_01_t* ctx);
int postprocessing_ssr_manager_01_debug_print(postprocessing_ssr_manager_01_t* ctx);

/* Module functions */
int postprocessing_ssr_manager_01_module_init(void);
int postprocessing_ssr_manager_01_module_shutdown(void);

#ifdef __cplusplus
}
#endif

#endif /* POSTPROCESSING_SSR_MANAGER_01_H */
