/*
 * postprocessing_taa_system_02.h
 *
 * Header file for postprocessing_taa_system_02 implementation
 * Part of the Advanced 3D Rendering Engine
 */

#ifndef POSTPROCESSING_TAA_SYSTEM_02_H
#define POSTPROCESSING_TAA_SYSTEM_02_H

#include <stdint.h>
#include <stdbool.h>
#include <stddef.h>

#ifdef __cplusplus
extern "C" {
#endif

/* Forward declarations */
typedef struct postprocessing_taa_system_02 postprocessing_taa_system_02_t;
typedef struct postprocessing_taa_system_02_desc postprocessing_taa_system_02_desc_t;
typedef struct postprocessing_taa_system_02_stats postprocessing_taa_system_02_stats_t;

/* Creation and destruction */
int postprocessing_taa_system_02_create(postprocessing_taa_system_02_t** out_ctx, const postprocessing_taa_system_02_desc_t* desc);
int postprocessing_taa_system_02_destroy(postprocessing_taa_system_02_t* ctx);

/* Core operations */
int postprocessing_taa_system_02_create_system(postprocessing_taa_system_02_t* ctx, void* params);
int postprocessing_taa_system_02_destroy_system(postprocessing_taa_system_02_t* ctx, void* params);
int postprocessing_taa_system_02_tick(postprocessing_taa_system_02_t* ctx, void* params);
int postprocessing_taa_system_02_process(postprocessing_taa_system_02_t* ctx, void* params);
int postprocessing_taa_system_02_submit(postprocessing_taa_system_02_t* ctx, void* params);
int postprocessing_taa_system_02_execute(postprocessing_taa_system_02_t* ctx, void* params);
int postprocessing_taa_system_02_sync(postprocessing_taa_system_02_t* ctx, void* params);
int postprocessing_taa_system_02_query(postprocessing_taa_system_02_t* ctx, void* params);
int postprocessing_taa_system_02_configure(postprocessing_taa_system_02_t* ctx, void* params);
int postprocessing_taa_system_02_optimize(postprocessing_taa_system_02_t* ctx, void* params);

/* Utility functions */
int postprocessing_taa_system_02_get_stats(postprocessing_taa_system_02_t* ctx);
int postprocessing_taa_system_02_set_callback(postprocessing_taa_system_02_t* ctx);
int postprocessing_taa_system_02_get_memory_usage(postprocessing_taa_system_02_t* ctx);
int postprocessing_taa_system_02_optimize(postprocessing_taa_system_02_t* ctx);
int postprocessing_taa_system_02_debug_print(postprocessing_taa_system_02_t* ctx);

/* Module functions */
int postprocessing_taa_system_02_module_init(void);
int postprocessing_taa_system_02_module_shutdown(void);

#ifdef __cplusplus
}
#endif

#endif /* POSTPROCESSING_TAA_SYSTEM_02_H */
