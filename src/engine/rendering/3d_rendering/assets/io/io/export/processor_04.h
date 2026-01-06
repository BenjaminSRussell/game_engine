/*
 * io_export_processor_04.h
 *
 * Header file for io_export_processor_04 implementation
 * Part of the Advanced 3D Rendering Engine
 */

#ifndef IO_EXPORT_PROCESSOR_04_H
#define IO_EXPORT_PROCESSOR_04_H

#include <stdint.h>
#include <stdbool.h>
#include <stddef.h>

#ifdef __cplusplus
extern "C" {
#endif

/* Forward declarations */
typedef struct io_export_processor_04 io_export_processor_04_t;
typedef struct io_export_processor_04_desc io_export_processor_04_desc_t;
typedef struct io_export_processor_04_stats io_export_processor_04_stats_t;

/* Creation and destruction */
int io_export_processor_04_create(io_export_processor_04_t** out_ctx, const io_export_processor_04_desc_t* desc);
int io_export_processor_04_destroy(io_export_processor_04_t* ctx);

/* Core operations */
int io_export_processor_04_process_batch(io_export_processor_04_t* ctx, void* params);
int io_export_processor_04_process_single(io_export_processor_04_t* ctx, void* params);
int io_export_processor_04_transform(io_export_processor_04_t* ctx, void* params);
int io_export_processor_04_filter(io_export_processor_04_t* ctx, void* params);
int io_export_processor_04_aggregate(io_export_processor_04_t* ctx, void* params);
int io_export_processor_04_dispatch(io_export_processor_04_t* ctx, void* params);
int io_export_processor_04_finalize(io_export_processor_04_t* ctx, void* params);
int io_export_processor_04_validate_input(io_export_processor_04_t* ctx, void* params);
int io_export_processor_04_optimize_output(io_export_processor_04_t* ctx, void* params);
int io_export_processor_04_profile(io_export_processor_04_t* ctx, void* params);

/* Utility functions */
int io_export_processor_04_get_stats(io_export_processor_04_t* ctx);
int io_export_processor_04_set_callback(io_export_processor_04_t* ctx);
int io_export_processor_04_get_memory_usage(io_export_processor_04_t* ctx);
int io_export_processor_04_optimize(io_export_processor_04_t* ctx);
int io_export_processor_04_debug_print(io_export_processor_04_t* ctx);

/* Module functions */
int io_export_processor_04_module_init(void);
int io_export_processor_04_module_shutdown(void);

#ifdef __cplusplus
}
#endif

#endif /* IO_EXPORT_PROCESSOR_04_H */
