/*
 * effects_beams_builder_05.h
 *
 * Header file for effects_beams_builder_05 implementation
 * Part of the Advanced 3D Rendering Engine
 */

#ifndef EFFECTS_BEAMS_BUILDER_05_H
#define EFFECTS_BEAMS_BUILDER_05_H

#include <stdint.h>
#include <stdbool.h>
#include <stddef.h>

#ifdef __cplusplus
extern "C" {
#endif

/* Forward declarations */
typedef struct effects_beams_builder_05 effects_beams_builder_05_t;
typedef struct effects_beams_builder_05_desc effects_beams_builder_05_desc_t;
typedef struct effects_beams_builder_05_stats effects_beams_builder_05_stats_t;

/* Creation and destruction */
int effects_beams_builder_05_create(effects_beams_builder_05_t** out_ctx, const effects_beams_builder_05_desc_t* desc);
int effects_beams_builder_05_destroy(effects_beams_builder_05_t* ctx);

/* Core operations */
int effects_beams_builder_05_begin(effects_beams_builder_05_t* ctx, void* params);
int effects_beams_builder_05_end(effects_beams_builder_05_t* ctx, void* params);
int effects_beams_builder_05_add(effects_beams_builder_05_t* ctx, void* params);
int effects_beams_builder_05_remove(effects_beams_builder_05_t* ctx, void* params);
int effects_beams_builder_05_modify(effects_beams_builder_05_t* ctx, void* params);
int effects_beams_builder_05_finalize(effects_beams_builder_05_t* ctx, void* params);
int effects_beams_builder_05_validate(effects_beams_builder_05_t* ctx, void* params);
int effects_beams_builder_05_optimize(effects_beams_builder_05_t* ctx, void* params);
int effects_beams_builder_05_compile(effects_beams_builder_05_t* ctx, void* params);
int effects_beams_builder_05_link(effects_beams_builder_05_t* ctx, void* params);

/* Utility functions */
int effects_beams_builder_05_get_stats(effects_beams_builder_05_t* ctx);
int effects_beams_builder_05_set_callback(effects_beams_builder_05_t* ctx);
int effects_beams_builder_05_get_memory_usage(effects_beams_builder_05_t* ctx);
int effects_beams_builder_05_optimize(effects_beams_builder_05_t* ctx);
int effects_beams_builder_05_debug_print(effects_beams_builder_05_t* ctx);

/* Module functions */
int effects_beams_builder_05_module_init(void);
int effects_beams_builder_05_module_shutdown(void);

#ifdef __cplusplus
}
#endif

#endif /* EFFECTS_BEAMS_BUILDER_05_H */
