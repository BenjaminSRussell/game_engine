/*
 * landscape_foliage_builder_05.h
 *
 * Header file for landscape_foliage_builder_05 implementation
 * Part of the Advanced 3D Rendering Engine
 */

#ifndef LANDSCAPE_FOLIAGE_BUILDER_05_H
#define LANDSCAPE_FOLIAGE_BUILDER_05_H

#include <stdint.h>
#include <stdbool.h>
#include <stddef.h>

#ifdef __cplusplus
extern "C" {
#endif

/* Forward declarations */
typedef struct landscape_foliage_builder_05 landscape_foliage_builder_05_t;
typedef struct landscape_foliage_builder_05_desc landscape_foliage_builder_05_desc_t;
typedef struct landscape_foliage_builder_05_stats landscape_foliage_builder_05_stats_t;

/* Creation and destruction */
int landscape_foliage_builder_05_create(landscape_foliage_builder_05_t** out_ctx, const landscape_foliage_builder_05_desc_t* desc);
int landscape_foliage_builder_05_destroy(landscape_foliage_builder_05_t* ctx);

/* Core operations */
int landscape_foliage_builder_05_begin(landscape_foliage_builder_05_t* ctx, void* params);
int landscape_foliage_builder_05_end(landscape_foliage_builder_05_t* ctx, void* params);
int landscape_foliage_builder_05_add(landscape_foliage_builder_05_t* ctx, void* params);
int landscape_foliage_builder_05_remove(landscape_foliage_builder_05_t* ctx, void* params);
int landscape_foliage_builder_05_modify(landscape_foliage_builder_05_t* ctx, void* params);
int landscape_foliage_builder_05_finalize(landscape_foliage_builder_05_t* ctx, void* params);
int landscape_foliage_builder_05_validate(landscape_foliage_builder_05_t* ctx, void* params);
int landscape_foliage_builder_05_optimize(landscape_foliage_builder_05_t* ctx, void* params);
int landscape_foliage_builder_05_compile(landscape_foliage_builder_05_t* ctx, void* params);
int landscape_foliage_builder_05_link(landscape_foliage_builder_05_t* ctx, void* params);

/* Utility functions */
int landscape_foliage_builder_05_get_stats(landscape_foliage_builder_05_t* ctx);
int landscape_foliage_builder_05_set_callback(landscape_foliage_builder_05_t* ctx);
int landscape_foliage_builder_05_get_memory_usage(landscape_foliage_builder_05_t* ctx);
int landscape_foliage_builder_05_optimize(landscape_foliage_builder_05_t* ctx);
int landscape_foliage_builder_05_debug_print(landscape_foliage_builder_05_t* ctx);

/* Module functions */
int landscape_foliage_builder_05_module_init(void);
int landscape_foliage_builder_05_module_shutdown(void);

#ifdef __cplusplus
}
#endif

#endif /* LANDSCAPE_FOLIAGE_BUILDER_05_H */
