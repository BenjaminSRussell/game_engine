/*
 * framebuffer_resolve_builder_05.h
 *
 * Header file for framebuffer_resolve_builder_05 implementation
 * Part of the Advanced 3D Rendering Engine
 */

#ifndef FRAMEBUFFER_RESOLVE_BUILDER_05_H
#define FRAMEBUFFER_RESOLVE_BUILDER_05_H

#include <stdint.h>
#include <stdbool.h>
#include <stddef.h>

#ifdef __cplusplus
extern "C" {
#endif

/* Forward declarations */
typedef struct framebuffer_resolve_builder_05 framebuffer_resolve_builder_05_t;
typedef struct framebuffer_resolve_builder_05_desc framebuffer_resolve_builder_05_desc_t;
typedef struct framebuffer_resolve_builder_05_stats framebuffer_resolve_builder_05_stats_t;

/* Creation and destruction */
int framebuffer_resolve_builder_05_create(framebuffer_resolve_builder_05_t** out_ctx, const framebuffer_resolve_builder_05_desc_t* desc);
int framebuffer_resolve_builder_05_destroy(framebuffer_resolve_builder_05_t* ctx);

/* Core operations */
int framebuffer_resolve_builder_05_begin(framebuffer_resolve_builder_05_t* ctx, void* params);
int framebuffer_resolve_builder_05_end(framebuffer_resolve_builder_05_t* ctx, void* params);
int framebuffer_resolve_builder_05_add(framebuffer_resolve_builder_05_t* ctx, void* params);
int framebuffer_resolve_builder_05_remove(framebuffer_resolve_builder_05_t* ctx, void* params);
int framebuffer_resolve_builder_05_modify(framebuffer_resolve_builder_05_t* ctx, void* params);
int framebuffer_resolve_builder_05_finalize(framebuffer_resolve_builder_05_t* ctx, void* params);
int framebuffer_resolve_builder_05_validate(framebuffer_resolve_builder_05_t* ctx, void* params);
int framebuffer_resolve_builder_05_optimize(framebuffer_resolve_builder_05_t* ctx, void* params);
int framebuffer_resolve_builder_05_compile(framebuffer_resolve_builder_05_t* ctx, void* params);
int framebuffer_resolve_builder_05_link(framebuffer_resolve_builder_05_t* ctx, void* params);

/* Utility functions */
int framebuffer_resolve_builder_05_get_stats(framebuffer_resolve_builder_05_t* ctx);
int framebuffer_resolve_builder_05_set_callback(framebuffer_resolve_builder_05_t* ctx);
int framebuffer_resolve_builder_05_get_memory_usage(framebuffer_resolve_builder_05_t* ctx);
int framebuffer_resolve_builder_05_optimize(framebuffer_resolve_builder_05_t* ctx);
int framebuffer_resolve_builder_05_debug_print(framebuffer_resolve_builder_05_t* ctx);

/* Module functions */
int framebuffer_resolve_builder_05_module_init(void);
int framebuffer_resolve_builder_05_module_shutdown(void);

#ifdef __cplusplus
}
#endif

#endif /* FRAMEBUFFER_RESOLVE_BUILDER_05_H */
