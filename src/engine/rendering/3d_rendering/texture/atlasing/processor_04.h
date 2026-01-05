/*
 * texture_atlasing_processor_04.h
 *
 * Header file for texture_atlasing_processor_04 implementation
 * Part of the Advanced 3D Rendering Engine
 */

#ifndef TEXTURE_ATLASING_PROCESSOR_04_H
#define TEXTURE_ATLASING_PROCESSOR_04_H

#include <stdint.h>
#include <stdbool.h>
#include <stddef.h>

#ifdef __cplusplus
extern "C" {
#endif

/* Forward declarations */
typedef struct texture_atlasing_processor_04 texture_atlasing_processor_04_t;
typedef struct texture_atlasing_processor_04_desc texture_atlasing_processor_04_desc_t;
typedef struct texture_atlasing_processor_04_stats texture_atlasing_processor_04_stats_t;

/* Creation and destruction */
int texture_atlasing_processor_04_create(texture_atlasing_processor_04_t** out_ctx, const texture_atlasing_processor_04_desc_t* desc);
int texture_atlasing_processor_04_destroy(texture_atlasing_processor_04_t* ctx);

/* Core operations */
int texture_atlasing_processor_04_process_batch(texture_atlasing_processor_04_t* ctx, void* params);
int texture_atlasing_processor_04_process_single(texture_atlasing_processor_04_t* ctx, void* params);
int texture_atlasing_processor_04_transform(texture_atlasing_processor_04_t* ctx, void* params);
int texture_atlasing_processor_04_filter(texture_atlasing_processor_04_t* ctx, void* params);
int texture_atlasing_processor_04_aggregate(texture_atlasing_processor_04_t* ctx, void* params);
int texture_atlasing_processor_04_dispatch(texture_atlasing_processor_04_t* ctx, void* params);
int texture_atlasing_processor_04_finalize(texture_atlasing_processor_04_t* ctx, void* params);
int texture_atlasing_processor_04_validate_input(texture_atlasing_processor_04_t* ctx, void* params);
int texture_atlasing_processor_04_optimize_output(texture_atlasing_processor_04_t* ctx, void* params);
int texture_atlasing_processor_04_profile(texture_atlasing_processor_04_t* ctx, void* params);

/* Utility functions */
int texture_atlasing_processor_04_get_stats(texture_atlasing_processor_04_t* ctx);
int texture_atlasing_processor_04_set_callback(texture_atlasing_processor_04_t* ctx);
int texture_atlasing_processor_04_get_memory_usage(texture_atlasing_processor_04_t* ctx);
int texture_atlasing_processor_04_optimize(texture_atlasing_processor_04_t* ctx);
int texture_atlasing_processor_04_debug_print(texture_atlasing_processor_04_t* ctx);

/* Module functions */
int texture_atlasing_processor_04_module_init(void);
int texture_atlasing_processor_04_module_shutdown(void);

#ifdef __cplusplus
}
#endif

#endif /* TEXTURE_ATLASING_PROCESSOR_04_H */
