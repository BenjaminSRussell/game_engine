/*
 * postprocessing_color_grade_processor_04.h
 *
 * Header file for postprocessing_color_grade_processor_04 implementation
 * Part of the Advanced 3D Rendering Engine
 */

#ifndef POSTPROCESSING_COLOR_GRADE_PROCESSOR_04_H
#define POSTPROCESSING_COLOR_GRADE_PROCESSOR_04_H

#include <stdint.h>
#include <stdbool.h>
#include <stddef.h>

#ifdef __cplusplus
extern "C" {
#endif

/* Forward declarations */
typedef struct postprocessing_color_grade_processor_04 postprocessing_color_grade_processor_04_t;
typedef struct postprocessing_color_grade_processor_04_desc postprocessing_color_grade_processor_04_desc_t;
typedef struct postprocessing_color_grade_processor_04_stats postprocessing_color_grade_processor_04_stats_t;

/* Creation and destruction */
int postprocessing_color_grade_processor_04_create(postprocessing_color_grade_processor_04_t** out_ctx, const postprocessing_color_grade_processor_04_desc_t* desc);
int postprocessing_color_grade_processor_04_destroy(postprocessing_color_grade_processor_04_t* ctx);

/* Core operations */
int postprocessing_color_grade_processor_04_process_batch(postprocessing_color_grade_processor_04_t* ctx, void* params);
int postprocessing_color_grade_processor_04_process_single(postprocessing_color_grade_processor_04_t* ctx, void* params);
int postprocessing_color_grade_processor_04_transform(postprocessing_color_grade_processor_04_t* ctx, void* params);
int postprocessing_color_grade_processor_04_filter(postprocessing_color_grade_processor_04_t* ctx, void* params);
int postprocessing_color_grade_processor_04_aggregate(postprocessing_color_grade_processor_04_t* ctx, void* params);
int postprocessing_color_grade_processor_04_dispatch(postprocessing_color_grade_processor_04_t* ctx, void* params);
int postprocessing_color_grade_processor_04_finalize(postprocessing_color_grade_processor_04_t* ctx, void* params);
int postprocessing_color_grade_processor_04_validate_input(postprocessing_color_grade_processor_04_t* ctx, void* params);
int postprocessing_color_grade_processor_04_optimize_output(postprocessing_color_grade_processor_04_t* ctx, void* params);
int postprocessing_color_grade_processor_04_profile(postprocessing_color_grade_processor_04_t* ctx, void* params);

/* Utility functions */
int postprocessing_color_grade_processor_04_get_stats(postprocessing_color_grade_processor_04_t* ctx);
int postprocessing_color_grade_processor_04_set_callback(postprocessing_color_grade_processor_04_t* ctx);
int postprocessing_color_grade_processor_04_get_memory_usage(postprocessing_color_grade_processor_04_t* ctx);
int postprocessing_color_grade_processor_04_optimize(postprocessing_color_grade_processor_04_t* ctx);
int postprocessing_color_grade_processor_04_debug_print(postprocessing_color_grade_processor_04_t* ctx);

/* Module functions */
int postprocessing_color_grade_processor_04_module_init(void);
int postprocessing_color_grade_processor_04_module_shutdown(void);

#ifdef __cplusplus
}
#endif

#endif /* POSTPROCESSING_COLOR_GRADE_PROCESSOR_04_H */
