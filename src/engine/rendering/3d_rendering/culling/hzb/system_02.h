/*
 * culling_hzb_system_02.h
 *
 * Header file for culling_hzb_system_02 implementation
 * Part of the Advanced 3D Rendering Engine
 */

#ifndef CULLING_HZB_SYSTEM_02_H
#define CULLING_HZB_SYSTEM_02_H

#include <stdint.h>
#include <stdbool.h>
#include <stddef.h>

#ifdef __cplusplus
extern "C" {
#endif

/* Forward declarations */
typedef struct culling_hzb_system_02 culling_hzb_system_02_t;
typedef struct culling_hzb_system_02_desc culling_hzb_system_02_desc_t;
typedef struct culling_hzb_system_02_stats culling_hzb_system_02_stats_t;

/* Creation and destruction */
int culling_hzb_system_02_create(culling_hzb_system_02_t** out_ctx, const culling_hzb_system_02_desc_t* desc);
int culling_hzb_system_02_destroy(culling_hzb_system_02_t* ctx);

/* Core operations */
int culling_hzb_system_02_create_system(culling_hzb_system_02_t* ctx, void* params);
int culling_hzb_system_02_destroy_system(culling_hzb_system_02_t* ctx, void* params);
int culling_hzb_system_02_tick(culling_hzb_system_02_t* ctx, void* params);
int culling_hzb_system_02_process(culling_hzb_system_02_t* ctx, void* params);
int culling_hzb_system_02_submit(culling_hzb_system_02_t* ctx, void* params);
int culling_hzb_system_02_execute(culling_hzb_system_02_t* ctx, void* params);
int culling_hzb_system_02_sync(culling_hzb_system_02_t* ctx, void* params);
int culling_hzb_system_02_query(culling_hzb_system_02_t* ctx, void* params);
int culling_hzb_system_02_configure(culling_hzb_system_02_t* ctx, void* params);
int culling_hzb_system_02_optimize(culling_hzb_system_02_t* ctx, void* params);

/* Utility functions */
int culling_hzb_system_02_get_stats(culling_hzb_system_02_t* ctx);
int culling_hzb_system_02_set_callback(culling_hzb_system_02_t* ctx);
int culling_hzb_system_02_get_memory_usage(culling_hzb_system_02_t* ctx);
int culling_hzb_system_02_optimize(culling_hzb_system_02_t* ctx);
int culling_hzb_system_02_debug_print(culling_hzb_system_02_t* ctx);

/* Module functions */
int culling_hzb_system_02_module_init(void);
int culling_hzb_system_02_module_shutdown(void);

#ifdef __cplusplus
}
#endif

#endif /* CULLING_HZB_SYSTEM_02_H */
