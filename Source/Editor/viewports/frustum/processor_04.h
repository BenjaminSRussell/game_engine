/*
 * viewports_frustum_processor_04.h
 *
 * Header file for viewports_frustum_processor_04 implementation
 * Part of the Advanced 3D Rendering Engine
 */

#ifndef VIEWPORTS_FRUSTUM_PROCESSOR_04_H
#define VIEWPORTS_FRUSTUM_PROCESSOR_04_H

#include <stdint.h>
#include <stdbool.h>
#include <stddef.h>

#ifdef __cplusplus
extern "C" {
#endif

/* Forward declarations */
typedef struct viewports_frustum_processor_04 viewports_frustum_processor_04_t;
typedef struct viewports_frustum_processor_04_desc viewports_frustum_processor_04_desc_t;
typedef struct viewports_frustum_processor_04_stats viewports_frustum_processor_04_stats_t;

/* Creation and destruction */
int viewports_frustum_processor_04_create(viewports_frustum_processor_04_t** out_ctx, const viewports_frustum_processor_04_desc_t* desc);
int viewports_frustum_processor_04_destroy(viewports_frustum_processor_04_t* ctx);

/* Core operations */
int viewports_frustum_processor_04_process_batch(viewports_frustum_processor_04_t* ctx, void* params);
int viewports_frustum_processor_04_process_single(viewports_frustum_processor_04_t* ctx, void* params);
int viewports_frustum_processor_04_transform(viewports_frustum_processor_04_t* ctx, void* params);
int viewports_frustum_processor_04_filter(viewports_frustum_processor_04_t* ctx, void* params);
int viewports_frustum_processor_04_aggregate(viewports_frustum_processor_04_t* ctx, void* params);
int viewports_frustum_processor_04_dispatch(viewports_frustum_processor_04_t* ctx, void* params);
int viewports_frustum_processor_04_finalize(viewports_frustum_processor_04_t* ctx, void* params);
int viewports_frustum_processor_04_validate_input(viewports_frustum_processor_04_t* ctx, void* params);
int viewports_frustum_processor_04_optimize_output(viewports_frustum_processor_04_t* ctx, void* params);
int viewports_frustum_processor_04_profile(viewports_frustum_processor_04_t* ctx, void* params);

/* Utility functions */
int viewports_frustum_processor_04_get_stats(viewports_frustum_processor_04_t* ctx);
int viewports_frustum_processor_04_set_callback(viewports_frustum_processor_04_t* ctx);
int viewports_frustum_processor_04_get_memory_usage(viewports_frustum_processor_04_t* ctx);
int viewports_frustum_processor_04_optimize(viewports_frustum_processor_04_t* ctx);
int viewports_frustum_processor_04_debug_print(viewports_frustum_processor_04_t* ctx);

/* Module functions */
int viewports_frustum_processor_04_module_init(void);
int viewports_frustum_processor_04_module_shutdown(void);

#ifdef __cplusplus
}
#endif

#endif /* VIEWPORTS_FRUSTUM_PROCESSOR_04_H */
