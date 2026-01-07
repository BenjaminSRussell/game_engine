/*
 * mtl_shader_library.h
 * Metal Shader Library Management
 *
 * Part of the Platform subsystem
 * Advanced 3D Rendering Engine
 */

#ifndef PLATFORM_MTL_SHADER_LIBRARY_H
#define PLATFORM_MTL_SHADER_LIBRARY_H

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
typedef void* MTLFunctionRef;

/* ============================================================================
 * ENUMS
 * ============================================================================ */

typedef enum metal_shader_function_type {
    METAL_FUNCTION_TYPE_VERTEX = 0,
    METAL_FUNCTION_TYPE_FRAGMENT = 1,
    METAL_FUNCTION_TYPE_COMPUTE = 2,
} metal_shader_function_type_t;

/* ============================================================================
 * TYPES
 * ============================================================================ */

typedef struct metal_library_entry {
    MTLLibraryRef library;
    char path[256];
    uint64_t file_mod_time;  // For hot reload
    uint32_t function_count;
    bool in_use;
} metal_library_entry_t;

typedef struct metal_function_metadata {
    char name[128];
    metal_shader_function_type_t type;
    uint32_t argument_count;
    uint32_t buffer_binding_count;
    uint32_t texture_binding_count;
} metal_function_metadata_t;

#define METAL_MAX_CACHED_LIBRARIES 32

typedef struct metal_shader_library_manager {
    MTLDeviceRef device;
    metal_library_entry_t libraries[METAL_MAX_CACHED_LIBRARIES];
    uint32_t library_count;
    size_t total_memory_bytes;
} metal_shader_library_manager_t;

typedef struct metal_library_stats {
    uint32_t total_libraries;
    uint32_t total_functions;
    size_t total_memory_bytes;
    uint32_t hot_reloads;
} metal_library_stats_t;

/* ============================================================================
 * API - LIBRARY MANAGER
 * ============================================================================ */

/* Initialize library manager */
metal_shader_library_manager_t* metal_shader_library_manager_create(MTLDeviceRef device);

/* Destroy library manager and release all libraries */
void metal_shader_library_manager_destroy(metal_shader_library_manager_t* manager);

/* ============================================================================
 * API - LIBRARY LOADING
 * ============================================================================ */

/* Load a .metallib file from disk */
MTLLibraryRef metal_library_manager_load_file(
    metal_shader_library_manager_t* manager,
    const char* path
);

/* Load a library from binary data */
MTLLibraryRef metal_library_manager_load_data(
    metal_shader_library_manager_t* manager,
    const void* data,
    size_t size,
    const char* debug_name
);

/* Get function from any loaded library by name */
MTLFunctionRef metal_library_manager_get_function(
    metal_shader_library_manager_t* manager,
    const char* name
);

/* Get function from specific library */
MTLFunctionRef metal_library_get_function(
    MTLLibraryRef library,
    const char* name
);

/* ============================================================================
 * API - FUNCTION METADATA & REFLECTION
 * ============================================================================ */

/* Get metadata for a function (type, argument count, bindings) */
bool metal_function_get_metadata(
    MTLFunctionRef function,
    metal_function_metadata_t* out_metadata
);

/* Validate function signature matches expected type */
bool metal_function_validate_type(
    MTLFunctionRef function,
    metal_shader_function_type_t expected_type
);

/* Get buffer binding indices used by function */
uint32_t metal_function_get_buffer_bindings(
    MTLFunctionRef function,
    uint32_t* out_indices,
    uint32_t max_count
);

/* Get texture binding indices used by function */
uint32_t metal_function_get_texture_bindings(
    MTLFunctionRef function,
    uint32_t* out_indices,
    uint32_t max_count
);

/* ============================================================================
 * API - SPECIALIZATION CONSTANTS
 * ============================================================================ */

/* Create function with specialization constants */
MTLFunctionRef metal_function_create_specialized(
    MTLLibraryRef library,
    const char* name,
    const void* constant_values,
    size_t constant_values_size
);

/* ============================================================================
 * API - HOT RELOAD
 * ============================================================================ */

/* Check if any library files have been modified */
bool metal_library_manager_check_for_changes(metal_shader_library_manager_t* manager);

/* Reload libraries that have changed on disk */
uint32_t metal_library_manager_reload_changed(metal_shader_library_manager_t* manager);

/* ============================================================================
 * API - STATISTICS
 * ============================================================================ */

/* Get library manager statistics */
metal_library_stats_t metal_library_manager_get_stats(
    const metal_shader_library_manager_t* manager
);

/* Print library manager statistics to log */
void metal_library_manager_print_stats(
    const metal_shader_library_manager_t* manager
);

#ifdef __cplusplus
}
#endif

#endif /* PLATFORM_MTL_SHADER_LIBRARY_H */
