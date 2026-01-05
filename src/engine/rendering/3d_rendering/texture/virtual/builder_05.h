/*
 * texture_virtual_builder_05.h
 *
 * Header file for texture_virtual_builder_05 implementation
 * Part of the Advanced 3D Rendering Engine
 */

#ifndef TEXTURE_VIRTUAL_BUILDER_05_H
#define TEXTURE_VIRTUAL_BUILDER_05_H

#include <stdint.h>
#include <stdbool.h>
#include <stddef.h>

#ifdef __cplusplus
extern "C" {
#endif

/* Forward declarations */
typedef struct texture_virtual_builder_05 texture_virtual_builder_05_t;
typedef struct texture_virtual_builder_05_desc texture_virtual_builder_05_desc_t;
typedef struct texture_virtual_builder_05_stats texture_virtual_builder_05_stats_t;

/* Creation and destruction */
int texture_virtual_builder_05_create(texture_virtual_builder_05_t** out_ctx, const texture_virtual_builder_05_desc_t* desc);
int texture_virtual_builder_05_destroy(texture_virtual_builder_05_t* ctx);

/* Core operations */
int texture_virtual_builder_05_begin(texture_virtual_builder_05_t* ctx, void* params);
int texture_virtual_builder_05_end(texture_virtual_builder_05_t* ctx, void* params);
int texture_virtual_builder_05_add(texture_virtual_builder_05_t* ctx, void* params);
int texture_virtual_builder_05_remove(texture_virtual_builder_05_t* ctx, void* params);
int texture_virtual_builder_05_modify(texture_virtual_builder_05_t* ctx, void* params);
int texture_virtual_builder_05_finalize(texture_virtual_builder_05_t* ctx, void* params);
int texture_virtual_builder_05_validate(texture_virtual_builder_05_t* ctx, void* params);
int texture_virtual_builder_05_optimize(texture_virtual_builder_05_t* ctx, void* params);
int texture_virtual_builder_05_compile(texture_virtual_builder_05_t* ctx, void* params);
int texture_virtual_builder_05_link(texture_virtual_builder_05_t* ctx, void* params);

/* Utility functions */
int texture_virtual_builder_05_get_stats(texture_virtual_builder_05_t* ctx);
int texture_virtual_builder_05_set_callback(texture_virtual_builder_05_t* ctx);
int texture_virtual_builder_05_get_memory_usage(texture_virtual_builder_05_t* ctx);
int texture_virtual_builder_05_optimize(texture_virtual_builder_05_t* ctx);
int texture_virtual_builder_05_debug_print(texture_virtual_builder_05_t* ctx);

/* Module functions */
int texture_virtual_builder_05_module_init(void);
int texture_virtual_builder_05_module_shutdown(void);

#ifdef __cplusplus
}
#endif

#endif /* TEXTURE_VIRTUAL_BUILDER_05_H */
