/*
 * postprocessing_color_grade_renderer_03.h
 *
 * Header file for postprocessing_color_grade_renderer_03 implementation
 * Part of the Advanced 3D Rendering Engine
 */

#ifndef POSTPROCESSING_COLOR_GRADE_RENDERER_03_H
#define POSTPROCESSING_COLOR_GRADE_RENDERER_03_H

#include <stdint.h>
#include <stdbool.h>
#include <stddef.h>

#ifdef __cplusplus
extern "C" {
#endif

/* Forward declarations */
typedef struct postprocessing_color_grade_renderer_03 postprocessing_color_grade_renderer_03_t;
typedef struct postprocessing_color_grade_renderer_03_desc postprocessing_color_grade_renderer_03_desc_t;
typedef struct postprocessing_color_grade_renderer_03_stats postprocessing_color_grade_renderer_03_stats_t;

/* Creation and destruction */
int postprocessing_color_grade_renderer_03_create(postprocessing_color_grade_renderer_03_t** out_ctx, const postprocessing_color_grade_renderer_03_desc_t* desc);
int postprocessing_color_grade_renderer_03_destroy(postprocessing_color_grade_renderer_03_t* ctx);

/* Core operations */
int postprocessing_color_grade_renderer_03_render(postprocessing_color_grade_renderer_03_t* ctx, void* params);
int postprocessing_color_grade_renderer_03_prepare(postprocessing_color_grade_renderer_03_t* ctx, void* params);
int postprocessing_color_grade_renderer_03_bind(postprocessing_color_grade_renderer_03_t* ctx, void* params);
int postprocessing_color_grade_renderer_03_draw(postprocessing_color_grade_renderer_03_t* ctx, void* params);
int postprocessing_color_grade_renderer_03_dispatch(postprocessing_color_grade_renderer_03_t* ctx, void* params);
int postprocessing_color_grade_renderer_03_submit_commands(postprocessing_color_grade_renderer_03_t* ctx, void* params);
int postprocessing_color_grade_renderer_03_build_commands(postprocessing_color_grade_renderer_03_t* ctx, void* params);
int postprocessing_color_grade_renderer_03_sort(postprocessing_color_grade_renderer_03_t* ctx, void* params);
int postprocessing_color_grade_renderer_03_batch(postprocessing_color_grade_renderer_03_t* ctx, void* params);
int postprocessing_color_grade_renderer_03_cull(postprocessing_color_grade_renderer_03_t* ctx, void* params);

/* Utility functions */
int postprocessing_color_grade_renderer_03_get_stats(postprocessing_color_grade_renderer_03_t* ctx);
int postprocessing_color_grade_renderer_03_set_callback(postprocessing_color_grade_renderer_03_t* ctx);
int postprocessing_color_grade_renderer_03_get_memory_usage(postprocessing_color_grade_renderer_03_t* ctx);
int postprocessing_color_grade_renderer_03_optimize(postprocessing_color_grade_renderer_03_t* ctx);
int postprocessing_color_grade_renderer_03_debug_print(postprocessing_color_grade_renderer_03_t* ctx);

/* Module functions */
int postprocessing_color_grade_renderer_03_module_init(void);
int postprocessing_color_grade_renderer_03_module_shutdown(void);

#ifdef __cplusplus
}
#endif

#endif /* POSTPROCESSING_COLOR_GRADE_RENDERER_03_H */
