/*
 * culling_prediction_manager_01.h
 *
 * Header file for culling_prediction_manager_01 implementation
 * Part of the Advanced 3D Rendering Engine
 */

#ifndef CULLING_PREDICTION_MANAGER_01_H
#define CULLING_PREDICTION_MANAGER_01_H

#include <stdint.h>
#include <stdbool.h>
#include <stddef.h>

#ifdef __cplusplus
extern "C" {
#endif

/* Forward declarations */
typedef struct culling_prediction_manager_01 culling_prediction_manager_01_t;
typedef struct culling_prediction_manager_01_desc culling_prediction_manager_01_desc_t;
typedef struct culling_prediction_manager_01_stats culling_prediction_manager_01_stats_t;

/* Creation and destruction */
int culling_prediction_manager_01_create(culling_prediction_manager_01_t** out_ctx, const culling_prediction_manager_01_desc_t* desc);
int culling_prediction_manager_01_destroy(culling_prediction_manager_01_t* ctx);

/* Core operations */
int culling_prediction_manager_01_init(culling_prediction_manager_01_t* ctx, void* params);
int culling_prediction_manager_01_shutdown(culling_prediction_manager_01_t* ctx, void* params);
int culling_prediction_manager_01_update(culling_prediction_manager_01_t* ctx, void* params);
int culling_prediction_manager_01_create(culling_prediction_manager_01_t* ctx, void* params);
int culling_prediction_manager_01_destroy(culling_prediction_manager_01_t* ctx, void* params);
int culling_prediction_manager_01_get(culling_prediction_manager_01_t* ctx, void* params);
int culling_prediction_manager_01_set(culling_prediction_manager_01_t* ctx, void* params);
int culling_prediction_manager_01_reset(culling_prediction_manager_01_t* ctx, void* params);
int culling_prediction_manager_01_validate(culling_prediction_manager_01_t* ctx, void* params);
int culling_prediction_manager_01_flush(culling_prediction_manager_01_t* ctx, void* params);

/* Utility functions */
int culling_prediction_manager_01_get_stats(culling_prediction_manager_01_t* ctx);
int culling_prediction_manager_01_set_callback(culling_prediction_manager_01_t* ctx);
int culling_prediction_manager_01_get_memory_usage(culling_prediction_manager_01_t* ctx);
int culling_prediction_manager_01_optimize(culling_prediction_manager_01_t* ctx);
int culling_prediction_manager_01_debug_print(culling_prediction_manager_01_t* ctx);

/* Module functions */
int culling_prediction_manager_01_module_init(void);
int culling_prediction_manager_01_module_shutdown(void);

#ifdef __cplusplus
}
#endif

#endif /* CULLING_PREDICTION_MANAGER_01_H */
