/*
 * editor_gizmos_system_02.h
 *
 * Header file for editor_gizmos_system_02 implementation
 * Part of the Advanced 3D Rendering Engine
 */

#ifndef EDITOR_GIZMOS_SYSTEM_02_H
#define EDITOR_GIZMOS_SYSTEM_02_H

#include <stdint.h>
#include <stdbool.h>
#include <stddef.h>

#ifdef __cplusplus
extern "C" {
#endif

/* Forward declarations */
typedef struct editor_gizmos_system_02 editor_gizmos_system_02_t;
typedef struct editor_gizmos_system_02_desc editor_gizmos_system_02_desc_t;
typedef struct editor_gizmos_system_02_stats editor_gizmos_system_02_stats_t;

/* Creation and destruction */
int editor_gizmos_system_02_create(editor_gizmos_system_02_t** out_ctx, const editor_gizmos_system_02_desc_t* desc);
int editor_gizmos_system_02_destroy(editor_gizmos_system_02_t* ctx);

/* Core operations */
int editor_gizmos_system_02_create_system(editor_gizmos_system_02_t* ctx, void* params);
int editor_gizmos_system_02_destroy_system(editor_gizmos_system_02_t* ctx, void* params);
int editor_gizmos_system_02_tick(editor_gizmos_system_02_t* ctx, void* params);
int editor_gizmos_system_02_process(editor_gizmos_system_02_t* ctx, void* params);
int editor_gizmos_system_02_submit(editor_gizmos_system_02_t* ctx, void* params);
int editor_gizmos_system_02_execute(editor_gizmos_system_02_t* ctx, void* params);
int editor_gizmos_system_02_sync(editor_gizmos_system_02_t* ctx, void* params);
int editor_gizmos_system_02_query(editor_gizmos_system_02_t* ctx, void* params);
int editor_gizmos_system_02_configure(editor_gizmos_system_02_t* ctx, void* params);
int editor_gizmos_system_02_optimize(editor_gizmos_system_02_t* ctx, void* params);

/* Utility functions */
int editor_gizmos_system_02_get_stats(editor_gizmos_system_02_t* ctx);
int editor_gizmos_system_02_set_callback(editor_gizmos_system_02_t* ctx);
int editor_gizmos_system_02_get_memory_usage(editor_gizmos_system_02_t* ctx);
int editor_gizmos_system_02_optimize(editor_gizmos_system_02_t* ctx);
int editor_gizmos_system_02_debug_print(editor_gizmos_system_02_t* ctx);

/* Module functions */
int editor_gizmos_system_02_module_init(void);
int editor_gizmos_system_02_module_shutdown(void);

#ifdef __cplusplus
}
#endif

#endif /* EDITOR_GIZMOS_SYSTEM_02_H */
