/*
 * materials_parameters_system_02.h
 *
 * Header file for materials_parameters_system_02 implementation
 * Part of the Advanced 3D Rendering Engine
 */

#ifndef MATERIALS_PARAMETERS_SYSTEM_02_H
#define MATERIALS_PARAMETERS_SYSTEM_02_H

#include <stdint.h>
#include <stdbool.h>
#include <stddef.h>

#ifdef __cplusplus
extern "C" {
#endif

/* Forward declarations */
typedef struct materials_parameters_system_02 materials_parameters_system_02_t;
typedef struct materials_parameters_system_02_desc materials_parameters_system_02_desc_t;
typedef struct materials_parameters_system_02_stats materials_parameters_system_02_stats_t;

/* Creation and destruction */
int materials_parameters_system_02_create(materials_parameters_system_02_t** out_ctx, const materials_parameters_system_02_desc_t* desc);
int materials_parameters_system_02_destroy(materials_parameters_system_02_t* ctx);

/* Core operations */
int materials_parameters_system_02_create_system(materials_parameters_system_02_t* ctx, void* params);
int materials_parameters_system_02_destroy_system(materials_parameters_system_02_t* ctx, void* params);
int materials_parameters_system_02_tick(materials_parameters_system_02_t* ctx, void* params);
int materials_parameters_system_02_process(materials_parameters_system_02_t* ctx, void* params);
int materials_parameters_system_02_submit(materials_parameters_system_02_t* ctx, void* params);
int materials_parameters_system_02_execute(materials_parameters_system_02_t* ctx, void* params);
int materials_parameters_system_02_sync(materials_parameters_system_02_t* ctx, void* params);
int materials_parameters_system_02_query(materials_parameters_system_02_t* ctx, void* params);
int materials_parameters_system_02_configure(materials_parameters_system_02_t* ctx, void* params);
int materials_parameters_system_02_optimize(materials_parameters_system_02_t* ctx, void* params);

/* Utility functions */
int materials_parameters_system_02_get_stats(materials_parameters_system_02_t* ctx);
int materials_parameters_system_02_set_callback(materials_parameters_system_02_t* ctx);
int materials_parameters_system_02_get_memory_usage(materials_parameters_system_02_t* ctx);
int materials_parameters_system_02_optimize(materials_parameters_system_02_t* ctx);
int materials_parameters_system_02_debug_print(materials_parameters_system_02_t* ctx);

/* Module functions */
int materials_parameters_system_02_module_init(void);
int materials_parameters_system_02_module_shutdown(void);

#ifdef __cplusplus
}
#endif

#endif /* MATERIALS_PARAMETERS_SYSTEM_02_H */
