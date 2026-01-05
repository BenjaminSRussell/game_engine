/*
 * framebuffer_multiview_processor_04.h
 *
 * Header file for framebuffer_multiview_processor_04 implementation
 * Part of the Advanced 3D Rendering Engine
 */

#ifndef FRAMEBUFFER_MULTIVIEW_PROCESSOR_04_H
#define FRAMEBUFFER_MULTIVIEW_PROCESSOR_04_H

#include <stdint.h>
#include <stdbool.h>
#include <stddef.h>

#ifdef __cplusplus
extern "C" {
#endif

/* Forward declarations */
typedef struct framebuffer_multiview_processor_04 framebuffer_multiview_processor_04_t;
typedef struct framebuffer_multiview_processor_04_desc framebuffer_multiview_processor_04_desc_t;
typedef struct framebuffer_multiview_processor_04_stats framebuffer_multiview_processor_04_stats_t;

/* Creation and destruction */
int framebuffer_multiview_processor_04_create(framebuffer_multiview_processor_04_t** out_ctx, const framebuffer_multiview_processor_04_desc_t* desc);
int framebuffer_multiview_processor_04_destroy(framebuffer_multiview_processor_04_t* ctx);

/* Core operations */
int framebuffer_multiview_processor_04_process_batch(framebuffer_multiview_processor_04_t* ctx, void* params);
int framebuffer_multiview_processor_04_process_single(framebuffer_multiview_processor_04_t* ctx, void* params);
int framebuffer_multiview_processor_04_transform(framebuffer_multiview_processor_04_t* ctx, void* params);
int framebuffer_multiview_processor_04_filter(framebuffer_multiview_processor_04_t* ctx, void* params);
int framebuffer_multiview_processor_04_aggregate(framebuffer_multiview_processor_04_t* ctx, void* params);
int framebuffer_multiview_processor_04_dispatch(framebuffer_multiview_processor_04_t* ctx, void* params);
int framebuffer_multiview_processor_04_finalize(framebuffer_multiview_processor_04_t* ctx, void* params);
int framebuffer_multiview_processor_04_validate_input(framebuffer_multiview_processor_04_t* ctx, void* params);
int framebuffer_multiview_processor_04_optimize_output(framebuffer_multiview_processor_04_t* ctx, void* params);
int framebuffer_multiview_processor_04_profile(framebuffer_multiview_processor_04_t* ctx, void* params);

/* Utility functions */
int framebuffer_multiview_processor_04_get_stats(framebuffer_multiview_processor_04_t* ctx);
int framebuffer_multiview_processor_04_set_callback(framebuffer_multiview_processor_04_t* ctx);
int framebuffer_multiview_processor_04_get_memory_usage(framebuffer_multiview_processor_04_t* ctx);
int framebuffer_multiview_processor_04_optimize(framebuffer_multiview_processor_04_t* ctx);
int framebuffer_multiview_processor_04_debug_print(framebuffer_multiview_processor_04_t* ctx);

/* Module functions */
int framebuffer_multiview_processor_04_module_init(void);
int framebuffer_multiview_processor_04_module_shutdown(void);

#ifdef __cplusplus
}
#endif

#endif /* FRAMEBUFFER_MULTIVIEW_PROCESSOR_04_H */
