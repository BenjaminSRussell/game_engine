/*
 * landscape_erosion_builder_05.h
 *
 * Header file for landscape_erosion_builder_05 implementation
 * Part of the Advanced 3D Rendering Engine
 */

#ifndef LANDSCAPE_EROSION_BUILDER_05_H
#define LANDSCAPE_EROSION_BUILDER_05_H

#include <stdint.h>
#include <stdbool.h>
#include <stddef.h>

#ifdef __cplusplus
extern "C" {
#endif

/* Forward declarations */
typedef struct landscape_erosion_builder_05 landscape_erosion_builder_05_t;
typedef struct landscape_erosion_builder_05_desc landscape_erosion_builder_05_desc_t;
typedef struct landscape_erosion_builder_05_stats landscape_erosion_builder_05_stats_t;

/* Creation and destruction */
int landscape_erosion_builder_05_create(landscape_erosion_builder_05_t** out_ctx, const landscape_erosion_builder_05_desc_t* desc);
int landscape_erosion_builder_05_destroy(landscape_erosion_builder_05_t* ctx);

/* Core operations */
int landscape_erosion_builder_05_begin(landscape_erosion_builder_05_t* ctx, void* params);
int landscape_erosion_builder_05_end(landscape_erosion_builder_05_t* ctx, void* params);
int landscape_erosion_builder_05_add(landscape_erosion_builder_05_t* ctx, void* params);
int landscape_erosion_builder_05_remove(landscape_erosion_builder_05_t* ctx, void* params);
int landscape_erosion_builder_05_modify(landscape_erosion_builder_05_t* ctx, void* params);
int landscape_erosion_builder_05_finalize(landscape_erosion_builder_05_t* ctx, void* params);
int landscape_erosion_builder_05_validate(landscape_erosion_builder_05_t* ctx, void* params);
int landscape_erosion_builder_05_optimize(landscape_erosion_builder_05_t* ctx, void* params);
int landscape_erosion_builder_05_compile(landscape_erosion_builder_05_t* ctx, void* params);
int landscape_erosion_builder_05_link(landscape_erosion_builder_05_t* ctx, void* params);

/* Utility functions */
int landscape_erosion_builder_05_get_stats(landscape_erosion_builder_05_t* ctx);
int landscape_erosion_builder_05_set_callback(landscape_erosion_builder_05_t* ctx);
int landscape_erosion_builder_05_get_memory_usage(landscape_erosion_builder_05_t* ctx);
int landscape_erosion_builder_05_optimize(landscape_erosion_builder_05_t* ctx);
int landscape_erosion_builder_05_debug_print(landscape_erosion_builder_05_t* ctx);

/* Module functions */
int landscape_erosion_builder_05_module_init(void);
int landscape_erosion_builder_05_module_shutdown(void);

#ifdef __cplusplus
}
#endif

#endif /* LANDSCAPE_EROSION_BUILDER_05_H */
