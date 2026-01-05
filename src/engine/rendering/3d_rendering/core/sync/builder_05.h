/*
 * core_sync_builder_05.h
 *
 * Header file for core_sync_builder_05 implementation
 * Part of the Advanced 3D Rendering Engine
 */

#ifndef CORE_SYNC_BUILDER_05_H
#define CORE_SYNC_BUILDER_05_H

#include <stdint.h>
#include <stdbool.h>
#include <stddef.h>

#ifdef __cplusplus
extern "C" {
#endif

/* Forward declarations */
typedef struct core_sync_builder_05 core_sync_builder_05_t;
typedef struct core_sync_builder_05_desc core_sync_builder_05_desc_t;
typedef struct core_sync_builder_05_stats core_sync_builder_05_stats_t;

/* Creation and destruction */
int core_sync_builder_05_create(core_sync_builder_05_t** out_ctx, const core_sync_builder_05_desc_t* desc);
int core_sync_builder_05_destroy(core_sync_builder_05_t* ctx);

/* Core operations */
int core_sync_builder_05_begin(core_sync_builder_05_t* ctx, void* params);
int core_sync_builder_05_end(core_sync_builder_05_t* ctx, void* params);
int core_sync_builder_05_add(core_sync_builder_05_t* ctx, void* params);
int core_sync_builder_05_remove(core_sync_builder_05_t* ctx, void* params);
int core_sync_builder_05_modify(core_sync_builder_05_t* ctx, void* params);
int core_sync_builder_05_finalize(core_sync_builder_05_t* ctx, void* params);
int core_sync_builder_05_validate(core_sync_builder_05_t* ctx, void* params);
int core_sync_builder_05_optimize(core_sync_builder_05_t* ctx, void* params);
int core_sync_builder_05_compile(core_sync_builder_05_t* ctx, void* params);
int core_sync_builder_05_link(core_sync_builder_05_t* ctx, void* params);

/* Utility functions */
int core_sync_builder_05_get_stats(core_sync_builder_05_t* ctx);
int core_sync_builder_05_set_callback(core_sync_builder_05_t* ctx);
int core_sync_builder_05_get_memory_usage(core_sync_builder_05_t* ctx);
int core_sync_builder_05_optimize(core_sync_builder_05_t* ctx);
int core_sync_builder_05_debug_print(core_sync_builder_05_t* ctx);

/* Module functions */
int core_sync_builder_05_module_init(void);
int core_sync_builder_05_module_shutdown(void);

#ifdef __cplusplus
}
#endif

#endif /* CORE_SYNC_BUILDER_05_H */
