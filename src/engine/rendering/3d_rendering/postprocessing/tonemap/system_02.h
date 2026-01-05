/*
 * postprocessing_tonemap_system_02.h
 *
 * Header file for postprocessing_tonemap_system_02 implementation
 * Part of the Advanced 3D Rendering Engine
 */

#ifndef POSTPROCESSING_TONEMAP_SYSTEM_02_H
#define POSTPROCESSING_TONEMAP_SYSTEM_02_H

#include <stdint.h>
#include <stdbool.h>
#include <stddef.h>

#ifdef __cplusplus
extern "C" {
#endif

/* Forward declarations */
typedef struct postprocessing_tonemap_system_02 postprocessing_tonemap_system_02_t;
typedef struct postprocessing_tonemap_system_02_desc postprocessing_tonemap_system_02_desc_t;
typedef struct postprocessing_tonemap_system_02_stats postprocessing_tonemap_system_02_stats_t;

/* Creation and destruction */
int postprocessing_tonemap_system_02_create(postprocessing_tonemap_system_02_t** out_ctx, const postprocessing_tonemap_system_02_desc_t* desc);
int postprocessing_tonemap_system_02_destroy(postprocessing_tonemap_system_02_t* ctx);

/* Core operations */
int postprocessing_tonemap_system_02_create_system(postprocessing_tonemap_system_02_t* ctx, void* params);
int postprocessing_tonemap_system_02_destroy_system(postprocessing_tonemap_system_02_t* ctx, void* params);
int postprocessing_tonemap_system_02_tick(postprocessing_tonemap_system_02_t* ctx, void* params);
int postprocessing_tonemap_system_02_process(postprocessing_tonemap_system_02_t* ctx, void* params);
int postprocessing_tonemap_system_02_submit(postprocessing_tonemap_system_02_t* ctx, void* params);
int postprocessing_tonemap_system_02_execute(postprocessing_tonemap_system_02_t* ctx, void* params);
int postprocessing_tonemap_system_02_sync(postprocessing_tonemap_system_02_t* ctx, void* params);
int postprocessing_tonemap_system_02_query(postprocessing_tonemap_system_02_t* ctx, void* params);
int postprocessing_tonemap_system_02_configure(postprocessing_tonemap_system_02_t* ctx, void* params);
int postprocessing_tonemap_system_02_optimize(postprocessing_tonemap_system_02_t* ctx, void* params);

/* Utility functions */
int postprocessing_tonemap_system_02_get_stats(postprocessing_tonemap_system_02_t* ctx);
int postprocessing_tonemap_system_02_set_callback(postprocessing_tonemap_system_02_t* ctx);
int postprocessing_tonemap_system_02_get_memory_usage(postprocessing_tonemap_system_02_t* ctx);
int postprocessing_tonemap_system_02_optimize(postprocessing_tonemap_system_02_t* ctx);
int postprocessing_tonemap_system_02_debug_print(postprocessing_tonemap_system_02_t* ctx);

/* Module functions */
int postprocessing_tonemap_system_02_module_init(void);
int postprocessing_tonemap_system_02_module_shutdown(void);

#ifdef __cplusplus
}
#endif

#endif /* POSTPROCESSING_TONEMAP_SYSTEM_02_H */
