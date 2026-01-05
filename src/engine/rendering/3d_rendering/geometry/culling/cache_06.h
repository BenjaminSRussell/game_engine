/*
 * geometry_culling_cache_06.h
 *
 * Header file for geometry_culling_cache_06 implementation
 * Part of the Advanced 3D Rendering Engine
 */

#ifndef GEOMETRY_CULLING_CACHE_06_H
#define GEOMETRY_CULLING_CACHE_06_H

#include <stdint.h>
#include <stdbool.h>
#include <stddef.h>

#ifdef __cplusplus
extern "C" {
#endif

/* Forward declarations */
typedef struct geometry_culling_cache_06 geometry_culling_cache_06_t;
typedef struct geometry_culling_cache_06_desc geometry_culling_cache_06_desc_t;
typedef struct geometry_culling_cache_06_stats geometry_culling_cache_06_stats_t;

/* Creation and destruction */
int geometry_culling_cache_06_create(geometry_culling_cache_06_t** out_ctx, const geometry_culling_cache_06_desc_t* desc);
int geometry_culling_cache_06_destroy(geometry_culling_cache_06_t* ctx);

/* Core operations */
int geometry_culling_cache_06_cache_init(geometry_culling_cache_06_t* ctx, void* params);
int geometry_culling_cache_06_cache_get(geometry_culling_cache_06_t* ctx, void* params);
int geometry_culling_cache_06_cache_put(geometry_culling_cache_06_t* ctx, void* params);
int geometry_culling_cache_06_cache_evict(geometry_culling_cache_06_t* ctx, void* params);
int geometry_culling_cache_06_cache_clear(geometry_culling_cache_06_t* ctx, void* params);
int geometry_culling_cache_06_cache_resize(geometry_culling_cache_06_t* ctx, void* params);
int geometry_culling_cache_06_cache_stats(geometry_culling_cache_06_t* ctx, void* params);
int geometry_culling_cache_06_cache_prefetch(geometry_culling_cache_06_t* ctx, void* params);
int geometry_culling_cache_06_cache_validate(geometry_culling_cache_06_t* ctx, void* params);
int geometry_culling_cache_06_cache_serialize(geometry_culling_cache_06_t* ctx, void* params);

/* Utility functions */
int geometry_culling_cache_06_get_stats(geometry_culling_cache_06_t* ctx);
int geometry_culling_cache_06_set_callback(geometry_culling_cache_06_t* ctx);
int geometry_culling_cache_06_get_memory_usage(geometry_culling_cache_06_t* ctx);
int geometry_culling_cache_06_optimize(geometry_culling_cache_06_t* ctx);
int geometry_culling_cache_06_debug_print(geometry_culling_cache_06_t* ctx);

/* Module functions */
int geometry_culling_cache_06_module_init(void);
int geometry_culling_cache_06_module_shutdown(void);

#ifdef __cplusplus
}
#endif

#endif /* GEOMETRY_CULLING_CACHE_06_H */
