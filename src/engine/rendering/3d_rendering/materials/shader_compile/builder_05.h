/*
 * materials_shader_compile_builder_05.h
 *
 * Header file for materials_shader_compile_builder_05 implementation
 * Part of the Advanced 3D Rendering Engine
 */

#ifndef MATERIALS_SHADER_COMPILE_BUILDER_05_H
#define MATERIALS_SHADER_COMPILE_BUILDER_05_H

#include <stdint.h>
#include <stdbool.h>
#include <stddef.h>

#ifdef __cplusplus
extern "C" {
#endif

/* Forward declarations */
typedef struct materials_shader_compile_builder_05 materials_shader_compile_builder_05_t;
typedef struct materials_shader_compile_builder_05_desc materials_shader_compile_builder_05_desc_t;
typedef struct materials_shader_compile_builder_05_stats materials_shader_compile_builder_05_stats_t;

/* Creation and destruction */
int materials_shader_compile_builder_05_create(materials_shader_compile_builder_05_t** out_ctx, const materials_shader_compile_builder_05_desc_t* desc);
int materials_shader_compile_builder_05_destroy(materials_shader_compile_builder_05_t* ctx);

/* Core operations */
int materials_shader_compile_builder_05_begin(materials_shader_compile_builder_05_t* ctx, void* params);
int materials_shader_compile_builder_05_end(materials_shader_compile_builder_05_t* ctx, void* params);
int materials_shader_compile_builder_05_add(materials_shader_compile_builder_05_t* ctx, void* params);
int materials_shader_compile_builder_05_remove(materials_shader_compile_builder_05_t* ctx, void* params);
int materials_shader_compile_builder_05_modify(materials_shader_compile_builder_05_t* ctx, void* params);
int materials_shader_compile_builder_05_finalize(materials_shader_compile_builder_05_t* ctx, void* params);
int materials_shader_compile_builder_05_validate(materials_shader_compile_builder_05_t* ctx, void* params);
int materials_shader_compile_builder_05_optimize(materials_shader_compile_builder_05_t* ctx, void* params);
int materials_shader_compile_builder_05_compile(materials_shader_compile_builder_05_t* ctx, void* params);
int materials_shader_compile_builder_05_link(materials_shader_compile_builder_05_t* ctx, void* params);

/* Utility functions */
int materials_shader_compile_builder_05_get_stats(materials_shader_compile_builder_05_t* ctx);
int materials_shader_compile_builder_05_set_callback(materials_shader_compile_builder_05_t* ctx);
int materials_shader_compile_builder_05_get_memory_usage(materials_shader_compile_builder_05_t* ctx);
int materials_shader_compile_builder_05_optimize(materials_shader_compile_builder_05_t* ctx);
int materials_shader_compile_builder_05_debug_print(materials_shader_compile_builder_05_t* ctx);

/* Module functions */
int materials_shader_compile_builder_05_module_init(void);
int materials_shader_compile_builder_05_module_shutdown(void);

#ifdef __cplusplus
}
#endif

#endif /* MATERIALS_SHADER_COMPILE_BUILDER_05_H */
