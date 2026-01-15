#ifndef RENDER_TARGET_VERIFICATION_H
#define RENDER_TARGET_VERIFICATION_H

#include <stdint.h>
#include <stdbool.h>
#include <pthread.h>

#ifdef __cplusplus
extern "C" {
#endif

// Render target formats
typedef enum {
    RT_FORMAT_R8_UNORM,
    RT_FORMAT_R8G8_UNORM,
    RT_FORMAT_R8G8B8A8_UNORM,
    RT_FORMAT_R16_FLOAT,
    RT_FORMAT_R16G16_FLOAT,
    RT_FORMAT_R16G16B16A16_FLOAT,
    RT_FORMAT_R32_FLOAT,
    RT_FORMAT_R32G32_FLOAT,
    RT_FORMAT_R32G32B32A32_FLOAT,
    RT_FORMAT_R11G11B10_FLOAT,
    RT_FORMAT_D16_UNORM,
    RT_FORMAT_D24_UNORM_S8_UINT,
    RT_FORMAT_D32_FLOAT
} rt_format_t;

// Render target types
typedef enum {
    RT_TYPE_COLOR,
    RT_TYPE_DEPTH_STENCIL,
    RT_TYPE_RENDER_TARGET_VIEW,
    RT_TYPE_SHADER_RESOURCE_VIEW,
    RT_TYPE_UNORDERED_ACCESS_VIEW
} rt_type_t;

// Render target usage flags
typedef enum {
    RT_USAGE_NONE = 0x0,
    RT_USAGE_RENDER_TARGET = 0x1,
    RT_USAGE_SHADER_RESOURCE = 0x2,
    RT_USAGE_UNORDERED_ACCESS = 0x4,
    RT_USAGE_DEPTH_STENCIL = 0x8,
    RT_USAGE_INPUT_ATTACHMENT = 0x10,
    RT_USAGE_TRANSFER_SRC = 0x20,
    RT_USAGE_TRANSFER_DST = 0x40
} rt_usage_flags_t;

// Render target sample counts
typedef enum {
    RT_SAMPLES_1 = 1,
    RT_SAMPLES_2 = 2,
    RT_SAMPLES_4 = 4,
    RT_SAMPLES_8 = 8,
    RT_SAMPLES_16 = 16
} rt_sample_count_t;

// Forward declarations
typedef struct render_target_manager_t render_target_manager_t;
typedef struct render_target_t render_target_t;
typedef struct render_target_array_t render_target_array_t;

// Render target descriptor
typedef struct {
    uint32_t width;
    uint32_t height;
    uint32_t depth;
    rt_format_t format;
    rt_type_t type;
    rt_usage_flags_t usage;
    rt_sample_count_t samples;
    uint32_t mip_levels;
    uint32_t array_size;
    const char* name;
} rt_desc_t;

// Render target information
typedef struct {
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
} rt_info_t;

// MRT (Multiple Render Targets) configuration
typedef struct {
    render_target_t** color_targets;
    uint32_t color_target_count;
    render_target_t* depth_target;
    uint32_t width;
    uint32_t height;
    uint32_t sample_count;
    bool is_valid;
} mrt_config_t;

// Render target compatibility check result
typedef struct {
    bool is_compatible;
    bool format_supported;
    bool size_supported;
    bool usage_supported;
    bool sample_count_supported;
    char* error_message;
} rt_compatibility_result_t;

// Render target manager statistics
typedef struct {
    uint32_t total_targets_created;
    uint32_t total_targets_destroyed;
    uint32_t active_targets;
    uint64_t total_memory_used;
    uint64_t peak_memory_usage;
    uint32_t mrt_configurations;
    uint32_t validation_failures;
    uint32_t format_conversions;
} rt_manager_stats_t;

// Error codes
typedef enum {
    RT_SUCCESS = 0,
    RT_ERROR_INVALID_PARAM = -1,
    RT_ERROR_OUT_OF_MEMORY = -2,
    RT_ERROR_FORMAT_NOT_SUPPORTED = -3,
    RT_ERROR_SIZE_NOT_SUPPORTED = -4,
    RT_ERROR_USAGE_NOT_SUPPORTED = -5,
    RT_ERROR_SAMPLE_COUNT_NOT_SUPPORTED = -6,
    RT_ERROR_CREATION_FAILED = -7,
    RT_ERROR_NOT_FOUND = -8,
    RT_ERROR_ALREADY_EXISTS = -9,
    RT_ERROR_INVALID_MRT_CONFIG = -10,
    RT_ERROR_INCOMPATIBLE_TARGETS = -11,
    RT_ERROR_NOT_INITIALIZED = -12
} rt_error_t;

// Manager lifecycle
rt_error_t rt_manager_init(render_target_manager_t** manager);
void rt_manager_shutdown(render_target_manager_t* manager);

// Render target creation and management
rt_error_t rt_create_target(render_target_manager_t* manager, 
                           const rt_desc_t* desc, 
                           render_target_t** target);
rt_error_t rt_destroy_target(render_target_manager_t* manager, render_target_t* target);
rt_error_t rt_resize_target(render_target_manager_t* manager,
                           render_target_t* target,
                           uint32_t new_width,
                           uint32_t new_height);
rt_error_t rt_get_target_info(render_target_manager_t* manager,
                             render_target_t* target,
                             rt_info_t* info);

// MRT (Multiple Render Targets) support
rt_error_t rt_create_mrt_config(render_target_manager_t* manager,
                               render_target_t** color_targets,
                               uint32_t color_target_count,
                               render_target_t* depth_target,
                               mrt_config_t** config);
rt_error_t rt_destroy_mrt_config(render_target_manager_t* manager, mrt_config_t* config);
rt_error_t rt_validate_mrt_config(render_target_manager_t* manager, mrt_config_t* config);
rt_error_t rt_bind_mrt_config(render_target_manager_t* manager, mrt_config_t* config);

// Format validation and compatibility
rt_error_t rt_validate_format(rt_format_t format, rt_usage_flags_t usage);
rt_error_t rt_check_compatibility(render_target_manager_t* manager,
                                 const rt_desc_t* desc,
                                 rt_compatibility_result_t* result);
rt_error_t rt_get_format_info(rt_format_t format, uint32_t* pixel_size, bool* is_compressed);

// Render target arrays
rt_error_t rt_create_target_array(render_target_manager_t* manager,
                                 const rt_desc_t* desc,
                                 render_target_array_t** array);
rt_error_t rt_destroy_target_array(render_target_manager_t* manager, render_target_array_t* array);
rt_error_t rt_get_array_slice(render_target_manager_t* manager,
                             render_target_array_t* array,
                             uint32_t slice,
                             render_target_t** target);

// Format conversion
rt_error_t rt_convert_format(render_target_manager_t* manager,
                            render_target_t* source,
                            render_target_t* destination);
rt_error_t rt_generate_mipmaps(render_target_manager_t* manager, render_target_t* target);

// Validation and debugging
rt_error_t rt_validate_all_targets(render_target_manager_t* manager);
rt_error_t rt_debug_print_targets(render_target_manager_t* manager);
rt_error_t rt_debug_print_mrt_configs(render_target_manager_t* manager);
rt_error_t rt_check_memory_leaks(render_target_manager_t* manager);

// Statistics and monitoring
rt_error_t rt_get_statistics(render_target_manager_t* manager, rt_manager_stats_t* stats);
rt_error_t rt_reset_statistics(render_target_manager_t* manager);
rt_error_t rt_get_memory_usage(render_target_manager_t* manager, uint64_t* used, uint64_t* peak);

// Automatic fallback system
rt_error_t rt_create_with_fallback(render_target_manager_t* manager,
                                  const rt_desc_t* desc,
                                  render_target_t** target,
                                  rt_format_t* fallback_format);

// Render target binding and unbinding
rt_error_t rt_bind_target(render_target_manager_t* manager, render_target_t* target, uint32_t slot);
rt_error_t rt_unbind_target(render_target_manager_t* manager, uint32_t slot);
rt_error_t rt_clear_target(render_target_manager_t* manager,
                          render_target_t* target,
                          const float* clear_color,
                          float clear_depth,
                          uint8_t clear_stencil);

// Utility functions
const char* rt_format_string(rt_format_t format);
const char* rt_error_string(rt_error_t error);
uint32_t rt_get_pixel_size(rt_format_t format);
bool rt_is_depth_format(rt_format_t format);
bool rt_is_color_format(rt_format_t format);
bool rt_is_compressed_format(rt_format_t format);

// Utility macros
#define RT_CREATE_TARGET(manager, width, height, format, target) \
    rt_create_target(manager, &(rt_desc_t){ \
        .width = width, \
        .height = height, \
        .format = format, \
        .type = RT_TYPE_COLOR, \
        .usage = RT_USAGE_RENDER_TARGET | RT_USAGE_SHADER_RESOURCE, \
        .samples = RT_SAMPLES_1, \
        .name = #target \
    }, target)

#define RT_VALIDATE_TARGET(manager, target) \
    rt_validate_target(manager, target)

#ifdef __cplusplus
}
#endif

#endif // RENDER_TARGET_VERIFICATION_H
