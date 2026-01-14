/*
 * io_async_manager_01.h
 *
 * Header file for io_async_manager_01 implementation
 * Part of the Advanced 3D Rendering Engine
 */

#ifndef IO_ASYNC_MANAGER_01_H
#define IO_ASYNC_MANAGER_01_H

#include <stdint.h>
#include <stdbool.h>
#include <stddef.h>

#ifdef __cplusplus
extern "C" {
#endif

/* Forward declarations */
typedef struct io_async_manager_01 io_async_manager_01_t;
typedef struct io_async_manager_01_desc io_async_manager_01_desc_t;
typedef struct io_async_manager_01_stats io_async_manager_01_stats_t;

/* Creation and destruction */
int io_async_manager_01_create(io_async_manager_01_t** out_ctx, const io_async_manager_01_desc_t* desc);
int io_async_manager_01_destroy(io_async_manager_01_t* ctx);

/* Core operations */
int io_async_manager_01_init(io_async_manager_01_t* ctx, void* params);
int io_async_manager_01_shutdown(io_async_manager_01_t* ctx, void* params);
int io_async_manager_01_update(io_async_manager_01_t* ctx, void* params);
int io_async_manager_01_create_legacy(io_async_manager_01_t* ctx, void* params);
int io_async_manager_01_destroy_legacy(io_async_manager_01_t* ctx, void* params);
int io_async_manager_01_get(io_async_manager_01_t* ctx, void* params);
int io_async_manager_01_set(io_async_manager_01_t* ctx, void* params);
int io_async_manager_01_reset(io_async_manager_01_t* ctx, void* params);
int io_async_manager_01_validate(io_async_manager_01_t* ctx, void* params);
int io_async_manager_01_flush(io_async_manager_01_t* ctx, void* params);

/* Utility functions */
int io_async_manager_01_get_stats(io_async_manager_01_t* ctx);
int io_async_manager_01_set_callback(io_async_manager_01_t* ctx);
int io_async_manager_01_get_memory_usage(io_async_manager_01_t* ctx);
int io_async_manager_01_optimize(io_async_manager_01_t* ctx);
int io_async_manager_01_debug_print(io_async_manager_01_t* ctx);

/* Module functions */
int io_async_manager_01_module_init(void);
int io_async_manager_01_module_shutdown(void);

#ifdef __cplusplus
}
#endif

#endif /* IO_ASYNC_MANAGER_01_H */
