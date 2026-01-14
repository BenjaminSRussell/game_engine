#include "compute_shader_pipeline.h"
#include <stdlib.h>
#include <string.h>
#include <stdio.h>
#include <time.h>

// Internal structures
struct compute_shader_t {
    uint32_t id;
    compute_backend_t backend;
    char* source_code;
    char* entry_point;
    uint32_t thread_group_size[3];
    char* name;
    void* backend_handle;
    bool is_compiled;
    uint64_t compilation_time_ns;
};

struct compute_resource_t {
    uint32_t id;
    compute_resource_type_t type;
    compute_resource_flags_t flags;
    uint32_t size;
    uint32_t width, height, depth;
    uint32_t format;
    char* name;
    void* backend_handle;
    void* mapped_data;
    uint64_t creation_time_ns;
};

struct compute_command_buffer_t {
    uint32_t id;
    bool is_recording;
    void* backend_handle;
    uint32_t command_count;
    uint64_t creation_time_ns;
};

struct compute_pipeline_t {
    compute_backend_t backend;
    bool is_initialized;
    pthread_mutex_t mutex;
    
    // Resource management
    compute_shader_t** shaders;
    uint32_t shader_count;
    uint32_t shader_capacity;
    
    compute_resource_t** resources;
    uint32_t resource_count;
    uint32_t resource_capacity;
    
    compute_command_buffer_t** command_buffers;
    uint32_t command_buffer_count;
    uint32_t command_buffer_capacity;
    
    // Memory tracking
    uint64_t total_memory_allocated;
    uint64_t memory_budget;
    
    // Statistics
    compute_pipeline_stats_t stats;
    
    // Backend-specific data
    void* backend_data;
    
    // Synchronization
    uint64_t next_fence_id;
    pthread_mutex_t fence_mutex;
};

// Global pipeline instance
static compute_pipeline_t* g_pipeline = NULL;
static pthread_mutex_t g_pipeline_mutex = PTHREAD_MUTEX_INITIALIZER;

// Helper functions
static uint64_t get_timestamp_ns(void) {
    struct timespec ts;
    clock_gettime(CLOCK_MONOTONIC, &ts);
    return (uint64_t)ts.tv_sec * 1000000000ULL + (uint64_t)ts.tv_nsec;
}

static uint32_t generate_id(void) {
    static uint32_t next_id = 1;
    return __sync_fetch_and_add(&next_id, 1);
}

static compute_error_t validate_shader_desc(const compute_shader_desc_t* desc) {
    if (!desc || !desc->source_code || !desc->entry_point) {
        return COMPUTE_ERROR_INVALID_PARAM;
    }
    
    if (desc->thread_group_size_x == 0 || desc->thread_group_size_y == 0 || desc->thread_group_size_z == 0) {
        return COMPUTE_ERROR_INVALID_PARAM;
    }
    
    return COMPUTE_SUCCESS;
}

static compute_error_t validate_resource_desc(const compute_resource_desc_t* desc) {
    if (!desc) {
        return COMPUTE_ERROR_INVALID_PARAM;
    }
    
    if (desc->type == COMPUTE_RESOURCE_BUFFER && desc->size == 0) {
        return COMPUTE_ERROR_INVALID_PARAM;
    }
    
    if (desc->type == COMPUTE_RESOURCE_TEXTURE && (desc->width == 0 || desc->height == 0)) {
        return COMPUTE_ERROR_INVALID_PARAM;
    }
    
    return COMPUTE_SUCCESS;
}

