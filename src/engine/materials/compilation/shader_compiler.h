/*
 * shader_compiler.h
 * Shader compilation pipeline
 *
 * Part of the Materials subsystem
 * Advanced 3D Rendering Engine
 */

#ifndef MATERIALS_SHADER_COMPILER_H
#define MATERIALS_SHADER_COMPILER_H

#include <stdint.h>
#include <stdbool.h>
#include <stddef.h>

#ifdef __cplusplus
extern "C" {
#endif

// ============================================================================
// TYPES AND CONSTANTS
// ============================================================================

typedef struct shader_compiler_handle {
    uint32_t id;
} shader_compiler_handle_t;

typedef enum shader_stage_e {
    SHADER_STAGE_VERTEX = 0,
    SHADER_STAGE_FRAGMENT = 1,
    SHADER_STAGE_GEOMETRY = 2,
    SHADER_STAGE_COMPUTE = 3,
    SHADER_STAGE_TESSELLATION_CONTROL = 4,
    SHADER_STAGE_TESSELLATION_EVALUATION = 5,
    SHADER_STAGE_COUNT = 6
} shader_stage_e;

typedef enum shader_language_e {
    SHADER_LANGUAGE_GLSL = 0,
    SHADER_LANGUAGE_HLSL = 1,
    SHADER_LANGUAGE_SPIRV = 2,
    SHADER_LANGUAGE_MSL = 3,  // Metal Shading Language
    SHADER_LANGUAGE_COUNT = 4
} shader_language_e;

typedef enum shader_optimization_level_e {
    SHADER_OPTIMIZATION_NONE = 0,
    SHADER_OPTIMIZATION_BASIC = 1,
    SHADER_OPTIMIZATION_FULL = 2,
    SHADER_OPTIMIZATION_AGGRESSIVE = 3
} shader_optimization_level_e;

typedef struct shader_source_t {
    char* code;
    size_t length;
    shader_stage_e stage;
    shader_language_e language;
    char entry_point[64];
    char file_path[256];
} shader_source_t;

typedef struct shader_compilation_options_t {
    shader_language_e target_language;
    shader_optimization_level_e optimization_level;
    bool debug_info;
    bool validate;
    bool generate_reflection;
    bool strip_debug;
    char include_paths[8][256];
    uint32_t include_path_count;
    char defines[32][128];
    uint32_t define_count;
} shader_compilation_options_t;

typedef struct shader_compilation_result_t {
    bool success;
    char* compiled_code;
    size_t compiled_size;
    char* error_log;
    char* warning_log;
    uint32_t compilation_time_ms;
    shader_language_e output_language;
} shader_compilation_result_t;

typedef struct shader_reflection_info_t {
    uint32_t uniform_count;
    uint32_t attribute_count;
    uint32_t sampler_count;
    uint32_t image_count;
    uint32_t buffer_count;
    
    struct {
        char name[64];
        uint32_t binding;
        uint32_t set;
        uint32_t size;
        uint32_t offset;
        uint32_t type;
    } uniforms[64];
    
    struct {
        char name[64];
        uint32_t location;
        uint32_t format;
        uint32_t size;
    } attributes[16];
    
    struct {
        char name[64];
        uint32_t binding;
        uint32_t set;
        uint32_t type;
    } samplers[32];
} shader_reflection_info_t;

typedef struct shader_compiler_desc_t {
    uint32_t flags;
    void* user_data;
} shader_compiler_desc_t;

typedef struct shader_compiler_info_t {
    uint32_t id;
    uint32_t flags;
    bool initialized;
    char supported_languages[16];
    uint32_t language_count;
} shader_compiler_info_t;

// Default compilation options
extern const shader_compilation_options_t SHADER_COMPILATION_OPTIONS_DEFAULT;

// ============================================================================
// COMPILER LIFECYCLE
// ============================================================================

// Initialize shader compiler system
bool shader_compiler_init(void);

// Shutdown shader compiler system
void shader_compiler_shutdown(void);

// Create shader compiler instance
shader_compiler_handle_t shader_compiler_create(const shader_compiler_desc_t* desc);

// Destroy shader compiler instance
void shader_compiler_destroy(shader_compiler_handle_t handle);

// Get compiler information
bool shader_compiler_get_info(shader_compiler_handle_t handle, shader_compiler_info_t* info);

// ============================================================================
// SHADER COMPILATION
// ============================================================================

// Compile single shader source
shader_compilation_result_t shader_compiler_compile(shader_compiler_handle_t handle,
                                                   const shader_source_t* source,
                                                   const shader_compilation_options_t* options);

// Compile multiple shader stages (pipeline)
shader_compilation_result_t shader_compiler_compile_pipeline(shader_compiler_handle_t handle,
                                                           const shader_source_t* sources,
                                                           uint32_t source_count,
                                                           const shader_compilation_options_t* options);

// Compile from file
shader_compilation_result_t shader_compiler_compile_file(shader_compiler_handle_t handle,
                                                         const char* file_path,
                                                         shader_stage_e stage,
                                                         const shader_compilation_options_t* options);

// Compile from string
shader_compilation_result_t shader_compiler_compile_string(shader_compiler_handle_t handle,
                                                           const char* code,
                                                           shader_stage_e stage,
                                                           const shader_language_e language,
                                                           const shader_compilation_options_t* options);

// ============================================================================
// SHADER TRANSLATION
// ============================================================================

// Translate between shader languages
shader_compilation_result_t shader_compiler_translate(shader_compiler_handle_t handle,
                                                     const char* source_code,
                                                     shader_language_e source_language,
                                                     shader_language_e target_language,
                                                     shader_stage_e stage,
                                                     const shader_compilation_options_t* options);

// Cross-compile for different platforms
shader_compilation_result_t shader_compiler_cross_compile(shader_compiler_handle_t handle,
                                                          const shader_source_t* source,
                                                          const char* target_platform,
                                                          const shader_compilation_options_t* options);

// ============================================================================
// SHADER VALIDATION
// ============================================================================

// Validate shader syntax
bool shader_compiler_validate_syntax(shader_compiler_handle_t handle,
                                     const char* code,
                                     shader_language_e language,
                                     char* error_log,
                                     size_t error_log_size);

// Validate shader pipeline compatibility
bool shader_compiler_validate_pipeline(shader_compiler_handle_t handle,
                                        const shader_source_t* sources,
                                        uint32_t source_count,
                                        char* error_log,
                                        size_t error_log_size);

// Check shader requirements
typedef struct shader_requirements_t {
    bool requires_tessellation;
    bool requires_geometry;
    bool requires_compute;
    bool requires_fragment;
    uint32_t min_vulkan_version;
    uint32_t min_opengl_version;
    char required_extensions[16][64];
    uint32_t extension_count;
} shader_requirements_t;

bool shader_compiler_get_requirements(shader_compiler_handle_t handle,
                                       const shader_source_t* source,
                                       shader_requirements_t* requirements);

// ============================================================================
// SHADER REFLECTION
// ============================================================================

// Generate shader reflection information
bool shader_compiler_generate_reflection(shader_compiler_handle_t handle,
                                          const char* compiled_code,
                                          size_t code_size,
                                          shader_language_e language,
                                          shader_reflection_info_t* reflection);

// Parse shader reflection from SPIR-V
bool shader_compiler_parse_spirv_reflection(const uint32_t* spirv_data,
                                            uint32_t word_count,
                                            shader_reflection_info_t* reflection);

// Get uniform information by name
bool shader_compiler_get_uniform_info(const shader_reflection_info_t* reflection,
                                       const char* name,
                                       uint32_t* binding,
                                       uint32_t* set,
                                       uint32_t* size);

// ============================================================================
// SHADER OPTIMIZATION
// ============================================================================

// Optimize compiled shader
shader_compilation_result_t shader_compiler_optimize(shader_compiler_handle_t handle,
                                                     const char* shader_code,
                                                     size_t code_size,
                                                     shader_language_e language,
                                                     shader_optimization_level_e level);

// Perform dead code elimination
shader_compilation_result_t shader_compiler_eliminate_dead_code(shader_compiler_handle_t handle,
                                                               const char* shader_code,
                                                               size_t code_size,
                                                               shader_language_e language);

// Perform constant folding
shader_compilation_result_t shader_compiler_fold_constants(shader_compiler_handle_t handle,
                                                          const char* shader_code,
                                                          size_t code_size,
                                                          shader_language_e language);

// ============================================================================
// PREPROCESSING
// ============================================================================

// Preprocess shader source
bool shader_compiler_preprocess(shader_compiler_handle_t handle,
                                const char* source,
                                char* processed_output,
                                size_t output_size,
                                const shader_compilation_options_t* options);

// Add include path
void shader_compiler_add_include_path(shader_compilation_options_t* options,
                                     const char* path);

// Add define
void shader_compiler_add_define(shader_compilation_options_t* options,
                               const char* define,
                               const char* value);

// ============================================================================
#// HOT RELOAD AND CACHING
// ============================================================================

// Enable/disable shader caching
void shader_compiler_set_cache_enabled(shader_compiler_handle_t handle, bool enabled);

// Clear shader cache
void shader_compiler_clear_cache(shader_compiler_handle_t handle);

// Get cache statistics
typedef struct shader_cache_stats_t {
    uint32_t cache_hits;
    uint32_t cache_misses;
    uint32_t cache_size_bytes;
    uint32_t cached_shaders;
} shader_cache_stats_t;

bool shader_compiler_get_cache_stats(shader_compiler_handle_t handle, shader_cache_stats_t* stats);

// Check if shader needs recompilation
bool shader_compiler_needs_recompilation(shader_compiler_handle_t handle,
                                          const char* file_path,
                                          uint64_t timestamp);

// ============================================================================
// DIAGNOSTICS AND DEBUGGING
// ============================================================================

// Get last compilation error
const char* shader_compiler_get_last_error(shader_compiler_handle_t handle);

// Get last compilation warnings
const char* shader_compiler_get_last_warnings(shader_compiler_handle_t handle);

// Enable/disable verbose logging
void shader_compiler_set_verbose_logging(shader_compiler_handle_t handle, bool enabled);

// Generate compilation report
typedef struct shader_compilation_report_t {
    char shader_name[256];
    shader_stage_e stage;
    bool success;
    uint32_t compilation_time_ms;
    uint32_t optimization_time_ms;
    uint32_t instruction_count;
    uint32_t register_count;
    uint32_t max_stack_depth;
    char error_log[1024];
    char warning_log[1024];
} shader_compilation_report_t;

bool shader_compiler_generate_report(shader_compiler_handle_t handle,
                                      const shader_source_t* source,
                                      shader_compilation_report_t* report);

// ============================================================================
// UTILITIES
// ============================================================================

// Get supported shader languages
bool shader_compiler_get_supported_languages(shader_compiler_handle_t handle,
                                             char languages[16][32],
                                             uint32_t* language_count);

// Get shader stage name
const char* shader_compiler_get_stage_name(shader_stage_e stage);

// Get language name
const char* shader_compiler_get_language_name(shader_language_e language);

// Convert between shader languages
bool shader_compiler_can_translate(shader_compiler_handle_t handle,
                                    shader_language_e from,
                                    shader_language_e to);

// Free compilation result
void shader_compiler_free_result(shader_compilation_result_t* result);

// Free reflection info
void shader_compiler_free_reflection(shader_reflection_info_t* reflection);

#ifdef __cplusplus
}
#endif

#endif // MATERIALS_SHADER_COMPILER_H
