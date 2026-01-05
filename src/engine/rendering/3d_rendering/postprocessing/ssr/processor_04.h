/*
 * postprocessing_ssr_processor_04.h
 *
 * Header file for postprocessing_ssr_processor_04 implementation
 * Part of the Advanced 3D Rendering Engine
 */

#ifndef POSTPROCESSING_SSR_PROCESSOR_04_H
#define POSTPROCESSING_SSR_PROCESSOR_04_H

#include <stdint.h>
#include <stdbool.h>
#include <stddef.h>

#ifdef __cplusplus
extern "C" {
#endif

/* Forward declarations */
typedef struct postprocessing_ssr_processor_04 postprocessing_ssr_processor_04_t;
typedef struct postprocessing_ssr_processor_04_desc postprocessing_ssr_processor_04_desc_t;
typedef struct postprocessing_ssr_processor_04_stats postprocessing_ssr_processor_04_stats_t;

/* Creation and destruction */
int postprocessing_ssr_processor_04_create(postprocessing_ssr_processor_04_t** out_ctx, const postprocessing_ssr_processor_04_desc_t* desc);
int postprocessing_ssr_processor_04_destroy(postprocessing_ssr_processor_04_t* ctx);

/* Core operations */
int postprocessing_ssr_processor_04_process_batch(postprocessing_ssr_processor_04_t* ctx, void* params);
int postprocessing_ssr_processor_04_process_single(postprocessing_ssr_processor_04_t* ctx, void* params);
int postprocessing_ssr_processor_04_transform(postprocessing_ssr_processor_04_t* ctx, void* params);
int postprocessing_ssr_processor_04_filter(postprocessing_ssr_processor_04_t* ctx, void* params);
int postprocessing_ssr_processor_04_aggregate(postprocessing_ssr_processor_04_t* ctx, void* params);
int postprocessing_ssr_processor_04_dispatch(postprocessing_ssr_processor_04_t* ctx, void* params);
int postprocessing_ssr_processor_04_finalize(postprocessing_ssr_processor_04_t* ctx, void* params);
int postprocessing_ssr_processor_04_validate_input(postprocessing_ssr_processor_04_t* ctx, void* params);
int postprocessing_ssr_processor_04_optimize_output(postprocessing_ssr_processor_04_t* ctx, void* params);
int postprocessing_ssr_processor_04_profile(postprocessing_ssr_processor_04_t* ctx, void* params);

/* Utility functions */
int postprocessing_ssr_processor_04_get_stats(postprocessing_ssr_processor_04_t* ctx);
int postprocessing_ssr_processor_04_set_callback(postprocessing_ssr_processor_04_t* ctx);
int postprocessing_ssr_processor_04_get_memory_usage(postprocessing_ssr_processor_04_t* ctx);
int postprocessing_ssr_processor_04_optimize(postprocessing_ssr_processor_04_t* ctx);
int postprocessing_ssr_processor_04_debug_print(postprocessing_ssr_processor_04_t* ctx);

/* Module functions */
int postprocessing_ssr_processor_04_module_init(void);
int postprocessing_ssr_processor_04_module_shutdown(void);

#ifdef __cplusplus
}
#endif

#endif /* POSTPROCESSING_SSR_PROCESSOR_04_H */
