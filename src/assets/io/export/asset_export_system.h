/*
 * io_export_system_02.h
 *
 * Header file for io_export_system_02 implementation
 * Part of the Advanced 3D Rendering Engine
 */

#ifndef IO_EXPORT_SYSTEM_02_H
#define IO_EXPORT_SYSTEM_02_H

#include <stdint.h>
#include <stdbool.h>
#include <stddef.h>

#ifdef __cplusplus
extern "C" {
#endif

/* Forward declarations */
typedef struct io_export_system_02 io_export_system_02_t;
typedef struct io_export_system_02_desc io_export_system_02_desc_t;
typedef struct io_export_system_02_stats io_export_system_02_stats_t;

/* Creation and destruction */
int io_export_system_02_create(io_export_system_02_t** out_ctx, const io_export_system_02_desc_t* desc);
int io_export_system_02_destroy(io_export_system_02_t* ctx);

/* Core operations */
int io_export_system_02_create_system(io_export_system_02_t* ctx, void* params);
int io_export_system_02_destroy_system(io_export_system_02_t* ctx, void* params);
int io_export_system_02_tick(io_export_system_02_t* ctx, void* params);
int io_export_system_02_process(io_export_system_02_t* ctx, void* params);
int io_export_system_02_submit(io_export_system_02_t* ctx, void* params);
int io_export_system_02_execute(io_export_system_02_t* ctx, void* params);
int io_export_system_02_sync(io_export_system_02_t* ctx, void* params);
int io_export_system_02_query(io_export_system_02_t* ctx, void* params);
int io_export_system_02_configure(io_export_system_02_t* ctx, void* params);

/* Utility functions */
int io_export_system_02_get_stats(io_export_system_02_t* ctx);
int io_export_system_02_set_callback(io_export_system_02_t* ctx);
int io_export_system_02_get_memory_usage(io_export_system_02_t* ctx);
int io_export_system_02_optimize_legacy(io_export_system_02_t* ctx);
int io_export_system_02_debug_print(io_export_system_02_t* ctx);

/* Module functions */
int io_export_system_02_module_init(void);
int io_export_system_02_module_shutdown(void);

#ifdef __cplusplus
}
#endif

#endif /* IO_EXPORT_SYSTEM_02_H */
