/*
 * tools_shader_tools_manager_01.h
 *
 * Header file for tools_shader_tools_manager_01 implementation
 * Part of the Advanced 3D Rendering Engine
 */

#ifndef TOOLS_SHADER_TOOLS_MANAGER_01_H
#define TOOLS_SHADER_TOOLS_MANAGER_01_H

#include <stdint.h>
#include <stdbool.h>
#include <stddef.h>

#ifdef __cplusplus
extern "C" {
#endif

/* Forward declarations */
typedef struct tools_shader_tools_manager_01 tools_shader_tools_manager_01_t;
typedef struct tools_shader_tools_manager_01_desc tools_shader_tools_manager_01_desc_t;
typedef struct tools_shader_tools_manager_01_stats tools_shader_tools_manager_01_stats_t;

/* Creation and destruction */
int tools_shader_tools_manager_01_create(tools_shader_tools_manager_01_t** out_ctx, const tools_shader_tools_manager_01_desc_t* desc);
int tools_shader_tools_manager_01_destroy(tools_shader_tools_manager_01_t* ctx);

/* Core operations */
int tools_shader_tools_manager_01_init(tools_shader_tools_manager_01_t* ctx, void* params);
int tools_shader_tools_manager_01_shutdown(tools_shader_tools_manager_01_t* ctx, void* params);
int tools_shader_tools_manager_01_update(tools_shader_tools_manager_01_t* ctx, void* params);
int tools_shader_tools_manager_01_create_legacy(tools_shader_tools_manager_01_t* ctx, void* params);
int tools_shader_tools_manager_01_destroy_legacy(tools_shader_tools_manager_01_t* ctx, void* params);
int tools_shader_tools_manager_01_get(tools_shader_tools_manager_01_t* ctx, void* params);
int tools_shader_tools_manager_01_set(tools_shader_tools_manager_01_t* ctx, void* params);
int tools_shader_tools_manager_01_reset(tools_shader_tools_manager_01_t* ctx, void* params);
int tools_shader_tools_manager_01_validate(tools_shader_tools_manager_01_t* ctx, void* params);
int tools_shader_tools_manager_01_flush(tools_shader_tools_manager_01_t* ctx, void* params);

/* Utility functions */
int tools_shader_tools_manager_01_get_stats(tools_shader_tools_manager_01_t* ctx);
int tools_shader_tools_manager_01_set_callback(tools_shader_tools_manager_01_t* ctx);
int tools_shader_tools_manager_01_get_memory_usage(tools_shader_tools_manager_01_t* ctx);
int tools_shader_tools_manager_01_optimize(tools_shader_tools_manager_01_t* ctx);
int tools_shader_tools_manager_01_debug_print(tools_shader_tools_manager_01_t* ctx);

/* Module functions */
int tools_shader_tools_manager_01_module_init(void);
int tools_shader_tools_manager_01_module_shutdown(void);

#ifdef __cplusplus
}
#endif

#endif /* TOOLS_SHADER_TOOLS_MANAGER_01_H */
