/*
 * editor_viewport_manager_01.h
 *
 * Header file for editor_viewport_manager_01 implementation
 * Part of the Advanced 3D Rendering Engine
 */

#ifndef EDITOR_VIEWPORT_MANAGER_01_H
#define EDITOR_VIEWPORT_MANAGER_01_H

#include <stdint.h>
#include <stdbool.h>
#include <stddef.h>

#ifdef __cplusplus
extern "C" {
#endif

/* Forward declarations */
typedef struct editor_viewport_manager_01 editor_viewport_manager_01_t;
typedef struct editor_viewport_manager_01_desc editor_viewport_manager_01_desc_t;
typedef struct editor_viewport_manager_01_stats editor_viewport_manager_01_stats_t;

/* Creation and destruction */
int editor_viewport_manager_01_create(editor_viewport_manager_01_t** out_ctx, const editor_viewport_manager_01_desc_t* desc);
int editor_viewport_manager_01_destroy(editor_viewport_manager_01_t* ctx);

/* Core operations */
int editor_viewport_manager_01_init(editor_viewport_manager_01_t* ctx, void* params);
int editor_viewport_manager_01_shutdown(editor_viewport_manager_01_t* ctx, void* params);
int editor_viewport_manager_01_update(editor_viewport_manager_01_t* ctx, void* params);
int editor_viewport_manager_01_create(editor_viewport_manager_01_t* ctx, void* params);
int editor_viewport_manager_01_destroy(editor_viewport_manager_01_t* ctx, void* params);
int editor_viewport_manager_01_get(editor_viewport_manager_01_t* ctx, void* params);
int editor_viewport_manager_01_set(editor_viewport_manager_01_t* ctx, void* params);
int editor_viewport_manager_01_reset(editor_viewport_manager_01_t* ctx, void* params);
int editor_viewport_manager_01_validate(editor_viewport_manager_01_t* ctx, void* params);
int editor_viewport_manager_01_flush(editor_viewport_manager_01_t* ctx, void* params);

/* Utility functions */
int editor_viewport_manager_01_get_stats(editor_viewport_manager_01_t* ctx);
int editor_viewport_manager_01_set_callback(editor_viewport_manager_01_t* ctx);
int editor_viewport_manager_01_get_memory_usage(editor_viewport_manager_01_t* ctx);
int editor_viewport_manager_01_optimize(editor_viewport_manager_01_t* ctx);
int editor_viewport_manager_01_debug_print(editor_viewport_manager_01_t* ctx);

/* Module functions */
int editor_viewport_manager_01_module_init(void);
int editor_viewport_manager_01_module_shutdown(void);

#ifdef __cplusplus
}
#endif

#endif /* EDITOR_VIEWPORT_MANAGER_01_H */
