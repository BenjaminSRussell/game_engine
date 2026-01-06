/*
 * debugging_logging_renderer_03.h
 *
 * Header file for debugging_logging_renderer_03 implementation
 * Part of the Advanced 3D Rendering Engine
 */

#ifndef DEBUGGING_LOGGING_RENDERER_03_H
#define DEBUGGING_LOGGING_RENDERER_03_H

#include <stdint.h>
#include <stdbool.h>
#include <stddef.h>

#ifdef __cplusplus
extern "C" {
#endif

/* Forward declarations */
typedef struct debugging_logging_renderer_03 debugging_logging_renderer_03_t;
typedef struct debugging_logging_renderer_03_desc debugging_logging_renderer_03_desc_t;
typedef struct debugging_logging_renderer_03_stats debugging_logging_renderer_03_stats_t;

/* Creation and destruction */
int debugging_logging_renderer_03_create(debugging_logging_renderer_03_t** out_ctx, const debugging_logging_renderer_03_desc_t* desc);
int debugging_logging_renderer_03_destroy(debugging_logging_renderer_03_t* ctx);

/* Core operations */
int debugging_logging_renderer_03_render(debugging_logging_renderer_03_t* ctx, void* params);
int debugging_logging_renderer_03_prepare(debugging_logging_renderer_03_t* ctx, void* params);
int debugging_logging_renderer_03_bind(debugging_logging_renderer_03_t* ctx, void* params);
int debugging_logging_renderer_03_draw(debugging_logging_renderer_03_t* ctx, void* params);
int debugging_logging_renderer_03_dispatch(debugging_logging_renderer_03_t* ctx, void* params);
int debugging_logging_renderer_03_submit_commands(debugging_logging_renderer_03_t* ctx, void* params);
int debugging_logging_renderer_03_build_commands(debugging_logging_renderer_03_t* ctx, void* params);
int debugging_logging_renderer_03_sort(debugging_logging_renderer_03_t* ctx, void* params);
int debugging_logging_renderer_03_batch(debugging_logging_renderer_03_t* ctx, void* params);
int debugging_logging_renderer_03_cull(debugging_logging_renderer_03_t* ctx, void* params);

/* Utility functions */
int debugging_logging_renderer_03_get_stats(debugging_logging_renderer_03_t* ctx);
int debugging_logging_renderer_03_set_callback(debugging_logging_renderer_03_t* ctx);
int debugging_logging_renderer_03_get_memory_usage(debugging_logging_renderer_03_t* ctx);
int debugging_logging_renderer_03_optimize(debugging_logging_renderer_03_t* ctx);
int debugging_logging_renderer_03_debug_print(debugging_logging_renderer_03_t* ctx);

/* Module functions */
int debugging_logging_renderer_03_module_init(void);
int debugging_logging_renderer_03_module_shutdown(void);

#ifdef __cplusplus
}
#endif

#endif /* DEBUGGING_LOGGING_RENDERER_03_H */
