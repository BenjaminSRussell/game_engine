/*
 * lighting_lightmaps_cache_06.h
 *
 * Header file for lighting_lightmaps_cache_06 implementation
 * Part of the Advanced 3D Rendering Engine
 */

#ifndef LIGHTING_LIGHTMAPS_CACHE_06_H
#define LIGHTING_LIGHTMAPS_CACHE_06_H

#include <stdint.h>
#include <stdbool.h>
#include <stddef.h>

#ifdef __cplusplus
extern "C" {
#endif

/* Forward declarations */
typedef struct lighting_lightmaps_cache_06 lighting_lightmaps_cache_06_t;
typedef struct lighting_lightmaps_cache_06_desc lighting_lightmaps_cache_06_desc_t;
typedef struct lighting_lightmaps_cache_06_stats lighting_lightmaps_cache_06_stats_t;

/* Creation and destruction */
int lighting_lightmaps_cache_06_create(lighting_lightmaps_cache_06_t** out_ctx, const lighting_lightmaps_cache_06_desc_t* desc);
int lighting_lightmaps_cache_06_destroy(lighting_lightmaps_cache_06_t* ctx);

/* Core operations */
int lighting_lightmaps_cache_06_cache_init(lighting_lightmaps_cache_06_t* ctx, void* params);
int lighting_lightmaps_cache_06_cache_get(lighting_lightmaps_cache_06_t* ctx, void* params);
int lighting_lightmaps_cache_06_cache_put(lighting_lightmaps_cache_06_t* ctx, void* params);
int lighting_lightmaps_cache_06_cache_evict(lighting_lightmaps_cache_06_t* ctx, void* params);
int lighting_lightmaps_cache_06_cache_clear(lighting_lightmaps_cache_06_t* ctx, void* params);
int lighting_lightmaps_cache_06_cache_resize(lighting_lightmaps_cache_06_t* ctx, void* params);
int lighting_lightmaps_cache_06_cache_stats(lighting_lightmaps_cache_06_t* ctx, void* params);
int lighting_lightmaps_cache_06_cache_prefetch(lighting_lightmaps_cache_06_t* ctx, void* params);
int lighting_lightmaps_cache_06_cache_validate(lighting_lightmaps_cache_06_t* ctx, void* params);
int lighting_lightmaps_cache_06_cache_serialize(lighting_lightmaps_cache_06_t* ctx, void* params);

/* Utility functions */
int lighting_lightmaps_cache_06_get_stats(lighting_lightmaps_cache_06_t* ctx);
int lighting_lightmaps_cache_06_set_callback(lighting_lightmaps_cache_06_t* ctx);
int lighting_lightmaps_cache_06_get_memory_usage(lighting_lightmaps_cache_06_t* ctx);
int lighting_lightmaps_cache_06_optimize(lighting_lightmaps_cache_06_t* ctx);
int lighting_lightmaps_cache_06_debug_print(lighting_lightmaps_cache_06_t* ctx);

/* Module functions */
int lighting_lightmaps_cache_06_module_init(void);
int lighting_lightmaps_cache_06_module_shutdown(void);

#ifdef __cplusplus
}
#endif

#endif /* LIGHTING_LIGHTMAPS_CACHE_06_H */
