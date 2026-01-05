/*
 * lighting_sources_cache_06.h
 *
 * Header file for lighting_sources_cache_06 implementation
 * Part of the Advanced 3D Rendering Engine
 */

#ifndef LIGHTING_SOURCES_CACHE_06_H
#define LIGHTING_SOURCES_CACHE_06_H

#include <stdint.h>
#include <stdbool.h>
#include <stddef.h>

#ifdef __cplusplus
extern "C" {
#endif

/* Forward declarations */
typedef struct lighting_sources_cache_06 lighting_sources_cache_06_t;
typedef struct lighting_sources_cache_06_desc lighting_sources_cache_06_desc_t;
typedef struct lighting_sources_cache_06_stats lighting_sources_cache_06_stats_t;

/* Creation and destruction */
int lighting_sources_cache_06_create(lighting_sources_cache_06_t** out_ctx, const lighting_sources_cache_06_desc_t* desc);
int lighting_sources_cache_06_destroy(lighting_sources_cache_06_t* ctx);

/* Core operations */
int lighting_sources_cache_06_cache_init(lighting_sources_cache_06_t* ctx, void* params);
int lighting_sources_cache_06_cache_get(lighting_sources_cache_06_t* ctx, void* params);
int lighting_sources_cache_06_cache_put(lighting_sources_cache_06_t* ctx, void* params);
int lighting_sources_cache_06_cache_evict(lighting_sources_cache_06_t* ctx, void* params);
int lighting_sources_cache_06_cache_clear(lighting_sources_cache_06_t* ctx, void* params);
int lighting_sources_cache_06_cache_resize(lighting_sources_cache_06_t* ctx, void* params);
int lighting_sources_cache_06_cache_stats(lighting_sources_cache_06_t* ctx, void* params);
int lighting_sources_cache_06_cache_prefetch(lighting_sources_cache_06_t* ctx, void* params);
int lighting_sources_cache_06_cache_validate(lighting_sources_cache_06_t* ctx, void* params);
int lighting_sources_cache_06_cache_serialize(lighting_sources_cache_06_t* ctx, void* params);

/* Utility functions */
int lighting_sources_cache_06_get_stats(lighting_sources_cache_06_t* ctx);
int lighting_sources_cache_06_set_callback(lighting_sources_cache_06_t* ctx);
int lighting_sources_cache_06_get_memory_usage(lighting_sources_cache_06_t* ctx);
int lighting_sources_cache_06_optimize(lighting_sources_cache_06_t* ctx);
int lighting_sources_cache_06_debug_print(lighting_sources_cache_06_t* ctx);

/* Module functions */
int lighting_sources_cache_06_module_init(void);
int lighting_sources_cache_06_module_shutdown(void);

#ifdef __cplusplus
}
#endif

#endif /* LIGHTING_SOURCES_CACHE_06_H */
