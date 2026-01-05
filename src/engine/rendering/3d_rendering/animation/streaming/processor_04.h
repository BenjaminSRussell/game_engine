/*
 * animation_streaming_processor_04.h
 *
 * Header file for animation_streaming_processor_04 implementation
 * Part of the Advanced 3D Rendering Engine
 */

#ifndef ANIMATION_STREAMING_PROCESSOR_04_H
#define ANIMATION_STREAMING_PROCESSOR_04_H

#include <stdint.h>
#include <stdbool.h>
#include <stddef.h>

#ifdef __cplusplus
extern "C" {
#endif

/* Forward declarations */
typedef struct animation_streaming_processor_04 animation_streaming_processor_04_t;
typedef struct animation_streaming_processor_04_desc animation_streaming_processor_04_desc_t;
typedef struct animation_streaming_processor_04_stats animation_streaming_processor_04_stats_t;

/* Creation and destruction */
int animation_streaming_processor_04_create(animation_streaming_processor_04_t** out_ctx, const animation_streaming_processor_04_desc_t* desc);
int animation_streaming_processor_04_destroy(animation_streaming_processor_04_t* ctx);

/* Core operations */
int animation_streaming_processor_04_process_batch(animation_streaming_processor_04_t* ctx, void* params);
int animation_streaming_processor_04_process_single(animation_streaming_processor_04_t* ctx, void* params);
int animation_streaming_processor_04_transform(animation_streaming_processor_04_t* ctx, void* params);
int animation_streaming_processor_04_filter(animation_streaming_processor_04_t* ctx, void* params);
int animation_streaming_processor_04_aggregate(animation_streaming_processor_04_t* ctx, void* params);
int animation_streaming_processor_04_dispatch(animation_streaming_processor_04_t* ctx, void* params);
int animation_streaming_processor_04_finalize(animation_streaming_processor_04_t* ctx, void* params);
int animation_streaming_processor_04_validate_input(animation_streaming_processor_04_t* ctx, void* params);
int animation_streaming_processor_04_optimize_output(animation_streaming_processor_04_t* ctx, void* params);
int animation_streaming_processor_04_profile(animation_streaming_processor_04_t* ctx, void* params);

/* Utility functions */
int animation_streaming_processor_04_get_stats(animation_streaming_processor_04_t* ctx);
int animation_streaming_processor_04_set_callback(animation_streaming_processor_04_t* ctx);
int animation_streaming_processor_04_get_memory_usage(animation_streaming_processor_04_t* ctx);
int animation_streaming_processor_04_optimize(animation_streaming_processor_04_t* ctx);
int animation_streaming_processor_04_debug_print(animation_streaming_processor_04_t* ctx);

/* Module functions */
int animation_streaming_processor_04_module_init(void);
int animation_streaming_processor_04_module_shutdown(void);

#ifdef __cplusplus
}
#endif

#endif /* ANIMATION_STREAMING_PROCESSOR_04_H */
