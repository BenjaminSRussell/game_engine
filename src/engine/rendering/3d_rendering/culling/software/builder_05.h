/*
 * culling_software_builder_05.h
 *
 * Header file for culling_software_builder_05 implementation
 * Part of the Advanced 3D Rendering Engine
 */

#ifndef CULLING_SOFTWARE_BUILDER_05_H
#define CULLING_SOFTWARE_BUILDER_05_H

#include <stdint.h>
#include <stdbool.h>
#include <stddef.h>

#ifdef __cplusplus
extern "C" {
#endif

/* Forward declarations */
typedef struct culling_software_builder_05 culling_software_builder_05_t;
typedef struct culling_software_builder_05_desc culling_software_builder_05_desc_t;
typedef struct culling_software_builder_05_stats culling_software_builder_05_stats_t;

/* Creation and destruction */
int culling_software_builder_05_create(culling_software_builder_05_t** out_ctx, const culling_software_builder_05_desc_t* desc);
int culling_software_builder_05_destroy(culling_software_builder_05_t* ctx);

/* Core operations */
int culling_software_builder_05_begin(culling_software_builder_05_t* ctx, void* params);
int culling_software_builder_05_end(culling_software_builder_05_t* ctx, void* params);
int culling_software_builder_05_add(culling_software_builder_05_t* ctx, void* params);
int culling_software_builder_05_remove(culling_software_builder_05_t* ctx, void* params);
int culling_software_builder_05_modify(culling_software_builder_05_t* ctx, void* params);
int culling_software_builder_05_finalize(culling_software_builder_05_t* ctx, void* params);
int culling_software_builder_05_validate(culling_software_builder_05_t* ctx, void* params);
int culling_software_builder_05_optimize(culling_software_builder_05_t* ctx, void* params);
int culling_software_builder_05_compile(culling_software_builder_05_t* ctx, void* params);
int culling_software_builder_05_link(culling_software_builder_05_t* ctx, void* params);

/* Utility functions */
int culling_software_builder_05_get_stats(culling_software_builder_05_t* ctx);
int culling_software_builder_05_set_callback(culling_software_builder_05_t* ctx);
int culling_software_builder_05_get_memory_usage(culling_software_builder_05_t* ctx);
int culling_software_builder_05_optimize(culling_software_builder_05_t* ctx);
int culling_software_builder_05_debug_print(culling_software_builder_05_t* ctx);

/* Module functions */
int culling_software_builder_05_module_init(void);
int culling_software_builder_05_module_shutdown(void);

#ifdef __cplusplus
}
#endif

#endif /* CULLING_SOFTWARE_BUILDER_05_H */
