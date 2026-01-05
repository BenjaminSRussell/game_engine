/*
 * landscape_virtual_manager_01.h
 *
 * Header file for landscape_virtual_manager_01 implementation
 * Part of the Advanced 3D Rendering Engine
 */

#ifndef LANDSCAPE_VIRTUAL_MANAGER_01_H
#define LANDSCAPE_VIRTUAL_MANAGER_01_H

#include <stdint.h>
#include <stdbool.h>
#include <stddef.h>

#ifdef __cplusplus
extern "C" {
#endif

/* Forward declarations */
typedef struct landscape_virtual_manager_01 landscape_virtual_manager_01_t;
typedef struct landscape_virtual_manager_01_desc landscape_virtual_manager_01_desc_t;
typedef struct landscape_virtual_manager_01_stats landscape_virtual_manager_01_stats_t;

/* Creation and destruction */
int landscape_virtual_manager_01_create(landscape_virtual_manager_01_t** out_ctx, const landscape_virtual_manager_01_desc_t* desc);
int landscape_virtual_manager_01_destroy(landscape_virtual_manager_01_t* ctx);

/* Core operations */
int landscape_virtual_manager_01_init(landscape_virtual_manager_01_t* ctx, void* params);
int landscape_virtual_manager_01_shutdown(landscape_virtual_manager_01_t* ctx, void* params);
int landscape_virtual_manager_01_update(landscape_virtual_manager_01_t* ctx, void* params);
int landscape_virtual_manager_01_create(landscape_virtual_manager_01_t* ctx, void* params);
int landscape_virtual_manager_01_destroy(landscape_virtual_manager_01_t* ctx, void* params);
int landscape_virtual_manager_01_get(landscape_virtual_manager_01_t* ctx, void* params);
int landscape_virtual_manager_01_set(landscape_virtual_manager_01_t* ctx, void* params);
int landscape_virtual_manager_01_reset(landscape_virtual_manager_01_t* ctx, void* params);
int landscape_virtual_manager_01_validate(landscape_virtual_manager_01_t* ctx, void* params);
int landscape_virtual_manager_01_flush(landscape_virtual_manager_01_t* ctx, void* params);

/* Utility functions */
int landscape_virtual_manager_01_get_stats(landscape_virtual_manager_01_t* ctx);
int landscape_virtual_manager_01_set_callback(landscape_virtual_manager_01_t* ctx);
int landscape_virtual_manager_01_get_memory_usage(landscape_virtual_manager_01_t* ctx);
int landscape_virtual_manager_01_optimize(landscape_virtual_manager_01_t* ctx);
int landscape_virtual_manager_01_debug_print(landscape_virtual_manager_01_t* ctx);

/* Module functions */
int landscape_virtual_manager_01_module_init(void);
int landscape_virtual_manager_01_module_shutdown(void);

#ifdef __cplusplus
}
#endif

#endif /* LANDSCAPE_VIRTUAL_MANAGER_01_H */
