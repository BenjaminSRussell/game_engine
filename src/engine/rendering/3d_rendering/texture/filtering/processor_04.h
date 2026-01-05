/*
 * texture_filtering_processor_04.h
 *
 * Header file for texture_filtering_processor_04 implementation
 * Part of the Advanced 3D Rendering Engine
 */

#ifndef TEXTURE_FILTERING_PROCESSOR_04_H
#define TEXTURE_FILTERING_PROCESSOR_04_H

#include <stdint.h>
#include <stdbool.h>
#include <stddef.h>

#ifdef __cplusplus
extern "C" {
#endif

/* Forward declarations */
typedef struct texture_filtering_processor_04 texture_filtering_processor_04_t;
typedef struct texture_filtering_processor_04_desc texture_filtering_processor_04_desc_t;
typedef struct texture_filtering_processor_04_stats texture_filtering_processor_04_stats_t;

/* Creation and destruction */
int texture_filtering_processor_04_create(texture_filtering_processor_04_t** out_ctx, const texture_filtering_processor_04_desc_t* desc);
int texture_filtering_processor_04_destroy(texture_filtering_processor_04_t* ctx);

/* Core operations */
int texture_filtering_processor_04_process_batch(texture_filtering_processor_04_t* ctx, void* params);
int texture_filtering_processor_04_process_single(texture_filtering_processor_04_t* ctx, void* params);
int texture_filtering_processor_04_transform(texture_filtering_processor_04_t* ctx, void* params);
int texture_filtering_processor_04_filter(texture_filtering_processor_04_t* ctx, void* params);
int texture_filtering_processor_04_aggregate(texture_filtering_processor_04_t* ctx, void* params);
int texture_filtering_processor_04_dispatch(texture_filtering_processor_04_t* ctx, void* params);
int texture_filtering_processor_04_finalize(texture_filtering_processor_04_t* ctx, void* params);
int texture_filtering_processor_04_validate_input(texture_filtering_processor_04_t* ctx, void* params);
int texture_filtering_processor_04_optimize_output(texture_filtering_processor_04_t* ctx, void* params);
int texture_filtering_processor_04_profile(texture_filtering_processor_04_t* ctx, void* params);

/* Utility functions */
int texture_filtering_processor_04_get_stats(texture_filtering_processor_04_t* ctx);
int texture_filtering_processor_04_set_callback(texture_filtering_processor_04_t* ctx);
int texture_filtering_processor_04_get_memory_usage(texture_filtering_processor_04_t* ctx);
int texture_filtering_processor_04_optimize(texture_filtering_processor_04_t* ctx);
int texture_filtering_processor_04_debug_print(texture_filtering_processor_04_t* ctx);

/* Module functions */
int texture_filtering_processor_04_module_init(void);
int texture_filtering_processor_04_module_shutdown(void);

#ifdef __cplusplus
}
#endif

#endif /* TEXTURE_FILTERING_PROCESSOR_04_H */
