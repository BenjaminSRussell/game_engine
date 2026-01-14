/*
 * io_bundling_processor_04.h
 *
 * Header file for io_bundling_processor_04 implementation
 * Part of the Advanced 3D Rendering Engine
 */

#ifndef IO_BUNDLING_PROCESSOR_04_H
#define IO_BUNDLING_PROCESSOR_04_H

#include <stdint.h>
#include <stdbool.h>
#include <stddef.h>

/* Constants */
#define IO_BUNDLING_PROCESSOR_04_MAX_CONVERTERS 16
#define IO_BUNDLING_PROCESSOR_04_MAX_QUEUE_SIZE 1024
#define IO_BUNDLING_PROCESSOR_04_MAX_OPERATIONS 64
#define IO_BUNDLING_PROCESSOR_04_SIMD_BUFFER_SIZE 65536

/* Format constants */
#define IO_BUNDLING_PROCESSOR_04_FORMAT_GLB 1
#define IO_BUNDLING_PROCESSOR_04_FORMAT_FBX 2
#define IO_BUNDLING_PROCESSOR_04_FORMAT_OBJ 3
#define IO_BUNDLING_PROCESSOR_04_FORMAT_CUSTOM 4

#ifdef __cplusplus
extern "C" {
#endif

/* Forward declarations */
typedef struct io_bundling_processor_04 io_bundling_processor_04_t;
typedef struct io_bundling_processor_04_desc io_bundling_processor_04_desc_t;
typedef struct io_bundling_processor_04_stats io_bundling_processor_04_stats_t;

/* Creation and destruction */
int io_bundling_processor_04_create(io_bundling_processor_04_t** out_ctx, const io_bundling_processor_04_desc_t* desc);
int io_bundling_processor_04_destroy(io_bundling_processor_04_t* ctx);

/* Core operations */
int io_bundling_processor_04_process_batch(io_bundling_processor_04_t* ctx, void* params);
int io_bundling_processor_04_process_single(io_bundling_processor_04_t* ctx, void* params);
int io_bundling_processor_04_transform(io_bundling_processor_04_t* ctx, void* params);
int io_bundling_processor_04_filter(io_bundling_processor_04_t* ctx, void* params);
int io_bundling_processor_04_aggregate(io_bundling_processor_04_t* ctx, void* params);
int io_bundling_processor_04_dispatch(io_bundling_processor_04_t* ctx, void* params);
int io_bundling_processor_04_finalize(io_bundling_processor_04_t* ctx, void* params);
int io_bundling_processor_04_validate_input(io_bundling_processor_04_t* ctx, void* params);
int io_bundling_processor_04_optimize_output(io_bundling_processor_04_t* ctx, void* params);
int io_bundling_processor_04_profile(io_bundling_processor_04_t* ctx, void* params);

/* Utility functions */
int io_bundling_processor_04_get_stats(io_bundling_processor_04_t* ctx);
int io_bundling_processor_04_set_callback(io_bundling_processor_04_t* ctx);
int io_bundling_processor_04_get_memory_usage(io_bundling_processor_04_t* ctx);
int io_bundling_processor_04_optimize(io_bundling_processor_04_t* ctx);
int io_bundling_processor_04_debug_print(io_bundling_processor_04_t* ctx);

/* Module functions */
int io_bundling_processor_04_module_init(void);
int io_bundling_processor_04_module_shutdown(void);

#ifdef __cplusplus
}
#endif

#endif /* IO_BUNDLING_PROCESSOR_04_H */
