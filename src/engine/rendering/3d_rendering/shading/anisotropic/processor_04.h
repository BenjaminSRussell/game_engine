/*
 * shading_anisotropic_processor_04.h
 *
 * Header file for shading_anisotropic_processor_04 implementation
 * Part of the Advanced 3D Rendering Engine
 */

#ifndef SHADING_ANISOTROPIC_PROCESSOR_04_H
#define SHADING_ANISOTROPIC_PROCESSOR_04_H

#include <stdint.h>
#include <stdbool.h>
#include <stddef.h>

#ifdef __cplusplus
extern "C" {
#endif

/* Forward declarations */
typedef struct shading_anisotropic_processor_04 shading_anisotropic_processor_04_t;
typedef struct shading_anisotropic_processor_04_desc shading_anisotropic_processor_04_desc_t;
typedef struct shading_anisotropic_processor_04_stats shading_anisotropic_processor_04_stats_t;

/* Creation and destruction */
int shading_anisotropic_processor_04_create(shading_anisotropic_processor_04_t** out_ctx, const shading_anisotropic_processor_04_desc_t* desc);
int shading_anisotropic_processor_04_destroy(shading_anisotropic_processor_04_t* ctx);

/* Core operations */
int shading_anisotropic_processor_04_process_batch(shading_anisotropic_processor_04_t* ctx, void* params);
int shading_anisotropic_processor_04_process_single(shading_anisotropic_processor_04_t* ctx, void* params);
int shading_anisotropic_processor_04_transform(shading_anisotropic_processor_04_t* ctx, void* params);
int shading_anisotropic_processor_04_filter(shading_anisotropic_processor_04_t* ctx, void* params);
int shading_anisotropic_processor_04_aggregate(shading_anisotropic_processor_04_t* ctx, void* params);
int shading_anisotropic_processor_04_dispatch(shading_anisotropic_processor_04_t* ctx, void* params);
int shading_anisotropic_processor_04_finalize(shading_anisotropic_processor_04_t* ctx, void* params);
int shading_anisotropic_processor_04_validate_input(shading_anisotropic_processor_04_t* ctx, void* params);
int shading_anisotropic_processor_04_optimize_output(shading_anisotropic_processor_04_t* ctx, void* params);
int shading_anisotropic_processor_04_profile(shading_anisotropic_processor_04_t* ctx, void* params);

/* Utility functions */
int shading_anisotropic_processor_04_get_stats(shading_anisotropic_processor_04_t* ctx);
int shading_anisotropic_processor_04_set_callback(shading_anisotropic_processor_04_t* ctx);
int shading_anisotropic_processor_04_get_memory_usage(shading_anisotropic_processor_04_t* ctx);
int shading_anisotropic_processor_04_optimize(shading_anisotropic_processor_04_t* ctx);
int shading_anisotropic_processor_04_debug_print(shading_anisotropic_processor_04_t* ctx);

/* Module functions */
int shading_anisotropic_processor_04_module_init(void);
int shading_anisotropic_processor_04_module_shutdown(void);

#ifdef __cplusplus
}
#endif

#endif /* SHADING_ANISOTROPIC_PROCESSOR_04_H */
