/*
 * materials_graph_processor_04.h
 *
 * Header file for materials_graph_processor_04 implementation
 * Part of the Advanced 3D Rendering Engine
 */

#ifndef MATERIALS_GRAPH_PROCESSOR_04_H
#define MATERIALS_GRAPH_PROCESSOR_04_H

#include <stdint.h>
#include <stdbool.h>
#include <stddef.h>

#ifdef __cplusplus
extern "C" {
#endif

/* Forward declarations */
typedef struct materials_graph_processor_04 materials_graph_processor_04_t;
typedef struct materials_graph_processor_04_desc materials_graph_processor_04_desc_t;
typedef struct materials_graph_processor_04_stats materials_graph_processor_04_stats_t;

/* Creation and destruction */
int materials_graph_processor_04_create(materials_graph_processor_04_t** out_ctx, const materials_graph_processor_04_desc_t* desc);
int materials_graph_processor_04_destroy(materials_graph_processor_04_t* ctx);

/* Core operations */
int materials_graph_processor_04_process_batch(materials_graph_processor_04_t* ctx, void* params);
int materials_graph_processor_04_process_single(materials_graph_processor_04_t* ctx, void* params);
int materials_graph_processor_04_transform(materials_graph_processor_04_t* ctx, void* params);
int materials_graph_processor_04_filter(materials_graph_processor_04_t* ctx, void* params);
int materials_graph_processor_04_aggregate(materials_graph_processor_04_t* ctx, void* params);
int materials_graph_processor_04_dispatch(materials_graph_processor_04_t* ctx, void* params);
int materials_graph_processor_04_finalize(materials_graph_processor_04_t* ctx, void* params);
int materials_graph_processor_04_validate_input(materials_graph_processor_04_t* ctx, void* params);
int materials_graph_processor_04_optimize_output(materials_graph_processor_04_t* ctx, void* params);
int materials_graph_processor_04_profile(materials_graph_processor_04_t* ctx, void* params);

/* Utility functions */
int materials_graph_processor_04_get_stats(materials_graph_processor_04_t* ctx);
int materials_graph_processor_04_set_callback(materials_graph_processor_04_t* ctx);
int materials_graph_processor_04_get_memory_usage(materials_graph_processor_04_t* ctx);
int materials_graph_processor_04_optimize(materials_graph_processor_04_t* ctx);
int materials_graph_processor_04_debug_print(materials_graph_processor_04_t* ctx);

/* Module functions */
int materials_graph_processor_04_module_init(void);
int materials_graph_processor_04_module_shutdown(void);

#ifdef __cplusplus
}
#endif

#endif /* MATERIALS_GRAPH_PROCESSOR_04_H */
