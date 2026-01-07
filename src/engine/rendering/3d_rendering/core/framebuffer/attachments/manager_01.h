/*
 * framebuffer_attachments_manager_01.h
 *
 * Header file for framebuffer_attachments_manager_01 implementation
 * Part of the Advanced 3D Rendering Engine
 */

#ifndef FRAMEBUFFER_ATTACHMENTS_MANAGER_01_H
#define FRAMEBUFFER_ATTACHMENTS_MANAGER_01_H

#include <stdint.h>
#include <stdbool.h>
#include <stddef.h>

#ifdef __cplusplus
extern "C" {
#endif

/* Forward declarations */
typedef struct framebuffer_attachments_manager_01 framebuffer_attachments_manager_01_t;
typedef struct framebuffer_attachments_manager_01_desc framebuffer_attachments_manager_01_desc_t;
typedef struct framebuffer_attachments_manager_01_stats framebuffer_attachments_manager_01_stats_t;

/* Creation and destruction */
int framebuffer_attachments_manager_01_create(framebuffer_attachments_manager_01_t** out_ctx, const framebuffer_attachments_manager_01_desc_t* desc);
int framebuffer_attachments_manager_01_destroy(framebuffer_attachments_manager_01_t* ctx);

/* Core operations */
int framebuffer_attachments_manager_01_init(framebuffer_attachments_manager_01_t* ctx, void* params);
int framebuffer_attachments_manager_01_shutdown(framebuffer_attachments_manager_01_t* ctx, void* params);
int framebuffer_attachments_manager_01_update(framebuffer_attachments_manager_01_t* ctx, void* params);
int framebuffer_attachments_manager_01_create(framebuffer_attachments_manager_01_t* ctx, void* params);
int framebuffer_attachments_manager_01_destroy(framebuffer_attachments_manager_01_t* ctx, void* params);
int framebuffer_attachments_manager_01_get(framebuffer_attachments_manager_01_t* ctx, void* params);
int framebuffer_attachments_manager_01_set(framebuffer_attachments_manager_01_t* ctx, void* params);
int framebuffer_attachments_manager_01_reset(framebuffer_attachments_manager_01_t* ctx, void* params);
int framebuffer_attachments_manager_01_validate(framebuffer_attachments_manager_01_t* ctx, void* params);
int framebuffer_attachments_manager_01_flush(framebuffer_attachments_manager_01_t* ctx, void* params);

/* Utility functions */
int framebuffer_attachments_manager_01_get_stats(framebuffer_attachments_manager_01_t* ctx);
int framebuffer_attachments_manager_01_set_callback(framebuffer_attachments_manager_01_t* ctx);
int framebuffer_attachments_manager_01_get_memory_usage(framebuffer_attachments_manager_01_t* ctx);
int framebuffer_attachments_manager_01_optimize(framebuffer_attachments_manager_01_t* ctx);
int framebuffer_attachments_manager_01_debug_print(framebuffer_attachments_manager_01_t* ctx);

/* Module functions */
int framebuffer_attachments_manager_01_module_init(void);
int framebuffer_attachments_manager_01_module_shutdown(void);

#ifdef __cplusplus
}
#endif

#endif /* FRAMEBUFFER_ATTACHMENTS_MANAGER_01_H */
