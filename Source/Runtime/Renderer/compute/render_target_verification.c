#include "render_target_verification.h"
#include <stdlib.h>
#include <string.h>
#include <stdio.h>
#include <time.h>

// Internal structures
struct render_target_t {
    uint32_t id;
    uint32_t width;
    uint32_t height;
    uint32_t depth;
    rt_format_t format;
    rt_type_t type;
    rt_usage_flags_t usage;
    rt_sample_count_t samples;
    uint32_t mip_levels;
    uint32_t array_size;
    char* name;
    uint64_t size_bytes;
    uint32_t pixel_size;
    bool is_valid;
    void* backend_handle;
    uint64_t creation_time;
};

struct render_target_array_t {
    render_target_t** targets;
    uint32_t count;
    uint32_t capacity;
    uint32_t array_size;
    rt_format_t format;
};

struct render_target_manager_t {
    bool is_initialized;
    pthread_mutex_t mutex;
    
    // Target management
    render_target_t** targets;
    uint32_t target_count;
    uint32_t target_capacity;
    uint64_t next_target_id;
    
    // MRT configurations
    mrt_config_t** mrt_configs;
    uint32_t mrt_config_count;
    uint32_t mrt_config_capacity;
    
    // Target arrays
    render_target_array_t** arrays;
    uint32_t array_count;
    uint32_t array_capacity;
    
    // Statistics
    rt_manager_stats_t stats;
    
    // Memory tracking
    uint64_t total_memory_used;
    uint64_t peak_memory_usage;
};

// Format information table
typedef struct {
    rt_format_t format;
    uint32_t pixel_size;
    bool is_depth;
    bool is_compressed;
    const char* name;
} format_info_t;

static const format_info_t format_table[] = {
    {RT_FORMAT_R8_UNORM, 1, false, false, "R8_UNORM"},
    {RT_FORMAT_R8G8_UNORM, 2, false, false, "R8G8_UNORM"},
    {RT_FORMAT_R8G8B8A8_UNORM, 4, false, false, "R8G8B8A8_UNORM"},
    {RT_FORMAT_R16_FLOAT, 2, false, false, "R16_FLOAT"},
    {RT_FORMAT_R16G16_FLOAT, 4, false, false, "R16G16_FLOAT"},
    {RT_FORMAT_R16G16B16A16_FLOAT, 8, false, false, "R16G16B16A16_FLOAT"},
    {RT_FORMAT_R32_FLOAT, 4, false, false, "R32_FLOAT"},
    {RT_FORMAT_R32G32_FLOAT, 8, false, false, "R32G32_FLOAT"},
    {RT_FORMAT_R32G32B32A32_FLOAT, 16, false, false, "R32G32B32A32_FLOAT"},
    {RT_FORMAT_R11G11B10_FLOAT, 4, false, false, "R11G11B10_FLOAT"},
    {RT_FORMAT_D16_UNORM, 2, true, false, "D16_UNORM"},
    {RT_FORMAT_D24_UNORM_S8_UINT, 4, true, false, "D24_UNORM_S8_UINT"},
    {RT_FORMAT_D32_FLOAT, 4, true, false, "D32_FLOAT"}
};

static const uint32_t format_table_size = sizeof(format_table) / sizeof(format_table[0]);

static uint64_t get_timestamp_ns(void) {
    struct timespec ts;
    clock_gettime(CLOCK_MONOTONIC, &ts);
    return (uint64_t)ts.tv_sec * 1000000000ULL + (uint64_t)ts.tv_nsec;
}

static const format_info_t* get_format_info(rt_format_t format) {
    for (uint32_t i = 0; i < format_table_size; i++) {
        if (format_table[i].format == format) {
            return &format_table[i];
        }
    }
    return NULL;
}

static uint64_t calculate_target_size(const rt_desc_t* desc) {
    const format_info_t* format_info = get_format_info(desc->format);
    if (!format_info) {
        return 0;
    }
    
    uint64_t size = (uint64_t)desc->width * desc->height * desc->depth * format_info->pixel_size;
    
    // Account for mip levels
    if (desc->mip_levels > 1) {
        uint64_t mip_size = size;
        for (uint32_t i = 1; i < desc->mip_levels; i++) {
            mip_size /= 4; // Each mip level is 1/4 the size
            size += mip_size;
        }
    }
    
    // Account for array size
    size *= desc->array_size;
    
    // Account for sample count
    size *= desc->samples;
    
    return size;
}

