/*
 * lighting_probes_system_02.h
 *
 * Header file for lighting_probes_system_02 implementation
 * Part of the Advanced 3D Rendering Engine
 */

#ifndef LIGHTING_PROBES_SYSTEM_02_H
#define LIGHTING_PROBES_SYSTEM_02_H

#include <stdint.h>
#include <stdbool.h>
#include <stddef.h>

#ifdef __cplusplus
extern "C" {
#endif

/* Forward declarations */
typedef struct lighting_probes_system_02 lighting_probes_system_02_t;
typedef struct lighting_probes_system_02_desc lighting_probes_system_02_desc_t;
typedef struct lighting_probes_system_02_stats lighting_probes_system_02_stats_t;

/* Creation and destruction */
int lighting_probes_system_02_create(lighting_probes_system_02_t** out_ctx, const lighting_probes_system_02_desc_t* desc);
int lighting_probes_system_02_destroy(lighting_probes_system_02_t* ctx);

/* Core operations */
int lighting_probes_system_02_create_system(lighting_probes_system_02_t* ctx, void* params);
int lighting_probes_system_02_destroy_system(lighting_probes_system_02_t* ctx, void* params);
int lighting_probes_system_02_tick(lighting_probes_system_02_t* ctx, void* params);
int lighting_probes_system_02_process(lighting_probes_system_02_t* ctx, void* params);
int lighting_probes_system_02_submit(lighting_probes_system_02_t* ctx, void* params);
int lighting_probes_system_02_execute(lighting_probes_system_02_t* ctx, void* params);
int lighting_probes_system_02_sync(lighting_probes_system_02_t* ctx, void* params);
int lighting_probes_system_02_query(lighting_probes_system_02_t* ctx, void* params);
int lighting_probes_system_02_configure(lighting_probes_system_02_t* ctx, void* params);
int lighting_probes_system_02_optimize(lighting_probes_system_02_t* ctx, void* params);

/* Utility functions */
int lighting_probes_system_02_get_stats(lighting_probes_system_02_t* ctx);
int lighting_probes_system_02_set_callback(lighting_probes_system_02_t* ctx);
int lighting_probes_system_02_get_memory_usage(lighting_probes_system_02_t* ctx);
int lighting_probes_system_02_optimize(lighting_probes_system_02_t* ctx);
int lighting_probes_system_02_debug_print(lighting_probes_system_02_t* ctx);

/* Module functions */
int lighting_probes_system_02_module_init(void);
int lighting_probes_system_02_module_shutdown(void);

#ifdef __cplusplus
}
#endif

#endif /* LIGHTING_PROBES_SYSTEM_02_H */
