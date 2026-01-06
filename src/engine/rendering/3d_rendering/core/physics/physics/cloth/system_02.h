/*
 * physics_cloth_system_02.h
 *
 * Header file for physics_cloth_system_02 implementation
 * Part of the Advanced 3D Rendering Engine
 */

#ifndef PHYSICS_CLOTH_SYSTEM_02_H
#define PHYSICS_CLOTH_SYSTEM_02_H

#include <stdint.h>
#include <stdbool.h>
#include <stddef.h>

#ifdef __cplusplus
extern "C" {
#endif

/* Forward declarations */
typedef struct physics_cloth_system_02 physics_cloth_system_02_t;
typedef struct physics_cloth_system_02_desc physics_cloth_system_02_desc_t;
typedef struct physics_cloth_system_02_stats physics_cloth_system_02_stats_t;

/* Creation and destruction */
int physics_cloth_system_02_create(physics_cloth_system_02_t** out_ctx, const physics_cloth_system_02_desc_t* desc);
int physics_cloth_system_02_destroy(physics_cloth_system_02_t* ctx);

/* Core operations */
int physics_cloth_system_02_create_system(physics_cloth_system_02_t* ctx, void* params);
int physics_cloth_system_02_destroy_system(physics_cloth_system_02_t* ctx, void* params);
int physics_cloth_system_02_tick(physics_cloth_system_02_t* ctx, void* params);
int physics_cloth_system_02_process(physics_cloth_system_02_t* ctx, void* params);
int physics_cloth_system_02_submit(physics_cloth_system_02_t* ctx, void* params);
int physics_cloth_system_02_execute(physics_cloth_system_02_t* ctx, void* params);
int physics_cloth_system_02_sync(physics_cloth_system_02_t* ctx, void* params);
int physics_cloth_system_02_query(physics_cloth_system_02_t* ctx, void* params);
int physics_cloth_system_02_configure(physics_cloth_system_02_t* ctx, void* params);
int physics_cloth_system_02_optimize(physics_cloth_system_02_t* ctx, void* params);

/* Utility functions */
int physics_cloth_system_02_get_stats(physics_cloth_system_02_t* ctx);
int physics_cloth_system_02_set_callback(physics_cloth_system_02_t* ctx);
int physics_cloth_system_02_get_memory_usage(physics_cloth_system_02_t* ctx);
int physics_cloth_system_02_optimize(physics_cloth_system_02_t* ctx);
int physics_cloth_system_02_debug_print(physics_cloth_system_02_t* ctx);

/* Module functions */
int physics_cloth_system_02_module_init(void);
int physics_cloth_system_02_module_shutdown(void);

#ifdef __cplusplus
}
#endif

#endif /* PHYSICS_CLOTH_SYSTEM_02_H */
