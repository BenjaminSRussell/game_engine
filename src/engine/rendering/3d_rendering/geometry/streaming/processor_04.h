/*
 * geometry_streaming_processor_04.h
 *
 * Header file for geometry_streaming_processor_04 implementation
 * Part of the Advanced 3D Rendering Engine
 */

#ifndef GEOMETRY_STREAMING_PROCESSOR_04_H
#define GEOMETRY_STREAMING_PROCESSOR_04_H

#include <stdint.h>
#include <stdbool.h>
#include <stddef.h>

#ifdef __cplusplus
extern "C" {
#endif

/* Forward declarations */
typedef struct geometry_streaming_processor_04 geometry_streaming_processor_04_t;
typedef struct geometry_streaming_processor_04_desc geometry_streaming_processor_04_desc_t;
typedef struct geometry_streaming_processor_04_stats geometry_streaming_processor_04_stats_t;

/* Creation and destruction */
int geometry_streaming_processor_04_create(geometry_streaming_processor_04_t** out_ctx, const geometry_streaming_processor_04_desc_t* desc);
int geometry_streaming_processor_04_destroy(geometry_streaming_processor_04_t* ctx);

/* Core operations */
int geometry_streaming_processor_04_process_batch(geometry_streaming_processor_04_t* ctx, void* params);
int geometry_streaming_processor_04_process_single(geometry_streaming_processor_04_t* ctx, void* params);
int geometry_streaming_processor_04_transform(geometry_streaming_processor_04_t* ctx, void* params);
int geometry_streaming_processor_04_filter(geometry_streaming_processor_04_t* ctx, void* params);
int geometry_streaming_processor_04_aggregate(geometry_streaming_processor_04_t* ctx, void* params);
int geometry_streaming_processor_04_dispatch(geometry_streaming_processor_04_t* ctx, void* params);
int geometry_streaming_processor_04_finalize(geometry_streaming_processor_04_t* ctx, void* params);
int geometry_streaming_processor_04_validate_input(geometry_streaming_processor_04_t* ctx, void* params);
int geometry_streaming_processor_04_optimize_output(geometry_streaming_processor_04_t* ctx, void* params);
int geometry_streaming_processor_04_profile(geometry_streaming_processor_04_t* ctx, void* params);

/* Utility functions */
int geometry_streaming_processor_04_get_stats(geometry_streaming_processor_04_t* ctx);
int geometry_streaming_processor_04_set_callback(geometry_streaming_processor_04_t* ctx);
int geometry_streaming_processor_04_get_memory_usage(geometry_streaming_processor_04_t* ctx);
int geometry_streaming_processor_04_optimize(geometry_streaming_processor_04_t* ctx);
int geometry_streaming_processor_04_debug_print(geometry_streaming_processor_04_t* ctx);

/* Module functions */
int geometry_streaming_processor_04_module_init(void);
int geometry_streaming_processor_04_module_shutdown(void);

#ifdef __cplusplus
}
#endif

#endif /* GEOMETRY_STREAMING_PROCESSOR_04_H */