// Pipeline initialization
compute_error_t compute_pipeline_init(compute_pipeline_t** pipeline, compute_backend_t backend) {
    if (!pipeline) {
        return COMPUTE_ERROR_INVALID_PARAM;
    }
    
    pthread_mutex_lock(&g_pipeline_mutex);
    
    // Create pipeline structure
    compute_pipeline_t* new_pipeline = (compute_pipeline_t*)calloc(1, sizeof(compute_pipeline_t));
    if (!new_pipeline) {
        pthread_mutex_unlock(&g_pipeline_mutex);
        return COMPUTE_ERROR_OUT_OF_MEMORY;
    }
    
    new_pipeline->backend = backend;
    new_pipeline->memory_budget = 1024 * 1024 * 1024; // 1GB default
    
    // Initialize mutexes
    if (pthread_mutex_init(&new_pipeline->mutex, NULL) != 0) {
        free(new_pipeline);
        pthread_mutex_unlock(&g_pipeline_mutex);
        return COMPUTE_ERROR_BACKEND_INITIALIZATION_FAILED;
    }
    
    if (pthread_mutex_init(&new_pipeline->fence_mutex, NULL) != 0) {
        pthread_mutex_destroy(&new_pipeline->mutex);
        free(new_pipeline);
        pthread_mutex_unlock(&g_pipeline_mutex);
        return COMPUTE_ERROR_BACKEND_INITIALIZATION_FAILED;
    }
    
    // Allocate arrays
    new_pipeline->shader_capacity = 64;
    new_pipeline->shaders = (compute_shader_t**)calloc(new_pipeline->shader_capacity, sizeof(compute_shader_t*));
    
    new_pipeline->resource_capacity = 256;
    new_pipeline->resources = (compute_resource_t**)calloc(new_pipeline->resource_capacity, sizeof(compute_resource_t*));
    
    new_pipeline->command_buffer_capacity = 16;
    new_pipeline->command_buffers = (compute_command_buffer_t**)calloc(new_pipeline->command_buffer_capacity, sizeof(compute_command_buffer_t*));
    
    if (!new_pipeline->shaders || !new_pipeline->resources || !new_pipeline->command_buffers) {
        pthread_mutex_destroy(&new_pipeline->mutex);
        pthread_mutex_destroy(&new_pipeline->fence_mutex);
        free(new_pipeline->shaders);
        free(new_pipeline->resources);
        free(new_pipeline->command_buffers);
        free(new_pipeline);
        pthread_mutex_unlock(&g_pipeline_mutex);
        return COMPUTE_ERROR_OUT_OF_MEMORY;
    }
    
    // Initialize backend
    compute_error_t backend_result = compute_backend_init(new_pipeline);
    if (backend_result != COMPUTE_SUCCESS) {
        pthread_mutex_destroy(&new_pipeline->mutex);
        pthread_mutex_destroy(&new_pipeline->fence_mutex);
        free(new_pipeline->shaders);
        free(new_pipeline->resources);
        free(new_pipeline->command_buffers);
        free(new_pipeline);
        pthread_mutex_unlock(&g_pipeline_mutex);
        return backend_result;
    }
    
    new_pipeline->is_initialized = true;
    *pipeline = new_pipeline;
    g_pipeline = new_pipeline;
    
    pthread_mutex_unlock(&g_pipeline_mutex);
    return COMPUTE_SUCCESS;
}

void compute_pipeline_shutdown(compute_pipeline_t* pipeline) {
    if (!pipeline) {
        return;
    }
    
    pthread_mutex_lock(&pipeline->mutex);
    
    if (!pipeline->is_initialized) {
        pthread_mutex_unlock(&pipeline->mutex);
        return;
    }
    
    // Destroy all resources
    for (uint32_t i = 0; i < pipeline->resource_count; i++) {
        compute_resource_destroy(pipeline, pipeline->resources[i]);
    }
    
    // Destroy all shaders
    for (uint32_t i = 0; i < pipeline->shader_count; i++) {
        compute_shader_destroy(pipeline, pipeline->shaders[i]);
    }
    
    // Destroy all command buffers
    for (uint32_t i = 0; i < pipeline->command_buffer_count; i++) {
        compute_command_buffer_destroy(pipeline, pipeline->command_buffers[i]);
    }
    
    // Shutdown backend
    compute_backend_shutdown(pipeline);
    
    // Free arrays
    free(pipeline->shaders);
    free(pipeline->resources);
    free(pipeline->command_buffers);
    
    // Destroy mutexes
    pthread_mutex_destroy(&pipeline->mutex);
    pthread_mutex_destroy(&pipeline->fence_mutex);
    
    pipeline->is_initialized = false;
    
    pthread_mutex_unlock(&pipeline->mutex);
    
    if (g_pipeline == pipeline) {
        g_pipeline = NULL;
    }
    
    free(pipeline);
}

