/*
 * postprocessing_ao_system_02.h
 *
 * Header file for postprocessing_ao_system_02 implementation
 * Part of the Advanced 3D Rendering Engine
 */

#ifndef POSTPROCESSING_AO_SYSTEM_02_H
#define POSTPROCESSING_AO_SYSTEM_02_H

#include <stdint.h>
#include <stdbool.h>
#include <stddef.h>

#ifdef __cplusplus
extern "C" {
#endif

/* Forward declarations */
typedef struct postprocessing_ao_system_02 postprocessing_ao_system_02_t;
typedef struct postprocessing_ao_system_02_desc postprocessing_ao_system_02_desc_t;
typedef struct postprocessing_ao_system_02_stats postprocessing_ao_system_02_stats_t;

/* Creation and destruction */
int postprocessing_ao_system_02_create(postprocessing_ao_system_02_t** out_ctx, const postprocessing_ao_system_02_desc_t* desc);
int postprocessing_ao_system_02_destroy(postprocessing_ao_system_02_t* ctx);

/* Core operations */
int postprocessing_ao_system_02_create_system(postprocessing_ao_system_02_t* ctx, void* params);
int postprocessing_ao_system_02_destroy_system(postprocessing_ao_system_02_t* ctx, void* params);
int postprocessing_ao_system_02_tick(postprocessing_ao_system_02_t* ctx, void* params);
int postprocessing_ao_system_02_process(postprocessing_ao_system_02_t* ctx, void* params);
int postprocessing_ao_system_02_submit(postprocessing_ao_system_02_t* ctx, void* params);
int postprocessing_ao_system_02_execute(postprocessing_ao_system_02_t* ctx, void* params);
int postprocessing_ao_system_02_sync(postprocessing_ao_system_02_t* ctx, void* params);
int postprocessing_ao_system_02_query(postprocessing_ao_system_02_t* ctx, void* params);
int postprocessing_ao_system_02_configure(postprocessing_ao_system_02_t* ctx, void* params);
int postprocessing_ao_system_02_optimize(postprocessing_ao_system_02_t* ctx, void* params);

/* Utility functions */
int postprocessing_ao_system_02_get_stats(postprocessing_ao_system_02_t* ctx);
int postprocessing_ao_system_02_set_callback(postprocessing_ao_system_02_t* ctx);
int postprocessing_ao_system_02_get_memory_usage(postprocessing_ao_system_02_t* ctx);
int postprocessing_ao_system_02_optimize(postprocessing_ao_system_02_t* ctx);
int postprocessing_ao_system_02_debug_print(postprocessing_ao_system_02_t* ctx);

/* Module functions */
int postprocessing_ao_system_02_module_init(void);
int postprocessing_ao_system_02_module_shutdown(void);

#ifdef __cplusplus
}
#endif

#endif /* POSTPROCESSING_AO_SYSTEM_02_H */
