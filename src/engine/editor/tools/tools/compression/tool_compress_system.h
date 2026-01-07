/*
 * tools_compression_system_02.h
 *
 * Header file for tools_compression_system_02 implementation
 * Part of the Advanced 3D Rendering Engine
 */

#ifndef TOOLS_COMPRESSION_SYSTEM_02_H
#define TOOLS_COMPRESSION_SYSTEM_02_H

#include <stdint.h>
#include <stdbool.h>
#include <stddef.h>

#ifdef __cplusplus
extern "C" {
#endif

/* Forward declarations */
typedef struct tools_compression_system_02 tools_compression_system_02_t;
typedef struct tools_compression_system_02_desc tools_compression_system_02_desc_t;
typedef struct tools_compression_system_02_stats tools_compression_system_02_stats_t;

/* Creation and destruction */
int tools_compression_system_02_create(tools_compression_system_02_t** out_ctx, const tools_compression_system_02_desc_t* desc);
int tools_compression_system_02_destroy(tools_compression_system_02_t* ctx);

/* Core operations */
int tools_compression_system_02_create_system(tools_compression_system_02_t* ctx, void* params);
int tools_compression_system_02_destroy_system(tools_compression_system_02_t* ctx, void* params);
int tools_compression_system_02_tick(tools_compression_system_02_t* ctx, void* params);
int tools_compression_system_02_process(tools_compression_system_02_t* ctx, void* params);
int tools_compression_system_02_submit(tools_compression_system_02_t* ctx, void* params);
int tools_compression_system_02_execute(tools_compression_system_02_t* ctx, void* params);
int tools_compression_system_02_sync(tools_compression_system_02_t* ctx, void* params);
int tools_compression_system_02_query(tools_compression_system_02_t* ctx, void* params);
int tools_compression_system_02_configure(tools_compression_system_02_t* ctx, void* params);
int tools_compression_system_02_optimize(tools_compression_system_02_t* ctx, void* params);

/* Utility functions */
int tools_compression_system_02_get_stats(tools_compression_system_02_t* ctx);
int tools_compression_system_02_set_callback(tools_compression_system_02_t* ctx);
int tools_compression_system_02_get_memory_usage(tools_compression_system_02_t* ctx);
int tools_compression_system_02_optimize_legacy(tools_compression_system_02_t* ctx);
int tools_compression_system_02_debug_print(tools_compression_system_02_t* ctx);

/* Module functions */
int tools_compression_system_02_module_init(void);
int tools_compression_system_02_module_shutdown(void);

#ifdef __cplusplus
}
#endif

#endif /* TOOLS_COMPRESSION_SYSTEM_02_H */
