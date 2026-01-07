/*
 * debugging_visualization_processor_04.h
 *
 * Header file for debugging_visualization_processor_04 implementation
 * Part of the Advanced 3D Rendering Engine
 */

#ifndef DEBUGGING_VISUALIZATION_PROCESSOR_04_H
#define DEBUGGING_VISUALIZATION_PROCESSOR_04_H

#include <stdint.h>
#include <stdbool.h>
#include <stddef.h>

#ifdef __cplusplus
extern "C" {
#endif

/* Forward declarations */
typedef struct debugging_visualization_processor_04 debugging_visualization_processor_04_t;
typedef struct debugging_visualization_processor_04_desc debugging_visualization_processor_04_desc_t;
typedef struct debugging_visualization_processor_04_stats debugging_visualization_processor_04_stats_t;

/* Creation and destruction */
int debugging_visualization_processor_04_create(debugging_visualization_processor_04_t** out_ctx, const debugging_visualization_processor_04_desc_t* desc);
int debugging_visualization_processor_04_destroy(debugging_visualization_processor_04_t* ctx);

/* Core operations */
int debugging_visualization_processor_04_process_batch(debugging_visualization_processor_04_t* ctx, void* params);
int debugging_visualization_processor_04_process_single(debugging_visualization_processor_04_t* ctx, void* params);
int debugging_visualization_processor_04_transform(debugging_visualization_processor_04_t* ctx, void* params);
int debugging_visualization_processor_04_filter(debugging_visualization_processor_04_t* ctx, void* params);
int debugging_visualization_processor_04_aggregate(debugging_visualization_processor_04_t* ctx, void* params);
int debugging_visualization_processor_04_dispatch(debugging_visualization_processor_04_t* ctx, void* params);
int debugging_visualization_processor_04_finalize(debugging_visualization_processor_04_t* ctx, void* params);
int debugging_visualization_processor_04_validate_input(debugging_visualization_processor_04_t* ctx, void* params);
int debugging_visualization_processor_04_optimize_output(debugging_visualization_processor_04_t* ctx, void* params);
int debugging_visualization_processor_04_profile(debugging_visualization_processor_04_t* ctx, void* params);

/* Utility functions */
int debugging_visualization_processor_04_get_stats(debugging_visualization_processor_04_t* ctx);
int debugging_visualization_processor_04_set_callback(debugging_visualization_processor_04_t* ctx);
int debugging_visualization_processor_04_get_memory_usage(debugging_visualization_processor_04_t* ctx);
int debugging_visualization_processor_04_optimize(debugging_visualization_processor_04_t* ctx);
int debugging_visualization_processor_04_debug_print(debugging_visualization_processor_04_t* ctx);

/* Module functions */
int debugging_visualization_processor_04_module_init(void);
int debugging_visualization_processor_04_module_shutdown(void);

#ifdef __cplusplus
}
#endif

#endif /* DEBUGGING_VISUALIZATION_PROCESSOR_04_H */
