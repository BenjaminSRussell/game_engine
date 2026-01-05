/*
 * landscape_foliage_processor_04.h
 *
 * Header file for landscape_foliage_processor_04 implementation
 * Part of the Advanced 3D Rendering Engine
 */

#ifndef LANDSCAPE_FOLIAGE_PROCESSOR_04_H
#define LANDSCAPE_FOLIAGE_PROCESSOR_04_H

#include <stdint.h>
#include <stdbool.h>
#include <stddef.h>

#ifdef __cplusplus
extern "C" {
#endif

/* Forward declarations */
typedef struct landscape_foliage_processor_04 landscape_foliage_processor_04_t;
typedef struct landscape_foliage_processor_04_desc landscape_foliage_processor_04_desc_t;
typedef struct landscape_foliage_processor_04_stats landscape_foliage_processor_04_stats_t;

/* Creation and destruction */
int landscape_foliage_processor_04_create(landscape_foliage_processor_04_t** out_ctx, const landscape_foliage_processor_04_desc_t* desc);
int landscape_foliage_processor_04_destroy(landscape_foliage_processor_04_t* ctx);

/* Core operations */
int landscape_foliage_processor_04_process_batch(landscape_foliage_processor_04_t* ctx, void* params);
int landscape_foliage_processor_04_process_single(landscape_foliage_processor_04_t* ctx, void* params);
int landscape_foliage_processor_04_transform(landscape_foliage_processor_04_t* ctx, void* params);
int landscape_foliage_processor_04_filter(landscape_foliage_processor_04_t* ctx, void* params);
int landscape_foliage_processor_04_aggregate(landscape_foliage_processor_04_t* ctx, void* params);
int landscape_foliage_processor_04_dispatch(landscape_foliage_processor_04_t* ctx, void* params);
int landscape_foliage_processor_04_finalize(landscape_foliage_processor_04_t* ctx, void* params);
int landscape_foliage_processor_04_validate_input(landscape_foliage_processor_04_t* ctx, void* params);
int landscape_foliage_processor_04_optimize_output(landscape_foliage_processor_04_t* ctx, void* params);
int landscape_foliage_processor_04_profile(landscape_foliage_processor_04_t* ctx, void* params);

/* Utility functions */
int landscape_foliage_processor_04_get_stats(landscape_foliage_processor_04_t* ctx);
int landscape_foliage_processor_04_set_callback(landscape_foliage_processor_04_t* ctx);
int landscape_foliage_processor_04_get_memory_usage(landscape_foliage_processor_04_t* ctx);
int landscape_foliage_processor_04_optimize(landscape_foliage_processor_04_t* ctx);
int landscape_foliage_processor_04_debug_print(landscape_foliage_processor_04_t* ctx);

/* Module functions */
int landscape_foliage_processor_04_module_init(void);
int landscape_foliage_processor_04_module_shutdown(void);

#ifdef __cplusplus
}
#endif

#endif /* LANDSCAPE_FOLIAGE_PROCESSOR_04_H */
