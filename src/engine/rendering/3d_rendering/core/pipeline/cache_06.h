/*
 * core_pipeline_cache_06.h
 *
 * Header file for core_pipeline_cache_06 implementation
 * Part of the Advanced 3D Rendering Engine
 */

#ifndef CORE_PIPELINE_CACHE_06_H
#define CORE_PIPELINE_CACHE_06_H

#include <stdint.h>
#include <stdbool.h>
#include <stddef.h>

#ifdef __cplusplus
extern "C" {
#endif

/* Forward declarations */
typedef struct core_pipeline_cache_06 core_pipeline_cache_06_t;
typedef struct core_pipeline_cache_06_desc core_pipeline_cache_06_desc_t;
typedef struct core_pipeline_cache_06_stats core_pipeline_cache_06_stats_t;

/* Creation and destruction */
int core_pipeline_cache_06_create(core_pipeline_cache_06_t** out_ctx, const core_pipeline_cache_06_desc_t* desc);
int core_pipeline_cache_06_destroy(core_pipeline_cache_06_t* ctx);

/* Core operations */
int core_pipeline_cache_06_cache_init(core_pipeline_cache_06_t* ctx, void* params);
int core_pipeline_cache_06_cache_get(core_pipeline_cache_06_t* ctx, void* params);
int core_pipeline_cache_06_cache_put(core_pipeline_cache_06_t* ctx, void* params);
int core_pipeline_cache_06_cache_evict(core_pipeline_cache_06_t* ctx, void* params);
int core_pipeline_cache_06_cache_clear(core_pipeline_cache_06_t* ctx, void* params);
int core_pipeline_cache_06_cache_resize(core_pipeline_cache_06_t* ctx, void* params);
int core_pipeline_cache_06_cache_stats(core_pipeline_cache_06_t* ctx, void* params);
int core_pipeline_cache_06_cache_prefetch(core_pipeline_cache_06_t* ctx, void* params);
int core_pipeline_cache_06_cache_validate(core_pipeline_cache_06_t* ctx, void* params);
int core_pipeline_cache_06_cache_serialize(core_pipeline_cache_06_t* ctx, void* params);

/* Utility functions */
int core_pipeline_cache_06_get_stats(core_pipeline_cache_06_t* ctx);
int core_pipeline_cache_06_set_callback(core_pipeline_cache_06_t* ctx);
int core_pipeline_cache_06_get_memory_usage(core_pipeline_cache_06_t* ctx);
int core_pipeline_cache_06_optimize(core_pipeline_cache_06_t* ctx);
int core_pipeline_cache_06_debug_print(core_pipeline_cache_06_t* ctx);

/* Module functions */
int core_pipeline_cache_06_module_init(void);
int core_pipeline_cache_06_module_shutdown(void);

#ifdef __cplusplus
}
#endif

#endif /* CORE_PIPELINE_CACHE_06_H */
