#ifndef SHADER_COMPILER_H
#define SHADER_COMPILER_H

#import <Metal/Metal.h>
#include <sys/stat.h>

// Forward declaration of metal_device_t (assuming it exists in your engine's Metal backend)
typedef struct metal_device {
    id<MTLDevice> device;
} metal_device_t;

typedef struct metal_shader_library {
    id<MTLLibrary> library;
    char path[256];
    time_t last_modified;
} metal_shader_library_t;

typedef struct metal_shader {
    id<MTLFunction> function;
    char name[64];
    MTLFunctionType type;  // Vertex, Fragment, Kernel
} metal_shader_t;

typedef struct shader_system {
    metal_shader_library_t* libraries;
    uint32_t library_count;
    uint32_t library_capacity;
    metal_device_t* device;
} shader_system_t;

// Initialize the shader system
shader_system_t* shader_system_create(metal_device_t* device);
void shader_system_destroy(shader_system_t* sys);

// Load shader library
metal_shader_library_t* shader_load_library(shader_system_t* sys, const char* path);

// Get shader function (no constants)
metal_shader_t* shader_get_function(metal_shader_library_t* lib, const char* name);

// Get shader function with constants (permutations)
metal_shader_t* shader_get_function_with_constants(metal_shader_library_t* lib, const char* name, MTLFunctionConstantValues* constants);

// Hot reload (development only)
void shader_system_check_reload(shader_system_t* sys);

#endif // SHADER_COMPILER_H
