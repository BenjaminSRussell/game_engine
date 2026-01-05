/*
 * effects_vfx_builder_05.h
 *
 * Header file for effects_vfx_builder_05 implementation
 * Part of the Advanced 3D Rendering Engine
 */

#ifndef EFFECTS_VFX_BUILDER_05_H
#define EFFECTS_VFX_BUILDER_05_H

#include <stdint.h>
#include <stdbool.h>
#include <stddef.h>

#ifdef __cplusplus
extern "C" {
#endif

/* Forward declarations */
typedef struct effects_vfx_builder_05 effects_vfx_builder_05_t;
typedef struct effects_vfx_builder_05_desc effects_vfx_builder_05_desc_t;
typedef struct effects_vfx_builder_05_stats effects_vfx_builder_05_stats_t;

/* Creation and destruction */
int effects_vfx_builder_05_create(effects_vfx_builder_05_t** out_ctx, const effects_vfx_builder_05_desc_t* desc);
int effects_vfx_builder_05_destroy(effects_vfx_builder_05_t* ctx);

/* Core operations */
int effects_vfx_builder_05_begin(effects_vfx_builder_05_t* ctx, void* params);
int effects_vfx_builder_05_end(effects_vfx_builder_05_t* ctx, void* params);
int effects_vfx_builder_05_add(effects_vfx_builder_05_t* ctx, void* params);
int effects_vfx_builder_05_remove(effects_vfx_builder_05_t* ctx, void* params);
int effects_vfx_builder_05_modify(effects_vfx_builder_05_t* ctx, void* params);
int effects_vfx_builder_05_finalize(effects_vfx_builder_05_t* ctx, void* params);
int effects_vfx_builder_05_validate(effects_vfx_builder_05_t* ctx, void* params);
int effects_vfx_builder_05_optimize(effects_vfx_builder_05_t* ctx, void* params);
int effects_vfx_builder_05_compile(effects_vfx_builder_05_t* ctx, void* params);
int effects_vfx_builder_05_link(effects_vfx_builder_05_t* ctx, void* params);

/* Utility functions */
int effects_vfx_builder_05_get_stats(effects_vfx_builder_05_t* ctx);
int effects_vfx_builder_05_set_callback(effects_vfx_builder_05_t* ctx);
int effects_vfx_builder_05_get_memory_usage(effects_vfx_builder_05_t* ctx);
int effects_vfx_builder_05_optimize(effects_vfx_builder_05_t* ctx);
int effects_vfx_builder_05_debug_print(effects_vfx_builder_05_t* ctx);

/* Module functions */
int effects_vfx_builder_05_module_init(void);
int effects_vfx_builder_05_module_shutdown(void);

#ifdef __cplusplus
}
#endif

#endif /* EFFECTS_VFX_BUILDER_05_H */
