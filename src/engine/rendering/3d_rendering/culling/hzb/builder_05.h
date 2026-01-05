/*
 * culling_hzb_builder_05.h
 *
 * Header file for culling_hzb_builder_05 implementation
 * Part of the Advanced 3D Rendering Engine
 */

#ifndef CULLING_HZB_BUILDER_05_H
#define CULLING_HZB_BUILDER_05_H

#include <stdint.h>
#include <stdbool.h>
#include <stddef.h>

#ifdef __cplusplus
extern "C" {
#endif

/* Forward declarations */
typedef struct culling_hzb_builder_05 culling_hzb_builder_05_t;
typedef struct culling_hzb_builder_05_desc culling_hzb_builder_05_desc_t;
typedef struct culling_hzb_builder_05_stats culling_hzb_builder_05_stats_t;

/* Creation and destruction */
int culling_hzb_builder_05_create(culling_hzb_builder_05_t** out_ctx, const culling_hzb_builder_05_desc_t* desc);
int culling_hzb_builder_05_destroy(culling_hzb_builder_05_t* ctx);

/* Core operations */
int culling_hzb_builder_05_begin(culling_hzb_builder_05_t* ctx, void* params);
int culling_hzb_builder_05_end(culling_hzb_builder_05_t* ctx, void* params);
int culling_hzb_builder_05_add(culling_hzb_builder_05_t* ctx, void* params);
int culling_hzb_builder_05_remove(culling_hzb_builder_05_t* ctx, void* params);
int culling_hzb_builder_05_modify(culling_hzb_builder_05_t* ctx, void* params);
int culling_hzb_builder_05_finalize(culling_hzb_builder_05_t* ctx, void* params);
int culling_hzb_builder_05_validate(culling_hzb_builder_05_t* ctx, void* params);
int culling_hzb_builder_05_optimize(culling_hzb_builder_05_t* ctx, void* params);
int culling_hzb_builder_05_compile(culling_hzb_builder_05_t* ctx, void* params);
int culling_hzb_builder_05_link(culling_hzb_builder_05_t* ctx, void* params);

/* Utility functions */
int culling_hzb_builder_05_get_stats(culling_hzb_builder_05_t* ctx);
int culling_hzb_builder_05_set_callback(culling_hzb_builder_05_t* ctx);
int culling_hzb_builder_05_get_memory_usage(culling_hzb_builder_05_t* ctx);
int culling_hzb_builder_05_optimize(culling_hzb_builder_05_t* ctx);
int culling_hzb_builder_05_debug_print(culling_hzb_builder_05_t* ctx);

/* Module functions */
int culling_hzb_builder_05_module_init(void);
int culling_hzb_builder_05_module_shutdown(void);

#ifdef __cplusplus
}
#endif

#endif /* CULLING_HZB_BUILDER_05_H */
