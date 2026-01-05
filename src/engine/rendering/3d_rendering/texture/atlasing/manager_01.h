/*
 * texture_atlasing_manager_01.h
 *
 * Header file for texture_atlasing_manager_01 implementation
 * Part of the Advanced 3D Rendering Engine
 */

#ifndef TEXTURE_ATLASING_MANAGER_01_H
#define TEXTURE_ATLASING_MANAGER_01_H

#include <stdint.h>
#include <stdbool.h>
#include <stddef.h>

#ifdef __cplusplus
extern "C" {
#endif

/* Forward declarations */
typedef struct texture_atlasing_manager_01 texture_atlasing_manager_01_t;
typedef struct texture_atlasing_manager_01_desc texture_atlasing_manager_01_desc_t;
typedef struct texture_atlasing_manager_01_stats texture_atlasing_manager_01_stats_t;

/* Creation and destruction */
int texture_atlasing_manager_01_create(texture_atlasing_manager_01_t** out_ctx, const texture_atlasing_manager_01_desc_t* desc);
int texture_atlasing_manager_01_destroy(texture_atlasing_manager_01_t* ctx);

/* Core operations */
int texture_atlasing_manager_01_init(texture_atlasing_manager_01_t* ctx, void* params);
int texture_atlasing_manager_01_shutdown(texture_atlasing_manager_01_t* ctx, void* params);
int texture_atlasing_manager_01_update(texture_atlasing_manager_01_t* ctx, void* params);
int texture_atlasing_manager_01_create(texture_atlasing_manager_01_t* ctx, void* params);
int texture_atlasing_manager_01_destroy(texture_atlasing_manager_01_t* ctx, void* params);
int texture_atlasing_manager_01_get(texture_atlasing_manager_01_t* ctx, void* params);
int texture_atlasing_manager_01_set(texture_atlasing_manager_01_t* ctx, void* params);
int texture_atlasing_manager_01_reset(texture_atlasing_manager_01_t* ctx, void* params);
int texture_atlasing_manager_01_validate(texture_atlasing_manager_01_t* ctx, void* params);
int texture_atlasing_manager_01_flush(texture_atlasing_manager_01_t* ctx, void* params);

/* Utility functions */
int texture_atlasing_manager_01_get_stats(texture_atlasing_manager_01_t* ctx);
int texture_atlasing_manager_01_set_callback(texture_atlasing_manager_01_t* ctx);
int texture_atlasing_manager_01_get_memory_usage(texture_atlasing_manager_01_t* ctx);
int texture_atlasing_manager_01_optimize(texture_atlasing_manager_01_t* ctx);
int texture_atlasing_manager_01_debug_print(texture_atlasing_manager_01_t* ctx);

/* Module functions */
int texture_atlasing_manager_01_module_init(void);
int texture_atlasing_manager_01_module_shutdown(void);

#ifdef __cplusplus
}
#endif

#endif /* TEXTURE_ATLASING_MANAGER_01_H */
