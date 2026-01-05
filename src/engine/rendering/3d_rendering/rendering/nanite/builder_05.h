/*
 * rendering_nanite_builder_05.h
 *
 * Header file for rendering_nanite_builder_05 implementation
 * Part of the Advanced 3D Rendering Engine
 */

#ifndef RENDERING_NANITE_BUILDER_05_H
#define RENDERING_NANITE_BUILDER_05_H

#include <stdint.h>
#include <stdbool.h>
#include <stddef.h>

#ifdef __cplusplus
extern "C" {
#endif

/* Forward declarations */
typedef struct rendering_nanite_builder_05 rendering_nanite_builder_05_t;
typedef struct rendering_nanite_builder_05_desc rendering_nanite_builder_05_desc_t;
typedef struct rendering_nanite_builder_05_stats rendering_nanite_builder_05_stats_t;

/* Creation and destruction */
int rendering_nanite_builder_05_create(rendering_nanite_builder_05_t** out_ctx, const rendering_nanite_builder_05_desc_t* desc);
int rendering_nanite_builder_05_destroy(rendering_nanite_builder_05_t* ctx);

/* Core operations */
int rendering_nanite_builder_05_begin(rendering_nanite_builder_05_t* ctx, void* params);
int rendering_nanite_builder_05_end(rendering_nanite_builder_05_t* ctx, void* params);
int rendering_nanite_builder_05_add(rendering_nanite_builder_05_t* ctx, void* params);
int rendering_nanite_builder_05_remove(rendering_nanite_builder_05_t* ctx, void* params);
int rendering_nanite_builder_05_modify(rendering_nanite_builder_05_t* ctx, void* params);
int rendering_nanite_builder_05_finalize(rendering_nanite_builder_05_t* ctx, void* params);
int rendering_nanite_builder_05_validate(rendering_nanite_builder_05_t* ctx, void* params);
int rendering_nanite_builder_05_optimize(rendering_nanite_builder_05_t* ctx, void* params);
int rendering_nanite_builder_05_compile(rendering_nanite_builder_05_t* ctx, void* params);
int rendering_nanite_builder_05_link(rendering_nanite_builder_05_t* ctx, void* params);

/* Utility functions */
int rendering_nanite_builder_05_get_stats(rendering_nanite_builder_05_t* ctx);
int rendering_nanite_builder_05_set_callback(rendering_nanite_builder_05_t* ctx);
int rendering_nanite_builder_05_get_memory_usage(rendering_nanite_builder_05_t* ctx);
int rendering_nanite_builder_05_optimize(rendering_nanite_builder_05_t* ctx);
int rendering_nanite_builder_05_debug_print(rendering_nanite_builder_05_t* ctx);

/* Module functions */
int rendering_nanite_builder_05_module_init(void);
int rendering_nanite_builder_05_module_shutdown(void);

#ifdef __cplusplus
}
#endif

#endif /* RENDERING_NANITE_BUILDER_05_H */
