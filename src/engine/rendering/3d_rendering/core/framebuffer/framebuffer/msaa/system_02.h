/*
 * framebuffer_msaa_system_02.h
 *
 * Header file for framebuffer_msaa_system_02 implementation
 * Part of the Advanced 3D Rendering Engine
 */

#ifndef FRAMEBUFFER_MSAA_SYSTEM_02_H
#define FRAMEBUFFER_MSAA_SYSTEM_02_H

#include <stdint.h>
#include <stdbool.h>
#include <stddef.h>

#ifdef __cplusplus
extern "C" {
#endif

/* Forward declarations */
typedef struct framebuffer_msaa_system_02 framebuffer_msaa_system_02_t;
typedef struct framebuffer_msaa_system_02_desc framebuffer_msaa_system_02_desc_t;
typedef struct framebuffer_msaa_system_02_stats framebuffer_msaa_system_02_stats_t;

/* Creation and destruction */
int framebuffer_msaa_system_02_create(framebuffer_msaa_system_02_t** out_ctx, const framebuffer_msaa_system_02_desc_t* desc);
int framebuffer_msaa_system_02_destroy(framebuffer_msaa_system_02_t* ctx);

/* Core operations */
int framebuffer_msaa_system_02_create_system(framebuffer_msaa_system_02_t* ctx, void* params);
int framebuffer_msaa_system_02_destroy_system(framebuffer_msaa_system_02_t* ctx, void* params);
int framebuffer_msaa_system_02_tick(framebuffer_msaa_system_02_t* ctx, void* params);
int framebuffer_msaa_system_02_process(framebuffer_msaa_system_02_t* ctx, void* params);
int framebuffer_msaa_system_02_submit(framebuffer_msaa_system_02_t* ctx, void* params);
int framebuffer_msaa_system_02_execute(framebuffer_msaa_system_02_t* ctx, void* params);
int framebuffer_msaa_system_02_sync(framebuffer_msaa_system_02_t* ctx, void* params);
int framebuffer_msaa_system_02_query(framebuffer_msaa_system_02_t* ctx, void* params);
int framebuffer_msaa_system_02_configure(framebuffer_msaa_system_02_t* ctx, void* params);
int framebuffer_msaa_system_02_optimize(framebuffer_msaa_system_02_t* ctx, void* params);

/* Utility functions */
int framebuffer_msaa_system_02_get_stats(framebuffer_msaa_system_02_t* ctx);
int framebuffer_msaa_system_02_set_callback(framebuffer_msaa_system_02_t* ctx);
int framebuffer_msaa_system_02_get_memory_usage(framebuffer_msaa_system_02_t* ctx);
int framebuffer_msaa_system_02_optimize(framebuffer_msaa_system_02_t* ctx);
int framebuffer_msaa_system_02_debug_print(framebuffer_msaa_system_02_t* ctx);

/* Module functions */
int framebuffer_msaa_system_02_module_init(void);
int framebuffer_msaa_system_02_module_shutdown(void);

#ifdef __cplusplus
}
#endif

#endif /* FRAMEBUFFER_MSAA_SYSTEM_02_H */
