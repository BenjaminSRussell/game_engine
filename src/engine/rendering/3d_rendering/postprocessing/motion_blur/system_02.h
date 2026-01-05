/*
 * postprocessing_motion_blur_system_02.h
 *
 * Header file for postprocessing_motion_blur_system_02 implementation
 * Part of the Advanced 3D Rendering Engine
 */

#ifndef POSTPROCESSING_MOTION_BLUR_SYSTEM_02_H
#define POSTPROCESSING_MOTION_BLUR_SYSTEM_02_H

#include <stdint.h>
#include <stdbool.h>
#include <stddef.h>

#ifdef __cplusplus
extern "C" {
#endif

/* Forward declarations */
typedef struct postprocessing_motion_blur_system_02 postprocessing_motion_blur_system_02_t;
typedef struct postprocessing_motion_blur_system_02_desc postprocessing_motion_blur_system_02_desc_t;
typedef struct postprocessing_motion_blur_system_02_stats postprocessing_motion_blur_system_02_stats_t;

/* Creation and destruction */
int postprocessing_motion_blur_system_02_create(postprocessing_motion_blur_system_02_t** out_ctx, const postprocessing_motion_blur_system_02_desc_t* desc);
int postprocessing_motion_blur_system_02_destroy(postprocessing_motion_blur_system_02_t* ctx);

/* Core operations */
int postprocessing_motion_blur_system_02_create_system(postprocessing_motion_blur_system_02_t* ctx, void* params);
int postprocessing_motion_blur_system_02_destroy_system(postprocessing_motion_blur_system_02_t* ctx, void* params);
int postprocessing_motion_blur_system_02_tick(postprocessing_motion_blur_system_02_t* ctx, void* params);
int postprocessing_motion_blur_system_02_process(postprocessing_motion_blur_system_02_t* ctx, void* params);
int postprocessing_motion_blur_system_02_submit(postprocessing_motion_blur_system_02_t* ctx, void* params);
int postprocessing_motion_blur_system_02_execute(postprocessing_motion_blur_system_02_t* ctx, void* params);
int postprocessing_motion_blur_system_02_sync(postprocessing_motion_blur_system_02_t* ctx, void* params);
int postprocessing_motion_blur_system_02_query(postprocessing_motion_blur_system_02_t* ctx, void* params);
int postprocessing_motion_blur_system_02_configure(postprocessing_motion_blur_system_02_t* ctx, void* params);
int postprocessing_motion_blur_system_02_optimize(postprocessing_motion_blur_system_02_t* ctx, void* params);

/* Utility functions */
int postprocessing_motion_blur_system_02_get_stats(postprocessing_motion_blur_system_02_t* ctx);
int postprocessing_motion_blur_system_02_set_callback(postprocessing_motion_blur_system_02_t* ctx);
int postprocessing_motion_blur_system_02_get_memory_usage(postprocessing_motion_blur_system_02_t* ctx);
int postprocessing_motion_blur_system_02_optimize(postprocessing_motion_blur_system_02_t* ctx);
int postprocessing_motion_blur_system_02_debug_print(postprocessing_motion_blur_system_02_t* ctx);

/* Module functions */
int postprocessing_motion_blur_system_02_module_init(void);
int postprocessing_motion_blur_system_02_module_shutdown(void);

#ifdef __cplusplus
}
#endif

#endif /* POSTPROCESSING_MOTION_BLUR_SYSTEM_02_H */