// Shader management
compute_error_t compute_shader_create(compute_pipeline_t* pipeline, const compute_shader_desc_t* desc, compute_shader_t** shader) {
    if (!pipeline || !desc || !shader) {
        return COMPUTE_ERROR_INVALID_PARAM;
    }
    
    if (!pipeline->is_initialized) {
        return COMPUTE_ERROR_NOT_INITIALIZED;
    }
    
    compute_error_t validation_result = validate_shader_desc(desc);
    if (validation_result != COMPUTE_SUCCESS) {
        return validation_result;
    }
    
    pthread_mutex_lock(&pipeline->mutex);
    
    // Check capacity
    if (pipeline->shader_count >= pipeline->shader_capacity) {
        uint32_t new_capacity = pipeline->shader_capacity * 2;
        compute_shader_t** new_shaders = (compute_shader_t**)realloc(pipeline->shaders, new_capacity * sizeof(compute_shader_t*));
        if (!new_shaders) {
            pthread_mutex_unlock(&pipeline->mutex);
            return COMPUTE_ERROR_OUT_OF_MEMORY;
        }
        pipeline->shaders = new_shaders;
        pipeline->shader_capacity = new_capacity;
    }
    
    // Create shader
    compute_shader_t* new_shader = (compute_shader_t*)calloc(1, sizeof(compute_shader_t));
    if (!new_shader) {
        pthread_mutex_unlock(&pipeline->mutex);
        return COMPUTE_ERROR_OUT_OF_MEMORY;
    }
    
    new_shader->id = generate_id();
    new_shader->backend = desc->backend;
    new_shader->thread_group_size[0] = desc->thread_group_size_x;
    new_shader->thread_group_size[1] = desc->thread_group_size_y;
    new_shader->thread_group_size[2] = desc->thread_group_size_z;
    
    // Copy strings
    new_shader->source_code = strdup(desc->source_code);
    new_shader->entry_point = strdup(desc->entry_point);
    new_shader->name = desc->name ? strdup(desc->name) : strdup("unnamed_shader");
    
    if (!new_shader->source_code || !new_shader->entry_point || !new_shader->name) {
        free(new_shader->source_code);
        free(new_shader->entry_point);
        free(new_shader->name);
        free(new_shader);
        pthread_mutex_unlock(&pipeline->mutex);
        return COMPUTE_ERROR_OUT_OF_MEMORY;
    }
    
    // Create backend shader
    uint64_t start_time = get_timestamp_ns();
    compute_error_t backend_result = compute_backend_create_shader(pipeline, desc, &new_shader);
    uint64_t end_time = get_timestamp_ns();
    
    if (backend_result != COMPUTE_SUCCESS) {
        free(new_shader->source_code);
        free(new_shader->entry_point);
        free(new_shader->name);
        free(new_shader);
        pthread_mutex_unlock(&pipeline->mutex);
        return backend_result;
    }
    
    new_shader->is_compiled = true;
    new_shader->compilation_time_ns = end_time - start_time;
    
    // Add to pipeline
    pipeline->shaders[pipeline->shader_count++] = new_shader;
    
    // Update statistics
    pipeline->stats.total_shaders_created++;
    pipeline->stats.active_shaders++;
    
    *shader = new_shader;
    pthread_mutex_unlock(&pipeline->mutex);
    
    return COMPUTE_SUCCESS;
}

void compute_shader_destroy(compute_pipeline_t* pipeline, compute_shader_t* shader) {
    if (!pipeline || !shader) {
        return;
    }
    
    pthread_mutex_lock(&pipeline->mutex);
    
    // Find and remove from pipeline
    for (uint32_t i = 0; i < pipeline->shader_count; i++) {
        if (pipeline->shaders[i] == shader) {
            // Remove from array
            for (uint32_t j = i; j < pipeline->shader_count - 1; j++) {
                pipeline->shaders[j] = pipeline->shaders[j + 1];
            }
            pipeline->shader_count--;
            break;
        }
    }
    
    // Free resources
    free(shader->source_code);
    free(shader->entry_point);
    free(shader->name);
    
    // Backend cleanup would go here
    // if (shader->backend_handle) { backend_destroy_shader(shader->backend_handle); }
    
    free(shader);
    
    // Update statistics
    pipeline->stats.active_shaders--;
    
    pthread_mutex_unlock(&pipeline->mutex);
}

compute_error_t compute_shader_bind(compute_pipeline_t* pipeline, compute_shader_t* shader) {
    if (!pipeline || !shader) {
        return COMPUTE_ERROR_INVALID_PARAM;
    }
    
    if (!pipeline->is_initialized) {
        return COMPUTE_ERROR_NOT_INITIALIZED;
    }
    
    if (!shader->is_compiled) {
        return COMPUTE_ERROR_SHADER_COMPILATION_FAILED;
    }
    
    // Backend-specific binding would go here
    // return backend_bind_shader(shader->backend_handle);
    
    return COMPUTE_SUCCESS;
}