static bool validate_dimensions(uint32_t width, uint32_t height, uint32_t depth) {
    // Check for reasonable size limits
    if (width == 0 || height == 0 || depth == 0) {
        return false;
    }
    
    if (width > 16384 || height > 16384 || depth > 2048) {
        return false;
    }
    
    // Check for power-of-two requirements for certain formats
    // This would be more sophisticated in a real implementation
    
    return true;
}

static bool validate_sample_count(rt_sample_count_t samples) {
    return samples == RT_SAMPLES_1 || samples == RT_SAMPLES_2 || 
           samples == RT_SAMPLES_4 || samples == RT_SAMPLES_8 || samples == RT_SAMPLES_16;
}

rt_error_t rt_manager_init(render_target_manager_t** manager) {
    if (!manager) {
        return RT_ERROR_INVALID_PARAM;
    }
    
    render_target_manager_t* new_manager = (render_target_manager_t*)calloc(1, sizeof(render_target_manager_t));
    if (!new_manager) {
        return RT_ERROR_OUT_OF_MEMORY;
    }
    
    if (pthread_mutex_init(&new_manager->mutex, NULL) != 0) {
        free(new_manager);
        return RT_ERROR_OUT_OF_MEMORY;
    }
    
    new_manager->target_capacity = 256;
    new_manager->targets = (render_target_t**)calloc(new_manager->target_capacity, sizeof(render_target_t*));
    
    new_manager->mrt_config_capacity = 64;
    new_manager->mrt_configs = (mrt_config_t**)calloc(new_manager->mrt_config_capacity, sizeof(mrt_config_t*));
    
    new_manager->array_capacity = 64;
    new_manager->arrays = (render_target_array_t**)calloc(new_manager->array_capacity, sizeof(render_target_array_t*));
    
    if (!new_manager->targets || !new_manager->mrt_configs || !new_manager->arrays) {
        pthread_mutex_destroy(&new_manager->mutex);
        free(new_manager->targets);
        free(new_manager->mrt_configs);
        free(new_manager->arrays);
        free(new_manager);
        return RT_ERROR_OUT_OF_MEMORY;
    }
    
    new_manager->is_initialized = true;
    *manager = new_manager;
    
    return RT_SUCCESS;
}

void rt_manager_shutdown(render_target_manager_t* manager) {
    if (!manager) {
        return;
    }
    
    pthread_mutex_lock(&manager->mutex);
    
    if (!manager->is_initialized) {
        pthread_mutex_unlock(&manager->mutex);
        return;
    }
    
    // Check for leaks
    if (manager->target_count > 0) {
        printf("Warning: %u render target leaks detected\n", manager->target_count);
    }
    
    // Free all targets
    for (uint32_t i = 0; i < manager->target_count; i++) {
        render_target_t* target = manager->targets[i];
        free(target->name);
        free(target);
    }
    
    // Free all MRT configs
    for (uint32_t i = 0; i < manager->mrt_config_count; i++) {
        mrt_config_t* config = manager->mrt_configs[i];
        free(config->color_targets);
        free(config);
    }
    
    // Free all arrays
    for (uint32_t i = 0; i < manager->array_count; i++) {
        render_target_array_t* array = manager->arrays[i];
        free(array->targets);
        free(array);
    }
    
    free(manager->targets);
    free(manager->mrt_configs);
    free(manager->arrays);
    
    manager->is_initialized = false;
    pthread_mutex_unlock(&manager->mutex);
    pthread_mutex_destroy(&manager->mutex);
    
    free(manager);
}

