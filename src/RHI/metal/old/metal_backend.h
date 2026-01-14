#ifndef METAL_BACKEND_H
#define METAL_BACKEND_H

#include <rendering/renderer.h>
#include <core/types.h>
#include "engine/include/core/logger.h"

// Metal Renderer Type
typedef struct MetalRenderer {
    IRenderer interface; // Base interface
    void* device;        // MTLDevice (id<MTLDevice>)
    void* command_queue; // MTLCommandQueue (id<MTLCommandQueue>)
    void* metal_layer;   // CAMetalLayer
    u32 current_frame_index;
} MetalRenderer;

// Shader Reflection Data
typedef struct MetalArgumentInfo {
    char name[64];
    u32 index;
    u32 type; // 0: Buffer, 1: Texture, 2: Sampler
    u32 buffer_data_size;
} MetalArgumentInfo;

typedef struct MetalReflectionData {
    char entry_point_name[64];
    u32 arg_count;
    MetalArgumentInfo args[16];
    u32 thread_group_size_x;
    u32 thread_group_size_y;
    u32 thread_group_size_z;
    u32 simd_group_size;
    u64 signature_hash; 
} MetalReflectionData;

// Shader Variants
typedef struct MetalShaderVariant {
    const char** macros;
    u32 macro_count;
    void* constant_values; // MTLFunctionConstantValues
} MetalShaderVariant;

// Compilation with Variants
void* metal_compile_shader_with_variant(void* device, const char* source, const char* name, const MetalShaderVariant* variant, char** out_error);
void* metal_get_function_with_constants(void* library, const char* function_name, void* constant_values, char** out_error);


// Compilation
void* metal_compile_shader_from_source(void* device, const char* source, const char* name, char** out_error);
void* metal_load_shader_library(void* device, const char* path, char** out_error);
void* metal_get_function(void* library, const char* function_name);
void* metal_reflect_function(void* function_ptr, MetalReflectionData* out_data);
void metal_start_shader_hot_reload(const char* shader_dir);

// Creation
IRenderer* metal_renderer_create(void);

// C-Bridge functions for Metal Device (implemented in metal_device.m)
bool metal_device_init(void);
void metal_device_shutdown(void);
void* metal_device_get(void);
void* metal_device_get_command_queue(void);
void metal_device_set_layer(void* layer);

// Vertex Format Bridge
struct VertexFormat; // Forward declaration
void* mtl_create_vertex_descriptor(const struct VertexFormat* fmt);

// Pipeline Creation
void* mtl_create_render_pipeline(void* device_ptr, void* vertex_fn_ptr, void* fragment_fn_ptr, const struct VertexFormat* vertex_fmt, int color_format, int depth_format, bool blending);

#endif // METAL_BACKEND_H