// Resource management
compute_error_t compute_resource_create(compute_pipeline_t* pipeline, const compute_resource_desc_t* desc, compute_resource_t** resource) {
    if (!pipeline || !desc || !resource) {
        return COMPUTE_ERROR_INVALID_PARAM;
    }
    
    if (!pipeline->is_initialized) {
        return COMPUTE_ERROR_NOT_INITIALIZED;
    }
    
    compute_error_t validation_result = validate_resource_desc(desc);
    if (validation_result != COMPUTE_SUCCESS) {
        return validation_result;
    }
    
    pthread_mutex_lock(&pipeline->mutex);
    
    // Check memory budget
    uint64_t resource_size = desc->type == COMPUTE_RESOURCE_BUFFER ? desc->size : 
                             desc->width * desc->height * desc->depth * 4; // Approximate
    
    if (pipeline->total_memory_allocated + resource_size > pipeline->memory_budget) {
        pthread_mutex_unlock(&pipeline->mutex);
        return COMPUTE_ERROR_OUT_OF_MEMORY;
    }
    
    // Check capacity
    if (pipeline->resource_count >= pipeline->resource_capacity) {
        uint32_t new_capacity = pipeline->resource_capacity * 2;
        compute_resource_t** new_resources = (compute_resource_t**)realloc(pipeline->resources, new_capacity * sizeof(compute_resource_t*));
        if (!new_resources) {
            pthread_mutex_unlock(&pipeline->mutex);
            return COMPUTE_ERROR_OUT_OF_MEMORY;
        }
        pipeline->resources = new_resources;
        pipeline->resource_capacity = new_capacity;
    }
    
    // Create resource
    compute_resource_t* new_resource = (compute_resource_t*)calloc(1, sizeof(compute_resource_t));
    if (!new_resource) {
        pthread_mutex_unlock(&pipeline->mutex);
        return COMPUTE_ERROR_OUT_OF_MEMORY;
    }
    
    new_resource->id = generate_id();
    new_resource->type = desc->type;
    new_resource->flags = desc->flags;
    new_resource->size = desc->size;
    new_resource->width = desc->width;
    new_resource->height = desc->height;
    new_resource->depth = desc->depth;
    new_resource->format = desc->format;
    new_resource->name = desc->name ? strdup(desc->name) : strdup("unnamed_resource");
    new_resource->creation_time_ns = get_timestamp_ns();
    
    if (!new_resource->name) {
        free(new_resource);
        pthread_mutex_unlock(&pipeline->mutex);
        return COMPUTE_ERROR_OUT_OF_MEMORY;
    }
    
    // Create backend resource
    compute_error_t backend_result = compute_backend_create_resource(pipeline, desc, &new_resource);
    if (backend_result != COMPUTE_SUCCESS) {
        free(new_resource->name);
        free(new_resource);
        pthread_mutex_unlock(&pipeline->mutex);
        return backend_result;
    }
    
    // Add to pipeline
    pipeline->resources[pipeline->resource_count++] = new_resource;
    pipeline->total_memory_allocated += resource_size;
    
    // Update statistics
    pipeline->stats.total_resources_created++;
    pipeline->stats.active_resources++;
    
    *resource = new_resource;
    pthread_mutex_unlock(&pipeline->mutex);
    
    return COMPUTE_SUCCESS;
}

void compute_resource_destroy(compute_pipeline_t* pipeline, compute_resource_t* resource) {
    if (!pipeline || !resource) {
        return;
    }
    
    pthread_mutex_lock(&pipeline->mutex);
    
    // Find and remove from pipeline
    for (uint32_t i = 0; i < pipeline->resource_count; i++) {
        if (pipeline->resources[i] == resource) {
            // Remove from array
            for (uint32_t j = i; j < pipeline->resource_count - 1; j++) {
                pipeline->resources[j] = pipeline->resources[j + 1];
            }
            pipeline->resource_count--;
            break;
        }
    }
    
    // Update memory tracking
    uint64_t resource_size = resource->type == COMPUTE_RESOURCE_BUFFER ? resource->size :
                             resource->width * resource->height * resource->depth * 4;
    pipeline->total_memory_allocated -= resource_size;
    
    // Free resources
    free(resource->name);
    
    // Backend cleanup would go here
    // if (resource->backend_handle) { backend_destroy_resource(resource->backend_handle); }
    
    free(resource);
    
    // Update statistics
    pipeline->stats.active_resources--;
    
    pthread_mutex_unlock(&pipeline->mutex);
}

