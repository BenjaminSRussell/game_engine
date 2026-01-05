/*
 * editor_gizmos_processor_04.h
 *
 * Header file for editor_gizmos_processor_04 implementation
 * Part of the Advanced 3D Rendering Engine
 */

#ifndef EDITOR_GIZMOS_PROCESSOR_04_H
#define EDITOR_GIZMOS_PROCESSOR_04_H

#include <stdint.h>
#include <stdbool.h>
#include <stddef.h>

#ifdef __cplusplus
extern "C" {
#endif

/* Forward declarations */
typedef struct editor_gizmos_processor_04 editor_gizmos_processor_04_t;
typedef struct editor_gizmos_processor_04_desc editor_gizmos_processor_04_desc_t;
typedef struct editor_gizmos_processor_04_stats editor_gizmos_processor_04_stats_t;

/* Creation and destruction */
int editor_gizmos_processor_04_create(editor_gizmos_processor_04_t** out_ctx, const editor_gizmos_processor_04_desc_t* desc);
int editor_gizmos_processor_04_destroy(editor_gizmos_processor_04_t* ctx);

/* Core operations */
int editor_gizmos_processor_04_process_batch(editor_gizmos_processor_04_t* ctx, void* params);
int editor_gizmos_processor_04_process_single(editor_gizmos_processor_04_t* ctx, void* params);
int editor_gizmos_processor_04_transform(editor_gizmos_processor_04_t* ctx, void* params);
int editor_gizmos_processor_04_filter(editor_gizmos_processor_04_t* ctx, void* params);
int editor_gizmos_processor_04_aggregate(editor_gizmos_processor_04_t* ctx, void* params);
int editor_gizmos_processor_04_dispatch(editor_gizmos_processor_04_t* ctx, void* params);
int editor_gizmos_processor_04_finalize(editor_gizmos_processor_04_t* ctx, void* params);
int editor_gizmos_processor_04_validate_input(editor_gizmos_processor_04_t* ctx, void* params);
int editor_gizmos_processor_04_optimize_output(editor_gizmos_processor_04_t* ctx, void* params);
int editor_gizmos_processor_04_profile(editor_gizmos_processor_04_t* ctx, void* params);

/* Utility functions */
int editor_gizmos_processor_04_get_stats(editor_gizmos_processor_04_t* ctx);
int editor_gizmos_processor_04_set_callback(editor_gizmos_processor_04_t* ctx);
int editor_gizmos_processor_04_get_memory_usage(editor_gizmos_processor_04_t* ctx);
int editor_gizmos_processor_04_optimize(editor_gizmos_processor_04_t* ctx);
int editor_gizmos_processor_04_debug_print(editor_gizmos_processor_04_t* ctx);

/* Module functions */
int editor_gizmos_processor_04_module_init(void);
int editor_gizmos_processor_04_module_shutdown(void);

#ifdef __cplusplus
}
#endif

#endif /* EDITOR_GIZMOS_PROCESSOR_04_H */
