/*
 * physics_fluid_processor_04.h
 *
 * Header file for physics_fluid_processor_04 implementation
 * Part of the Advanced 3D Rendering Engine
 */

#ifndef PHYSICS_FLUID_PROCESSOR_04_H
#define PHYSICS_FLUID_PROCESSOR_04_H

#include <stdint.h>
#include <stdbool.h>
#include <stddef.h>

#ifdef __cplusplus
extern "C" {
#endif

/* Forward declarations */
typedef struct physics_fluid_processor_04 physics_fluid_processor_04_t;
typedef struct physics_fluid_processor_04_desc physics_fluid_processor_04_desc_t;
typedef struct physics_fluid_processor_04_stats physics_fluid_processor_04_stats_t;

/* Creation and destruction */
int physics_fluid_processor_04_create(physics_fluid_processor_04_t** out_ctx, const physics_fluid_processor_04_desc_t* desc);
int physics_fluid_processor_04_destroy(physics_fluid_processor_04_t* ctx);

/* Core operations */
int physics_fluid_processor_04_process_batch(physics_fluid_processor_04_t* ctx, void* params);
int physics_fluid_processor_04_process_single(physics_fluid_processor_04_t* ctx, void* params);
int physics_fluid_processor_04_transform(physics_fluid_processor_04_t* ctx, void* params);
int physics_fluid_processor_04_filter(physics_fluid_processor_04_t* ctx, void* params);
int physics_fluid_processor_04_aggregate(physics_fluid_processor_04_t* ctx, void* params);
int physics_fluid_processor_04_dispatch(physics_fluid_processor_04_t* ctx, void* params);
int physics_fluid_processor_04_finalize(physics_fluid_processor_04_t* ctx, void* params);
int physics_fluid_processor_04_validate_input(physics_fluid_processor_04_t* ctx, void* params);
int physics_fluid_processor_04_optimize_output(physics_fluid_processor_04_t* ctx, void* params);
int physics_fluid_processor_04_profile(physics_fluid_processor_04_t* ctx, void* params);

/* Utility functions */
int physics_fluid_processor_04_get_stats(physics_fluid_processor_04_t* ctx);
int physics_fluid_processor_04_set_callback(physics_fluid_processor_04_t* ctx);
int physics_fluid_processor_04_get_memory_usage(physics_fluid_processor_04_t* ctx);
int physics_fluid_processor_04_optimize(physics_fluid_processor_04_t* ctx);
int physics_fluid_processor_04_debug_print(physics_fluid_processor_04_t* ctx);

/* Module functions */
int physics_fluid_processor_04_module_init(void);
int physics_fluid_processor_04_module_shutdown(void);

#ifdef __cplusplus
}
#endif

#endif /* PHYSICS_FLUID_PROCESSOR_04_H */
