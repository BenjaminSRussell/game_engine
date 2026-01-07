/*
 * physics_cloth_processor_04.h
 *
 * Header file for physics_cloth_processor_04 implementation
 * Part of the Advanced 3D Rendering Engine
 */

#ifndef PHYSICS_CLOTH_PROCESSOR_04_H
#define PHYSICS_CLOTH_PROCESSOR_04_H

#include <stdint.h>
#include <stdbool.h>
#include <stddef.h>

#ifdef __cplusplus
extern "C" {
#endif

/* Forward declarations */
typedef struct physics_cloth_processor_04 physics_cloth_processor_04_t;
typedef struct physics_cloth_processor_04_desc physics_cloth_processor_04_desc_t;
typedef struct physics_cloth_processor_04_stats physics_cloth_processor_04_stats_t;

/* Creation and destruction */
int physics_cloth_processor_04_create(physics_cloth_processor_04_t** out_ctx, const physics_cloth_processor_04_desc_t* desc);
int physics_cloth_processor_04_destroy(physics_cloth_processor_04_t* ctx);

/* Core operations */
int physics_cloth_processor_04_process_batch(physics_cloth_processor_04_t* ctx, void* params);
int physics_cloth_processor_04_process_single(physics_cloth_processor_04_t* ctx, void* params);
int physics_cloth_processor_04_transform(physics_cloth_processor_04_t* ctx, void* params);
int physics_cloth_processor_04_filter(physics_cloth_processor_04_t* ctx, void* params);
int physics_cloth_processor_04_aggregate(physics_cloth_processor_04_t* ctx, void* params);
int physics_cloth_processor_04_dispatch(physics_cloth_processor_04_t* ctx, void* params);
int physics_cloth_processor_04_finalize(physics_cloth_processor_04_t* ctx, void* params);
int physics_cloth_processor_04_validate_input(physics_cloth_processor_04_t* ctx, void* params);
int physics_cloth_processor_04_optimize_output(physics_cloth_processor_04_t* ctx, void* params);
int physics_cloth_processor_04_profile(physics_cloth_processor_04_t* ctx, void* params);

/* Utility functions */
int physics_cloth_processor_04_get_stats(physics_cloth_processor_04_t* ctx);
int physics_cloth_processor_04_set_callback(physics_cloth_processor_04_t* ctx);
int physics_cloth_processor_04_get_memory_usage(physics_cloth_processor_04_t* ctx);
int physics_cloth_processor_04_optimize(physics_cloth_processor_04_t* ctx);
int physics_cloth_processor_04_debug_print(physics_cloth_processor_04_t* ctx);

/* Module functions */
int physics_cloth_processor_04_module_init(void);
int physics_cloth_processor_04_module_shutdown(void);

#ifdef __cplusplus
}
#endif

#endif /* PHYSICS_CLOTH_PROCESSOR_04_H */
