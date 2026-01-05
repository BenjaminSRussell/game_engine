/*
 * culling_streaming_processor_04.h
 *
 * Header file for culling_streaming_processor_04 implementation
 * Part of the Advanced 3D Rendering Engine
 */

#ifndef CULLING_STREAMING_PROCESSOR_04_H
#define CULLING_STREAMING_PROCESSOR_04_H

#include <stdint.h>
#include <stdbool.h>
#include <stddef.h>

#ifdef __cplusplus
extern "C" {
#endif

/* Forward declarations */
typedef struct culling_streaming_processor_04 culling_streaming_processor_04_t;
typedef struct culling_streaming_processor_04_desc culling_streaming_processor_04_desc_t;
typedef struct culling_streaming_processor_04_stats culling_streaming_processor_04_stats_t;

/* Creation and destruction */
int culling_streaming_processor_04_create(culling_streaming_processor_04_t** out_ctx, const culling_streaming_processor_04_desc_t* desc);
int culling_streaming_processor_04_destroy(culling_streaming_processor_04_t* ctx);

/* Core operations */
int culling_streaming_processor_04_process_batch(culling_streaming_processor_04_t* ctx, void* params);
int culling_streaming_processor_04_process_single(culling_streaming_processor_04_t* ctx, void* params);
int culling_streaming_processor_04_transform(culling_streaming_processor_04_t* ctx, void* params);
int culling_streaming_processor_04_filter(culling_streaming_processor_04_t* ctx, void* params);
int culling_streaming_processor_04_aggregate(culling_streaming_processor_04_t* ctx, void* params);
int culling_streaming_processor_04_dispatch(culling_streaming_processor_04_t* ctx, void* params);
int culling_streaming_processor_04_finalize(culling_streaming_processor_04_t* ctx, void* params);
int culling_streaming_processor_04_validate_input(culling_streaming_processor_04_t* ctx, void* params);
int culling_streaming_processor_04_optimize_output(culling_streaming_processor_04_t* ctx, void* params);
int culling_streaming_processor_04_profile(culling_streaming_processor_04_t* ctx, void* params);

/* Utility functions */
int culling_streaming_processor_04_get_stats(culling_streaming_processor_04_t* ctx);
int culling_streaming_processor_04_set_callback(culling_streaming_processor_04_t* ctx);
int culling_streaming_processor_04_get_memory_usage(culling_streaming_processor_04_t* ctx);
int culling_streaming_processor_04_optimize(culling_streaming_processor_04_t* ctx);
int culling_streaming_processor_04_debug_print(culling_streaming_processor_04_t* ctx);

/* Module functions */
int culling_streaming_processor_04_module_init(void);
int culling_streaming_processor_04_module_shutdown(void);

#ifdef __cplusplus
}
#endif

#endif /* CULLING_STREAMING_PROCESSOR_04_H */
