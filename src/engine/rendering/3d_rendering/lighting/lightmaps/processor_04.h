/*
 * lighting_lightmaps_processor_04.h
 *
 * Header file for lighting_lightmaps_processor_04 implementation
 * Part of the Advanced 3D Rendering Engine
 */

#ifndef LIGHTING_LIGHTMAPS_PROCESSOR_04_H
#define LIGHTING_LIGHTMAPS_PROCESSOR_04_H

#include <stdint.h>
#include <stdbool.h>
#include <stddef.h>

#ifdef __cplusplus
extern "C" {
#endif

/* Forward declarations */
typedef struct lighting_lightmaps_processor_04 lighting_lightmaps_processor_04_t;
typedef struct lighting_lightmaps_processor_04_desc lighting_lightmaps_processor_04_desc_t;
typedef struct lighting_lightmaps_processor_04_stats lighting_lightmaps_processor_04_stats_t;

/* Creation and destruction */
int lighting_lightmaps_processor_04_create(lighting_lightmaps_processor_04_t** out_ctx, const lighting_lightmaps_processor_04_desc_t* desc);
int lighting_lightmaps_processor_04_destroy(lighting_lightmaps_processor_04_t* ctx);

/* Core operations */
int lighting_lightmaps_processor_04_process_batch(lighting_lightmaps_processor_04_t* ctx, void* params);
int lighting_lightmaps_processor_04_process_single(lighting_lightmaps_processor_04_t* ctx, void* params);
int lighting_lightmaps_processor_04_transform(lighting_lightmaps_processor_04_t* ctx, void* params);
int lighting_lightmaps_processor_04_filter(lighting_lightmaps_processor_04_t* ctx, void* params);
int lighting_lightmaps_processor_04_aggregate(lighting_lightmaps_processor_04_t* ctx, void* params);
int lighting_lightmaps_processor_04_dispatch(lighting_lightmaps_processor_04_t* ctx, void* params);
int lighting_lightmaps_processor_04_finalize(lighting_lightmaps_processor_04_t* ctx, void* params);
int lighting_lightmaps_processor_04_validate_input(lighting_lightmaps_processor_04_t* ctx, void* params);
int lighting_lightmaps_processor_04_optimize_output(lighting_lightmaps_processor_04_t* ctx, void* params);
int lighting_lightmaps_processor_04_profile(lighting_lightmaps_processor_04_t* ctx, void* params);

/* Utility functions */
int lighting_lightmaps_processor_04_get_stats(lighting_lightmaps_processor_04_t* ctx);
int lighting_lightmaps_processor_04_set_callback(lighting_lightmaps_processor_04_t* ctx);
int lighting_lightmaps_processor_04_get_memory_usage(lighting_lightmaps_processor_04_t* ctx);
int lighting_lightmaps_processor_04_optimize(lighting_lightmaps_processor_04_t* ctx);
int lighting_lightmaps_processor_04_debug_print(lighting_lightmaps_processor_04_t* ctx);

/* Module functions */
int lighting_lightmaps_processor_04_module_init(void);
int lighting_lightmaps_processor_04_module_shutdown(void);

#ifdef __cplusplus
}
#endif

#endif /* LIGHTING_LIGHTMAPS_PROCESSOR_04_H */
