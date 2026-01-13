/*
 * physics_soft_system_02.h
 *
 * Header file for physics_soft_system_02 implementation
 * Part of the Advanced 3D Rendering Engine
 */

#ifndef PHYSICS_SOFT_SYSTEM_02_H
#define PHYSICS_SOFT_SYSTEM_02_H

#include <stdint.h>
#include <stdbool.h>
#include <stddef.h>

#ifdef __cplusplus
extern "C" {
#endif

/* Forward declarations */
typedef struct physics_soft_system_02 physics_soft_system_02_t;
typedef struct physics_soft_system_02_desc physics_soft_system_02_desc_t;
typedef struct physics_soft_system_02_stats physics_soft_system_02_stats_t;

/* Creation and destruction */
int physics_soft_system_02_create(physics_soft_system_02_t** out_ctx, const physics_soft_system_02_desc_t* desc);
int physics_soft_system_02_destroy(physics_soft_system_02_t* ctx);

/* Core operations */
int physics_soft_system_02_create_system(physics_soft_system_02_t* ctx, void* params);
int physics_soft_system_02_destroy_system(physics_soft_system_02_t* ctx, void* params);
int physics_soft_system_02_tick(physics_soft_system_02_t* ctx, void* params);
int physics_soft_system_02_process(physics_soft_system_02_t* ctx, void* params);
int physics_soft_system_02_submit(physics_soft_system_02_t* ctx, void* params);
int physics_soft_system_02_execute(physics_soft_system_02_t* ctx, void* params);
int physics_soft_system_02_sync(physics_soft_system_02_t* ctx, void* params);
int physics_soft_system_02_query(physics_soft_system_02_t* ctx, void* params);
int physics_soft_system_02_configure(physics_soft_system_02_t* ctx, void* params);
int physics_soft_system_02_optimize(physics_soft_system_02_t* ctx, void* params);

/* Utility functions */
int physics_soft_system_02_get_stats(physics_soft_system_02_t* ctx);
int physics_soft_system_02_set_callback(physics_soft_system_02_t* ctx);
int physics_soft_system_02_get_memory_usage(physics_soft_system_02_t* ctx);
int physics_soft_system_02_optimize_legacy(physics_soft_system_02_t* ctx);
int physics_soft_system_02_debug_print(physics_soft_system_02_t* ctx);

/* Module functions */
int physics_soft_system_02_module_init(void);
int physics_soft_system_02_module_shutdown(void);

#ifdef __cplusplus
}
#endif

#endif /* PHYSICS_SOFT_SYSTEM_02_H */
