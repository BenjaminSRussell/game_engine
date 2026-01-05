/*
 * landscape_tessellation_system_02.h
 *
 * Header file for landscape_tessellation_system_02 implementation
 * Part of the Advanced 3D Rendering Engine
 */

#ifndef LANDSCAPE_TESSELLATION_SYSTEM_02_H
#define LANDSCAPE_TESSELLATION_SYSTEM_02_H

#include <stdint.h>
#include <stdbool.h>
#include <stddef.h>

#ifdef __cplusplus
extern "C" {
#endif

/* Forward declarations */
typedef struct landscape_tessellation_system_02 landscape_tessellation_system_02_t;
typedef struct landscape_tessellation_system_02_desc landscape_tessellation_system_02_desc_t;
typedef struct landscape_tessellation_system_02_stats landscape_tessellation_system_02_stats_t;

/* Creation and destruction */
int landscape_tessellation_system_02_create(landscape_tessellation_system_02_t** out_ctx, const landscape_tessellation_system_02_desc_t* desc);
int landscape_tessellation_system_02_destroy(landscape_tessellation_system_02_t* ctx);

/* Core operations */
int landscape_tessellation_system_02_create_system(landscape_tessellation_system_02_t* ctx, void* params);
int landscape_tessellation_system_02_destroy_system(landscape_tessellation_system_02_t* ctx, void* params);
int landscape_tessellation_system_02_tick(landscape_tessellation_system_02_t* ctx, void* params);
int landscape_tessellation_system_02_process(landscape_tessellation_system_02_t* ctx, void* params);
int landscape_tessellation_system_02_submit(landscape_tessellation_system_02_t* ctx, void* params);
int landscape_tessellation_system_02_execute(landscape_tessellation_system_02_t* ctx, void* params);
int landscape_tessellation_system_02_sync(landscape_tessellation_system_02_t* ctx, void* params);
int landscape_tessellation_system_02_query(landscape_tessellation_system_02_t* ctx, void* params);
int landscape_tessellation_system_02_configure(landscape_tessellation_system_02_t* ctx, void* params);
int landscape_tessellation_system_02_optimize(landscape_tessellation_system_02_t* ctx, void* params);

/* Utility functions */
int landscape_tessellation_system_02_get_stats(landscape_tessellation_system_02_t* ctx);
int landscape_tessellation_system_02_set_callback(landscape_tessellation_system_02_t* ctx);
int landscape_tessellation_system_02_get_memory_usage(landscape_tessellation_system_02_t* ctx);
int landscape_tessellation_system_02_optimize(landscape_tessellation_system_02_t* ctx);
int landscape_tessellation_system_02_debug_print(landscape_tessellation_system_02_t* ctx);

/* Module functions */
int landscape_tessellation_system_02_module_init(void);
int landscape_tessellation_system_02_module_shutdown(void);

#ifdef __cplusplus
}
#endif

#endif /* LANDSCAPE_TESSELLATION_SYSTEM_02_H */
