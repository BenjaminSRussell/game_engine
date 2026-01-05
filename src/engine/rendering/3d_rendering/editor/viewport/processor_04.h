/*
 * editor_viewport_processor_04.h
 *
 * Header file for editor_viewport_processor_04 implementation
 * Part of the Advanced 3D Rendering Engine
 */

#ifndef EDITOR_VIEWPORT_PROCESSOR_04_H
#define EDITOR_VIEWPORT_PROCESSOR_04_H

#include <stdint.h>
#include <stdbool.h>
#include <stddef.h>

#ifdef __cplusplus
extern "C" {
#endif

/* Forward declarations */
typedef struct editor_viewport_processor_04 editor_viewport_processor_04_t;
typedef struct editor_viewport_processor_04_desc editor_viewport_processor_04_desc_t;
typedef struct editor_viewport_processor_04_stats editor_viewport_processor_04_stats_t;

/* Creation and destruction */
int editor_viewport_processor_04_create(editor_viewport_processor_04_t** out_ctx, const editor_viewport_processor_04_desc_t* desc);
int editor_viewport_processor_04_destroy(editor_viewport_processor_04_t* ctx);

/* Core operations */
int editor_viewport_processor_04_process_batch(editor_viewport_processor_04_t* ctx, void* params);
int editor_viewport_processor_04_process_single(editor_viewport_processor_04_t* ctx, void* params);
int editor_viewport_processor_04_transform(editor_viewport_processor_04_t* ctx, void* params);
int editor_viewport_processor_04_filter(editor_viewport_processor_04_t* ctx, void* params);
int editor_viewport_processor_04_aggregate(editor_viewport_processor_04_t* ctx, void* params);
int editor_viewport_processor_04_dispatch(editor_viewport_processor_04_t* ctx, void* params);
int editor_viewport_processor_04_finalize(editor_viewport_processor_04_t* ctx, void* params);
int editor_viewport_processor_04_validate_input(editor_viewport_processor_04_t* ctx, void* params);
int editor_viewport_processor_04_optimize_output(editor_viewport_processor_04_t* ctx, void* params);
int editor_viewport_processor_04_profile(editor_viewport_processor_04_t* ctx, void* params);

/* Utility functions */
int editor_viewport_processor_04_get_stats(editor_viewport_processor_04_t* ctx);
int editor_viewport_processor_04_set_callback(editor_viewport_processor_04_t* ctx);
int editor_viewport_processor_04_get_memory_usage(editor_viewport_processor_04_t* ctx);
int editor_viewport_processor_04_optimize(editor_viewport_processor_04_t* ctx);
int editor_viewport_processor_04_debug_print(editor_viewport_processor_04_t* ctx);

/* Module functions */
int editor_viewport_processor_04_module_init(void);
int editor_viewport_processor_04_module_shutdown(void);

#ifdef __cplusplus
}
#endif

#endif /* EDITOR_VIEWPORT_PROCESSOR_04_H */
