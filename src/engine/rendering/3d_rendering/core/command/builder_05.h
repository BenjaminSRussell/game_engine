/*
 * core_command_builder_05.h
 *
 * Header file for core_command_builder_05 implementation
 * Part of the Advanced 3D Rendering Engine
 */

#ifndef CORE_COMMAND_BUILDER_05_H
#define CORE_COMMAND_BUILDER_05_H

#include <stdint.h>
#include <stdbool.h>
#include <stddef.h>

#ifdef __cplusplus
extern "C" {
#endif

/* Forward declarations */
typedef struct core_command_builder_05 core_command_builder_05_t;
typedef struct core_command_builder_05_desc core_command_builder_05_desc_t;
typedef struct core_command_builder_05_stats core_command_builder_05_stats_t;

/* Creation and destruction */
int core_command_builder_05_create(core_command_builder_05_t** out_ctx, const core_command_builder_05_desc_t* desc);
int core_command_builder_05_destroy(core_command_builder_05_t* ctx);

/* Core operations */
int core_command_builder_05_begin(core_command_builder_05_t* ctx, void* params);
int core_command_builder_05_end(core_command_builder_05_t* ctx, void* params);
int core_command_builder_05_add(core_command_builder_05_t* ctx, void* params);
int core_command_builder_05_remove(core_command_builder_05_t* ctx, void* params);
int core_command_builder_05_modify(core_command_builder_05_t* ctx, void* params);
int core_command_builder_05_finalize(core_command_builder_05_t* ctx, void* params);
int core_command_builder_05_validate(core_command_builder_05_t* ctx, void* params);
int core_command_builder_05_optimize(core_command_builder_05_t* ctx, void* params);
int core_command_builder_05_compile(core_command_builder_05_t* ctx, void* params);
int core_command_builder_05_link(core_command_builder_05_t* ctx, void* params);

/* Utility functions */
int core_command_builder_05_get_stats(core_command_builder_05_t* ctx);
int core_command_builder_05_set_callback(core_command_builder_05_t* ctx);
int core_command_builder_05_get_memory_usage(core_command_builder_05_t* ctx);
int core_command_builder_05_optimize(core_command_builder_05_t* ctx);
int core_command_builder_05_debug_print(core_command_builder_05_t* ctx);

/* Module functions */
int core_command_builder_05_module_init(void);
int core_command_builder_05_module_shutdown(void);

#ifdef __cplusplus
}
#endif

#endif /* CORE_COMMAND_BUILDER_05_H */
