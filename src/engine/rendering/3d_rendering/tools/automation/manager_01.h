/*
 * tools_automation_manager_01.h
 *
 * Header file for tools_automation_manager_01 implementation
 * Part of the Advanced 3D Rendering Engine
 */

#ifndef TOOLS_AUTOMATION_MANAGER_01_H
#define TOOLS_AUTOMATION_MANAGER_01_H

#include <stdint.h>
#include <stdbool.h>
#include <stddef.h>

#ifdef __cplusplus
extern "C" {
#endif

/* Forward declarations */
typedef struct tools_automation_manager_01 tools_automation_manager_01_t;
typedef struct tools_automation_manager_01_desc tools_automation_manager_01_desc_t;
typedef struct tools_automation_manager_01_stats tools_automation_manager_01_stats_t;

/* Creation and destruction */
int tools_automation_manager_01_create(tools_automation_manager_01_t** out_ctx, const tools_automation_manager_01_desc_t* desc);
int tools_automation_manager_01_destroy(tools_automation_manager_01_t* ctx);

/* Core operations */
int tools_automation_manager_01_init(tools_automation_manager_01_t* ctx, void* params);
int tools_automation_manager_01_shutdown(tools_automation_manager_01_t* ctx, void* params);
int tools_automation_manager_01_update(tools_automation_manager_01_t* ctx, void* params);
int tools_automation_manager_01_create(tools_automation_manager_01_t* ctx, void* params);
int tools_automation_manager_01_destroy(tools_automation_manager_01_t* ctx, void* params);
int tools_automation_manager_01_get(tools_automation_manager_01_t* ctx, void* params);
int tools_automation_manager_01_set(tools_automation_manager_01_t* ctx, void* params);
int tools_automation_manager_01_reset(tools_automation_manager_01_t* ctx, void* params);
int tools_automation_manager_01_validate(tools_automation_manager_01_t* ctx, void* params);
int tools_automation_manager_01_flush(tools_automation_manager_01_t* ctx, void* params);

/* Utility functions */
int tools_automation_manager_01_get_stats(tools_automation_manager_01_t* ctx);
int tools_automation_manager_01_set_callback(tools_automation_manager_01_t* ctx);
int tools_automation_manager_01_get_memory_usage(tools_automation_manager_01_t* ctx);
int tools_automation_manager_01_optimize(tools_automation_manager_01_t* ctx);
int tools_automation_manager_01_debug_print(tools_automation_manager_01_t* ctx);

/* Module functions */
int tools_automation_manager_01_module_init(void);
int tools_automation_manager_01_module_shutdown(void);

#ifdef __cplusplus
}
#endif

#endif /* TOOLS_AUTOMATION_MANAGER_01_H */
