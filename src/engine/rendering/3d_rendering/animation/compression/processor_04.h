/*
 * animation_compression_processor_04.h
 *
 * Header file for animation_compression_processor_04 implementation
 * Part of the Advanced 3D Rendering Engine
 */

#ifndef ANIMATION_COMPRESSION_PROCESSOR_04_H
#define ANIMATION_COMPRESSION_PROCESSOR_04_H

#include <stdint.h>
#include <stdbool.h>
#include <stddef.h>

#ifdef __cplusplus
extern "C" {
#endif

/* Forward declarations */
typedef struct animation_compression_processor_04 animation_compression_processor_04_t;
typedef struct animation_compression_processor_04_desc animation_compression_processor_04_desc_t;
typedef struct animation_compression_processor_04_stats animation_compression_processor_04_stats_t;

/* Creation and destruction */
int animation_compression_processor_04_create(animation_compression_processor_04_t** out_ctx, const animation_compression_processor_04_desc_t* desc);
int animation_compression_processor_04_destroy(animation_compression_processor_04_t* ctx);

/* Core operations */
int animation_compression_processor_04_process_batch(animation_compression_processor_04_t* ctx, void* params);
int animation_compression_processor_04_process_single(animation_compression_processor_04_t* ctx, void* params);
int animation_compression_processor_04_transform(animation_compression_processor_04_t* ctx, void* params);
int animation_compression_processor_04_filter(animation_compression_processor_04_t* ctx, void* params);
int animation_compression_processor_04_aggregate(animation_compression_processor_04_t* ctx, void* params);
int animation_compression_processor_04_dispatch(animation_compression_processor_04_t* ctx, void* params);
int animation_compression_processor_04_finalize(animation_compression_processor_04_t* ctx, void* params);
int animation_compression_processor_04_validate_input(animation_compression_processor_04_t* ctx, void* params);
int animation_compression_processor_04_optimize_output(animation_compression_processor_04_t* ctx, void* params);
int animation_compression_processor_04_profile(animation_compression_processor_04_t* ctx, void* params);

/* Utility functions */
int animation_compression_processor_04_get_stats(animation_compression_processor_04_t* ctx);
int animation_compression_processor_04_set_callback(animation_compression_processor_04_t* ctx);
int animation_compression_processor_04_get_memory_usage(animation_compression_processor_04_t* ctx);
int animation_compression_processor_04_optimize(animation_compression_processor_04_t* ctx);
int animation_compression_processor_04_debug_print(animation_compression_processor_04_t* ctx);

/* Module functions */
int animation_compression_processor_04_module_init(void);
int animation_compression_processor_04_module_shutdown(void);

#ifdef __cplusplus
}
#endif

#endif /* ANIMATION_COMPRESSION_PROCESSOR_04_H */
