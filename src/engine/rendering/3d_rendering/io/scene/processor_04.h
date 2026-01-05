/*
 * io_scene_processor_04.h
 *
 * Header file for io_scene_processor_04 implementation
 * Part of the Advanced 3D Rendering Engine
 */

#ifndef IO_SCENE_PROCESSOR_04_H
#define IO_SCENE_PROCESSOR_04_H

#include <stdint.h>
#include <stdbool.h>
#include <stddef.h>

#ifdef __cplusplus
extern "C" {
#endif

/* Forward declarations */
typedef struct io_scene_processor_04 io_scene_processor_04_t;
typedef struct io_scene_processor_04_desc io_scene_processor_04_desc_t;
typedef struct io_scene_processor_04_stats io_scene_processor_04_stats_t;

/* Creation and destruction */
int io_scene_processor_04_create(io_scene_processor_04_t** out_ctx, const io_scene_processor_04_desc_t* desc);
int io_scene_processor_04_destroy(io_scene_processor_04_t* ctx);

/* Core operations */
int io_scene_processor_04_process_batch(io_scene_processor_04_t* ctx, void* params);
int io_scene_processor_04_process_single(io_scene_processor_04_t* ctx, void* params);
int io_scene_processor_04_transform(io_scene_processor_04_t* ctx, void* params);
int io_scene_processor_04_filter(io_scene_processor_04_t* ctx, void* params);
int io_scene_processor_04_aggregate(io_scene_processor_04_t* ctx, void* params);
int io_scene_processor_04_dispatch(io_scene_processor_04_t* ctx, void* params);
int io_scene_processor_04_finalize(io_scene_processor_04_t* ctx, void* params);
int io_scene_processor_04_validate_input(io_scene_processor_04_t* ctx, void* params);
int io_scene_processor_04_optimize_output(io_scene_processor_04_t* ctx, void* params);
int io_scene_processor_04_profile(io_scene_processor_04_t* ctx, void* params);

/* Utility functions */
int io_scene_processor_04_get_stats(io_scene_processor_04_t* ctx);
int io_scene_processor_04_set_callback(io_scene_processor_04_t* ctx);
int io_scene_processor_04_get_memory_usage(io_scene_processor_04_t* ctx);
int io_scene_processor_04_optimize(io_scene_processor_04_t* ctx);
int io_scene_processor_04_debug_print(io_scene_processor_04_t* ctx);

/* Module functions */
int io_scene_processor_04_module_init(void);
int io_scene_processor_04_module_shutdown(void);

#ifdef __cplusplus
}
#endif

#endif /* IO_SCENE_PROCESSOR_04_H */
