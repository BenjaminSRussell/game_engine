/*
 * viewports_cinematic_processor_04.h
 *
 * Header file for viewports_cinematic_processor_04 implementation
 * Part of the Advanced 3D Rendering Engine
 */

#ifndef VIEWPORTS_CINEMATIC_PROCESSOR_04_H
#define VIEWPORTS_CINEMATIC_PROCESSOR_04_H

#include <stdint.h>
#include <stdbool.h>
#include <stddef.h>

#ifdef __cplusplus
extern "C" {
#endif

/* Forward declarations */
typedef struct viewports_cinematic_processor_04 viewports_cinematic_processor_04_t;
typedef struct viewports_cinematic_processor_04_desc viewports_cinematic_processor_04_desc_t;
typedef struct viewports_cinematic_processor_04_stats viewports_cinematic_processor_04_stats_t;

/* Creation and destruction */
int viewports_cinematic_processor_04_create(viewports_cinematic_processor_04_t** out_ctx, const viewports_cinematic_processor_04_desc_t* desc);
int viewports_cinematic_processor_04_destroy(viewports_cinematic_processor_04_t* ctx);

/* Core operations */
int viewports_cinematic_processor_04_process_batch(viewports_cinematic_processor_04_t* ctx, void* params);
int viewports_cinematic_processor_04_process_single(viewports_cinematic_processor_04_t* ctx, void* params);
int viewports_cinematic_processor_04_transform(viewports_cinematic_processor_04_t* ctx, void* params);
int viewports_cinematic_processor_04_filter(viewports_cinematic_processor_04_t* ctx, void* params);
int viewports_cinematic_processor_04_aggregate(viewports_cinematic_processor_04_t* ctx, void* params);
int viewports_cinematic_processor_04_dispatch(viewports_cinematic_processor_04_t* ctx, void* params);
int viewports_cinematic_processor_04_finalize(viewports_cinematic_processor_04_t* ctx, void* params);
int viewports_cinematic_processor_04_validate_input(viewports_cinematic_processor_04_t* ctx, void* params);
int viewports_cinematic_processor_04_optimize_output(viewports_cinematic_processor_04_t* ctx, void* params);
int viewports_cinematic_processor_04_profile(viewports_cinematic_processor_04_t* ctx, void* params);

/* Utility functions */
int viewports_cinematic_processor_04_get_stats(viewports_cinematic_processor_04_t* ctx);
int viewports_cinematic_processor_04_set_callback(viewports_cinematic_processor_04_t* ctx);
int viewports_cinematic_processor_04_get_memory_usage(viewports_cinematic_processor_04_t* ctx);
int viewports_cinematic_processor_04_optimize(viewports_cinematic_processor_04_t* ctx);
int viewports_cinematic_processor_04_debug_print(viewports_cinematic_processor_04_t* ctx);

/* Module functions */
int viewports_cinematic_processor_04_module_init(void);
int viewports_cinematic_processor_04_module_shutdown(void);

#ifdef __cplusplus
}
#endif

#endif /* VIEWPORTS_CINEMATIC_PROCESSOR_04_H */
