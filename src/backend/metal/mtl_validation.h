#ifndef MTL_VALIDATION_H
#define MTL_VALIDATION_H

#include <Metal/Metal.h>
#include <Foundation/Foundation.h>
#include <stdio.h>
#include <stdbool.h>

#ifdef __cplusplus
extern "C" {
#endif

// Metal validation system
// Provides comprehensive validation for Metal API objects and operations

typedef struct mtl_validation_manager {
    id<MTLDevice> device;
    bool validation_enabled;
    bool verbose_logging;
    bool file_logging;
    char log_filename[256];
    
    // Statistics
    u32 total_validations;
    u32 error_count;
    u32 warning_count;
    u32 info_count;
    
    // Thread safety
    bool thread_safety_initialized;
    pthread_mutex_t validation_mutex;
    
    // Debug callback
    void (*debug_callback)(const char* message, void* user_data);
    void* callback_user_data;
} mtl_validation_manager_t;

// Core validation functions

// Initialize Metal validation system
bool mtl_validation_init(id<MTLDevice> device, bool enable_validation);

// Shutdown validation system
void mtl_validation_shutdown(void);

// Enable/disable validation
void mtl_validation_set_enabled(bool enabled);
bool mtl_validation_is_enabled(void);

// Logging configuration
void mtl_validation_set_verbose_logging(bool enabled);
void mtl_validation_set_file_logging(const char* filename);
bool mtl_validation_get_verbose_logging(void);
bool mtl_validation_get_file_logging(void);

// Device validation

// Validate Metal device
bool mtl_validation_validate_device(id<MTLDevice> device);

// Validate device capabilities
bool mtl_validation_validate_device_capabilities(id<MTLDevice> device);

// Validate feature support
bool mtl_validation_validate_feature_support(id<MTLDevice> device, MTLFeatureSet feature_set);

// Command queue validation

// Validate command queue
bool mtl_validation_validate_command_queue(id<MTLCommandQueue> command_queue);

// Validate command queue creation
bool mtl_validation_validate_command_queue_create(const MTLCommandQueueDescriptor* descriptor);

// Command buffer validation

// Validate command buffer
bool mtl_validation_validate_command_buffer(id<MTLCommandBuffer> command_buffer);

// Validate command buffer state
bool mtl_validation_validate_command_buffer_state(id<MTLCommandBuffer> command_buffer, MTLCommandBufferState expected_state);

// Validate command buffer encoding
bool mtl_validation_validate_command_buffer_encoding(id<MTLCommandBuffer> command_buffer);

// Render command encoder validation

// Validate render command encoder
bool mtl_validation_validate_render_command_encoder(id<MTLRenderCommandEncoder> encoder);

// Validate render pass descriptor
bool mtl_validation_validate_render_pass_descriptor(const MTLRenderPassDescriptor* descriptor);

// Validate render pipeline state
bool mtl_validation_validate_render_pipeline_state(id<MTLRenderPipelineState> pipeline_state);

// Validate vertex descriptor
bool mtl_validation_validate_vertex_descriptor(const MTLVertexDescriptor* descriptor);

// Validate viewport
bool mtl_validation_validate_viewport(const MTLViewport* viewport);

// Compute command encoder validation

// Validate compute command encoder
bool mtl_validation_validate_compute_command_encoder(id<MTLComputeCommandEncoder> encoder);

// Validate compute pipeline state
bool mtl_validation_validate_compute_pipeline_state(id<MTLComputePipelineState> pipeline_state);

// Validate threadgroup size
bool mtl_validation_validate_threadgroup_size(MTLSize threadgroup_size, MTLSize threads_per_grid);

// Blit command encoder validation

// Validate blit command encoder
bool mtl_validation_validate_blit_command_encoder(id<MTLBlitCommandEncoder> encoder);

// Validate blit options
bool mtl_validation_validate_blit_options(MTLBlitOption options);

// Resource validation

// Validate buffer
bool mtl_validation_validate_buffer(id<MTLBuffer> buffer);

// Validate buffer creation
bool mtl_validation_validate_buffer_creation(const MTLBufferDescriptor* descriptor);

// Validate texture
bool mtl_validation_validate_texture(id<MTLTexture> texture);

// Validate texture creation
bool mtl_validation_validate_texture_creation(const MTLTextureDescriptor* descriptor);

// Validate texture usage
bool mtl_validation_validate_texture_usage(MTLTextureUsage usage, MTLPixelFormat format);

// Validate sampler
bool mtl_validation_validate_sampler(id<MTLSampler> sampler);

// Validate sampler creation
bool mtl_validation_validate_sampler_creation(const MTLSamplerDescriptor* descriptor);

// Pipeline validation

// Validate render pipeline creation
bool mtl_validation_validate_render_pipeline_creation(const MTLRenderPipelineDescriptor* descriptor);

// Validate compute pipeline creation
bool mtl_validation_validate_compute_pipeline_creation(const MTLComputePipelineDescriptor* descriptor);

// Validate vertex function
bool mtl_validation_validate_vertex_function(id<MTLFunction> vertex_function);

// Validate fragment function
bool mtl_validation_validate_fragment_function(id<MTLFunction> fragment_function);

// Validate kernel function
bool mtl_validation_validate_kernel_function(id<MTLFunction> kernel_function);

// Shader validation

// Validate shader library
bool mtl_validation_validate_shader_library(id<MTLLibrary> library);

// Validate shader compilation
bool mtl_validation_validate_shader_compilation(id<MTLDevice> device, const char* source, const char* entry_point);

// Validate shader reflection data
bool mtl_validation_validate_shader_reflection(id<MTLReflection> reflection);

// Memory validation

// Validate heap
bool mtl_validation_validate_heap(id<MTLHeap> heap);

// Validate heap creation
bool mtl_validation_validate_heap_creation(const MTLHeapDescriptor* descriptor);

// Validate resource options
bool mtl_validation_validate_resource_options(MTLResourceOptions options);

// Validate storage mode
bool mtl_validation_validate_storage_mode(MTLStorageMode storage_mode);

// Synchronization validation

// Validate event
bool mtl_validation_validate_event(id<MTLEvent> event);

// Validate fence
bool mtl_validation_validate_fence(id<MTLFence> fence);

// Validate shared event
bool mtl_validation_validate_shared_event(id<MTLSharedEvent> shared_event);

// Argument buffer validation

// Validate argument encoder
bool mtl_validation_validate_argument_encoder(id<MTLArgumentEncoder> encoder);

// Validate argument buffer layout
bool mtl_validation_validate_argument_buffer_layout(const MTLArgumentDescriptor* arguments, u32 count);

// Validate buffer binding
bool mtl_validation_validate_buffer_binding(id<MTLBuffer> buffer, u32 offset, u32 size);

// Advanced validation

// Validate command buffer dependencies
bool mtl_validation_validate_command_dependencies(id<MTLCommandBuffer> command_buffer);

// Validate resource state transitions
bool mtl_validation_validate_resource_state_transition(id<MTLResource> resource, MTLResourceState old_state, MTLResourceState new_state);

// Validate heap allocation
bool mtl_validation_validate_heap_allocation(id<MTLHeap> heap, u32 size, u32 alignment);

// Validate timestamp queries
bool mtl_validation_validate_timestamp_query(id<MTLDevice> device);

// Validate visibility buffer
bool mtl_validation_validate_visibility_buffer(id<MTLDevice> device);

// Debug and diagnostics

// Validate debug layer
bool mtl_validation_validate_debug_layer(void);

// Generate validation report
bool mtl_validation_generate_report(const char* filename, bool include_details);

// Get validation statistics
void mtl_validation_get_stats(u32* total, u32* errors, u32* warnings, u32* info);

// Clear validation statistics
void mtl_validation_clear_stats(void);

// Set debug callback
void mtl_validation_set_debug_callback(void (*callback)(const char* message, void* user_data), void* user_data);

// Utility functions

// Check if format supports specific usage
bool mtl_validation_format_supports_usage(MTLPixelFormat format, MTLTextureUsage usage);

// Check if feature is supported
bool mtl_validation_is_feature_supported(id<MTLDevice> device, MTLFeatureSet feature_set);

// Validate pixel format compatibility
bool mtl_validation_validate_pixel_format_compatibility(MTLPixelFormat format1, MTLPixelFormat format2);

// Validate size alignment
bool mtl_validation_validate_size_alignment(u64 size, u64 alignment);

// Validate range
bool mtl_validation_validate_range(u64 offset, u64 size, u64 max_size);

// Performance validation

// Validate performance metrics
bool mtl_validation_validate_performance_metrics(id<MTLDevice> device);

// Validate GPU utilization
bool mtl_validation_validate_gpu_utilization(id<MTLDevice> device);

// Validate memory usage
bool mtl_validation_validate_memory_usage(id<MTLDevice> device);

// Error handling

// Log validation error
void mtl_validation_log_error(const char* format, ...);

// Log validation warning
void mtl_validation_log_warning(const char* format, ...);

// Log validation info
void mtl_validation_log_info(const char* format, ...);

// Get last validation error
const char* mtl_validation_get_last_error(void);

// Clear last validation error
void mtl_validation_clear_last_error(void);

#ifdef __cplusplus
}
#endif

#endif // MTL_VALIDATION_H
