/*
 * debugging_visualization_manager_01.h
 *
 * Header file for debugging_visualization_manager_01 implementation
 * Part of the Advanced 3D Rendering Engine
 */

#ifndef DEBUGGING_VISUALIZATION_MANAGER_01_H
#define DEBUGGING_VISUALIZATION_MANAGER_01_H

#include <stdint.h>
#include <stdbool.h>
#include <stddef.h>

#ifdef __cplusplus
extern "C" {
#endif

/* Forward declarations */
typedef struct debugging_visualization_manager_01 debugging_visualization_manager_01_t;
typedef struct debugging_visualization_manager_01_desc debugging_visualization_manager_01_desc_t;
typedef struct debugging_visualization_manager_01_stats debugging_visualization_manager_01_stats_t;

/* Creation and destruction */
int debugging_visualization_manager_01_create(debugging_visualization_manager_01_t** out_ctx, const debugging_visualization_manager_01_desc_t* desc);
int debugging_visualization_manager_01_destroy(debugging_visualization_manager_01_t* ctx);

/* Core operations */
int debugging_visualization_manager_01_init(debugging_visualization_manager_01_t* ctx, void* params);
int debugging_visualization_manager_01_shutdown(debugging_visualization_manager_01_t* ctx, void* params);
int debugging_visualization_manager_01_update(debugging_visualization_manager_01_t* ctx, void* params);
int debugging_visualization_manager_01_create_legacy(debugging_visualization_manager_01_t* ctx, void* params);
int debugging_visualization_manager_01_destroy_legacy(debugging_visualization_manager_01_t* ctx, void* params);
int debugging_visualization_manager_01_get(debugging_visualization_manager_01_t* ctx, void* params);
int debugging_visualization_manager_01_set(debugging_visualization_manager_01_t* ctx, void* params);
int debugging_visualization_manager_01_reset(debugging_visualization_manager_01_t* ctx, void* params);
int debugging_visualization_manager_01_validate(debugging_visualization_manager_01_t* ctx, void* params);
int debugging_visualization_manager_01_flush(debugging_visualization_manager_01_t* ctx, void* params);

/* Utility functions */
int debugging_visualization_manager_01_get_stats(debugging_visualization_manager_01_t* ctx);
int debugging_visualization_manager_01_set_callback(debugging_visualization_manager_01_t* ctx);
int debugging_visualization_manager_01_get_memory_usage(debugging_visualization_manager_01_t* ctx);
int debugging_visualization_manager_01_optimize(debugging_visualization_manager_01_t* ctx);
int debugging_visualization_manager_01_debug_print(debugging_visualization_manager_01_t* ctx);

/* Module functions */
int debugging_visualization_manager_01_module_init(void);
int debugging_visualization_manager_01_module_shutdown(void);

#ifdef __cplusplus
}
#endif

#endif /* DEBUGGING_VISUALIZATION_MANAGER_01_H */
