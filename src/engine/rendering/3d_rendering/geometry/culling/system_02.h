/*
 * geometry_culling_system_02.h
 *
 * Header file for geometry_culling_system_02 implementation
 * Part of the Advanced 3D Rendering Engine
 */

#ifndef GEOMETRY_CULLING_SYSTEM_02_H
#define GEOMETRY_CULLING_SYSTEM_02_H

#include <stdint.h>
#include <stdbool.h>
#include <stddef.h>

#ifdef __cplusplus
extern "C" {
#endif

/* Forward declarations */
typedef struct geometry_culling_system_02 geometry_culling_system_02_t;
typedef struct geometry_culling_system_02_desc geometry_culling_system_02_desc_t;
typedef struct geometry_culling_system_02_stats geometry_culling_system_02_stats_t;

/* Creation and destruction */
int geometry_culling_system_02_create(geometry_culling_system_02_t** out_ctx, const geometry_culling_system_02_desc_t* desc);
int geometry_culling_system_02_destroy(geometry_culling_system_02_t* ctx);

/* Core operations */
int geometry_culling_system_02_create_system(geometry_culling_system_02_t* ctx, void* params);
int geometry_culling_system_02_destroy_system(geometry_culling_system_02_t* ctx, void* params);
int geometry_culling_system_02_tick(geometry_culling_system_02_t* ctx, void* params);
int geometry_culling_system_02_process(geometry_culling_system_02_t* ctx, void* params);
int geometry_culling_system_02_submit(geometry_culling_system_02_t* ctx, void* params);
int geometry_culling_system_02_execute(geometry_culling_system_02_t* ctx, void* params);
int geometry_culling_system_02_sync(geometry_culling_system_02_t* ctx, void* params);
int geometry_culling_system_02_query(geometry_culling_system_02_t* ctx, void* params);
int geometry_culling_system_02_configure(geometry_culling_system_02_t* ctx, void* params);
int geometry_culling_system_02_optimize(geometry_culling_system_02_t* ctx, void* params);

/* Utility functions */
int geometry_culling_system_02_get_stats(geometry_culling_system_02_t* ctx);
int geometry_culling_system_02_set_callback(geometry_culling_system_02_t* ctx);
int geometry_culling_system_02_get_memory_usage(geometry_culling_system_02_t* ctx);
int geometry_culling_system_02_optimize(geometry_culling_system_02_t* ctx);
int geometry_culling_system_02_debug_print(geometry_culling_system_02_t* ctx);

/* Module functions */
int geometry_culling_system_02_module_init(void);
int geometry_culling_system_02_module_shutdown(void);

#ifdef __cplusplus
}
#endif

#endif /* GEOMETRY_CULLING_SYSTEM_02_H */
