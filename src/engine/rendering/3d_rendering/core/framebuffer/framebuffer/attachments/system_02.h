/*
 * framebuffer_attachments_system_02.h
 *
 * Header file for framebuffer_attachments_system_02 implementation
 * Part of the Advanced 3D Rendering Engine
 */

#ifndef FRAMEBUFFER_ATTACHMENTS_SYSTEM_02_H
#define FRAMEBUFFER_ATTACHMENTS_SYSTEM_02_H

#include <stdint.h>
#include <stdbool.h>
#include <stddef.h>

#ifdef __cplusplus
extern "C" {
#endif

/* Forward declarations */
typedef struct framebuffer_attachments_system_02 framebuffer_attachments_system_02_t;
typedef struct framebuffer_attachments_system_02_desc framebuffer_attachments_system_02_desc_t;
typedef struct framebuffer_attachments_system_02_stats framebuffer_attachments_system_02_stats_t;

/* Creation and destruction */
int framebuffer_attachments_system_02_create(framebuffer_attachments_system_02_t** out_ctx, const framebuffer_attachments_system_02_desc_t* desc);
int framebuffer_attachments_system_02_destroy(framebuffer_attachments_system_02_t* ctx);

/* Core operations */
int framebuffer_attachments_system_02_create_system(framebuffer_attachments_system_02_t* ctx, void* params);
int framebuffer_attachments_system_02_destroy_system(framebuffer_attachments_system_02_t* ctx, void* params);
int framebuffer_attachments_system_02_tick(framebuffer_attachments_system_02_t* ctx, void* params);
int framebuffer_attachments_system_02_process(framebuffer_attachments_system_02_t* ctx, void* params);
int framebuffer_attachments_system_02_submit(framebuffer_attachments_system_02_t* ctx, void* params);
int framebuffer_attachments_system_02_execute(framebuffer_attachments_system_02_t* ctx, void* params);
int framebuffer_attachments_system_02_sync(framebuffer_attachments_system_02_t* ctx, void* params);
int framebuffer_attachments_system_02_query(framebuffer_attachments_system_02_t* ctx, void* params);
int framebuffer_attachments_system_02_configure(framebuffer_attachments_system_02_t* ctx, void* params);
int framebuffer_attachments_system_02_optimize(framebuffer_attachments_system_02_t* ctx, void* params);

/* Utility functions */
int framebuffer_attachments_system_02_get_stats(framebuffer_attachments_system_02_t* ctx);
int framebuffer_attachments_system_02_set_callback(framebuffer_attachments_system_02_t* ctx);
int framebuffer_attachments_system_02_get_memory_usage(framebuffer_attachments_system_02_t* ctx);
int framebuffer_attachments_system_02_optimize(framebuffer_attachments_system_02_t* ctx);
int framebuffer_attachments_system_02_debug_print(framebuffer_attachments_system_02_t* ctx);

/* Module functions */
int framebuffer_attachments_system_02_module_init(void);
int framebuffer_attachments_system_02_module_shutdown(void);

#ifdef __cplusplus
}
#endif

#endif /* FRAMEBUFFER_ATTACHMENTS_SYSTEM_02_H */
