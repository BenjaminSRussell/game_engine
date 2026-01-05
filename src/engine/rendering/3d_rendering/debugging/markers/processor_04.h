/*
 * debugging_markers_processor_04.h
 *
 * Header file for debugging_markers_processor_04 implementation
 * Part of the Advanced 3D Rendering Engine
 */

#ifndef DEBUGGING_MARKERS_PROCESSOR_04_H
#define DEBUGGING_MARKERS_PROCESSOR_04_H

#include <stdint.h>
#include <stdbool.h>
#include <stddef.h>

#ifdef __cplusplus
extern "C" {
#endif

/* Forward declarations */
typedef struct debugging_markers_processor_04 debugging_markers_processor_04_t;
typedef struct debugging_markers_processor_04_desc debugging_markers_processor_04_desc_t;
typedef struct debugging_markers_processor_04_stats debugging_markers_processor_04_stats_t;

/* Creation and destruction */
int debugging_markers_processor_04_create(debugging_markers_processor_04_t** out_ctx, const debugging_markers_processor_04_desc_t* desc);
int debugging_markers_processor_04_destroy(debugging_markers_processor_04_t* ctx);

/* Core operations */
int debugging_markers_processor_04_process_batch(debugging_markers_processor_04_t* ctx, void* params);
int debugging_markers_processor_04_process_single(debugging_markers_processor_04_t* ctx, void* params);
int debugging_markers_processor_04_transform(debugging_markers_processor_04_t* ctx, void* params);
int debugging_markers_processor_04_filter(debugging_markers_processor_04_t* ctx, void* params);
int debugging_markers_processor_04_aggregate(debugging_markers_processor_04_t* ctx, void* params);
int debugging_markers_processor_04_dispatch(debugging_markers_processor_04_t* ctx, void* params);
int debugging_markers_processor_04_finalize(debugging_markers_processor_04_t* ctx, void* params);
int debugging_markers_processor_04_validate_input(debugging_markers_processor_04_t* ctx, void* params);
int debugging_markers_processor_04_optimize_output(debugging_markers_processor_04_t* ctx, void* params);
int debugging_markers_processor_04_profile(debugging_markers_processor_04_t* ctx, void* params);

/* Utility functions */
int debugging_markers_processor_04_get_stats(debugging_markers_processor_04_t* ctx);
int debugging_markers_processor_04_set_callback(debugging_markers_processor_04_t* ctx);
int debugging_markers_processor_04_get_memory_usage(debugging_markers_processor_04_t* ctx);
int debugging_markers_processor_04_optimize(debugging_markers_processor_04_t* ctx);
int debugging_markers_processor_04_debug_print(debugging_markers_processor_04_t* ctx);

/* Module functions */
int debugging_markers_processor_04_module_init(void);
int debugging_markers_processor_04_module_shutdown(void);

#ifdef __cplusplus
}
#endif

#endif /* DEBUGGING_MARKERS_PROCESSOR_04_H */
