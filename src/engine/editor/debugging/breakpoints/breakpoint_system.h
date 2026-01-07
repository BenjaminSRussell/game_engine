/*
 * debugging_breakpoints_system_02.h
 *
 * Header file for debugging_breakpoints_system_02 implementation
 * Part of the Advanced 3D Rendering Engine
 */

#ifndef DEBUGGING_BREAKPOINTS_SYSTEM_02_H
#define DEBUGGING_BREAKPOINTS_SYSTEM_02_H

#include <stdint.h>
#include <stdbool.h>
#include <stddef.h>

#ifdef __cplusplus
extern "C" {
#endif

/* Forward declarations */
typedef struct debugging_breakpoints_system_02 debugging_breakpoints_system_02_t;
typedef struct debugging_breakpoints_system_02_desc debugging_breakpoints_system_02_desc_t;
typedef struct debugging_breakpoints_system_02_stats debugging_breakpoints_system_02_stats_t;

/* Creation and destruction */
int debugging_breakpoints_system_02_create(debugging_breakpoints_system_02_t** out_ctx, const debugging_breakpoints_system_02_desc_t* desc);
int debugging_breakpoints_system_02_destroy(debugging_breakpoints_system_02_t* ctx);

/* Core operations */
int debugging_breakpoints_system_02_create_system(debugging_breakpoints_system_02_t* ctx, void* params);
int debugging_breakpoints_system_02_destroy_system(debugging_breakpoints_system_02_t* ctx, void* params);
int debugging_breakpoints_system_02_tick(debugging_breakpoints_system_02_t* ctx, void* params);
int debugging_breakpoints_system_02_process(debugging_breakpoints_system_02_t* ctx, void* params);
int debugging_breakpoints_system_02_submit(debugging_breakpoints_system_02_t* ctx, void* params);
int debugging_breakpoints_system_02_execute(debugging_breakpoints_system_02_t* ctx, void* params);
int debugging_breakpoints_system_02_sync(debugging_breakpoints_system_02_t* ctx, void* params);
int debugging_breakpoints_system_02_query(debugging_breakpoints_system_02_t* ctx, void* params);
int debugging_breakpoints_system_02_configure(debugging_breakpoints_system_02_t* ctx, void* params);
int debugging_breakpoints_system_02_optimize(debugging_breakpoints_system_02_t* ctx, void* params);

/* Utility functions */
int debugging_breakpoints_system_02_get_stats(debugging_breakpoints_system_02_t* ctx);
int debugging_breakpoints_system_02_set_callback(debugging_breakpoints_system_02_t* ctx);
int debugging_breakpoints_system_02_get_memory_usage(debugging_breakpoints_system_02_t* ctx);
int debugging_breakpoints_system_02_optimize_legacy(debugging_breakpoints_system_02_t* ctx);
int debugging_breakpoints_system_02_debug_print(debugging_breakpoints_system_02_t* ctx);

/* Module functions */
int debugging_breakpoints_system_02_module_init(void);
int debugging_breakpoints_system_02_module_shutdown(void);

#ifdef __cplusplus
}
#endif

#endif /* DEBUGGING_BREAKPOINTS_SYSTEM_02_H */
