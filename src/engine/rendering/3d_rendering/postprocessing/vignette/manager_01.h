/*
 * postprocessing_vignette_manager_01.h
 *
 * Header file for postprocessing_vignette_manager_01 implementation
 * Part of the Advanced 3D Rendering Engine
 */

#ifndef POSTPROCESSING_VIGNETTE_MANAGER_01_H
#define POSTPROCESSING_VIGNETTE_MANAGER_01_H

#include <stdint.h>
#include <stdbool.h>
#include <stddef.h>

#ifdef __cplusplus
extern "C" {
#endif

/* Forward declarations */
typedef struct postprocessing_vignette_manager_01 postprocessing_vignette_manager_01_t;
typedef struct postprocessing_vignette_manager_01_desc postprocessing_vignette_manager_01_desc_t;
typedef struct postprocessing_vignette_manager_01_stats postprocessing_vignette_manager_01_stats_t;

/* Creation and destruction */
int postprocessing_vignette_manager_01_create(postprocessing_vignette_manager_01_t** out_ctx, const postprocessing_vignette_manager_01_desc_t* desc);
int postprocessing_vignette_manager_01_destroy(postprocessing_vignette_manager_01_t* ctx);

/* Core operations */
int postprocessing_vignette_manager_01_init(postprocessing_vignette_manager_01_t* ctx, void* params);
int postprocessing_vignette_manager_01_shutdown(postprocessing_vignette_manager_01_t* ctx, void* params);
int postprocessing_vignette_manager_01_update(postprocessing_vignette_manager_01_t* ctx, void* params);
int postprocessing_vignette_manager_01_create(postprocessing_vignette_manager_01_t* ctx, void* params);
int postprocessing_vignette_manager_01_destroy(postprocessing_vignette_manager_01_t* ctx, void* params);
int postprocessing_vignette_manager_01_get(postprocessing_vignette_manager_01_t* ctx, void* params);
int postprocessing_vignette_manager_01_set(postprocessing_vignette_manager_01_t* ctx, void* params);
int postprocessing_vignette_manager_01_reset(postprocessing_vignette_manager_01_t* ctx, void* params);
int postprocessing_vignette_manager_01_validate(postprocessing_vignette_manager_01_t* ctx, void* params);
int postprocessing_vignette_manager_01_flush(postprocessing_vignette_manager_01_t* ctx, void* params);

/* Utility functions */
int postprocessing_vignette_manager_01_get_stats(postprocessing_vignette_manager_01_t* ctx);
int postprocessing_vignette_manager_01_set_callback(postprocessing_vignette_manager_01_t* ctx);
int postprocessing_vignette_manager_01_get_memory_usage(postprocessing_vignette_manager_01_t* ctx);
int postprocessing_vignette_manager_01_optimize(postprocessing_vignette_manager_01_t* ctx);
int postprocessing_vignette_manager_01_debug_print(postprocessing_vignette_manager_01_t* ctx);

/* Module functions */
int postprocessing_vignette_manager_01_module_init(void);
int postprocessing_vignette_manager_01_module_shutdown(void);

#ifdef __cplusplus
}
#endif

#endif /* POSTPROCESSING_VIGNETTE_MANAGER_01_H */
