/*
 * core_memory_cache_06.h
 *
 * Header file for core_memory_cache_06 implementation
 * Part of the Advanced 3D Rendering Engine
 */

#ifndef CORE_MEMORY_CACHE_06_H
#define CORE_MEMORY_CACHE_06_H

#include <stdint.h>
#include <stdbool.h>
#include <stddef.h>

#ifdef __cplusplus
extern "C" {
#endif

/* Forward declarations */
typedef struct core_memory_cache_06 core_memory_cache_06_t;
typedef struct core_memory_cache_06_desc core_memory_cache_06_desc_t;
typedef struct core_memory_cache_06_stats core_memory_cache_06_stats_t;

/* Creation and destruction */
int core_memory_cache_06_create(core_memory_cache_06_t** out_ctx, const core_memory_cache_06_desc_t* desc);
int core_memory_cache_06_destroy(core_memory_cache_06_t* ctx);

/* Core operations */
int core_memory_cache_06_cache_init(core_memory_cache_06_t* ctx, void* params);
int core_memory_cache_06_cache_get(core_memory_cache_06_t* ctx, void* params);
int core_memory_cache_06_cache_put(core_memory_cache_06_t* ctx, void* params);
int core_memory_cache_06_cache_evict(core_memory_cache_06_t* ctx, void* params);
int core_memory_cache_06_cache_clear(core_memory_cache_06_t* ctx, void* params);
int core_memory_cache_06_cache_resize(core_memory_cache_06_t* ctx, void* params);
int core_memory_cache_06_cache_stats(core_memory_cache_06_t* ctx, void* params);
int core_memory_cache_06_cache_prefetch(core_memory_cache_06_t* ctx, void* params);
int core_memory_cache_06_cache_validate(core_memory_cache_06_t* ctx, void* params);
int core_memory_cache_06_cache_serialize(core_memory_cache_06_t* ctx, void* params);

/* Utility functions */
int core_memory_cache_06_get_stats(core_memory_cache_06_t* ctx);
int core_memory_cache_06_set_callback(core_memory_cache_06_t* ctx);
int core_memory_cache_06_get_memory_usage(core_memory_cache_06_t* ctx);
int core_memory_cache_06_optimize(core_memory_cache_06_t* ctx);
int core_memory_cache_06_debug_print(core_memory_cache_06_t* ctx);

/* Module functions */
int core_memory_cache_06_module_init(void);
int core_memory_cache_06_module_shutdown(void);

#ifdef __cplusplus
}
#endif

#endif /* CORE_MEMORY_CACHE_06_H */
