/*
 * lighting_probes_cache_06.h
 *
 * Header file for lighting_probes_cache_06 implementation
 * Part of the Advanced 3D Rendering Engine
 */

#ifndef LIGHTING_PROBES_CACHE_06_H
#define LIGHTING_PROBES_CACHE_06_H

#include <stdint.h>
#include <stdbool.h>
#include <stddef.h>

#ifdef __cplusplus
extern "C" {
#endif

/* Forward declarations */
typedef struct lighting_probes_cache_06 lighting_probes_cache_06_t;
typedef struct lighting_probes_cache_06_desc lighting_probes_cache_06_desc_t;
typedef struct lighting_probes_cache_06_stats lighting_probes_cache_06_stats_t;

/* Creation and destruction */
int lighting_probes_cache_06_create(lighting_probes_cache_06_t** out_ctx, const lighting_probes_cache_06_desc_t* desc);
int lighting_probes_cache_06_destroy(lighting_probes_cache_06_t* ctx);

/* Core operations */
int lighting_probes_cache_06_cache_init(lighting_probes_cache_06_t* ctx, void* params);
int lighting_probes_cache_06_cache_get(lighting_probes_cache_06_t* ctx, void* params);
int lighting_probes_cache_06_cache_put(lighting_probes_cache_06_t* ctx, void* params);
int lighting_probes_cache_06_cache_evict(lighting_probes_cache_06_t* ctx, void* params);
int lighting_probes_cache_06_cache_clear(lighting_probes_cache_06_t* ctx, void* params);
int lighting_probes_cache_06_cache_resize(lighting_probes_cache_06_t* ctx, void* params);
int lighting_probes_cache_06_cache_stats(lighting_probes_cache_06_t* ctx, void* params);
int lighting_probes_cache_06_cache_prefetch(lighting_probes_cache_06_t* ctx, void* params);
int lighting_probes_cache_06_cache_validate(lighting_probes_cache_06_t* ctx, void* params);
int lighting_probes_cache_06_cache_serialize(lighting_probes_cache_06_t* ctx, void* params);

/* Utility functions */
int lighting_probes_cache_06_get_stats(lighting_probes_cache_06_t* ctx);
int lighting_probes_cache_06_set_callback(lighting_probes_cache_06_t* ctx);
int lighting_probes_cache_06_get_memory_usage(lighting_probes_cache_06_t* ctx);
int lighting_probes_cache_06_optimize(lighting_probes_cache_06_t* ctx);
int lighting_probes_cache_06_debug_print(lighting_probes_cache_06_t* ctx);

/* Module functions */
int lighting_probes_cache_06_module_init(void);
int lighting_probes_cache_06_module_shutdown(void);

#ifdef __cplusplus
}
#endif

#endif /* LIGHTING_PROBES_CACHE_06_H */
