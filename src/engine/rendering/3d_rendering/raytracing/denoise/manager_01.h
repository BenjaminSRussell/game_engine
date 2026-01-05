/*
 * raytracing_denoise_manager_01.h
 *
 * Header file for raytracing_denoise_manager_01 implementation
 * Part of the Advanced 3D Rendering Engine
 */

#ifndef RAYTRACING_DENOISE_MANAGER_01_H
#define RAYTRACING_DENOISE_MANAGER_01_H

#include <stdint.h>
#include <stdbool.h>
#include <stddef.h>

#ifdef __cplusplus
extern "C" {
#endif

/* Forward declarations */
typedef struct raytracing_denoise_manager_01 raytracing_denoise_manager_01_t;
typedef struct raytracing_denoise_manager_01_desc raytracing_denoise_manager_01_desc_t;
typedef struct raytracing_denoise_manager_01_stats raytracing_denoise_manager_01_stats_t;

/* Creation and destruction */
int raytracing_denoise_manager_01_create(raytracing_denoise_manager_01_t** out_ctx, const raytracing_denoise_manager_01_desc_t* desc);
int raytracing_denoise_manager_01_destroy(raytracing_denoise_manager_01_t* ctx);

/* Core operations */
int raytracing_denoise_manager_01_init(raytracing_denoise_manager_01_t* ctx, void* params);
int raytracing_denoise_manager_01_shutdown(raytracing_denoise_manager_01_t* ctx, void* params);
int raytracing_denoise_manager_01_update(raytracing_denoise_manager_01_t* ctx, void* params);
int raytracing_denoise_manager_01_create(raytracing_denoise_manager_01_t* ctx, void* params);
int raytracing_denoise_manager_01_destroy(raytracing_denoise_manager_01_t* ctx, void* params);
int raytracing_denoise_manager_01_get(raytracing_denoise_manager_01_t* ctx, void* params);
int raytracing_denoise_manager_01_set(raytracing_denoise_manager_01_t* ctx, void* params);
int raytracing_denoise_manager_01_reset(raytracing_denoise_manager_01_t* ctx, void* params);
int raytracing_denoise_manager_01_validate(raytracing_denoise_manager_01_t* ctx, void* params);
int raytracing_denoise_manager_01_flush(raytracing_denoise_manager_01_t* ctx, void* params);

/* Utility functions */
int raytracing_denoise_manager_01_get_stats(raytracing_denoise_manager_01_t* ctx);
int raytracing_denoise_manager_01_set_callback(raytracing_denoise_manager_01_t* ctx);
int raytracing_denoise_manager_01_get_memory_usage(raytracing_denoise_manager_01_t* ctx);
int raytracing_denoise_manager_01_optimize(raytracing_denoise_manager_01_t* ctx);
int raytracing_denoise_manager_01_debug_print(raytracing_denoise_manager_01_t* ctx);

/* Module functions */
int raytracing_denoise_manager_01_module_init(void);
int raytracing_denoise_manager_01_module_shutdown(void);

#ifdef __cplusplus
}
#endif

#endif /* RAYTRACING_DENOISE_MANAGER_01_H */
