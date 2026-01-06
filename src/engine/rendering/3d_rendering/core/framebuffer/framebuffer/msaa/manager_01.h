/*
 * framebuffer_msaa_manager_01.h
 *
 * Header file for framebuffer_msaa_manager_01 implementation
 * Part of the Advanced 3D Rendering Engine
 */

#ifndef FRAMEBUFFER_MSAA_MANAGER_01_H
#define FRAMEBUFFER_MSAA_MANAGER_01_H

#include <stdint.h>
#include <stdbool.h>
#include <stddef.h>

#ifdef __cplusplus
extern "C" {
#endif

/* Forward declarations */
typedef struct framebuffer_msaa_manager_01 framebuffer_msaa_manager_01_t;
typedef struct framebuffer_msaa_manager_01_desc framebuffer_msaa_manager_01_desc_t;
typedef struct framebuffer_msaa_manager_01_stats framebuffer_msaa_manager_01_stats_t;

/* Creation and destruction */
int framebuffer_msaa_manager_01_create(framebuffer_msaa_manager_01_t** out_ctx, const framebuffer_msaa_manager_01_desc_t* desc);
int framebuffer_msaa_manager_01_destroy(framebuffer_msaa_manager_01_t* ctx);

/* Core operations */
int framebuffer_msaa_manager_01_init(framebuffer_msaa_manager_01_t* ctx, void* params);
int framebuffer_msaa_manager_01_shutdown(framebuffer_msaa_manager_01_t* ctx, void* params);
int framebuffer_msaa_manager_01_update(framebuffer_msaa_manager_01_t* ctx, void* params);
int framebuffer_msaa_manager_01_create(framebuffer_msaa_manager_01_t* ctx, void* params);
int framebuffer_msaa_manager_01_destroy(framebuffer_msaa_manager_01_t* ctx, void* params);
int framebuffer_msaa_manager_01_get(framebuffer_msaa_manager_01_t* ctx, void* params);
int framebuffer_msaa_manager_01_set(framebuffer_msaa_manager_01_t* ctx, void* params);
int framebuffer_msaa_manager_01_reset(framebuffer_msaa_manager_01_t* ctx, void* params);
int framebuffer_msaa_manager_01_validate(framebuffer_msaa_manager_01_t* ctx, void* params);
int framebuffer_msaa_manager_01_flush(framebuffer_msaa_manager_01_t* ctx, void* params);

/* Utility functions */
int framebuffer_msaa_manager_01_get_stats(framebuffer_msaa_manager_01_t* ctx);
int framebuffer_msaa_manager_01_set_callback(framebuffer_msaa_manager_01_t* ctx);
int framebuffer_msaa_manager_01_get_memory_usage(framebuffer_msaa_manager_01_t* ctx);
int framebuffer_msaa_manager_01_optimize(framebuffer_msaa_manager_01_t* ctx);
int framebuffer_msaa_manager_01_debug_print(framebuffer_msaa_manager_01_t* ctx);

/* Module functions */
int framebuffer_msaa_manager_01_module_init(void);
int framebuffer_msaa_manager_01_module_shutdown(void);

#ifdef __cplusplus
}
#endif

#endif /* FRAMEBUFFER_MSAA_MANAGER_01_H */
