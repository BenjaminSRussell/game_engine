/*
 * tools_automation_system_02.h
 *
 * Header file for tools_automation_system_02 implementation
 * Part of the Advanced 3D Rendering Engine
 */

#ifndef TOOLS_AUTOMATION_SYSTEM_02_H
#define TOOLS_AUTOMATION_SYSTEM_02_H

#include <stdint.h>
#include <stdbool.h>
#include <stddef.h>

#ifdef __cplusplus
extern "C" {
#endif

/* Forward declarations */
typedef struct tools_automation_system_02 tools_automation_system_02_t;
typedef struct tools_automation_system_02_desc tools_automation_system_02_desc_t;
typedef struct tools_automation_system_02_stats tools_automation_system_02_stats_t;

/* Creation and destruction */
int tools_automation_system_02_create(tools_automation_system_02_t** out_ctx, const tools_automation_system_02_desc_t* desc);
int tools_automation_system_02_destroy(tools_automation_system_02_t* ctx);

/* Core operations */
int tools_automation_system_02_create_system(tools_automation_system_02_t* ctx, void* params);
int tools_automation_system_02_destroy_system(tools_automation_system_02_t* ctx, void* params);
int tools_automation_system_02_tick(tools_automation_system_02_t* ctx, void* params);
int tools_automation_system_02_process(tools_automation_system_02_t* ctx, void* params);
int tools_automation_system_02_submit(tools_automation_system_02_t* ctx, void* params);
int tools_automation_system_02_execute(tools_automation_system_02_t* ctx, void* params);
int tools_automation_system_02_sync(tools_automation_system_02_t* ctx, void* params);
int tools_automation_system_02_query(tools_automation_system_02_t* ctx, void* params);
int tools_automation_system_02_configure(tools_automation_system_02_t* ctx, void* params);
int tools_automation_system_02_optimize(tools_automation_system_02_t* ctx, void* params);

/* Utility functions */
int tools_automation_system_02_get_stats(tools_automation_system_02_t* ctx);
int tools_automation_system_02_set_callback(tools_automation_system_02_t* ctx);
int tools_automation_system_02_get_memory_usage(tools_automation_system_02_t* ctx);
int tools_automation_system_02_optimize(tools_automation_system_02_t* ctx);
int tools_automation_system_02_debug_print(tools_automation_system_02_t* ctx);

/* Module functions */
int tools_automation_system_02_module_init(void);
int tools_automation_system_02_module_shutdown(void);

#ifdef __cplusplus
}
#endif

#endif /* TOOLS_AUTOMATION_SYSTEM_02_H */
