/*
 * raytracing_reflections_builder_05.h
 *
 * Header file for raytracing_reflections_builder_05 implementation
 * Part of the Advanced 3D Rendering Engine
 */

#ifndef RAYTRACING_REFLECTIONS_BUILDER_05_H
#define RAYTRACING_REFLECTIONS_BUILDER_05_H

#include <stdint.h>
#include <stdbool.h>
#include <stddef.h>

#ifdef __cplusplus
extern "C" {
#endif

/* Forward declarations */
typedef struct raytracing_reflections_builder_05 raytracing_reflections_builder_05_t;
typedef struct raytracing_reflections_builder_05_desc raytracing_reflections_builder_05_desc_t;
typedef struct raytracing_reflections_builder_05_stats raytracing_reflections_builder_05_stats_t;

/* Creation and destruction */
int raytracing_reflections_builder_05_create(raytracing_reflections_builder_05_t** out_ctx, const raytracing_reflections_builder_05_desc_t* desc);
int raytracing_reflections_builder_05_destroy(raytracing_reflections_builder_05_t* ctx);

/* Core operations */
int raytracing_reflections_builder_05_begin(raytracing_reflections_builder_05_t* ctx, void* params);
int raytracing_reflections_builder_05_end(raytracing_reflections_builder_05_t* ctx, void* params);
int raytracing_reflections_builder_05_add(raytracing_reflections_builder_05_t* ctx, void* params);
int raytracing_reflections_builder_05_remove(raytracing_reflections_builder_05_t* ctx, void* params);
int raytracing_reflections_builder_05_modify(raytracing_reflections_builder_05_t* ctx, void* params);
int raytracing_reflections_builder_05_finalize(raytracing_reflections_builder_05_t* ctx, void* params);
int raytracing_reflections_builder_05_validate(raytracing_reflections_builder_05_t* ctx, void* params);
int raytracing_reflections_builder_05_optimize(raytracing_reflections_builder_05_t* ctx, void* params);
int raytracing_reflections_builder_05_compile(raytracing_reflections_builder_05_t* ctx, void* params);
int raytracing_reflections_builder_05_link(raytracing_reflections_builder_05_t* ctx, void* params);

/* Utility functions */
int raytracing_reflections_builder_05_get_stats(raytracing_reflections_builder_05_t* ctx);
int raytracing_reflections_builder_05_set_callback(raytracing_reflections_builder_05_t* ctx);
int raytracing_reflections_builder_05_get_memory_usage(raytracing_reflections_builder_05_t* ctx);
int raytracing_reflections_builder_05_optimize(raytracing_reflections_builder_05_t* ctx);
int raytracing_reflections_builder_05_debug_print(raytracing_reflections_builder_05_t* ctx);

/* Module functions */
int raytracing_reflections_builder_05_module_init(void);
int raytracing_reflections_builder_05_module_shutdown(void);

#ifdef __cplusplus
}
#endif

#endif /* RAYTRACING_REFLECTIONS_BUILDER_05_H */
