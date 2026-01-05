/*
 * io_serialization_manager_01.h
 *
 * Header file for io_serialization_manager_01 implementation
 * Part of the Advanced 3D Rendering Engine
 */

#ifndef IO_SERIALIZATION_MANAGER_01_H
#define IO_SERIALIZATION_MANAGER_01_H

#include <stdint.h>
#include <stdbool.h>
#include <stddef.h>

#ifdef __cplusplus
extern "C" {
#endif

/* Forward declarations */
typedef struct io_serialization_manager_01 io_serialization_manager_01_t;
typedef struct io_serialization_manager_01_desc io_serialization_manager_01_desc_t;
typedef struct io_serialization_manager_01_stats io_serialization_manager_01_stats_t;

/* Creation and destruction */
int io_serialization_manager_01_create(io_serialization_manager_01_t** out_ctx, const io_serialization_manager_01_desc_t* desc);
int io_serialization_manager_01_destroy(io_serialization_manager_01_t* ctx);

/* Core operations */
int io_serialization_manager_01_init(io_serialization_manager_01_t* ctx, void* params);
int io_serialization_manager_01_shutdown(io_serialization_manager_01_t* ctx, void* params);
int io_serialization_manager_01_update(io_serialization_manager_01_t* ctx, void* params);
int io_serialization_manager_01_create(io_serialization_manager_01_t* ctx, void* params);
int io_serialization_manager_01_destroy(io_serialization_manager_01_t* ctx, void* params);
int io_serialization_manager_01_get(io_serialization_manager_01_t* ctx, void* params);
int io_serialization_manager_01_set(io_serialization_manager_01_t* ctx, void* params);
int io_serialization_manager_01_reset(io_serialization_manager_01_t* ctx, void* params);
int io_serialization_manager_01_validate(io_serialization_manager_01_t* ctx, void* params);
int io_serialization_manager_01_flush(io_serialization_manager_01_t* ctx, void* params);

/* Utility functions */
int io_serialization_manager_01_get_stats(io_serialization_manager_01_t* ctx);
int io_serialization_manager_01_set_callback(io_serialization_manager_01_t* ctx);
int io_serialization_manager_01_get_memory_usage(io_serialization_manager_01_t* ctx);
int io_serialization_manager_01_optimize(io_serialization_manager_01_t* ctx);
int io_serialization_manager_01_debug_print(io_serialization_manager_01_t* ctx);

/* Module functions */
int io_serialization_manager_01_module_init(void);
int io_serialization_manager_01_module_shutdown(void);

#ifdef __cplusplus
}
#endif

#endif /* IO_SERIALIZATION_MANAGER_01_H */
