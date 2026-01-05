/*
 * postprocessing_vignette_system_02.h
 *
 * Header file for postprocessing_vignette_system_02 implementation
 * Part of the Advanced 3D Rendering Engine
 */

#ifndef POSTPROCESSING_VIGNETTE_SYSTEM_02_H
#define POSTPROCESSING_VIGNETTE_SYSTEM_02_H

#include <stdint.h>
#include <stdbool.h>
#include <stddef.h>

#ifdef __cplusplus
extern "C" {
#endif

/* Forward declarations */
typedef struct postprocessing_vignette_system_02 postprocessing_vignette_system_02_t;
typedef struct postprocessing_vignette_system_02_desc postprocessing_vignette_system_02_desc_t;
typedef struct postprocessing_vignette_system_02_stats postprocessing_vignette_system_02_stats_t;

/* Creation and destruction */
int postprocessing_vignette_system_02_create(postprocessing_vignette_system_02_t** out_ctx, const postprocessing_vignette_system_02_desc_t* desc);
int postprocessing_vignette_system_02_destroy(postprocessing_vignette_system_02_t* ctx);

/* Core operations */
int postprocessing_vignette_system_02_create_system(postprocessing_vignette_system_02_t* ctx, void* params);
int postprocessing_vignette_system_02_destroy_system(postprocessing_vignette_system_02_t* ctx, void* params);
int postprocessing_vignette_system_02_tick(postprocessing_vignette_system_02_t* ctx, void* params);
int postprocessing_vignette_system_02_process(postprocessing_vignette_system_02_t* ctx, void* params);
int postprocessing_vignette_system_02_submit(postprocessing_vignette_system_02_t* ctx, void* params);
int postprocessing_vignette_system_02_execute(postprocessing_vignette_system_02_t* ctx, void* params);
int postprocessing_vignette_system_02_sync(postprocessing_vignette_system_02_t* ctx, void* params);
int postprocessing_vignette_system_02_query(postprocessing_vignette_system_02_t* ctx, void* params);
int postprocessing_vignette_system_02_configure(postprocessing_vignette_system_02_t* ctx, void* params);
int postprocessing_vignette_system_02_optimize(postprocessing_vignette_system_02_t* ctx, void* params);

/* Utility functions */
int postprocessing_vignette_system_02_get_stats(postprocessing_vignette_system_02_t* ctx);
int postprocessing_vignette_system_02_set_callback(postprocessing_vignette_system_02_t* ctx);
int postprocessing_vignette_system_02_get_memory_usage(postprocessing_vignette_system_02_t* ctx);
int postprocessing_vignette_system_02_optimize(postprocessing_vignette_system_02_t* ctx);
int postprocessing_vignette_system_02_debug_print(postprocessing_vignette_system_02_t* ctx);

/* Module functions */
int postprocessing_vignette_system_02_module_init(void);
int postprocessing_vignette_system_02_module_shutdown(void);

#ifdef __cplusplus
}
#endif

#endif /* POSTPROCESSING_VIGNETTE_SYSTEM_02_H */
