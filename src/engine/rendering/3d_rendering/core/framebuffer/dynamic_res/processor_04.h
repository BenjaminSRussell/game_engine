/*
 * framebuffer_dynamic_res_processor_04.h
 *
 * Header file for framebuffer_dynamic_res_processor_04 implementation
 * Part of the Advanced 3D Rendering Engine
 */

#ifndef FRAMEBUFFER_DYNAMIC_RES_PROCESSOR_04_H
#define FRAMEBUFFER_DYNAMIC_RES_PROCESSOR_04_H

#include <stdint.h>
#include <stdbool.h>
#include <stddef.h>

#ifdef __cplusplus
extern "C" {
#endif

/* Forward declarations */
typedef struct framebuffer_dynamic_res_processor_04 framebuffer_dynamic_res_processor_04_t;
typedef struct framebuffer_dynamic_res_processor_04_desc framebuffer_dynamic_res_processor_04_desc_t;
typedef struct framebuffer_dynamic_res_processor_04_stats framebuffer_dynamic_res_processor_04_stats_t;

/* Creation and destruction */
int framebuffer_dynamic_res_processor_04_create(framebuffer_dynamic_res_processor_04_t** out_ctx, const framebuffer_dynamic_res_processor_04_desc_t* desc);
int framebuffer_dynamic_res_processor_04_destroy(framebuffer_dynamic_res_processor_04_t* ctx);

/* Core operations */
int framebuffer_dynamic_res_processor_04_process_batch(framebuffer_dynamic_res_processor_04_t* ctx, void* params);
int framebuffer_dynamic_res_processor_04_process_single(framebuffer_dynamic_res_processor_04_t* ctx, void* params);
int framebuffer_dynamic_res_processor_04_transform(framebuffer_dynamic_res_processor_04_t* ctx, void* params);
int framebuffer_dynamic_res_processor_04_filter(framebuffer_dynamic_res_processor_04_t* ctx, void* params);
int framebuffer_dynamic_res_processor_04_aggregate(framebuffer_dynamic_res_processor_04_t* ctx, void* params);
int framebuffer_dynamic_res_processor_04_dispatch(framebuffer_dynamic_res_processor_04_t* ctx, void* params);
int framebuffer_dynamic_res_processor_04_finalize(framebuffer_dynamic_res_processor_04_t* ctx, void* params);
int framebuffer_dynamic_res_processor_04_validate_input(framebuffer_dynamic_res_processor_04_t* ctx, void* params);
int framebuffer_dynamic_res_processor_04_optimize_output(framebuffer_dynamic_res_processor_04_t* ctx, void* params);
int framebuffer_dynamic_res_processor_04_profile(framebuffer_dynamic_res_processor_04_t* ctx, void* params);

/* Utility functions */
int framebuffer_dynamic_res_processor_04_get_stats(framebuffer_dynamic_res_processor_04_t* ctx);
int framebuffer_dynamic_res_processor_04_set_callback(framebuffer_dynamic_res_processor_04_t* ctx);
int framebuffer_dynamic_res_processor_04_get_memory_usage(framebuffer_dynamic_res_processor_04_t* ctx);
int framebuffer_dynamic_res_processor_04_optimize(framebuffer_dynamic_res_processor_04_t* ctx);
int framebuffer_dynamic_res_processor_04_debug_print(framebuffer_dynamic_res_processor_04_t* ctx);

/* Module functions */
int framebuffer_dynamic_res_processor_04_module_init(void);
int framebuffer_dynamic_res_processor_04_module_shutdown(void);

#ifdef __cplusplus
}
#endif

#endif /* FRAMEBUFFER_DYNAMIC_RES_PROCESSOR_04_H */
