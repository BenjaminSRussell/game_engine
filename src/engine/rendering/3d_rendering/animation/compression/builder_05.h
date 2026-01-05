/*
 * animation_compression_builder_05.h
 *
 * Header file for animation_compression_builder_05 implementation
 * Part of the Advanced 3D Rendering Engine
 */

#ifndef ANIMATION_COMPRESSION_BUILDER_05_H
#define ANIMATION_COMPRESSION_BUILDER_05_H

#include <stdint.h>
#include <stdbool.h>
#include <stddef.h>

#ifdef __cplusplus
extern "C" {
#endif

/* Forward declarations */
typedef struct animation_compression_builder_05 animation_compression_builder_05_t;
typedef struct animation_compression_builder_05_desc animation_compression_builder_05_desc_t;
typedef struct animation_compression_builder_05_stats animation_compression_builder_05_stats_t;

/* Creation and destruction */
int animation_compression_builder_05_create(animation_compression_builder_05_t** out_ctx, const animation_compression_builder_05_desc_t* desc);
int animation_compression_builder_05_destroy(animation_compression_builder_05_t* ctx);

/* Core operations */
int animation_compression_builder_05_begin(animation_compression_builder_05_t* ctx, void* params);
int animation_compression_builder_05_end(animation_compression_builder_05_t* ctx, void* params);
int animation_compression_builder_05_add(animation_compression_builder_05_t* ctx, void* params);
int animation_compression_builder_05_remove(animation_compression_builder_05_t* ctx, void* params);
int animation_compression_builder_05_modify(animation_compression_builder_05_t* ctx, void* params);
int animation_compression_builder_05_finalize(animation_compression_builder_05_t* ctx, void* params);
int animation_compression_builder_05_validate(animation_compression_builder_05_t* ctx, void* params);
int animation_compression_builder_05_optimize(animation_compression_builder_05_t* ctx, void* params);
int animation_compression_builder_05_compile(animation_compression_builder_05_t* ctx, void* params);
int animation_compression_builder_05_link(animation_compression_builder_05_t* ctx, void* params);

/* Utility functions */
int animation_compression_builder_05_get_stats(animation_compression_builder_05_t* ctx);
int animation_compression_builder_05_set_callback(animation_compression_builder_05_t* ctx);
int animation_compression_builder_05_get_memory_usage(animation_compression_builder_05_t* ctx);
int animation_compression_builder_05_optimize(animation_compression_builder_05_t* ctx);
int animation_compression_builder_05_debug_print(animation_compression_builder_05_t* ctx);

/* Module functions */
int animation_compression_builder_05_module_init(void);
int animation_compression_builder_05_module_shutdown(void);

#ifdef __cplusplus
}
#endif

#endif /* ANIMATION_COMPRESSION_BUILDER_05_H */
