/*
 * texture_streaming_builder_05.h
 *
 * Header file for texture_streaming_builder_05 implementation
 * Part of the Advanced 3D Rendering Engine
 */

#ifndef TEXTURE_STREAMING_BUILDER_05_H
#define TEXTURE_STREAMING_BUILDER_05_H

#include <stdint.h>
#include <stdbool.h>
#include <stddef.h>

#ifdef __cplusplus
extern "C" {
#endif

/* Forward declarations */
typedef struct texture_streaming_builder_05 texture_streaming_builder_05_t;
typedef struct texture_streaming_builder_05_desc texture_streaming_builder_05_desc_t;
typedef struct texture_streaming_builder_05_stats texture_streaming_builder_05_stats_t;

/* Creation and destruction */
int texture_streaming_builder_05_create(texture_streaming_builder_05_t** out_ctx, const texture_streaming_builder_05_desc_t* desc);
int texture_streaming_builder_05_destroy(texture_streaming_builder_05_t* ctx);

/* Core operations */
int texture_streaming_builder_05_begin(texture_streaming_builder_05_t* ctx, void* params);
int texture_streaming_builder_05_end(texture_streaming_builder_05_t* ctx, void* params);
int texture_streaming_builder_05_add(texture_streaming_builder_05_t* ctx, void* params);
int texture_streaming_builder_05_remove(texture_streaming_builder_05_t* ctx, void* params);
int texture_streaming_builder_05_modify(texture_streaming_builder_05_t* ctx, void* params);
int texture_streaming_builder_05_finalize(texture_streaming_builder_05_t* ctx, void* params);
int texture_streaming_builder_05_validate(texture_streaming_builder_05_t* ctx, void* params);
int texture_streaming_builder_05_optimize(texture_streaming_builder_05_t* ctx, void* params);
int texture_streaming_builder_05_compile(texture_streaming_builder_05_t* ctx, void* params);
int texture_streaming_builder_05_link(texture_streaming_builder_05_t* ctx, void* params);

/* Utility functions */
int texture_streaming_builder_05_get_stats(texture_streaming_builder_05_t* ctx);
int texture_streaming_builder_05_set_callback(texture_streaming_builder_05_t* ctx);
int texture_streaming_builder_05_get_memory_usage(texture_streaming_builder_05_t* ctx);
int texture_streaming_builder_05_optimize(texture_streaming_builder_05_t* ctx);
int texture_streaming_builder_05_debug_print(texture_streaming_builder_05_t* ctx);

/* Module functions */
int texture_streaming_builder_05_module_init(void);
int texture_streaming_builder_05_module_shutdown(void);

#ifdef __cplusplus
}
#endif

#endif /* TEXTURE_STREAMING_BUILDER_05_H */