rt_error_t rt_create_target(render_target_manager_t* manager, 
                           const rt_desc_t* desc, 
                           render_target_t** target) {
    if (!manager || !desc || !target) {
        return RT_ERROR_INVALID_PARAM;
    }
    
    if (!manager->is_initialized) {
        return RT_ERROR_NOT_INITIALIZED;
    }
    
    pthread_mutex_lock(&manager->mutex);
    
    // Validate format
    const format_info_t* format_info = get_format_info(desc->format);
    if (!format_info) {
        pthread_mutex_unlock(&manager->mutex);
        return RT_ERROR_FORMAT_NOT_SUPPORTED;
    }
    
    // Validate dimensions
    if (!validate_dimensions(desc->width, desc->height, desc->depth)) {
        pthread_mutex_unlock(&manager->mutex);
        return RT_ERROR_SIZE_NOT_SUPPORTED;
    }
    
    // Validate sample count
    if (!validate_sample_count(desc->samples)) {
        pthread_mutex_unlock(&manager->mutex);
        return RT_ERROR_SAMPLE_COUNT_NOT_SUPPORTED;
    }
    
    // Check capacity
    if (manager->target_count >= manager->target_capacity) {
        uint32_t new_capacity = manager->target_capacity * 2;
        render_target_t** new_targets = (render_target_t**)realloc(manager->targets, new_capacity * sizeof(render_target_t*));
        if (!new_targets) {
            pthread_mutex_unlock(&manager->mutex);
            return RT_ERROR_OUT_OF_MEMORY;
        }
        manager->targets = new_targets;
        manager->target_capacity = new_capacity;
    }
    
    // Create target
    render_target_t* new_target = (render_target_t*)calloc(1, sizeof(render_target_t));
    if (!new_target) {
        pthread_mutex_unlock(&manager->mutex);
        return RT_ERROR_OUT_OF_MEMORY;
    }
    
    new_target->id = manager->next_target_id++;
    new_target->width = desc->width;
    new_target->height = desc->height;
    new_target->depth = desc->depth;
    new_target->format = desc->format;
    new_target->type = desc->type;
    new_target->usage = desc->usage;
    new_target->samples = desc->samples;
    new_target->mip_levels = desc->mip_levels;
    new_target->array_size = desc->array_size;
    new_target->pixel_size = format_info->pixel_size;
    new_target->is_valid = true;
    new_target->creation_time = get_timestamp_ns();
    
    // Calculate size
    new_target->size_bytes = calculate_target_size(desc);
    
    // Copy name
    new_target->name = desc->name ? strdup(desc->name) : strdup("unnamed_target");
    if (!new_target->name) {
        free(new_target);
        pthread_mutex_unlock(&manager->mutex);
        return RT_ERROR_OUT_OF_MEMORY;
    }
    
    // Simulate backend handle creation
    new_target->backend_handle = malloc(new_target->size_bytes);
    if (!new_target->backend_handle) {
        free(new_target->name);
        free(new_target);
        pthread_mutex_unlock(&manager->mutex);
        return RT_ERROR_OUT_OF_MEMORY;
    }
    
    // Add to manager
    manager->targets[manager->target_count++] = new_target;
    
    // Update statistics
    manager->stats.total_targets_created++;
    manager->stats.active_targets++;
    manager->total_memory_used += new_target->size_bytes;
    
    if (manager->total_memory_used > manager->peak_memory_usage) {
        manager->peak_memory_usage = manager->total_memory_used;
    }
    
    *target = new_target;
    pthread_mutex_unlock(&manager->mutex);
    
    return RT_SUCCESS;
}

rt_error_t rt_destroy_target(render_target_manager_t* manager, render_target_t* target) {
    if (!manager || !target) {
        return RT_ERROR_INVALID_PARAM;
    }
    
    pthread_mutex_lock(&manager->mutex);
    
    // Find and remove target
    bool found = false;
    for (uint32_t i = 0; i < manager->target_count; i++) {
        if (manager->targets[i] == target) {
            found = true;
            
            // Update memory tracking
            manager->total_memory_used -= target->size_bytes;
            
            // Update statistics
            manager->stats.total_targets_destroyed--;
            manager->stats.active_targets--;
            
            // Remove from array
            for (uint32_t j = i; j < manager->target_count - 1; j++) {
                manager->targets[j] = manager->targets[j + 1];
            }
            manager->target_count--;
            
            // Free resources
            free(target->name);
            free(target->backend_handle);
            free(target);
            
            break;
        }
    }
    
    pthread_mutex_unlock(&manager->mutex);
    
    if (!found) {
        return RT_ERROR_NOT_FOUND;
    }
    
    return RT_SUCCESS;
}

