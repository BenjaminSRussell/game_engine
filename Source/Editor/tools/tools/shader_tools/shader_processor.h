/*
 * tools_shader_tools_processor_04.h
 *
 * Header file for tools_shader_tools_processor_04 implementation
 * Part of the Advanced 3D Rendering Engine
 */

#ifndef TOOLS_SHADER_TOOLS_PROCESSOR_04_H
#define TOOLS_SHADER_TOOLS_PROCESSOR_04_H

#include <stdint.h>
#include <stdbool.h>
#include <stddef.h>

#ifdef __cplusplus
extern "C" {
#endif

/* Forward declarations */
typedef struct tools_shader_tools_processor_04 tools_shader_tools_processor_04_t;
typedef struct tools_shader_tools_processor_04_desc tools_shader_tools_processor_04_desc_t;
typedef struct tools_shader_tools_processor_04_stats tools_shader_tools_processor_04_stats_t;

/* Creation and destruction */
int tools_shader_tools_processor_04_create(tools_shader_tools_processor_04_t** out_ctx, const tools_shader_tools_processor_04_desc_t* desc);
int tools_shader_tools_processor_04_destroy(tools_shader_tools_processor_04_t* ctx);

/* Core operations */
int tools_shader_tools_processor_04_process_batch(tools_shader_tools_processor_04_t* ctx, void* params);
int tools_shader_tools_processor_04_process_single(tools_shader_tools_processor_04_t* ctx, void* params);
int tools_shader_tools_processor_04_transform(tools_shader_tools_processor_04_t* ctx, void* params);
int tools_shader_tools_processor_04_filter(tools_shader_tools_processor_04_t* ctx, void* params);
int tools_shader_tools_processor_04_aggregate(tools_shader_tools_processor_04_t* ctx, void* params);
int tools_shader_tools_processor_04_dispatch(tools_shader_tools_processor_04_t* ctx, void* params);
int tools_shader_tools_processor_04_finalize(tools_shader_tools_processor_04_t* ctx, void* params);
int tools_shader_tools_processor_04_validate_input(tools_shader_tools_processor_04_t* ctx, void* params);
int tools_shader_tools_processor_04_optimize_output(tools_shader_tools_processor_04_t* ctx, void* params);
int tools_shader_tools_processor_04_profile(tools_shader_tools_processor_04_t* ctx, void* params);

/* Utility functions */
int tools_shader_tools_processor_04_get_stats(tools_shader_tools_processor_04_t* ctx);
int tools_shader_tools_processor_04_set_callback(tools_shader_tools_processor_04_t* ctx);
int tools_shader_tools_processor_04_get_memory_usage(tools_shader_tools_processor_04_t* ctx);
int tools_shader_tools_processor_04_optimize(tools_shader_tools_processor_04_t* ctx);
int tools_shader_tools_processor_04_debug_print(tools_shader_tools_processor_04_t* ctx);

/* Module functions */
int tools_shader_tools_processor_04_module_init(void);
int tools_shader_tools_processor_04_module_shutdown(void);

#ifdef __cplusplus
}
#endif

#endif /* TOOLS_SHADER_TOOLS_PROCESSOR_04_H */
