/*
 * materials_blending_processor_04.h
 *
 * Header file for materials_blending_processor_04 implementation
 * Part of the Advanced 3D Rendering Engine
 */

#ifndef MATERIALS_BLENDING_PROCESSOR_04_H
#define MATERIALS_BLENDING_PROCESSOR_04_H

#include <stdint.h>
#include <stdbool.h>
#include <stddef.h>

#ifdef __cplusplus
extern "C" {
#endif

/* Forward declarations */
typedef struct materials_blending_processor_04 materials_blending_processor_04_t;
typedef struct materials_blending_processor_04_desc materials_blending_processor_04_desc_t;
typedef struct materials_blending_processor_04_stats materials_blending_processor_04_stats_t;

/* Creation and destruction */
int materials_blending_processor_04_create(materials_blending_processor_04_t** out_ctx, const materials_blending_processor_04_desc_t* desc);
int materials_blending_processor_04_destroy(materials_blending_processor_04_t* ctx);

/* Core operations */
int materials_blending_processor_04_process_batch(materials_blending_processor_04_t* ctx, void* params);
int materials_blending_processor_04_process_single(materials_blending_processor_04_t* ctx, void* params);
int materials_blending_processor_04_transform(materials_blending_processor_04_t* ctx, void* params);
int materials_blending_processor_04_filter(materials_blending_processor_04_t* ctx, void* params);
int materials_blending_processor_04_aggregate(materials_blending_processor_04_t* ctx, void* params);
int materials_blending_processor_04_dispatch(materials_blending_processor_04_t* ctx, void* params);
int materials_blending_processor_04_finalize(materials_blending_processor_04_t* ctx, void* params);
int materials_blending_processor_04_validate_input(materials_blending_processor_04_t* ctx, void* params);
int materials_blending_processor_04_optimize_output(materials_blending_processor_04_t* ctx, void* params);
int materials_blending_processor_04_profile(materials_blending_processor_04_t* ctx, void* params);

/* Utility functions */
int materials_blending_processor_04_get_stats(materials_blending_processor_04_t* ctx);
int materials_blending_processor_04_set_callback(materials_blending_processor_04_t* ctx);
int materials_blending_processor_04_get_memory_usage(materials_blending_processor_04_t* ctx);
int materials_blending_processor_04_optimize(materials_blending_processor_04_t* ctx);
int materials_blending_processor_04_debug_print(materials_blending_processor_04_t* ctx);

/* Module functions */
int materials_blending_processor_04_module_init(void);
int materials_blending_processor_04_module_shutdown(void);

#ifdef __cplusplus
}
#endif

#endif /* MATERIALS_BLENDING_PROCESSOR_04_H */
