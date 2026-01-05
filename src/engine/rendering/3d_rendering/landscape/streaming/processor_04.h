/*
 * landscape_streaming_processor_04.h
 *
 * Header file for landscape_streaming_processor_04 implementation
 * Part of the Advanced 3D Rendering Engine
 */

#ifndef LANDSCAPE_STREAMING_PROCESSOR_04_H
#define LANDSCAPE_STREAMING_PROCESSOR_04_H

#include <stdint.h>
#include <stdbool.h>
#include <stddef.h>

#ifdef __cplusplus
extern "C" {
#endif

/* Forward declarations */
typedef struct landscape_streaming_processor_04 landscape_streaming_processor_04_t;
typedef struct landscape_streaming_processor_04_desc landscape_streaming_processor_04_desc_t;
typedef struct landscape_streaming_processor_04_stats landscape_streaming_processor_04_stats_t;

/* Creation and destruction */
int landscape_streaming_processor_04_create(landscape_streaming_processor_04_t** out_ctx, const landscape_streaming_processor_04_desc_t* desc);
int landscape_streaming_processor_04_destroy(landscape_streaming_processor_04_t* ctx);

/* Core operations */
int landscape_streaming_processor_04_process_batch(landscape_streaming_processor_04_t* ctx, void* params);
int landscape_streaming_processor_04_process_single(landscape_streaming_processor_04_t* ctx, void* params);
int landscape_streaming_processor_04_transform(landscape_streaming_processor_04_t* ctx, void* params);
int landscape_streaming_processor_04_filter(landscape_streaming_processor_04_t* ctx, void* params);
int landscape_streaming_processor_04_aggregate(landscape_streaming_processor_04_t* ctx, void* params);
int landscape_streaming_processor_04_dispatch(landscape_streaming_processor_04_t* ctx, void* params);
int landscape_streaming_processor_04_finalize(landscape_streaming_processor_04_t* ctx, void* params);
int landscape_streaming_processor_04_validate_input(landscape_streaming_processor_04_t* ctx, void* params);
int landscape_streaming_processor_04_optimize_output(landscape_streaming_processor_04_t* ctx, void* params);
int landscape_streaming_processor_04_profile(landscape_streaming_processor_04_t* ctx, void* params);

/* Utility functions */
int landscape_streaming_processor_04_get_stats(landscape_streaming_processor_04_t* ctx);
int landscape_streaming_processor_04_set_callback(landscape_streaming_processor_04_t* ctx);
int landscape_streaming_processor_04_get_memory_usage(landscape_streaming_processor_04_t* ctx);
int landscape_streaming_processor_04_optimize(landscape_streaming_processor_04_t* ctx);
int landscape_streaming_processor_04_debug_print(landscape_streaming_processor_04_t* ctx);

/* Module functions */
int landscape_streaming_processor_04_module_init(void);
int landscape_streaming_processor_04_module_shutdown(void);

#ifdef __cplusplus
}
#endif

#endif /* LANDSCAPE_STREAMING_PROCESSOR_04_H */
