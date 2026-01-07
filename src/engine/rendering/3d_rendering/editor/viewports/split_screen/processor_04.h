/*
 * viewports_split_screen_processor_04.h
 *
 * Header file for viewports_split_screen_processor_04 implementation
 * Part of the Advanced 3D Rendering Engine
 */

#ifndef VIEWPORTS_SPLIT_SCREEN_PROCESSOR_04_H
#define VIEWPORTS_SPLIT_SCREEN_PROCESSOR_04_H

#include <stdint.h>
#include <stdbool.h>
#include <stddef.h>

#ifdef __cplusplus
extern "C" {
#endif

/* Forward declarations */
typedef struct viewports_split_screen_processor_04 viewports_split_screen_processor_04_t;
typedef struct viewports_split_screen_processor_04_desc viewports_split_screen_processor_04_desc_t;
typedef struct viewports_split_screen_processor_04_stats viewports_split_screen_processor_04_stats_t;

/* Creation and destruction */
int viewports_split_screen_processor_04_create(viewports_split_screen_processor_04_t** out_ctx, const viewports_split_screen_processor_04_desc_t* desc);
int viewports_split_screen_processor_04_destroy(viewports_split_screen_processor_04_t* ctx);

/* Core operations */
int viewports_split_screen_processor_04_process_batch(viewports_split_screen_processor_04_t* ctx, void* params);
int viewports_split_screen_processor_04_process_single(viewports_split_screen_processor_04_t* ctx, void* params);
int viewports_split_screen_processor_04_transform(viewports_split_screen_processor_04_t* ctx, void* params);
int viewports_split_screen_processor_04_filter(viewports_split_screen_processor_04_t* ctx, void* params);
int viewports_split_screen_processor_04_aggregate(viewports_split_screen_processor_04_t* ctx, void* params);
int viewports_split_screen_processor_04_dispatch(viewports_split_screen_processor_04_t* ctx, void* params);
int viewports_split_screen_processor_04_finalize(viewports_split_screen_processor_04_t* ctx, void* params);
int viewports_split_screen_processor_04_validate_input(viewports_split_screen_processor_04_t* ctx, void* params);
int viewports_split_screen_processor_04_optimize_output(viewports_split_screen_processor_04_t* ctx, void* params);
int viewports_split_screen_processor_04_profile(viewports_split_screen_processor_04_t* ctx, void* params);

/* Utility functions */
int viewports_split_screen_processor_04_get_stats(viewports_split_screen_processor_04_t* ctx);
int viewports_split_screen_processor_04_set_callback(viewports_split_screen_processor_04_t* ctx);
int viewports_split_screen_processor_04_get_memory_usage(viewports_split_screen_processor_04_t* ctx);
int viewports_split_screen_processor_04_optimize(viewports_split_screen_processor_04_t* ctx);
int viewports_split_screen_processor_04_debug_print(viewports_split_screen_processor_04_t* ctx);

/* Module functions */
int viewports_split_screen_processor_04_module_init(void);
int viewports_split_screen_processor_04_module_shutdown(void);

#ifdef __cplusplus
}
#endif

#endif /* VIEWPORTS_SPLIT_SCREEN_PROCESSOR_04_H */
