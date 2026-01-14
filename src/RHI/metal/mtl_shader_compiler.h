/*
 * mtl_shader_compiler.h
 * Metal Shader Runtime Compilation
 *
 * Part of the Platform subsystem
 * Advanced 3D Rendering Engine
 */

#ifndef PLATFORM_MTL_SHADER_COMPILER_H
#define PLATFORM_MTL_SHADER_COMPILER_H

#include <stdint.h>
#include <stdbool.h>
#include <stddef.h>

#ifdef __cplusplus
extern "C" {
#endif

/* ============================================================================
 * FORWARD DECLARATIONS
 * ============================================================================ */

typedef void* MTLDeviceRef;
typedef void* MTLLibraryRef;

/* ============================================================================
 * ENUMS
 * ============================================================================ */

typedef enum metal_shader_optimization_level {
    METAL_OPTIMIZATION_NONE = 0,
    METAL_OPTIMIZATION_SIZE = 1,
    METAL_OPTIMIZATION_SPEED = 2,
    METAL_OPTIMIZATION_DEFAULT = 3,
} metal_shader_optimization_level_t;

/* ============================================================================
 * TYPES
 * ============================================================================ */

typedef struct metal_compile_options {
    metal_shader_optimization_level_t optimization_level;
    bool fast_math_enabled;
    bool enable_debug_info;
    const char** preprocessor_defines;  // Array of "KEY=VALUE" strings
    uint32_t preprocessor_define_count;
    const char* language_version;  // e.g., "2.4", "3.0"
} metal_compile_options_t;

typedef struct metal_compile_error {
    char message[512];
    uint32_t line;
    uint32_t column;
} metal_compile_error_t;

#define METAL_MAX_COMPILE_ERRORS 16

typedef struct metal_compile_result {
    MTLLibraryRef library;
    bool succeeded;
    metal_compile_error_t errors[METAL_MAX_COMPILE_ERRORS];
    uint32_t error_count;
    char warnings[1024];
} metal_compile_result_t;

typedef struct metal_shader_cache_entry {
    uint64_t source_hash;
    char* source;  // Owned copy of source for collision check
    MTLLibraryRef library;
    char debug_name[256];
    bool in_use;
} metal_shader_cache_entry_t;

#define METAL_MAX_CACHED_SHADERS 64

typedef struct metal_shader_compiler {
    MTLDeviceRef device;
    metal_compile_options_t default_options;
    metal_shader_cache_entry_t cache[METAL_MAX_CACHED_SHADERS];
    uint32_t cache_count;
    uint32_t compile_count;
    uint32_t cache_hits;
} metal_shader_compiler_t;

/* ============================================================================
 * API - COMPILER INITIALIZATION
 * ============================================================================ */

/* Create a shader compiler instance */
metal_shader_compiler_t* metal_shader_compiler_create(MTLDeviceRef device);

/* Destroy compiler and release all cached shaders */
void metal_shader_compiler_destroy(metal_shader_compiler_t* compiler);

/* Set default compile options */
void metal_shader_compiler_set_default_options(
    metal_shader_compiler_t* compiler,
    const metal_compile_options_t* options
);

/* ============================================================================
 * API - COMPILATION
 * ============================================================================ */

/* Compile Metal shader source code from a file */
metal_compile_result_t metal_shader_compile_file(
    metal_shader_compiler_t* compiler,
    const char* path,
    const metal_compile_options_t* options  // NULL = use defaults
);

/* Compile Metal shader source code from a string */
metal_compile_result_t metal_shader_compile_source(
    metal_shader_compiler_t* compiler,
    const char* source,
    const char* debug_name,
    const metal_compile_options_t* options  // NULL = use defaults
);

/* Free resources associated with a compile result */
void metal_compile_result_free(metal_compile_result_t* result);

/* ============================================================================
 * API - CACHE MANAGEMENT
 * ============================================================================ */

/* Check if compiled shader is in cache */
MTLLibraryRef metal_shader_cache_get(
    metal_shader_compiler_t* compiler,
    const char* source
);

/* Manually add a library to the cache */
void metal_shader_cache_add(
    metal_shader_compiler_t* compiler,
    const char* source,
    MTLLibraryRef library,
    const char* debug_name
);

/* Clear all cached shaders */
void metal_shader_cache_clear(metal_shader_compiler_t* compiler);

/* ============================================================================
 * API - UTILITIES
 * ============================================================================ */

/* Initialize compile options with defaults */
void metal_compile_options_init(metal_compile_options_t* options);

/* Get compiler statistics (compile count, cache hits) */
void metal_shader_compiler_get_stats(
    const metal_shader_compiler_t* compiler,
    uint32_t* out_compile_count,
    uint32_t* out_cache_hits,
    uint32_t* out_cached_shaders
);

/* Print compiler statistics to log */
void metal_shader_compiler_print_stats(const metal_shader_compiler_t* compiler);

#ifdef __cplusplus
}
#endif

#endif /* PLATFORM_MTL_SHADER_COMPILER_H */
