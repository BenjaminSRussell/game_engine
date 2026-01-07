/*
 * physics_narrowphase_processor_04.h
 *
 * Header file for physics_narrowphase_processor_04 implementation
 * Part of the Advanced 3D Rendering Engine
 */

#ifndef PHYSICS_NARROWPHASE_PROCESSOR_04_H
#define PHYSICS_NARROWPHASE_PROCESSOR_04_H

#include <stdint.h>
#include <stdbool.h>
#include <stddef.h>

#ifdef __cplusplus
extern "C" {
#endif

/* Forward declarations */
typedef struct physics_narrowphase_processor_04 physics_narrowphase_processor_04_t;
typedef struct physics_narrowphase_processor_04_desc physics_narrowphase_processor_04_desc_t;
typedef struct physics_narrowphase_processor_04_stats physics_narrowphase_processor_04_stats_t;

/* Creation and destruction */
int physics_narrowphase_processor_04_create(physics_narrowphase_processor_04_t** out_ctx, const physics_narrowphase_processor_04_desc_t* desc);
int physics_narrowphase_processor_04_destroy(physics_narrowphase_processor_04_t* ctx);

/* Core operations */
int physics_narrowphase_processor_04_process_batch(physics_narrowphase_processor_04_t* ctx, void* params);
int physics_narrowphase_processor_04_process_single(physics_narrowphase_processor_04_t* ctx, void* params);
int physics_narrowphase_processor_04_transform(physics_narrowphase_processor_04_t* ctx, void* params);
int physics_narrowphase_processor_04_filter(physics_narrowphase_processor_04_t* ctx, void* params);
int physics_narrowphase_processor_04_aggregate(physics_narrowphase_processor_04_t* ctx, void* params);
int physics_narrowphase_processor_04_dispatch(physics_narrowphase_processor_04_t* ctx, void* params);
int physics_narrowphase_processor_04_finalize(physics_narrowphase_processor_04_t* ctx, void* params);
int physics_narrowphase_processor_04_validate_input(physics_narrowphase_processor_04_t* ctx, void* params);
int physics_narrowphase_processor_04_optimize_output(physics_narrowphase_processor_04_t* ctx, void* params);
int physics_narrowphase_processor_04_profile(physics_narrowphase_processor_04_t* ctx, void* params);

/* Utility functions */
int physics_narrowphase_processor_04_get_stats(physics_narrowphase_processor_04_t* ctx);
int physics_narrowphase_processor_04_set_callback(physics_narrowphase_processor_04_t* ctx);
int physics_narrowphase_processor_04_get_memory_usage(physics_narrowphase_processor_04_t* ctx);
int physics_narrowphase_processor_04_optimize(physics_narrowphase_processor_04_t* ctx);
int physics_narrowphase_processor_04_debug_print(physics_narrowphase_processor_04_t* ctx);

/* Module functions */
int physics_narrowphase_processor_04_module_init(void);
int physics_narrowphase_processor_04_module_shutdown(void);

#ifdef __cplusplus
}
#endif

#endif /* PHYSICS_NARROWPHASE_PROCESSOR_04_H */