rt_error_t rt_create_mrt_config(render_target_manager_t* manager,
                               render_target_t** color_targets,
                               uint32_t color_target_count,
                               render_target_t* depth_target,
                               mrt_config_t** config) {
    if (!manager || !color_targets || color_target_count == 0 || !config) {
        return RT_ERROR_INVALID_PARAM;
    }
    
    if (color_target_count > 8) { // Typical limit for MRT
        return RT_ERROR_INVALID_MRT_CONFIG;
    }
    
    pthread_mutex_lock(&manager->mutex);
    
    // Check capacity
    if (manager->mrt_config_count >= manager->mrt_config_capacity) {
        uint32_t new_capacity = manager->mrt_config_capacity * 2;
        mrt_config_t** new_configs = (mrt_config_t**)realloc(manager->mrt_configs, new_capacity * sizeof(mrt_config_t*));
        if (!new_configs) {
            pthread_mutex_unlock(&manager->mutex);
            return RT_ERROR_OUT_OF_MEMORY;
        }
        manager->mrt_configs = new_configs;
        manager->mrt_config_capacity = new_capacity;
    }
    
    // Validate all targets have same dimensions
    uint32_t width = color_targets[0]->width;
    uint32_t height = color_targets[0]->height;
    uint32_t sample_count = color_targets[0]->samples;
    
    for (uint32_t i = 0; i < color_target_count; i++) {
        if (color_targets[i]->width != width || 
            color_targets[i]->height != height ||
            color_targets[i]->samples != sample_count) {
            pthread_mutex_unlock(&manager->mutex);
            return RT_ERROR_INCOMPATIBLE_TARGETS;
        }
    }
    
    if (depth_target && 
        (depth_target->width != width || 
         depth_target->height != height ||
         depth_target->samples != sample_count)) {
        pthread_mutex_unlock(&manager->mutex);
        return RT_ERROR_INCOMPATIBLE_TARGETS;
    }
    
    // Create MRT config
    mrt_config_t* new_config = (mrt_config_t*)calloc(1, sizeof(mrt_config_t));
    if (!new_config) {
        pthread_mutex_unlock(&manager->mutex);
        return RT_ERROR_OUT_OF_MEMORY;
    }
    
    new_config->color_targets = (render_target_t**)malloc(color_target_count * sizeof(render_target_t*));
    if (!new_config->color_targets) {
        free(new_config);
        pthread_mutex_unlock(&manager->mutex);
        return RT_ERROR_OUT_OF_MEMORY;
    }
    
    memcpy(new_config->color_targets, color_targets, color_target_count * sizeof(render_target_t*));
    new_config->color_target_count = color_target_count;
    new_config->depth_target = depth_target;
    new_config->width = width;
    new_config->height = height;
    new_config->sample_count = sample_count;
    new_config->is_valid = true;
    
    // Add to manager
    manager->mrt_configs[manager->mrt_config_count++] = new_config;
    
    // Update statistics
    manager->stats.mrt_configurations++;
    
    *config = new_config;
    pthread_mutex_unlock(&manager->mutex);
    
    return RT_SUCCESS;
}

rt_error_t rt_validate_mrt_config(render_target_manager_t* manager, mrt_config_t* config) {
    if (!manager || !config) {
        return RT_ERROR_INVALID_PARAM;
    }
    
    pthread_mutex_lock(&manager->mutex);
    
    // Check if all targets exist and are compatible
    for (uint32_t i = 0; i < config->color_target_count; i++) {
        bool found = false;
        for (uint32_t j = 0; j < manager->target_count; j++) {
            if (manager->targets[j] == config->color_targets[i]) {
                found = true;
                break;
            }
        }
        if (!found || !config->color_targets[i]->is_valid) {
            pthread_mutex_unlock(&manager->mutex);
            return RT_ERROR_INCOMPATIBLE_TARGETS;
        }
    }
    
    if (config->depth_target) {
        bool found = false;
        for (uint32_t j = 0; j < manager->target_count; j++) {
            if (manager->targets[j] == config->depth_target) {
                found = true;
                break;
            }
        }
        if (!found || !config->depth_target->is_valid) {
            pthread_mutex_unlock(&manager->mutex);
            return RT_ERROR_INCOMPATIBLE_TARGETS;
        }
    }
    
    config->is_valid = true;
    pthread_mutex_unlock(&manager->mutex);
    
    return RT_SUCCESS;
}

rt_error_t rt_validate_format(rt_format_t format, rt_usage_flags_t usage) {
    const format_info_t* format_info = get_format_info(format);
    if (!format_info) {
        return RT_ERROR_FORMAT_NOT_SUPPORTED;
    }
    
    // Check format-usage compatibility
    if (format_info->is_depth && !(usage & RT_USAGE_DEPTH_STENCIL)) {
        return RT_ERROR_USAGE_NOT_SUPPORTED;
    }
    
    if (!format_info->is_depth && (usage & RT_USAGE_DEPTH_STENCIL)) {
        return RT_ERROR_USAGE_NOT_SUPPORTED;
    }
    
    return RT_SUCCESS;
}

