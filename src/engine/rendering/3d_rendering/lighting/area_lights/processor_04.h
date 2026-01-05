/*
 * lighting_area_lights_processor_04.h
 *
 * Header file for lighting_area_lights_processor_04 implementation
 * Part of the Advanced 3D Rendering Engine
 */

#ifndef LIGHTING_AREA_LIGHTS_PROCESSOR_04_H
#define LIGHTING_AREA_LIGHTS_PROCESSOR_04_H

#include <stdint.h>
#include <stdbool.h>
#include <stddef.h>

#ifdef __cplusplus
extern "C" {
#endif

/* Forward declarations */
typedef struct lighting_area_lights_processor_04 lighting_area_lights_processor_04_t;
typedef struct lighting_area_lights_processor_04_desc lighting_area_lights_processor_04_desc_t;
typedef struct lighting_area_lights_processor_04_stats lighting_area_lights_processor_04_stats_t;

/* Creation and destruction */
int lighting_area_lights_processor_04_create(lighting_area_lights_processor_04_t** out_ctx, const lighting_area_lights_processor_04_desc_t* desc);
int lighting_area_lights_processor_04_destroy(lighting_area_lights_processor_04_t* ctx);

/* Core operations */
int lighting_area_lights_processor_04_process_batch(lighting_area_lights_processor_04_t* ctx, void* params);
int lighting_area_lights_processor_04_process_single(lighting_area_lights_processor_04_t* ctx, void* params);
int lighting_area_lights_processor_04_transform(lighting_area_lights_processor_04_t* ctx, void* params);
int lighting_area_lights_processor_04_filter(lighting_area_lights_processor_04_t* ctx, void* params);
int lighting_area_lights_processor_04_aggregate(lighting_area_lights_processor_04_t* ctx, void* params);
int lighting_area_lights_processor_04_dispatch(lighting_area_lights_processor_04_t* ctx, void* params);
int lighting_area_lights_processor_04_finalize(lighting_area_lights_processor_04_t* ctx, void* params);
int lighting_area_lights_processor_04_validate_input(lighting_area_lights_processor_04_t* ctx, void* params);
int lighting_area_lights_processor_04_optimize_output(lighting_area_lights_processor_04_t* ctx, void* params);
int lighting_area_lights_processor_04_profile(lighting_area_lights_processor_04_t* ctx, void* params);

/* Utility functions */
int lighting_area_lights_processor_04_get_stats(lighting_area_lights_processor_04_t* ctx);
int lighting_area_lights_processor_04_set_callback(lighting_area_lights_processor_04_t* ctx);
int lighting_area_lights_processor_04_get_memory_usage(lighting_area_lights_processor_04_t* ctx);
int lighting_area_lights_processor_04_optimize(lighting_area_lights_processor_04_t* ctx);
int lighting_area_lights_processor_04_debug_print(lighting_area_lights_processor_04_t* ctx);

/* Module functions */
int lighting_area_lights_processor_04_module_init(void);
int lighting_area_lights_processor_04_module_shutdown(void);

#ifdef __cplusplus
}
#endif

#endif /* LIGHTING_AREA_LIGHTS_PROCESSOR_04_H */