compute_error_t compute_resource_bind(compute_pipeline_t* pipeline, compute_resource_t* resource, uint32_t binding) {
    if (!pipeline || !resource) {
        return COMPUTE_ERROR_INVALID_PARAM;
    }
    
    if (!pipeline->is_initialized) {
        return COMPUTE_ERROR_NOT_INITIALIZED;
    }
    
    // Backend-specific binding would go here
    // return backend_bind_resource(resource->backend_handle, binding);
    
    return COMPUTE_SUCCESS;
}

compute_error_t compute_resource_update_data(compute_pipeline_t* pipeline, compute_resource_t* resource, const void* data, uint32_t size, uint32_t offset) {
    if (!pipeline || !resource || !data) {
        return COMPUTE_ERROR_INVALID_PARAM;
    }
    
    if (!pipeline->is_initialized) {
        return COMPUTE_ERROR_NOT_INITIALIZED;
    }
    
    if (!(resource->flags & COMPUTE_RESOURCE_WRITE) && !(resource->flags & COMPUTE_RESOURCE_READ_WRITE)) {
        return COMPUTE_ERROR_INVALID_PARAM;
    }
    
    if (offset + size > resource->size) {
        return COMPUTE_ERROR_INVALID_PARAM;
    }
    
    // Backend-specific data update would go here
    // return backend_update_resource_data(resource->backend_handle, data, size, offset);
    
    return COMPUTE_SUCCESS;
}

// Command buffer management
compute_error_t compute_command_buffer_create(compute_pipeline_t* pipeline, compute_command_buffer_t** cmd_buffer) {
    if (!pipeline || !cmd_buffer) {
        return COMPUTE_ERROR_INVALID_PARAM;
    }
    
    if (!pipeline->is_initialized) {
        return COMPUTE_ERROR_NOT_INITIALIZED;
    }
    
    pthread_mutex_lock(&pipeline->mutex);
    
    // Check capacity
    if (pipeline->command_buffer_count >= pipeline->command_buffer_capacity) {
        uint32_t new_capacity = pipeline->command_buffer_capacity * 2;
        compute_command_buffer_t** new_buffers = (compute_command_buffer_t**)realloc(pipeline->command_buffers, new_capacity * sizeof(compute_command_buffer_t*));
        if (!new_buffers) {
            pthread_mutex_unlock(&pipeline->mutex);
            return COMPUTE_ERROR_OUT_OF_MEMORY;
        }
        pipeline->command_buffers = new_buffers;
        pipeline->command_buffer_capacity = new_capacity;
    }
    
    // Create command buffer
    compute_command_buffer_t* new_buffer = (compute_command_buffer_t*)calloc(1, sizeof(compute_command_buffer_t));
    if (!new_buffer) {
        pthread_mutex_unlock(&pipeline->mutex);
        return COMPUTE_ERROR_OUT_OF_MEMORY;
    }
    
    new_buffer->id = generate_id();
    new_buffer->creation_time_ns = get_timestamp_ns();
    
    // Backend-specific creation would go here
    // result = backend_create_command_buffer(&new_buffer->backend_handle);
    
    // Add to pipeline
    pipeline->command_buffers[pipeline->command_buffer_count++] = new_buffer;
    
    *cmd_buffer = new_buffer;
    pthread_mutex_unlock(&pipeline->mutex);
    
    return COMPUTE_SUCCESS;
}

void compute_command_buffer_destroy(compute_pipeline_t* pipeline, compute_command_buffer_t* cmd_buffer) {
    if (!pipeline || !cmd_buffer) {
        return;
    }
    
    pthread_mutex_lock(&pipeline->mutex);
    
    // Find and remove from pipeline
    for (uint32_t i = 0; i < pipeline->command_buffer_count; i++) {
        if (pipeline->command_buffers[i] == cmd_buffer) {
            // Remove from array
            for (uint32_t j = i; j < pipeline->command_buffer_count - 1; j++) {
                pipeline->command_buffers[j] = pipeline->command_buffers[j + 1];
            }
            pipeline->command_buffer_count--;
            break;
        }
    }
    
    // Backend cleanup would go here
    // if (cmd_buffer->backend_handle) { backend_destroy_command_buffer(cmd_buffer->backend_handle); }
    
    free(cmd_buffer);
    pthread_mutex_unlock(&pipeline->mutex);
}

