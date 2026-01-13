/*
 * physics_constraints_processor_04.h
 *
 * Header file for physics_constraints_processor_04 implementation
 * Part of the Advanced 3D Rendering Engine
 */

#ifndef PHYSICS_CONSTRAINTS_PROCESSOR_04_H
#define PHYSICS_CONSTRAINTS_PROCESSOR_04_H

#include <stdint.h>
#include <stdbool.h>
#include <stddef.h>

#ifdef __cplusplus
extern "C" {
#endif

/* Forward declarations */
typedef struct physics_constraints_processor_04 physics_constraints_processor_04_t;
typedef struct physics_constraints_processor_04_desc physics_constraints_processor_04_desc_t;
typedef struct physics_constraints_processor_04_stats physics_constraints_processor_04_stats_t;

/* Creation and destruction */
int physics_constraints_processor_04_create(physics_constraints_processor_04_t** out_ctx, const physics_constraints_processor_04_desc_t* desc);
int physics_constraints_processor_04_destroy(physics_constraints_processor_04_t* ctx);

/* Core operations */
int physics_constraints_processor_04_process_batch(physics_constraints_processor_04_t* ctx, void* params);
int physics_constraints_processor_04_process_single(physics_constraints_processor_04_t* ctx, void* params);
int physics_constraints_processor_04_transform(physics_constraints_processor_04_t* ctx, void* params);
int physics_constraints_processor_04_filter(physics_constraints_processor_04_t* ctx, void* params);
int physics_constraints_processor_04_aggregate(physics_constraints_processor_04_t* ctx, void* params);
int physics_constraints_processor_04_dispatch(physics_constraints_processor_04_t* ctx, void* params);
int physics_constraints_processor_04_finalize(physics_constraints_processor_04_t* ctx, void* params);
int physics_constraints_processor_04_validate_input(physics_constraints_processor_04_t* ctx, void* params);
int physics_constraints_processor_04_optimize_output(physics_constraints_processor_04_t* ctx, void* params);
int physics_constraints_processor_04_profile(physics_constraints_processor_04_t* ctx, void* params);

/* Utility functions */
int physics_constraints_processor_04_get_stats(physics_constraints_processor_04_t* ctx);
int physics_constraints_processor_04_set_callback(physics_constraints_processor_04_t* ctx);
int physics_constraints_processor_04_get_memory_usage(physics_constraints_processor_04_t* ctx);
int physics_constraints_processor_04_optimize(physics_constraints_processor_04_t* ctx);
int physics_constraints_processor_04_debug_print(physics_constraints_processor_04_t* ctx);

/* Module functions */
int physics_constraints_processor_04_module_init(void);
int physics_constraints_processor_04_module_shutdown(void);

#ifdef __cplusplus
}
#endif

#endif /* PHYSICS_CONSTRAINTS_PROCESSOR_04_H */
