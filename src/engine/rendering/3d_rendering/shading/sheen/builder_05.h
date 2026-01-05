/*
 * shading_sheen_builder_05.h
 *
 * Header file for shading_sheen_builder_05 implementation
 * Part of the Advanced 3D Rendering Engine
 */

#ifndef SHADING_SHEEN_BUILDER_05_H
#define SHADING_SHEEN_BUILDER_05_H

#include <stdint.h>
#include <stdbool.h>
#include <stddef.h>

#ifdef __cplusplus
extern "C" {
#endif

/* Forward declarations */
typedef struct shading_sheen_builder_05 shading_sheen_builder_05_t;
typedef struct shading_sheen_builder_05_desc shading_sheen_builder_05_desc_t;
typedef struct shading_sheen_builder_05_stats shading_sheen_builder_05_stats_t;

/* Creation and destruction */
int shading_sheen_builder_05_create(shading_sheen_builder_05_t** out_ctx, const shading_sheen_builder_05_desc_t* desc);
int shading_sheen_builder_05_destroy(shading_sheen_builder_05_t* ctx);

/* Core operations */
int shading_sheen_builder_05_begin(shading_sheen_builder_05_t* ctx, void* params);
int shading_sheen_builder_05_end(shading_sheen_builder_05_t* ctx, void* params);
int shading_sheen_builder_05_add(shading_sheen_builder_05_t* ctx, void* params);
int shading_sheen_builder_05_remove(shading_sheen_builder_05_t* ctx, void* params);
int shading_sheen_builder_05_modify(shading_sheen_builder_05_t* ctx, void* params);
int shading_sheen_builder_05_finalize(shading_sheen_builder_05_t* ctx, void* params);
int shading_sheen_builder_05_validate(shading_sheen_builder_05_t* ctx, void* params);
int shading_sheen_builder_05_optimize(shading_sheen_builder_05_t* ctx, void* params);
int shading_sheen_builder_05_compile(shading_sheen_builder_05_t* ctx, void* params);
int shading_sheen_builder_05_link(shading_sheen_builder_05_t* ctx, void* params);

/* Utility functions */
int shading_sheen_builder_05_get_stats(shading_sheen_builder_05_t* ctx);
int shading_sheen_builder_05_set_callback(shading_sheen_builder_05_t* ctx);
int shading_sheen_builder_05_get_memory_usage(shading_sheen_builder_05_t* ctx);
int shading_sheen_builder_05_optimize(shading_sheen_builder_05_t* ctx);
int shading_sheen_builder_05_debug_print(shading_sheen_builder_05_t* ctx);

/* Module functions */
int shading_sheen_builder_05_module_init(void);
int shading_sheen_builder_05_module_shutdown(void);

#ifdef __cplusplus
}
#endif

#endif /* SHADING_SHEEN_BUILDER_05_H */
