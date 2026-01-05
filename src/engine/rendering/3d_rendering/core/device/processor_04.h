/*
 * core_device_processor_04.h
 *
 * Header file for core_device_processor_04 implementation
 * Part of the Advanced 3D Rendering Engine
 */

#ifndef CORE_DEVICE_PROCESSOR_04_H
#define CORE_DEVICE_PROCESSOR_04_H

#include <stdint.h>
#include <stdbool.h>
#include <stddef.h>

#ifdef __cplusplus
extern "C" {
#endif

/* Forward declarations */
typedef struct core_device_processor_04 core_device_processor_04_t;
typedef struct core_device_processor_04_desc core_device_processor_04_desc_t;
typedef struct core_device_processor_04_stats core_device_processor_04_stats_t;

/* Creation and destruction */
int core_device_processor_04_create(core_device_processor_04_t** out_ctx, const core_device_processor_04_desc_t* desc);
int core_device_processor_04_destroy(core_device_processor_04_t* ctx);

/* Core operations */
int core_device_processor_04_process_batch(core_device_processor_04_t* ctx, void* params);
int core_device_processor_04_process_single(core_device_processor_04_t* ctx, void* params);
int core_device_processor_04_transform(core_device_processor_04_t* ctx, void* params);
int core_device_processor_04_filter(core_device_processor_04_t* ctx, void* params);
int core_device_processor_04_aggregate(core_device_processor_04_t* ctx, void* params);
int core_device_processor_04_dispatch(core_device_processor_04_t* ctx, void* params);
int core_device_processor_04_finalize(core_device_processor_04_t* ctx, void* params);
int core_device_processor_04_validate_input(core_device_processor_04_t* ctx, void* params);
int core_device_processor_04_optimize_output(core_device_processor_04_t* ctx, void* params);
int core_device_processor_04_profile(core_device_processor_04_t* ctx, void* params);

/* Utility functions */
int core_device_processor_04_get_stats(core_device_processor_04_t* ctx);
int core_device_processor_04_set_callback(core_device_processor_04_t* ctx);
int core_device_processor_04_get_memory_usage(core_device_processor_04_t* ctx);
int core_device_processor_04_optimize(core_device_processor_04_t* ctx);
int core_device_processor_04_debug_print(core_device_processor_04_t* ctx);

/* Module functions */
int core_device_processor_04_module_init(void);
int core_device_processor_04_module_shutdown(void);

#ifdef __cplusplus
}
#endif

#endif /* CORE_DEVICE_PROCESSOR_04_H */
