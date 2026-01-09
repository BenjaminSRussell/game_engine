/*
 * io_scene_manager_01.h
 *
 * Header file for io_scene_manager_01 implementation
 * Part of the Advanced 3D Rendering Engine
 */

#ifndef IO_SCENE_MANAGER_01_H
#define IO_SCENE_MANAGER_01_H

#include <stdint.h>
#include <stdbool.h>
#include <stddef.h>

#ifdef __cplusplus
extern "C" {
#endif

/* Forward declarations */
typedef struct io_scene_manager_01 io_scene_manager_01_t;
typedef struct io_scene_manager_01_desc io_scene_manager_01_desc_t;
typedef struct io_scene_manager_01_stats io_scene_manager_01_stats_t;

/* Creation and destruction */
int io_scene_manager_01_create(io_scene_manager_01_t** out_ctx, const io_scene_manager_01_desc_t* desc);
int io_scene_manager_01_destroy(io_scene_manager_01_t* ctx);

/* Core operations */
int io_scene_manager_01_init(io_scene_manager_01_t* ctx, void* params);
int io_scene_manager_01_shutdown(io_scene_manager_01_t* ctx, void* params);
int io_scene_manager_01_update(io_scene_manager_01_t* ctx, void* params);
int io_scene_manager_01_create_legacy(void* ctx, void* params);
int io_scene_manager_01_destroy_legacy(void* ctx, void* params);
int io_scene_manager_01_get(io_scene_manager_01_t* ctx, void* params);
int io_scene_manager_01_set(io_scene_manager_01_t* ctx, void* params);
int io_scene_manager_01_reset(io_scene_manager_01_t* ctx, void* params);
int io_scene_manager_01_validate(io_scene_manager_01_t* ctx, void* params);
int io_scene_manager_01_flush(io_scene_manager_01_t* ctx, void* params);

/* Utility functions */
int io_scene_manager_01_get_stats(io_scene_manager_01_t* ctx);
int io_scene_manager_01_set_callback(io_scene_manager_01_t* ctx);
int io_scene_manager_01_get_memory_usage(io_scene_manager_01_t* ctx);
int io_scene_manager_01_optimize(io_scene_manager_01_t* ctx);
int io_scene_manager_01_debug_print(io_scene_manager_01_t* ctx);

/* Module functions */
int io_scene_manager_01_module_init(void);
int io_scene_manager_01_module_shutdown(void);

#ifdef __cplusplus
}
#endif

#endif /* IO_SCENE_MANAGER_01_H */
