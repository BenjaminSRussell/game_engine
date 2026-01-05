/*
 * lighting_volumetric_processor_04.h
 *
 * Header file for lighting_volumetric_processor_04 implementation
 * Part of the Advanced 3D Rendering Engine
 */

#ifndef LIGHTING_VOLUMETRIC_PROCESSOR_04_H
#define LIGHTING_VOLUMETRIC_PROCESSOR_04_H

#include <stdint.h>
#include <stdbool.h>
#include <stddef.h>

#ifdef __cplusplus
extern "C" {
#endif

/* Forward declarations */
typedef struct lighting_volumetric_processor_04 lighting_volumetric_processor_04_t;
typedef struct lighting_volumetric_processor_04_desc lighting_volumetric_processor_04_desc_t;
typedef struct lighting_volumetric_processor_04_stats lighting_volumetric_processor_04_stats_t;

/* Creation and destruction */
int lighting_volumetric_processor_04_create(lighting_volumetric_processor_04_t** out_ctx, const lighting_volumetric_processor_04_desc_t* desc);
int lighting_volumetric_processor_04_destroy(lighting_volumetric_processor_04_t* ctx);

/* Core operations */
int lighting_volumetric_processor_04_process_batch(lighting_volumetric_processor_04_t* ctx, void* params);
int lighting_volumetric_processor_04_process_single(lighting_volumetric_processor_04_t* ctx, void* params);
int lighting_volumetric_processor_04_transform(lighting_volumetric_processor_04_t* ctx, void* params);
int lighting_volumetric_processor_04_filter(lighting_volumetric_processor_04_t* ctx, void* params);
int lighting_volumetric_processor_04_aggregate(lighting_volumetric_processor_04_t* ctx, void* params);
int lighting_volumetric_processor_04_dispatch(lighting_volumetric_processor_04_t* ctx, void* params);
int lighting_volumetric_processor_04_finalize(lighting_volumetric_processor_04_t* ctx, void* params);
int lighting_volumetric_processor_04_validate_input(lighting_volumetric_processor_04_t* ctx, void* params);
int lighting_volumetric_processor_04_optimize_output(lighting_volumetric_processor_04_t* ctx, void* params);
int lighting_volumetric_processor_04_profile(lighting_volumetric_processor_04_t* ctx, void* params);

/* Utility functions */
int lighting_volumetric_processor_04_get_stats(lighting_volumetric_processor_04_t* ctx);
int lighting_volumetric_processor_04_set_callback(lighting_volumetric_processor_04_t* ctx);
int lighting_volumetric_processor_04_get_memory_usage(lighting_volumetric_processor_04_t* ctx);
int lighting_volumetric_processor_04_optimize(lighting_volumetric_processor_04_t* ctx);
int lighting_volumetric_processor_04_debug_print(lighting_volumetric_processor_04_t* ctx);

/* Module functions */
int lighting_volumetric_processor_04_module_init(void);
int lighting_volumetric_processor_04_module_shutdown(void);

#ifdef __cplusplus
}
#endif

#endif /* LIGHTING_VOLUMETRIC_PROCESSOR_04_H */