compute_error_t compute_command_buffer_begin(compute_command_buffer_t* cmd_buffer) {
    if (!cmd_buffer) {
        return COMPUTE_ERROR_INVALID_PARAM;
    }
    
    if (cmd_buffer->is_recording) {
        return COMPUTE_ERROR_INVALID_PARAM;
    }
    
    cmd_buffer->is_recording = true;
    cmd_buffer->command_count = 0;
    
    // Backend-specific begin would go here
    // return backend_begin_command_buffer(cmd_buffer->backend_handle);
    
    return COMPUTE_SUCCESS;
}

compute_error_t compute_command_buffer_end(compute_command_buffer_t* cmd_buffer) {
    if (!cmd_buffer) {
        return COMPUTE_ERROR_INVALID_PARAM;
    }
    
    if (!cmd_buffer->is_recording) {
        return COMPUTE_ERROR_INVALID_PARAM;
    }
    
    cmd_buffer->is_recording = false;
    
    // Backend-specific end would go here
    // return backend_end_command_buffer(cmd_buffer->backend_handle);
    
    return COMPUTE_SUCCESS;
}

compute_error_t compute_command_buffer_dispatch(compute_command_buffer_t* cmd_buffer, const compute_dispatch_params_t* params) {
    if (!cmd_buffer || !params) {
        return COMPUTE_ERROR_INVALID_PARAM;
    }
    
    if (!cmd_buffer->is_recording) {
        return COMPUTE_ERROR_INVALID_PARAM;
    }
    
    cmd_buffer->command_count++;
    
    // Backend-specific dispatch would go here
    // return backend_dispatch(cmd_buffer->backend_handle, params);
    
    return COMPUTE_SUCCESS;
}

compute_error_t compute_command_buffer_barrier(compute_command_buffer_t* cmd_buffer) {
    if (!cmd_buffer) {
        return COMPUTE_ERROR_INVALID_PARAM;
    }
    
    if (!cmd_buffer->is_recording) {
        return COMPUTE_ERROR_INVALID_PARAM;
    }
    
    // Backend-specific barrier would go here
    // return backend_barrier(cmd_buffer->backend_handle);
    
    return COMPUTE_SUCCESS;
}

// Pipeline execution
compute_error_t compute_execute(compute_pipeline_t* pipeline, compute_command_buffer_t* cmd_buffer) {
    if (!pipeline || !cmd_buffer) {
        return COMPUTE_ERROR_INVALID_PARAM;
    }
    
    if (!pipeline->is_initialized) {
        return COMPUTE_ERROR_NOT_INITIALIZED;
    }
    
    if (cmd_buffer->is_recording) {
        return COMPUTE_ERROR_INVALID_PARAM;
    }
    
    uint64_t start_time = get_timestamp_ns();
    
    // Backend-specific execution would go here
    // compute_error_t result = backend_execute_command_buffer(cmd_buffer->backend_handle);
    compute_error_t result = COMPUTE_SUCCESS;
    
    uint64_t end_time = get_timestamp_ns();
    
    // Update statistics
    pipeline->stats.total_dispatches++;
    pipeline->stats.total_gpu_time_ns += (end_time - start_time);
    
    return result;
}

compute_error_t compute_execute_immediate(compute_pipeline_t* pipeline, compute_shader_t* shader, const compute_dispatch_params_t* params) {
    if (!pipeline || !shader || !params) {
        return COMPUTE_ERROR_INVALID_PARAM;
    }
    
    if (!pipeline->is_initialized) {
        return COMPUTE_ERROR_NOT_INITIALIZED;
    }
    
    // Create temporary command buffer
    compute_command_buffer_t* temp_buffer;
    compute_error_t result = compute_command_buffer_create(pipeline, &temp_buffer);
    if (result != COMPUTE_SUCCESS) {
        return result;
    }
    
    result = compute_command_buffer_begin(temp_buffer);
    if (result != COMPUTE_SUCCESS) {
        compute_command_buffer_destroy(pipeline, temp_buffer);
        return result;
    }
    
    result = compute_shader_bind(pipeline, shader);
    if (result != COMPUTE_SUCCESS) {
        compute_command_buffer_destroy(pipeline, temp_buffer);
        return result;
    }
    
    result = compute_command_buffer_dispatch(temp_buffer, params);
    if (result != COMPUTE_SUCCESS) {
        compute_command_buffer_destroy(pipeline, temp_buffer);
        return result;
    }
    
    result = compute_command_buffer_end(temp_buffer);
    if (result != COMPUTE_SUCCESS) {
        compute_command_buffer_destroy(pipeline, temp_buffer);
        return result;
    }
    
    result = compute_execute(pipeline, temp_buffer);
    
    compute_command_buffer_destroy(pipeline, temp_buffer);
    
    return result;
}

