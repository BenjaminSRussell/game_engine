/*
 * tools_analysis_manager_01.h
 *
 * Header file for tools_analysis_manager_01 implementation
 * Part of the Advanced 3D Rendering Engine
 */

#ifndef TOOLS_ANALYSIS_MANAGER_01_H
#define TOOLS_ANALYSIS_MANAGER_01_H

#include <stdint.h>
#include <stdbool.h>
#include <stddef.h>

#ifdef __cplusplus
extern "C" {
#endif

/* Forward declarations */
typedef struct tools_analysis_manager_01 tools_analysis_manager_01_t;
typedef struct tools_analysis_manager_01_desc tools_analysis_manager_01_desc_t;
typedef struct tools_analysis_manager_01_stats tools_analysis_manager_01_stats_t;

/* Creation and destruction */
int tools_analysis_manager_01_create(tools_analysis_manager_01_t** out_ctx, const tools_analysis_manager_01_desc_t* desc);
int tools_analysis_manager_01_destroy(tools_analysis_manager_01_t* ctx);

/* Core operations */
int tools_analysis_manager_01_init(tools_analysis_manager_01_t* ctx, void* params);
int tools_analysis_manager_01_shutdown(tools_analysis_manager_01_t* ctx, void* params);
int tools_analysis_manager_01_update(tools_analysis_manager_01_t* ctx, void* params);
int tools_analysis_manager_01_create_legacy(tools_analysis_manager_01_t* ctx, void* params);
int tools_analysis_manager_01_destroy_legacy(tools_analysis_manager_01_t* ctx, void* params);
int tools_analysis_manager_01_get(tools_analysis_manager_01_t* ctx, void* params);
int tools_analysis_manager_01_set(tools_analysis_manager_01_t* ctx, void* params);
int tools_analysis_manager_01_reset(tools_analysis_manager_01_t* ctx, void* params);
int tools_analysis_manager_01_validate(tools_analysis_manager_01_t* ctx, void* params);
int tools_analysis_manager_01_flush(tools_analysis_manager_01_t* ctx, void* params);

/* Utility functions */
int tools_analysis_manager_01_get_stats(tools_analysis_manager_01_t* ctx);
int tools_analysis_manager_01_set_callback(tools_analysis_manager_01_t* ctx);
int tools_analysis_manager_01_get_memory_usage(tools_analysis_manager_01_t* ctx);
int tools_analysis_manager_01_optimize(tools_analysis_manager_01_t* ctx);
int tools_analysis_manager_01_debug_print(tools_analysis_manager_01_t* ctx);

/* Module functions */
int tools_analysis_manager_01_module_init(void);
int tools_analysis_manager_01_module_shutdown(void);

#ifdef __cplusplus
}
#endif

#endif /* TOOLS_ANALYSIS_MANAGER_01_H */
