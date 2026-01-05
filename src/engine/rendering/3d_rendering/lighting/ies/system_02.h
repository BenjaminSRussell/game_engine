/*
 * lighting_ies_system_02.h
 *
 * Header file for lighting_ies_system_02 implementation
 * Part of the Advanced 3D Rendering Engine
 */

#ifndef LIGHTING_IES_SYSTEM_02_H
#define LIGHTING_IES_SYSTEM_02_H

#include <stdint.h>
#include <stdbool.h>
#include <stddef.h>

#ifdef __cplusplus
extern "C" {
#endif

/* Forward declarations */
typedef struct lighting_ies_system_02 lighting_ies_system_02_t;
typedef struct lighting_ies_system_02_desc lighting_ies_system_02_desc_t;
typedef struct lighting_ies_system_02_stats lighting_ies_system_02_stats_t;

/* Creation and destruction */
int lighting_ies_system_02_create(lighting_ies_system_02_t** out_ctx, const lighting_ies_system_02_desc_t* desc);
int lighting_ies_system_02_destroy(lighting_ies_system_02_t* ctx);

/* Core operations */
int lighting_ies_system_02_create_system(lighting_ies_system_02_t* ctx, void* params);
int lighting_ies_system_02_destroy_system(lighting_ies_system_02_t* ctx, void* params);
int lighting_ies_system_02_tick(lighting_ies_system_02_t* ctx, void* params);
int lighting_ies_system_02_process(lighting_ies_system_02_t* ctx, void* params);
int lighting_ies_system_02_submit(lighting_ies_system_02_t* ctx, void* params);
int lighting_ies_system_02_execute(lighting_ies_system_02_t* ctx, void* params);
int lighting_ies_system_02_sync(lighting_ies_system_02_t* ctx, void* params);
int lighting_ies_system_02_query(lighting_ies_system_02_t* ctx, void* params);
int lighting_ies_system_02_configure(lighting_ies_system_02_t* ctx, void* params);
int lighting_ies_system_02_optimize(lighting_ies_system_02_t* ctx, void* params);

/* Utility functions */
int lighting_ies_system_02_get_stats(lighting_ies_system_02_t* ctx);
int lighting_ies_system_02_set_callback(lighting_ies_system_02_t* ctx);
int lighting_ies_system_02_get_memory_usage(lighting_ies_system_02_t* ctx);
int lighting_ies_system_02_optimize(lighting_ies_system_02_t* ctx);
int lighting_ies_system_02_debug_print(lighting_ies_system_02_t* ctx);

/* Module functions */
int lighting_ies_system_02_module_init(void);
int lighting_ies_system_02_module_shutdown(void);

#ifdef __cplusplus
}
#endif

#endif /* LIGHTING_IES_SYSTEM_02_H */
