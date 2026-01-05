/*
 * tools_automation_processor_04.h
 *
 * Header file for tools_automation_processor_04 implementation
 * Part of the Advanced 3D Rendering Engine
 */

#ifndef TOOLS_AUTOMATION_PROCESSOR_04_H
#define TOOLS_AUTOMATION_PROCESSOR_04_H

#include <stdint.h>
#include <stdbool.h>
#include <stddef.h>

#ifdef __cplusplus
extern "C" {
#endif

/* Forward declarations */
typedef struct tools_automation_processor_04 tools_automation_processor_04_t;
typedef struct tools_automation_processor_04_desc tools_automation_processor_04_desc_t;
typedef struct tools_automation_processor_04_stats tools_automation_processor_04_stats_t;

/* Creation and destruction */
int tools_automation_processor_04_create(tools_automation_processor_04_t** out_ctx, const tools_automation_processor_04_desc_t* desc);
int tools_automation_processor_04_destroy(tools_automation_processor_04_t* ctx);

/* Core operations */
int tools_automation_processor_04_process_batch(tools_automation_processor_04_t* ctx, void* params);
int tools_automation_processor_04_process_single(tools_automation_processor_04_t* ctx, void* params);
int tools_automation_processor_04_transform(tools_automation_processor_04_t* ctx, void* params);
int tools_automation_processor_04_filter(tools_automation_processor_04_t* ctx, void* params);
int tools_automation_processor_04_aggregate(tools_automation_processor_04_t* ctx, void* params);
int tools_automation_processor_04_dispatch(tools_automation_processor_04_t* ctx, void* params);
int tools_automation_processor_04_finalize(tools_automation_processor_04_t* ctx, void* params);
int tools_automation_processor_04_validate_input(tools_automation_processor_04_t* ctx, void* params);
int tools_automation_processor_04_optimize_output(tools_automation_processor_04_t* ctx, void* params);
int tools_automation_processor_04_profile(tools_automation_processor_04_t* ctx, void* params);

/* Utility functions */
int tools_automation_processor_04_get_stats(tools_automation_processor_04_t* ctx);
int tools_automation_processor_04_set_callback(tools_automation_processor_04_t* ctx);
int tools_automation_processor_04_get_memory_usage(tools_automation_processor_04_t* ctx);
int tools_automation_processor_04_optimize(tools_automation_processor_04_t* ctx);
int tools_automation_processor_04_debug_print(tools_automation_processor_04_t* ctx);

/* Module functions */
int tools_automation_processor_04_module_init(void);
int tools_automation_processor_04_module_shutdown(void);

#ifdef __cplusplus
}
#endif

#endif /* TOOLS_AUTOMATION_PROCESSOR_04_H */