rt_error_t rt_check_compatibility(render_target_manager_t* manager,
                                 const rt_desc_t* desc,
                                 rt_compatibility_result_t* result) {
    if (!manager || !desc || !result) {
        return RT_ERROR_INVALID_PARAM;
    }
    
    memset(result, 0, sizeof(rt_compatibility_result_t));
    result->is_compatible = true;
    
    // Check format
    const format_info_t* format_info = get_format_info(desc->format);
    if (!format_info) {
        result->is_compatible = false;
        result->format_supported = false;
        result->error_message = strdup("Format not supported");
        return RT_SUCCESS;
    }
    result->format_supported = true;
    
    // Check dimensions
    if (!validate_dimensions(desc->width, desc->height, desc->depth)) {
        result->is_compatible = false;
        result->size_supported = false;
        result->error_message = strdup("Invalid dimensions");
        return RT_SUCCESS;
    }
    result->size_supported = true;
    
    // Check usage
    if (rt_validate_format(desc->format, desc->usage) != RT_SUCCESS) {
        result->is_compatible = false;
        result->usage_supported = false;
        result->error_message = strdup("Usage not supported for format");
        return RT_SUCCESS;
    }
    result->usage_supported = true;
    
    // Check sample count
    if (!validate_sample_count(desc->samples)) {
        result->is_compatible = false;
        result->sample_count_supported = false;
        result->error_message = strdup("Sample count not supported");
        return RT_SUCCESS;
    }
    result->sample_count_supported = true;
    
    return RT_SUCCESS;
}

rt_error_t rt_get_statistics(render_target_manager_t* manager, rt_manager_stats_t* stats) {
    if (!manager || !stats) {
        return RT_ERROR_INVALID_PARAM;
    }
    
    pthread_mutex_lock(&manager->mutex);
    *stats = manager->stats;
    stats->total_memory_used = manager->total_memory_used;
    stats->peak_memory_usage = manager->peak_memory_usage;
    pthread_mutex_unlock(&manager->mutex);
    
    return RT_SUCCESS;
}

rt_error_t rt_debug_print_targets(render_target_manager_t* manager) {
    if (!manager) {
        return RT_ERROR_INVALID_PARAM;
    }
    
    pthread_mutex_lock(&manager->mutex);
    
    printf("=== Render Targets ===\n");
    printf("Total targets: %u\n", manager->target_count);
    printf("Total memory used: %llu bytes\n", (unsigned long long)manager->total_memory_used);
    
    for (uint32_t i = 0; i < manager->target_count; i++) {
        render_target_t* target = manager->targets[i];
        printf("ID: %u, %s, %ux%u, Format: %s, Size: %llu bytes\n",
               target->id, target->name, target->width, target->height,
               rt_format_string(target->format), (unsigned long long)target->size_bytes);
    }
    
    pthread_mutex_unlock(&manager->mutex);
    
    return RT_SUCCESS;
}

// Utility functions
const char* rt_format_string(rt_format_t format) {
    const format_info_t* info = get_format_info(format);
    return info ? info->name : "UNKNOWN";
}

const char* rt_error_string(rt_error_t error) {
    switch (error) {
        case RT_SUCCESS: return "Success";
        case RT_ERROR_INVALID_PARAM: return "Invalid parameter";
        case RT_ERROR_OUT_OF_MEMORY: return "Out of memory";
        case RT_ERROR_FORMAT_NOT_SUPPORTED: return "Format not supported";
        case RT_ERROR_SIZE_NOT_SUPPORTED: return "Size not supported";
        case RT_ERROR_USAGE_NOT_SUPPORTED: return "Usage not supported";
        case RT_ERROR_SAMPLE_COUNT_NOT_SUPPORTED: return "Sample count not supported";
        case RT_ERROR_CREATION_FAILED: return "Creation failed";
        case RT_ERROR_NOT_FOUND: return "Not found";
        case RT_ERROR_ALREADY_EXISTS: return "Already exists";
        case RT_ERROR_INVALID_MRT_CONFIG: return "Invalid MRT config";
        case RT_ERROR_INCOMPATIBLE_TARGETS: return "Incompatible targets";
        case RT_ERROR_NOT_INITIALIZED: return "Not initialized";
        default: return "Unknown error";
    }
}

uint32_t rt_get_pixel_size(rt_format_t format) {
    const format_info_t* info = get_format_info(format);
    return info ? info->pixel_size : 0;
}

bool rt_is_depth_format(rt_format_t format) {
    const format_info_t* info = get_format_info(format);
    return info ? info->is_depth : false;
}

bool rt_is_color_format(rt_format_t format) {
    const format_info_t* info = get_format_info(format);
    return info ? !info->is_depth : false;
}

bool rt_is_compressed_format(rt_format_t format) {
    const format_info_t* info = get_format_info(format);
    return info ? info->is_compressed : false;
}
