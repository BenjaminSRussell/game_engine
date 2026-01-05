/*
 * editor_grid_processor_04.h
 *
 * Header file for editor_grid_processor_04 implementation
 * Part of the Advanced 3D Rendering Engine
 */

#ifndef EDITOR_GRID_PROCESSOR_04_H
#define EDITOR_GRID_PROCESSOR_04_H

#include <stdint.h>
#include <stdbool.h>
#include <stddef.h>

#ifdef __cplusplus
extern "C" {
#endif

/* Forward declarations */
typedef struct editor_grid_processor_04 editor_grid_processor_04_t;
typedef struct editor_grid_processor_04_desc editor_grid_processor_04_desc_t;
typedef struct editor_grid_processor_04_stats editor_grid_processor_04_stats_t;

/* Creation and destruction */
int editor_grid_processor_04_create(editor_grid_processor_04_t** out_ctx, const editor_grid_processor_04_desc_t* desc);
int editor_grid_processor_04_destroy(editor_grid_processor_04_t* ctx);

/* Core operations */
int editor_grid_processor_04_process_batch(editor_grid_processor_04_t* ctx, void* params);
int editor_grid_processor_04_process_single(editor_grid_processor_04_t* ctx, void* params);
int editor_grid_processor_04_transform(editor_grid_processor_04_t* ctx, void* params);
int editor_grid_processor_04_filter(editor_grid_processor_04_t* ctx, void* params);
int editor_grid_processor_04_aggregate(editor_grid_processor_04_t* ctx, void* params);
int editor_grid_processor_04_dispatch(editor_grid_processor_04_t* ctx, void* params);
int editor_grid_processor_04_finalize(editor_grid_processor_04_t* ctx, void* params);
int editor_grid_processor_04_validate_input(editor_grid_processor_04_t* ctx, void* params);
int editor_grid_processor_04_optimize_output(editor_grid_processor_04_t* ctx, void* params);
int editor_grid_processor_04_profile(editor_grid_processor_04_t* ctx, void* params);

/* Utility functions */
int editor_grid_processor_04_get_stats(editor_grid_processor_04_t* ctx);
int editor_grid_processor_04_set_callback(editor_grid_processor_04_t* ctx);
int editor_grid_processor_04_get_memory_usage(editor_grid_processor_04_t* ctx);
int editor_grid_processor_04_optimize(editor_grid_processor_04_t* ctx);
int editor_grid_processor_04_debug_print(editor_grid_processor_04_t* ctx);

/* Module functions */
int editor_grid_processor_04_module_init(void);
int editor_grid_processor_04_module_shutdown(void);

#ifdef __cplusplus
}
#endif

#endif /* EDITOR_GRID_PROCESSOR_04_H */