// Synchronization
compute_error_t compute_fence_create(compute_pipeline_t* pipeline, uint64_t* fence) {
    if (!pipeline || !fence) {
        return COMPUTE_ERROR_INVALID_PARAM;
    }
    
    pthread_mutex_lock(&pipeline->fence_mutex);
    *fence = pipeline->next_fence_id++;
    pthread_mutex_unlock(&pipeline->fence_mutex);
    
    return COMPUTE_SUCCESS;
}

compute_error_t compute_fence_wait(compute_pipeline_t* pipeline, uint64_t fence, uint64_t timeout_ns) {
    if (!pipeline) {
        return COMPUTE_ERROR_INVALID_PARAM;
    }
    
    // Backend-specific fence wait would go here
    // return backend_fence_wait(fence, timeout_ns);
    
    return COMPUTE_SUCCESS;
}

compute_error_t compute_fence_signal(compute_pipeline_t* pipeline, uint64_t fence) {
    if (!pipeline) {
        return COMPUTE_ERROR_INVALID_PARAM;
    }
    
    // Backend-specific fence signal would go here
    // return backend_fence_signal(fence);
    
    return COMPUTE_SUCCESS;
}

// Memory management
compute_error_t compute_memory_allocate(compute_pipeline_t* pipeline, uint64_t size, uint64_t* allocation) {
    if (!pipeline || !allocation) {
        return COMPUTE_ERROR_INVALID_PARAM;
    }
    
    if (pipeline->total_memory_allocated + size > pipeline->memory_budget) {
        return COMPUTE_ERROR_OUT_OF_MEMORY;
    }
    
    pthread_mutex_lock(&pipeline->mutex);
    pipeline->total_memory_allocated += size;
    *allocation = (uint64_t)pipeline; // Use pipeline pointer as allocation handle for simplicity
    pthread_mutex_unlock(&pipeline->mutex);
    
    return COMPUTE_SUCCESS;
}

compute_error_t compute_memory_deallocate(compute_pipeline_t* pipeline, uint64_t allocation) {
    if (!pipeline || allocation == 0) {
        return COMPUTE_ERROR_INVALID_PARAM;
    }
    
    // For simplicity, we don't track individual allocations in this implementation
    // In a real implementation, you would track allocation sizes and update the total
    
    return COMPUTE_SUCCESS;
}

compute_error_t compute_memory_get_usage(compute_pipeline_t* pipeline, uint64_t* used, uint64_t* total) {
    if (!pipeline || !used || !total) {
        return COMPUTE_ERROR_INVALID_PARAM;
    }
    
    pthread_mutex_lock(&pipeline->mutex);
    *used = pipeline->total_memory_allocated;
    *total = pipeline->memory_budget;
    pthread_mutex_unlock(&pipeline->mutex);
    
    return COMPUTE_SUCCESS;
}

// Validation and debugging
compute_error_t compute_validate_shader(compute_pipeline_t* pipeline, compute_shader_t* shader) {
    if (!pipeline || !shader) {
        return COMPUTE_ERROR_INVALID_PARAM;
    }
    
    if (!shader->is_compiled) {
        return COMPUTE_ERROR_SHADER_COMPILATION_FAILED;
    }
    
    // Backend-specific validation would go here
    // return backend_validate_shader(shader->backend_handle);
    
    return COMPUTE_SUCCESS;
}

compute_error_t compute_validate_resources(compute_pipeline_t* pipeline) {
    if (!pipeline) {
        return COMPUTE_ERROR_INVALID_PARAM;
    }
    
    pthread_mutex_lock(&pipeline->mutex);
    
    for (uint32_t i = 0; i < pipeline->resource_count; i++) {
        compute_resource_t* resource = pipeline->resources[i];
        if (!resource || !resource->backend_handle) {
            pthread_mutex_unlock(&pipeline->mutex);
            return COMPUTE_ERROR_RESOURCE_CREATION_FAILED;
        }
    }
    
    pthread_mutex_unlock(&pipeline->mutex);
    
    return COMPUTE_SUCCESS;
}

