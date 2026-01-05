/*
 * core_descriptor_processor_04.h
 *
 * Header file for core_descriptor_processor_04 implementation
 * Part of the Advanced 3D Rendering Engine
 */

#ifndef CORE_DESCRIPTOR_PROCESSOR_04_H
#define CORE_DESCRIPTOR_PROCESSOR_04_H

#include <stdint.h>
#include <stdbool.h>
#include <stddef.h>

#ifdef __cplusplus
extern "C" {
#endif

/* Forward declarations */
typedef struct core_descriptor_processor_04 core_descriptor_processor_04_t;
typedef struct core_descriptor_processor_04_desc core_descriptor_processor_04_desc_t;
typedef struct core_descriptor_processor_04_stats core_descriptor_processor_04_stats_t;

/* Creation and destruction */
int core_descriptor_processor_04_create(core_descriptor_processor_04_t** out_ctx, const core_descriptor_processor_04_desc_t* desc);
int core_descriptor_processor_04_destroy(core_descriptor_processor_04_t* ctx);

/* Core operations */
int core_descriptor_processor_04_process_batch(core_descriptor_processor_04_t* ctx, void* params);
int core_descriptor_processor_04_process_single(core_descriptor_processor_04_t* ctx, void* params);
int core_descriptor_processor_04_transform(core_descriptor_processor_04_t* ctx, void* params);
int core_descriptor_processor_04_filter(core_descriptor_processor_04_t* ctx, void* params);
int core_descriptor_processor_04_aggregate(core_descriptor_processor_04_t* ctx, void* params);
int core_descriptor_processor_04_dispatch(core_descriptor_processor_04_t* ctx, void* params);
int core_descriptor_processor_04_finalize(core_descriptor_processor_04_t* ctx, void* params);
int core_descriptor_processor_04_validate_input(core_descriptor_processor_04_t* ctx, void* params);
int core_descriptor_processor_04_optimize_output(core_descriptor_processor_04_t* ctx, void* params);
int core_descriptor_processor_04_profile(core_descriptor_processor_04_t* ctx, void* params);

/* Utility functions */
int core_descriptor_processor_04_get_stats(core_descriptor_processor_04_t* ctx);
int core_descriptor_processor_04_set_callback(core_descriptor_processor_04_t* ctx);
int core_descriptor_processor_04_get_memory_usage(core_descriptor_processor_04_t* ctx);
int core_descriptor_processor_04_optimize(core_descriptor_processor_04_t* ctx);
int core_descriptor_processor_04_debug_print(core_descriptor_processor_04_t* ctx);

/* Module functions */
int core_descriptor_processor_04_module_init(void);
int core_descriptor_processor_04_module_shutdown(void);

#ifdef __cplusplus
}
#endif

#endif /* CORE_DESCRIPTOR_PROCESSOR_04_H */
