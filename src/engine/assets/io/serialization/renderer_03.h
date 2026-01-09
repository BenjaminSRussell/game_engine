/*
 * io_serialization_renderer_03.h
 *
 * Header file for io_serialization_renderer_03 implementation
 * Part of the Advanced 3D Rendering Engine
 */

#ifndef IO_SERIALIZATION_RENDERER_03_H
#define IO_SERIALIZATION_RENDERER_03_H

#include <stdint.h>
#include <stdbool.h>
#include <stddef.h>

#ifdef __cplusplus
extern "C" {
#endif

/* Forward declarations */
typedef struct io_serialization_renderer_03 io_serialization_renderer_03_t;
typedef struct io_serialization_renderer_03_desc io_serialization_renderer_03_desc_t;
typedef struct io_serialization_renderer_03_stats io_serialization_renderer_03_stats_t;

/* Creation and destruction */
int io_serialization_renderer_03_create(io_serialization_renderer_03_t** out_ctx, const io_serialization_renderer_03_desc_t* desc);
int io_serialization_renderer_03_destroy(io_serialization_renderer_03_t* ctx);

/* Core operations */
int io_serialization_renderer_03_render(io_serialization_renderer_03_t* ctx, void* params);
int io_serialization_renderer_03_prepare(io_serialization_renderer_03_t* ctx, void* params);
int io_serialization_renderer_03_bind(io_serialization_renderer_03_t* ctx, void* params);
int io_serialization_renderer_03_draw(io_serialization_renderer_03_t* ctx, void* params);
int io_serialization_renderer_03_dispatch(io_serialization_renderer_03_t* ctx, void* params);
int io_serialization_renderer_03_submit_commands(io_serialization_renderer_03_t* ctx, void* params);
int io_serialization_renderer_03_build_commands(io_serialization_renderer_03_t* ctx, void* params);
int io_serialization_renderer_03_sort(io_serialization_renderer_03_t* ctx, void* params);
int io_serialization_renderer_03_batch(io_serialization_renderer_03_t* ctx, void* params);
int io_serialization_renderer_03_cull(io_serialization_renderer_03_t* ctx, void* params);

/* Utility functions */
int io_serialization_renderer_03_get_stats(io_serialization_renderer_03_t* ctx);
int io_serialization_renderer_03_set_callback(io_serialization_renderer_03_t* ctx);
int io_serialization_renderer_03_get_memory_usage(io_serialization_renderer_03_t* ctx);
int io_serialization_renderer_03_optimize(io_serialization_renderer_03_t* ctx);
int io_serialization_renderer_03_debug_print(io_serialization_renderer_03_t* ctx);

/* Module functions */
int io_serialization_renderer_03_module_init(void);
int io_serialization_renderer_03_module_shutdown(void);

#ifdef __cplusplus
}
#endif

#endif /* IO_SERIALIZATION_RENDERER_03_H */
