/*
 * framebuffer_formats_system_02.h
 *
 * Header file for framebuffer_formats_system_02 implementation
 * Part of the Advanced 3D Rendering Engine
 */

#ifndef FRAMEBUFFER_FORMATS_SYSTEM_02_H
#define FRAMEBUFFER_FORMATS_SYSTEM_02_H

#include <stdint.h>
#include <stdbool.h>
#include <stddef.h>

#ifdef __cplusplus
extern "C" {
#endif

/* Forward declarations */
typedef struct framebuffer_formats_system_02 framebuffer_formats_system_02_t;
typedef struct framebuffer_formats_system_02_desc framebuffer_formats_system_02_desc_t;
typedef struct framebuffer_formats_system_02_stats framebuffer_formats_system_02_stats_t;

/* Creation and destruction */
int framebuffer_formats_system_02_create(framebuffer_formats_system_02_t** out_ctx, const framebuffer_formats_system_02_desc_t* desc);
int framebuffer_formats_system_02_destroy(framebuffer_formats_system_02_t* ctx);

/* Core operations */
int framebuffer_formats_system_02_create_system(framebuffer_formats_system_02_t* ctx, void* params);
int framebuffer_formats_system_02_destroy_system(framebuffer_formats_system_02_t* ctx, void* params);
int framebuffer_formats_system_02_tick(framebuffer_formats_system_02_t* ctx, void* params);
int framebuffer_formats_system_02_process(framebuffer_formats_system_02_t* ctx, void* params);
int framebuffer_formats_system_02_submit(framebuffer_formats_system_02_t* ctx, void* params);
int framebuffer_formats_system_02_execute(framebuffer_formats_system_02_t* ctx, void* params);
int framebuffer_formats_system_02_sync(framebuffer_formats_system_02_t* ctx, void* params);
int framebuffer_formats_system_02_query(framebuffer_formats_system_02_t* ctx, void* params);
int framebuffer_formats_system_02_configure(framebuffer_formats_system_02_t* ctx, void* params);
int framebuffer_formats_system_02_optimize(framebuffer_formats_system_02_t* ctx, void* params);

/* Utility functions */
int framebuffer_formats_system_02_get_stats(framebuffer_formats_system_02_t* ctx);
int framebuffer_formats_system_02_set_callback(framebuffer_formats_system_02_t* ctx);
int framebuffer_formats_system_02_get_memory_usage(framebuffer_formats_system_02_t* ctx);
int framebuffer_formats_system_02_optimize(framebuffer_formats_system_02_t* ctx);
int framebuffer_formats_system_02_debug_print(framebuffer_formats_system_02_t* ctx);

/* Module functions */
int framebuffer_formats_system_02_module_init(void);
int framebuffer_formats_system_02_module_shutdown(void);

#ifdef __cplusplus
}
#endif

#endif /* FRAMEBUFFER_FORMATS_SYSTEM_02_H */
