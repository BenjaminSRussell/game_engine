/*
 * core_memory_processor_04.h
 *
 * Header file for core_memory_processor_04 implementation
 * Part of the Advanced 3D Rendering Engine
 */

#ifndef CORE_MEMORY_PROCESSOR_04_H
#define CORE_MEMORY_PROCESSOR_04_H

#include <stdint.h>
#include <stdbool.h>
#include <stddef.h>

#ifdef __cplusplus
extern "C" {
#endif

/* Forward declarations */
typedef struct core_memory_processor_04 core_memory_processor_04_t;
typedef struct core_memory_processor_04_desc core_memory_processor_04_desc_t;
typedef struct core_memory_processor_04_stats core_memory_processor_04_stats_t;

/* Creation and destruction */
int core_memory_processor_04_create(core_memory_processor_04_t** out_ctx, const core_memory_processor_04_desc_t* desc);
int core_memory_processor_04_destroy(core_memory_processor_04_t* ctx);

/* Core operations */
int core_memory_processor_04_process_batch(core_memory_processor_04_t* ctx, void* params);
int core_memory_processor_04_process_single(core_memory_processor_04_t* ctx, void* params);
int core_memory_processor_04_transform(core_memory_processor_04_t* ctx, void* params);
int core_memory_processor_04_filter(core_memory_processor_04_t* ctx, void* params);
int core_memory_processor_04_aggregate(core_memory_processor_04_t* ctx, void* params);
int core_memory_processor_04_dispatch(core_memory_processor_04_t* ctx, void* params);
int core_memory_processor_04_finalize(core_memory_processor_04_t* ctx, void* params);
int core_memory_processor_04_validate_input(core_memory_processor_04_t* ctx, void* params);
int core_memory_processor_04_optimize_output(core_memory_processor_04_t* ctx, void* params);
int core_memory_processor_04_profile(core_memory_processor_04_t* ctx, void* params);

/* Utility functions */
int core_memory_processor_04_get_stats(core_memory_processor_04_t* ctx);
int core_memory_processor_04_set_callback(core_memory_processor_04_t* ctx);
int core_memory_processor_04_get_memory_usage(core_memory_processor_04_t* ctx);
int core_memory_processor_04_optimize(core_memory_processor_04_t* ctx);
int core_memory_processor_04_debug_print(core_memory_processor_04_t* ctx);

/* Module functions */
int core_memory_processor_04_module_init(void);
int core_memory_processor_04_module_shutdown(void);

#ifdef __cplusplus
}
#endif

#endif /* CORE_MEMORY_PROCESSOR_04_H */
