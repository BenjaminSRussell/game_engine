/*
 * water_ocean_builder_05.h
 *
 * Header file for water_ocean_builder_05 implementation
 * Part of the Advanced 3D Rendering Engine
 */

#ifndef WATER_OCEAN_BUILDER_05_H
#define WATER_OCEAN_BUILDER_05_H

#include <stdint.h>
#include <stdbool.h>
#include <stddef.h>

#ifdef __cplusplus
extern "C" {
#endif

/* Forward declarations */
typedef struct water_ocean_builder_05 water_ocean_builder_05_t;
typedef struct water_ocean_builder_05_desc water_ocean_builder_05_desc_t;
typedef struct water_ocean_builder_05_stats water_ocean_builder_05_stats_t;

/* Creation and destruction */
int water_ocean_builder_05_create(water_ocean_builder_05_t** out_ctx, const water_ocean_builder_05_desc_t* desc);
int water_ocean_builder_05_destroy(water_ocean_builder_05_t* ctx);

/* Core operations */
int water_ocean_builder_05_begin(water_ocean_builder_05_t* ctx, void* params);
int water_ocean_builder_05_end(water_ocean_builder_05_t* ctx, void* params);
int water_ocean_builder_05_add(water_ocean_builder_05_t* ctx, void* params);
int water_ocean_builder_05_remove(water_ocean_builder_05_t* ctx, void* params);
int water_ocean_builder_05_modify(water_ocean_builder_05_t* ctx, void* params);
int water_ocean_builder_05_finalize(water_ocean_builder_05_t* ctx, void* params);
int water_ocean_builder_05_validate(water_ocean_builder_05_t* ctx, void* params);
int water_ocean_builder_05_optimize(water_ocean_builder_05_t* ctx, void* params);
int water_ocean_builder_05_compile(water_ocean_builder_05_t* ctx, void* params);
int water_ocean_builder_05_link(water_ocean_builder_05_t* ctx, void* params);

/* Utility functions */
int water_ocean_builder_05_get_stats(water_ocean_builder_05_t* ctx);
int water_ocean_builder_05_set_callback(water_ocean_builder_05_t* ctx);
int water_ocean_builder_05_get_memory_usage(water_ocean_builder_05_t* ctx);
int water_ocean_builder_05_optimize(water_ocean_builder_05_t* ctx);
int water_ocean_builder_05_debug_print(water_ocean_builder_05_t* ctx);

/* Module functions */
int water_ocean_builder_05_module_init(void);
int water_ocean_builder_05_module_shutdown(void);

#ifdef __cplusplus
}
#endif

#endif /* WATER_OCEAN_BUILDER_05_H */
