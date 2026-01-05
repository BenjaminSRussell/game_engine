/*
 * texture_feedback_system_02.h
 *
 * Header file for texture_feedback_system_02 implementation
 * Part of the Advanced 3D Rendering Engine
 */

#ifndef TEXTURE_FEEDBACK_SYSTEM_02_H
#define TEXTURE_FEEDBACK_SYSTEM_02_H

#include <stdint.h>
#include <stdbool.h>
#include <stddef.h>

#ifdef __cplusplus
extern "C" {
#endif

/* Forward declarations */
typedef struct texture_feedback_system_02 texture_feedback_system_02_t;
typedef struct texture_feedback_system_02_desc texture_feedback_system_02_desc_t;
typedef struct texture_feedback_system_02_stats texture_feedback_system_02_stats_t;

/* Creation and destruction */
int texture_feedback_system_02_create(texture_feedback_system_02_t** out_ctx, const texture_feedback_system_02_desc_t* desc);
int texture_feedback_system_02_destroy(texture_feedback_system_02_t* ctx);

/* Core operations */
int texture_feedback_system_02_create_system(texture_feedback_system_02_t* ctx, void* params);
int texture_feedback_system_02_destroy_system(texture_feedback_system_02_t* ctx, void* params);
int texture_feedback_system_02_tick(texture_feedback_system_02_t* ctx, void* params);
int texture_feedback_system_02_process(texture_feedback_system_02_t* ctx, void* params);
int texture_feedback_system_02_submit(texture_feedback_system_02_t* ctx, void* params);
int texture_feedback_system_02_execute(texture_feedback_system_02_t* ctx, void* params);
int texture_feedback_system_02_sync(texture_feedback_system_02_t* ctx, void* params);
int texture_feedback_system_02_query(texture_feedback_system_02_t* ctx, void* params);
int texture_feedback_system_02_configure(texture_feedback_system_02_t* ctx, void* params);
int texture_feedback_system_02_optimize(texture_feedback_system_02_t* ctx, void* params);

/* Utility functions */
int texture_feedback_system_02_get_stats(texture_feedback_system_02_t* ctx);
int texture_feedback_system_02_set_callback(texture_feedback_system_02_t* ctx);
int texture_feedback_system_02_get_memory_usage(texture_feedback_system_02_t* ctx);
int texture_feedback_system_02_optimize(texture_feedback_system_02_t* ctx);
int texture_feedback_system_02_debug_print(texture_feedback_system_02_t* ctx);

/* Module functions */
int texture_feedback_system_02_module_init(void);
int texture_feedback_system_02_module_shutdown(void);

#ifdef __cplusplus
}
#endif

#endif /* TEXTURE_FEEDBACK_SYSTEM_02_H */
