/*
 * rendering_nanite_processor_04.h
 *
 * Header file for rendering_nanite_processor_04 implementation
 * Part of the Advanced 3D Rendering Engine
 */

#ifndef RENDERING_NANITE_PROCESSOR_04_H
#define RENDERING_NANITE_PROCESSOR_04_H

#include <stdint.h>
#include <stdbool.h>
#include <stddef.h>

#ifdef __cplusplus
extern "C" {
#endif

/* Forward declarations */
typedef struct rendering_nanite_processor_04 rendering_nanite_processor_04_t;
typedef struct rendering_nanite_processor_04_desc rendering_nanite_processor_04_desc_t;
typedef struct rendering_nanite_processor_04_stats rendering_nanite_processor_04_stats_t;

/* Creation and destruction */
int rendering_nanite_processor_04_create(rendering_nanite_processor_04_t** out_ctx, const rendering_nanite_processor_04_desc_t* desc);
int rendering_nanite_processor_04_destroy(rendering_nanite_processor_04_t* ctx);

/* Core operations */
int rendering_nanite_processor_04_process_batch(rendering_nanite_processor_04_t* ctx, void* params);
int rendering_nanite_processor_04_process_single(rendering_nanite_processor_04_t* ctx, void* params);
int rendering_nanite_processor_04_transform(rendering_nanite_processor_04_t* ctx, void* params);
int rendering_nanite_processor_04_filter(rendering_nanite_processor_04_t* ctx, void* params);
int rendering_nanite_processor_04_aggregate(rendering_nanite_processor_04_t* ctx, void* params);
int rendering_nanite_processor_04_dispatch(rendering_nanite_processor_04_t* ctx, void* params);
int rendering_nanite_processor_04_finalize(rendering_nanite_processor_04_t* ctx, void* params);
int rendering_nanite_processor_04_validate_input(rendering_nanite_processor_04_t* ctx, void* params);
int rendering_nanite_processor_04_optimize_output(rendering_nanite_processor_04_t* ctx, void* params);
int rendering_nanite_processor_04_profile(rendering_nanite_processor_04_t* ctx, void* params);

/* Utility functions */
int rendering_nanite_processor_04_get_stats(rendering_nanite_processor_04_t* ctx);
int rendering_nanite_processor_04_set_callback(rendering_nanite_processor_04_t* ctx);
int rendering_nanite_processor_04_get_memory_usage(rendering_nanite_processor_04_t* ctx);
int rendering_nanite_processor_04_optimize(rendering_nanite_processor_04_t* ctx);
int rendering_nanite_processor_04_debug_print(rendering_nanite_processor_04_t* ctx);

/* Module functions */
int rendering_nanite_processor_04_module_init(void);
int rendering_nanite_processor_04_module_shutdown(void);

#ifdef __cplusplus
}
#endif

#endif /* RENDERING_NANITE_PROCESSOR_04_H */
