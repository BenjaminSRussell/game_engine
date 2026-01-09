/*
 * io_export_renderer_03.h
 *
 * Header file for io_export_renderer_03 implementation
 * Part of the Advanced 3D Rendering Engine
 */

#ifndef IO_EXPORT_RENDERER_03_H
#define IO_EXPORT_RENDERER_03_H

#include <stdint.h>
#include <stdbool.h>
#include <stddef.h>

#ifdef __cplusplus
extern "C" {
#endif

/* Forward declarations */
typedef struct io_export_renderer_03 io_export_renderer_03_t;
typedef struct io_export_renderer_03_desc io_export_renderer_03_desc_t;
typedef struct io_export_renderer_03_stats io_export_renderer_03_stats_t;

/* Creation and destruction */
int io_export_renderer_03_create(io_export_renderer_03_t** out_ctx, const io_export_renderer_03_desc_t* desc);
int io_export_renderer_03_destroy(io_export_renderer_03_t* ctx);

/* Core operations */
int io_export_renderer_03_render(io_export_renderer_03_t* ctx, void* params);
int io_export_renderer_03_prepare(io_export_renderer_03_t* ctx, void* params);
int io_export_renderer_03_bind(io_export_renderer_03_t* ctx, void* params);
int io_export_renderer_03_draw(io_export_renderer_03_t* ctx, void* params);
int io_export_renderer_03_dispatch(io_export_renderer_03_t* ctx, void* params);
int io_export_renderer_03_submit_commands(io_export_renderer_03_t* ctx, void* params);
int io_export_renderer_03_build_commands(io_export_renderer_03_t* ctx, void* params);
int io_export_renderer_03_sort(io_export_renderer_03_t* ctx, void* params);
int io_export_renderer_03_batch(io_export_renderer_03_t* ctx, void* params);
int io_export_renderer_03_cull(io_export_renderer_03_t* ctx, void* params);

/* Utility functions */
int io_export_renderer_03_get_stats(io_export_renderer_03_t* ctx);
int io_export_renderer_03_set_callback(io_export_renderer_03_t* ctx);
int io_export_renderer_03_get_memory_usage(io_export_renderer_03_t* ctx);
int io_export_renderer_03_optimize(io_export_renderer_03_t* ctx);
int io_export_renderer_03_debug_print(io_export_renderer_03_t* ctx);

/* Module functions */
int io_export_renderer_03_module_init(void);
int io_export_renderer_03_module_shutdown(void);

#ifdef __cplusplus
}
#endif

#endif /* IO_EXPORT_RENDERER_03_H */