compute_error_t compute_debug_print_pipeline_state(compute_pipeline_t* pipeline) {
    if (!pipeline) {
        return COMPUTE_ERROR_INVALID_PARAM;
    }
    
    pthread_mutex_lock(&pipeline->mutex);
    
    printf("=== Compute Pipeline State ===\n");
    printf("Backend: %d\n", pipeline->backend);
    printf("Initialized: %s\n", pipeline->is_initialized ? "Yes" : "No");
    printf("Shaders: %u / %u\n", pipeline->shader_count, pipeline->shader_capacity);
    printf("Resources: %u / %u\n", pipeline->resource_count, pipeline->resource_capacity);
    printf("Command Buffers: %u / %u\n", pipeline->command_buffer_count, pipeline->command_buffer_capacity);
    printf("Memory Used: %llu / %llu bytes\n", (unsigned long long)pipeline->total_memory_allocated, (unsigned long long)pipeline->memory_budget);
    printf("Next Fence ID: %llu\n", (unsigned long long)pipeline->next_fence_id);
    
    printf("\n=== Statistics ===\n");
    printf("Total Shaders Created: %u\n", pipeline->stats.total_shaders_created);
    printf("Total Resources Created: %u\n", pipeline->stats.total_resources_created);
    printf("Total Dispatches: %u\n", pipeline->stats.total_dispatches);
    printf("Total GPU Time: %llu ns\n", (unsigned long long)pipeline->stats.total_gpu_time_ns);
    printf("Active Shaders: %u\n", pipeline->stats.active_shaders);
    printf("Active Resources: %u\n", pipeline->stats.active_resources);
    
    pthread_mutex_unlock(&pipeline->mutex);
    
    return COMPUTE_SUCCESS;
}

// Statistics
compute_error_t compute_get_statistics(compute_pipeline_t* pipeline, compute_pipeline_stats_t* stats) {
    if (!pipeline || !stats) {
        return COMPUTE_ERROR_INVALID_PARAM;
    }
    
    pthread_mutex_lock(&pipeline->mutex);
    *stats = pipeline->stats;
    pthread_mutex_unlock(&pipeline->mutex);
    
    return COMPUTE_SUCCESS;
}

compute_error_t compute_reset_statistics(compute_pipeline_t* pipeline) {
    if (!pipeline) {
        return COMPUTE_ERROR_INVALID_PARAM;
    }
    
    pthread_mutex_lock(&pipeline->mutex);
    memset(&pipeline->stats, 0, sizeof(pipeline->stats));
    pthread_mutex_unlock(&pipeline->mutex);
    
    return COMPUTE_SUCCESS;
}

// Backend-specific functions (stubs for now)
compute_error_t compute_backend_init(compute_pipeline_t* pipeline) {
    // Backend-specific initialization would go here
    // This would initialize Vulkan, Metal, D3D12, or OpenGL compute capabilities
    return COMPUTE_SUCCESS;
}

void compute_backend_shutdown(compute_pipeline_t* pipeline) {
    // Backend-specific cleanup would go here
}

compute_error_t compute_backend_create_shader(compute_pipeline_t* pipeline, const compute_shader_desc_t* desc, compute_shader_t** shader) {
    // Backend-specific shader creation would go here
    (*shader)->backend_handle = (void*)(uintptr_t)0x12345678; // Dummy handle
    return COMPUTE_SUCCESS;
}

compute_error_t compute_backend_create_resource(compute_pipeline_t* pipeline, const compute_resource_desc_t* desc, compute_resource_t** resource) {
    // Backend-specific resource creation would go here
    (*resource)->backend_handle = (void*)(uintptr_t)0x87654321; // Dummy handle
    return COMPUTE_SUCCESS;
}

compute_error_t compute_backend_dispatch(compute_pipeline_t* pipeline, const compute_dispatch_params_t* params) {
    // Backend-specific dispatch would go here
    return COMPUTE_SUCCESS;
}

// Error handling
const char* compute_error_string(compute_error_t error) {
    switch (error) {
        case COMPUTE_SUCCESS: return "Success";
        case COMPUTE_ERROR_INVALID_PARAM: return "Invalid parameter";
        case COMPUTE_ERROR_OUT_OF_MEMORY: return "Out of memory";
        case COMPUTE_ERROR_BACKEND_NOT_SUPPORTED: return "Backend not supported";
        case COMPUTE_ERROR_SHADER_COMPILATION_FAILED: return "Shader compilation failed";
        case COMPUTE_ERROR_RESOURCE_CREATION_FAILED: return "Resource creation failed";
        case COMPUTE_ERROR_DISPATCH_FAILED: return "Dispatch failed";
        case COMPUTE_ERROR_NOT_INITIALIZED: return "Not initialized";
        case COMPUTE_ERROR_BACKEND_INITIALIZATION_FAILED: return "Backend initialization failed";
        default: return "Unknown error";
    }
}
