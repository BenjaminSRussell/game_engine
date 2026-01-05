/*
 * landscape_vegetation_manager_01.h
 *
 * Header file for landscape_vegetation_manager_01 implementation
 * Part of the Advanced 3D Rendering Engine
 */

#ifndef LANDSCAPE_VEGETATION_MANAGER_01_H
#define LANDSCAPE_VEGETATION_MANAGER_01_H

#include <stdint.h>
#include <stdbool.h>
#include <stddef.h>

#ifdef __cplusplus
extern "C" {
#endif

/* Forward declarations */
typedef struct landscape_vegetation_manager_01 landscape_vegetation_manager_01_t;
typedef struct landscape_vegetation_manager_01_desc landscape_vegetation_manager_01_desc_t;
typedef struct landscape_vegetation_manager_01_stats landscape_vegetation_manager_01_stats_t;

/* Creation and destruction */
int landscape_vegetation_manager_01_create(landscape_vegetation_manager_01_t** out_ctx, const landscape_vegetation_manager_01_desc_t* desc);
int landscape_vegetation_manager_01_destroy(landscape_vegetation_manager_01_t* ctx);

/* Core operations */
int landscape_vegetation_manager_01_init(landscape_vegetation_manager_01_t* ctx, void* params);
int landscape_vegetation_manager_01_shutdown(landscape_vegetation_manager_01_t* ctx, void* params);
int landscape_vegetation_manager_01_update(landscape_vegetation_manager_01_t* ctx, void* params);
int landscape_vegetation_manager_01_create(landscape_vegetation_manager_01_t* ctx, void* params);
int landscape_vegetation_manager_01_destroy(landscape_vegetation_manager_01_t* ctx, void* params);
int landscape_vegetation_manager_01_get(landscape_vegetation_manager_01_t* ctx, void* params);
int landscape_vegetation_manager_01_set(landscape_vegetation_manager_01_t* ctx, void* params);
int landscape_vegetation_manager_01_reset(landscape_vegetation_manager_01_t* ctx, void* params);
int landscape_vegetation_manager_01_validate(landscape_vegetation_manager_01_t* ctx, void* params);
int landscape_vegetation_manager_01_flush(landscape_vegetation_manager_01_t* ctx, void* params);

/* Utility functions */
int landscape_vegetation_manager_01_get_stats(landscape_vegetation_manager_01_t* ctx);
int landscape_vegetation_manager_01_set_callback(landscape_vegetation_manager_01_t* ctx);
int landscape_vegetation_manager_01_get_memory_usage(landscape_vegetation_manager_01_t* ctx);
int landscape_vegetation_manager_01_optimize(landscape_vegetation_manager_01_t* ctx);
int landscape_vegetation_manager_01_debug_print(landscape_vegetation_manager_01_t* ctx);

/* Module functions */
int landscape_vegetation_manager_01_module_init(void);
int landscape_vegetation_manager_01_module_shutdown(void);

#ifdef __cplusplus
}
#endif

#endif /* LANDSCAPE_VEGETATION_MANAGER_01_H */
