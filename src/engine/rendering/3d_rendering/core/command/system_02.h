/*
 * core_command_system_02.h
 *
 * Header file for core_command_system_02 implementation
 * Part of the Advanced 3D Rendering Engine
 */

#ifndef CORE_COMMAND_SYSTEM_02_H
#define CORE_COMMAND_SYSTEM_02_H

#include <stdint.h>
#include <stdbool.h>
#include <stddef.h>

#ifdef __cplusplus
extern "C" {
#endif

/* Forward declarations */
typedef struct core_command_system_02 core_command_system_02_t;
typedef struct core_command_system_02_desc core_command_system_02_desc_t;
typedef struct core_command_system_02_stats core_command_system_02_stats_t;

/* Creation and destruction */
int core_command_system_02_create(core_command_system_02_t** out_ctx, const core_command_system_02_desc_t* desc);
int core_command_system_02_destroy(core_command_system_02_t* ctx);

/* Core operations */
int core_command_system_02_create_system(core_command_system_02_t* ctx, void* params);
int core_command_system_02_destroy_system(core_command_system_02_t* ctx, void* params);
int core_command_system_02_tick(core_command_system_02_t* ctx, void* params);
int core_command_system_02_process(core_command_system_02_t* ctx, void* params);
int core_command_system_02_submit(core_command_system_02_t* ctx, void* params);
int core_command_system_02_execute(core_command_system_02_t* ctx, void* params);
int core_command_system_02_sync(core_command_system_02_t* ctx, void* params);
int core_command_system_02_query(core_command_system_02_t* ctx, void* params);
int core_command_system_02_configure(core_command_system_02_t* ctx, void* params);
int core_command_system_02_optimize(core_command_system_02_t* ctx, void* params);

/* Utility functions */
int core_command_system_02_get_stats(core_command_system_02_t* ctx);
int core_command_system_02_set_callback(core_command_system_02_t* ctx);
int core_command_system_02_get_memory_usage(core_command_system_02_t* ctx);
int core_command_system_02_optimize(core_command_system_02_t* ctx);
int core_command_system_02_debug_print(core_command_system_02_t* ctx);

/* Module functions */
int core_command_system_02_module_init(void);
int core_command_system_02_module_shutdown(void);

#ifdef __cplusplus
}
#endif

#endif /* CORE_COMMAND_SYSTEM_02_H */
