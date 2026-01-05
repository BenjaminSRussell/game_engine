/*
 * shading_brdf_builder_05.h
 *
 * Header file for shading_brdf_builder_05 implementation
 * Part of the Advanced 3D Rendering Engine
 */

#ifndef SHADING_BRDF_BUILDER_05_H
#define SHADING_BRDF_BUILDER_05_H

#include <stdint.h>
#include <stdbool.h>
#include <stddef.h>

#ifdef __cplusplus
extern "C" {
#endif

/* Forward declarations */
typedef struct shading_brdf_builder_05 shading_brdf_builder_05_t;
typedef struct shading_brdf_builder_05_desc shading_brdf_builder_05_desc_t;
typedef struct shading_brdf_builder_05_stats shading_brdf_builder_05_stats_t;

/* Creation and destruction */
int shading_brdf_builder_05_create(shading_brdf_builder_05_t** out_ctx, const shading_brdf_builder_05_desc_t* desc);
int shading_brdf_builder_05_destroy(shading_brdf_builder_05_t* ctx);

/* Core operations */
int shading_brdf_builder_05_begin(shading_brdf_builder_05_t* ctx, void* params);
int shading_brdf_builder_05_end(shading_brdf_builder_05_t* ctx, void* params);
int shading_brdf_builder_05_add(shading_brdf_builder_05_t* ctx, void* params);
int shading_brdf_builder_05_remove(shading_brdf_builder_05_t* ctx, void* params);
int shading_brdf_builder_05_modify(shading_brdf_builder_05_t* ctx, void* params);
int shading_brdf_builder_05_finalize(shading_brdf_builder_05_t* ctx, void* params);
int shading_brdf_builder_05_validate(shading_brdf_builder_05_t* ctx, void* params);
int shading_brdf_builder_05_optimize(shading_brdf_builder_05_t* ctx, void* params);
int shading_brdf_builder_05_compile(shading_brdf_builder_05_t* ctx, void* params);
int shading_brdf_builder_05_link(shading_brdf_builder_05_t* ctx, void* params);

/* Utility functions */
int shading_brdf_builder_05_get_stats(shading_brdf_builder_05_t* ctx);
int shading_brdf_builder_05_set_callback(shading_brdf_builder_05_t* ctx);
int shading_brdf_builder_05_get_memory_usage(shading_brdf_builder_05_t* ctx);
int shading_brdf_builder_05_optimize(shading_brdf_builder_05_t* ctx);
int shading_brdf_builder_05_debug_print(shading_brdf_builder_05_t* ctx);

/* Module functions */
int shading_brdf_builder_05_module_init(void);
int shading_brdf_builder_05_module_shutdown(void);

#ifdef __cplusplus
}
#endif

#endif /* SHADING_BRDF_BUILDER_05_H */
