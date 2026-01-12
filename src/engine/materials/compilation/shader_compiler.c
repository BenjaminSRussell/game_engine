#include "materials/compilation/shader_compiler.h"
#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <time.h>

// ============================================================================
// INTERNAL STRUCTURES
// ============================================================================

#define SHADER_COMPILER_MAX_INSTANCES 8
#define SHADER_COMPILER_MAX_CACHE_SIZE 1024
#define SHADER_COMPILER_MAX_ERROR_LOG_SIZE 4096
#define SHADER_COMPILER_MAX_INCLUDE_DEPTH 16

typedef struct shader_cache_entry_t {
    char key[256];
    char* compiled_code;
    size_t compiled_size;
    uint64_t timestamp;
    uint32_t hash;
    bool is_valid;
} shader_cache_entry_t;

typedef struct shader_compiler_state_t {
    shader_compiler_handle_t handle;
    shader_compiler_desc_t desc;
    shader_compiler_info_t info;
    
    // Cache system
    shader_cache_entry_t cache[SHADER_COMPILER_MAX_CACHE_SIZE];
    uint32_t cache_count;
    bool cache_enabled;
    shader_cache_stats_t cache_stats;
    
    // Error handling
    char last_error[SHADER_COMPILER_MAX_ERROR_LOG_SIZE];
    char last_warnings[SHADER_COMPILER_MAX_ERROR_LOG_SIZE];
    bool verbose_logging;
    
    // Supported languages
    shader_language_e supported_languages[SHADER_LANGUAGE_COUNT];
    uint32_t supported_language_count;
    
    // Compilation state
    bool in_compilation;
    uint32_t current_compilation_time;
} shader_compiler_state_t;

static shader_compiler_state_t g_compilers[SHADER_COMPILER_MAX_INSTANCES];
static uint32_t g_compiler_count = 0;
static uint32_t g_next_compiler_id = 1;
static bool g_compiler_system_initialized = false;

// ============================================================================
// DEFAULT OPTIONS
// ============================================================================

const shader_compilation_options_t SHADER_COMPILATION_OPTIONS_DEFAULT = {
    .target_language = SHADER_LANGUAGE_SPIRV,
    .optimization_level = SHADER_OPTIMIZATION_FULL,
    .debug_info = true,
    .validate = true,
    .generate_reflection = true,
    .strip_debug = false,
    .include_path_count = 0,
    .define_count = 0
};

// ============================================================================
// UTILITY FUNCTIONS
// ============================================================================

static shader_compiler_state_t* find_compiler_by_handle(shader_compiler_handle_t handle) {
    for (uint32_t i = 0; i < g_compiler_count; i++) {
        if (g_compilers[i].handle.id == handle.id) {
            return &g_compilers[i];
        }
    }
    return NULL;
}

static shader_compiler_state_t* create_compiler_state(void) {
    if (g_compiler_count >= SHADER_COMPILER_MAX_INSTANCES) {
        return NULL;
    }
    
    shader_compiler_state_t* compiler = &g_compilers[g_compiler_count];
    memset(compiler, 0, sizeof(shader_compiler_state_t));
    
    compiler->handle.id = g_next_compiler_id++;
    compiler->info.id = compiler->handle.id;
    compiler->info.initialized = true;
    compiler->cache_enabled = true;
    compiler->verbose_logging = false;
    
    // Set supported languages (default to common ones)
    compiler->supported_languages[0] = SHADER_LANGUAGE_GLSL;
    compiler->supported_languages[1] = SHADER_LANGUAGE_SPIRV;
    compiler->supported_language_count = 2;
    
    // Initialize language names
    strcpy(compiler->info.supported_languages[0], "GLSL");
    strcpy(compiler->info.supported_languages[1], "SPIRV");
    compiler->info.language_count = 2;
    
    g_compiler_count++;
    return compiler;
}

static void destroy_compiler_state(shader_compiler_state_t* compiler) {
    if (!compiler) return;
    
    // Free cache entries
    for (uint32_t i = 0; i < compiler->cache_count; i++) {
        free(compiler->cache[i].compiled_code);
    }
    
    memset(compiler, 0, sizeof(shader_compiler_state_t));
}

static uint32_t hash_string(const char* str) {
    uint32_t hash = 5381;
    int c;
    while ((c = *str++)) {
        hash = ((hash << 5) + hash) + c;
    }
    return hash;
}

static uint64_t get_file_timestamp(const char* file_path) {
    // TODO: Implement actual file timestamp checking
    // For now, return current time
    return (uint64_t)time(NULL);
}

static bool generate_cache_key(const shader_source_t* source, 
                              const shader_compilation_options_t* options,
                              char* key, size_t key_size) {
    if (!source || !key || key_size < 256) return false;
    
    snprintf(key, key_size, "%s_%u_%u_%u_%lu", 
             source->file_path[0] ? source->file_path : "string",
             (uint32_t)source->stage, 
             (uint32_t)source->language,
             (uint32_t)options->optimization_level,
             (unsigned long)hash_string(source->code));
    
    return true;
}

static shader_cache_entry_t* find_cache_entry(shader_compiler_state_t* compiler, const char* key) {
    if (!compiler || !key) return NULL;
    
    for (uint32_t i = 0; i < compiler->cache_count; i++) {
        if (strcmp(compiler->cache[i].key, key) == 0) {
            return &compiler->cache[i];
        }
    }
    return NULL;
}

static bool add_cache_entry(shader_compiler_state_t* compiler, 
                           const char* key,
                           const char* compiled_code,
                           size_t compiled_size) {
    if (!compiler || !key || !compiled_code || compiler->cache_count >= SHADER_COMPILER_MAX_CACHE_SIZE) {
        return false;
    }
    
    shader_cache_entry_t* entry = &compiler->cache[compiler->cache_count];
    strncpy(entry->key, key, sizeof(entry->key) - 1);
    
    entry->compiled_code = malloc(compiled_size);
    if (!entry->compiled_code) return false;
    
    memcpy(entry->compiled_code, compiled_code, compiled_size);
    entry->compiled_size = compiled_size;
    entry->timestamp = (uint64_t)time(NULL);
    entry->hash = hash_string(compiled_code);
    entry->is_valid = true;
    
    compiler->cache_count++;
    compiler->cache_stats.cache_size_bytes += compiled_size;
    compiler->cache_stats.cached_shaders++;
    
    return true;
}

static void set_compiler_error(shader_compiler_state_t* compiler, const char* error) {
    if (!compiler || !error) return;
    
    strncpy(compiler->last_error, error, sizeof(compiler->last_error) - 1);
    compiler->last_error[sizeof(compiler->last_error) - 1] = '\0';
    
    if (compiler->verbose_logging) {
        printf("Shader Compiler Error: %s\n", error);
    }
}

static void set_compiler_warnings(shader_compiler_state_t* compiler, const char* warnings) {
    if (!compiler || !warnings) return;
    
    strncpy(compiler->last_warnings, warnings, sizeof(compiler->last_warnings) - 1);
    compiler->last_warnings[sizeof(compiler->last_warnings) - 1] = '\0';
    
    if (compiler->verbose_logging) {
        printf("Shader Compiler Warnings: %s\n", warnings);
    }
}

// ============================================================================
// COMPILER LIFECYCLE
// ============================================================================

bool shader_compiler_init(void) {
    if (g_compiler_system_initialized) {
        return true; // Already initialized
    }
    
    memset(g_compilers, 0, sizeof(g_compilers));
    g_compiler_count = 0;
    g_next_compiler_id = 1;
    g_compiler_system_initialized = true;
    
    printf("Shader compiler system initialized\n");
    return true;
}

void shader_compiler_shutdown(void) {
    if (!g_compiler_system_initialized) return;
    
    // Destroy all compilers
    for (uint32_t i = 0; i < g_compiler_count; i++) {
        destroy_compiler_state(&g_compilers[i]);
    }
    
    g_compiler_count = 0;
    g_compiler_system_initialized = false;
    
    printf("Shader compiler system shutdown\n");
}

shader_compiler_handle_t shader_compiler_create(const shader_compiler_desc_t* desc) {
    if (!g_compiler_system_initialized || !desc) {
        return (shader_compiler_handle_t){0};
    }
    
    shader_compiler_state_t* compiler = create_compiler_state();
    if (!compiler) {
        return (shader_compiler_handle_t){0};
    }
    
    compiler->desc = *desc;
    compiler->info.flags = desc->flags;
    
    printf("Created shader compiler instance %u\n", compiler->handle.id);
    return compiler->handle;
}

void shader_compiler_destroy(shader_compiler_handle_t handle) {
    if (!g_compiler_system_initialized) return;
    
    shader_compiler_state_t* compiler = find_compiler_by_handle(handle);
    if (!compiler) return;
    
    // Remove from array by shifting remaining elements
    for (uint32_t i = 0; i < g_compiler_count; i++) {
        if (g_compilers[i].handle.id == handle.id) {
            destroy_compiler_state(&g_compilers[i]);
            
            // Shift remaining compilers
            for (uint32_t j = i; j < g_compiler_count - 1; j++) {
                g_compilers[j] = g_compilers[j + 1];
            }
            g_compiler_count--;
            break;
        }
    }
    
    printf("Destroyed shader compiler instance %u\n", handle.id);
}

bool shader_compiler_get_info(shader_compiler_handle_t handle, shader_compiler_info_t* info) {
    if (!info) return false;
    
    shader_compiler_state_t* compiler = find_compiler_by_handle(handle);
    if (!compiler) return false;
    
    *info = compiler->info;
    return true;
}

// ============================================================================
// SHADER COMPILATION
// ============================================================================

shader_compilation_result_t shader_compiler_compile(shader_compiler_handle_t handle,
                                                   const shader_source_t* source,
                                                   const shader_compilation_options_t* options) {
    shader_compilation_result_t result = {0};
    
    if (!source) {
        set_compiler_error(find_compiler_by_handle(handle), "Invalid shader source");
        return result;
    }
    
    shader_compiler_state_t* compiler = find_compiler_by_handle(handle);
    if (!compiler) {
        set_compiler_error(NULL, "Invalid compiler handle");
        return result;
    }
    
    const shader_compilation_options_t* opts = options ? options : &SHADER_COMPILATION_OPTIONS_DEFAULT;
    
    // Check cache first
    if (compiler->cache_enabled) {
        char cache_key[256];
        if (generate_cache_key(source, opts, cache_key, sizeof(cache_key))) {
            shader_cache_entry_t* cached = find_cache_entry(compiler, cache_key);
            if (cached && cached->is_valid) {
                result.success = true;
                result.compiled_code = malloc(cached->compiled_size);
                if (result.compiled_code) {
                    memcpy(result.compiled_code, cached->compiled_code, cached->compiled_size);
                    result.compiled_size = cached->compiled_size;
                    result.output_language = opts->target_language;
                    compiler->cache_stats.cache_hits++;
                    
                    if (compiler->verbose_logging) {
                        printf("Shader cache hit: %s\n", cache_key);
                    }
                    return result;
                }
            }
            compiler->cache_stats.cache_misses++;
        }
    }
    
    // Perform actual compilation (placeholder implementation)
    clock_t start_time = clock();
    
    if (compiler->verbose_logging) {
        printf("Compiling shader: %s (stage: %u, language: %u)\n", 
               source->file_path[0] ? source->file_path : "<string>",
               (uint32_t)source->stage, (uint32_t)source->language);
    }
    
    // TODO: Implement actual shader compilation using glslang, SPIRV-Cross, etc.
    // For now, just copy the source as "compiled" output
    result.compiled_code = malloc(source->length + 1);
    if (result.compiled_code) {
        memcpy(result.compiled_code, source->code, source->length);
        result.compiled_code[source->length] = '\0';
        result.compiled_size = source->length;
        result.success = true;
        result.output_language = opts->target_language;
        
        // Add to cache
        if (compiler->cache_enabled) {
            char cache_key[256];
            if (generate_cache_key(source, opts, cache_key, sizeof(cache_key))) {
                add_cache_entry(compiler, cache_key, result.compiled_code, result.compiled_size);
            }
        }
    } else {
        set_compiler_error(compiler, "Failed to allocate memory for compiled shader");
        result.success = false;
    }
    
    clock_t end_time = clock();
    result.compilation_time_ms = (uint32_t)((end_time - start_time) * 1000 / CLOCKS_PER_SEC);
    compiler->current_compilation_time = result.compilation_time_ms;
    
    if (compiler->verbose_logging) {
        printf("Shader compilation %s in %u ms\n", 
               result.success ? "succeeded" : "failed", result.compilation_time_ms);
    }
    
    return result;
}

shader_compilation_result_t shader_compiler_compile_pipeline(shader_compiler_handle_t handle,
                                                           const shader_source_t* sources,
                                                           uint32_t source_count,
                                                           const shader_compilation_options_t* options) {
    shader_compilation_result_t result = {0};
    
    if (!sources || source_count == 0) {
        set_compiler_error(find_compiler_by_handle(handle), "Invalid shader sources");
        return result;
    }
    
    shader_compiler_state_t* compiler = find_compiler_by_handle(handle);
    if (!compiler) {
        set_compiler_error(NULL, "Invalid compiler handle");
        return result;
    }
    
    if (compiler->verbose_logging) {
        printf("Compiling shader pipeline with %u stages\n", source_count);
    }
    
    // Compile each stage
    bool all_success = true;
    size_t total_size = 0;
    
    for (uint32_t i = 0; i < source_count; i++) {
        shader_compilation_result_t stage_result = shader_compiler_compile(handle, &sources[i], options);
        
        if (!stage_result.success) {
            all_success = false;
            if (result.error_log) {
                free(result.error_log);
            }
            result.error_log = stage_result.error_log;
            stage_result.error_log = NULL;
        }
        
        total_size += stage_result.compiled_size;
        
        // Free intermediate results (we'll combine them later)
        shader_compiler_free_result(&stage_result);
    }
    
    result.success = all_success;
    result.compilation_time_ms = compiler->current_compilation_time;
    
    if (all_success) {
        // TODO: Combine all compiled stages into a single pipeline
        // For now, just indicate success
        result.compiled_code = malloc(1);
        if (result.compiled_code) {
            result.compiled_code[0] = '\0';
            result.compiled_size = 1;
        } else {
            result.success = false;
            set_compiler_error(compiler, "Failed to allocate memory for pipeline");
        }
    }
    
    return result;
}

shader_compilation_result_t shader_compiler_compile_file(shader_compiler_handle_t handle,
                                                         const char* file_path,
                                                         shader_stage_e stage,
                                                         const shader_compilation_options_t* options) {
    shader_compilation_result_t result = {0};
    
    if (!file_path) {
        set_compiler_error(find_compiler_by_handle(handle), "Invalid file path");
        return result;
    }
    
    // TODO: Load file content
    // For now, create a dummy source
    shader_source_t source = {0};
    source.stage = stage;
    source.language = SHADER_LANGUAGE_GLSL;
    strncpy(source.file_path, file_path, sizeof(source.file_path) - 1);
    strcpy(source.entry_point, "main");
    
    // Dummy shader code
    const char* dummy_code = "#version 450\nvoid main() { gl_Position = vec4(0.0); }";
    source.code = (char*)dummy_code;
    source.length = strlen(dummy_code);
    
    result = shader_compiler_compile(handle, &source, options);
    
    return result;
}

shader_compilation_result_t shader_compiler_compile_string(shader_compiler_handle_t handle,
                                                           const char* code,
                                                           shader_stage_e stage,
                                                           const shader_language_e language,
                                                           const shader_compilation_options_t* options) {
    shader_compilation_result_t result = {0};
    
    if (!code) {
        set_compiler_error(find_compiler_by_handle(handle), "Invalid shader code");
        return result;
    }
    
    shader_source_t source = {0};
    source.code = (char*)code;
    source.length = strlen(code);
    source.stage = stage;
    source.language = language;
    strcpy(source.entry_point, "main");
    
    result = shader_compiler_compile(handle, &source, options);
    
    return result;
}

// ============================================================================
// SHADER TRANSLATION
// ============================================================================

shader_compilation_result_t shader_compiler_translate(shader_compiler_handle_t handle,
                                                     const char* source_code,
                                                     shader_language_e source_language,
                                                     shader_language_e target_language,
                                                     shader_stage_e stage,
                                                     const shader_compilation_options_t* options) {
    shader_compilation_result_t result = {0};
    
    if (!source_code) {
        set_compiler_error(find_compiler_by_handle(handle), "Invalid source code");
        return result;
    }
    
    shader_compiler_state_t* compiler = find_compiler_by_handle(handle);
    if (!compiler) {
        set_compiler_error(NULL, "Invalid compiler handle");
        return result;
    }
    
    if (compiler->verbose_logging) {
        printf("Translating shader from %s to %s\n", 
               shader_compiler_get_language_name(source_language),
               shader_compiler_get_language_name(target_language));
    }
    
    // TODO: Implement actual translation using SPIRV-Cross or similar
    // For now, just copy the input
    result.compiled_code = malloc(strlen(source_code) + 1);
    if (result.compiled_code) {
        strcpy(result.compiled_code, source_code);
        result.compiled_size = strlen(source_code);
        result.success = true;
        result.output_language = target_language;
    } else {
        set_compiler_error(compiler, "Failed to allocate memory for translation");
        result.success = false;
    }
    
    return result;
}

shader_compilation_result_t shader_compiler_cross_compile(shader_compiler_handle_t handle,
                                                          const shader_source_t* source,
                                                          const char* target_platform,
                                                          const shader_compilation_options_t* options) {
    shader_compilation_result_t result = {0};
    
    if (!source || !target_platform) {
        set_compiler_error(find_compiler_by_handle(handle), "Invalid source or target platform");
        return result;
    }
    
    shader_compiler_state_t* compiler = find_compiler_by_handle(handle);
    if (!compiler) {
        set_compiler_error(NULL, "Invalid compiler handle");
        return result;
    }
    
    if (compiler->verbose_logging) {
        printf("Cross-compiling shader for platform: %s\n", target_platform);
    }
    
    // TODO: Implement platform-specific cross-compilation
    // For now, just do basic compilation
    result = shader_compiler_compile(handle, source, options);
    
    return result;
}

// ============================================================================
// SHADER VALIDATION
// ============================================================================

bool shader_compiler_validate_syntax(shader_compiler_handle_t handle,
                                     const char* code,
                                     shader_language_e language,
                                     char* error_log,
                                     size_t error_log_size) {
    if (!code || !error_log) return false;
    
    shader_compiler_state_t* compiler = find_compiler_by_handle(handle);
    if (!compiler) return false;
    
    if (compiler->verbose_logging) {
        printf("Validating %s syntax\n", shader_compiler_get_language_name(language));
    }
    
    // TODO: Implement actual syntax validation
    // For now, just do basic checks
    if (strlen(code) == 0) {
        snprintf(error_log, error_log_size, "Empty shader code");
        return false;
    }
    
    // Check for basic shader structure
    if (strstr(code, "main") == NULL) {
        snprintf(error_log, error_log_size, "Missing main function");
        return false;
    }
    
    error_log[0] = '\0';
    return true;
}

bool shader_compiler_validate_pipeline(shader_compiler_handle_t handle,
                                        const shader_source_t* sources,
                                        uint32_t source_count,
                                        char* error_log,
                                        size_t error_log_size) {
    if (!sources || source_count == 0 || !error_log) return false;
    
    shader_compiler_state_t* compiler = find_compiler_by_handle(handle);
    if (!compiler) return false;
    
    if (compiler->verbose_logging) {
        printf("Validating shader pipeline with %u stages\n", source_count);
    }
    
    // Check each stage individually
    for (uint32_t i = 0; i < source_count; i++) {
        char stage_error[1024];
        if (!shader_compiler_validate_syntax(handle, sources[i].code, sources[i].language, 
                                           stage_error, sizeof(stage_error))) {
            snprintf(error_log, error_log_size, "Stage %u validation failed: %s", i, stage_error);
            return false;
        }
    }
    
    // TODO: Check pipeline compatibility (matching inputs/outputs, etc.)
    
    error_log[0] = '\0';
    return true;
}

bool shader_compiler_get_requirements(shader_compiler_handle_t handle,
                                       const shader_source_t* source,
                                       shader_requirements_t* requirements) {
    if (!source || !requirements) return false;
    
    shader_compiler_state_t* compiler = find_compiler_by_handle(handle);
    if (!compiler) return false;
    
    memset(requirements, 0, sizeof(shader_requirements_t));
    
    // TODO: Parse shader to determine requirements
    // For now, set some basic defaults
    
    // Check for tessellation shaders
    if (source->stage == SHADER_STAGE_TESSELLATION_CONTROL || 
        source->stage == SHADER_STAGE_TESSELLATION_EVALUATION) {
        requirements->requires_tessellation = true;
    }
    
    // Check for geometry shader
    if (source->stage == SHADER_STAGE_GEOMETRY) {
        requirements->requires_geometry = true;
    }
    
    // Check for compute shader
    if (source->stage == SHADER_STAGE_COMPUTE) {
        requirements->requires_compute = true;
    }
    
    // Check for fragment shader
    if (source->stage == SHADER_STAGE_FRAGMENT) {
        requirements->requires_fragment = true;
    }
    
    // Set minimum version requirements
    requirements->min_vulkan_version = 0x400003; // Vulkan 1.0.3
    requirements->min_opengl_version = 0x030300; // OpenGL 3.3
    
    return true;
}

// ============================================================================
// SHADER REFLECTION
// ============================================================================

bool shader_compiler_generate_reflection(shader_compiler_handle_t handle,
                                          const char* compiled_code,
                                          size_t code_size,
                                          shader_language_e language,
                                          shader_reflection_info_t* reflection) {
    if (!compiled_code || !reflection) return false;
    
    shader_compiler_state_t* compiler = find_compiler_by_handle(handle);
    if (!compiler) return false;
    
    memset(reflection, 0, sizeof(shader_reflection_info_t));
    
    if (compiler->verbose_logging) {
        printf("Generating reflection for %s shader\n", shader_compiler_get_language_name(language));
    }
    
    // TODO: Implement actual reflection generation
    // For now, create dummy reflection data
    
    reflection->uniform_count = 1;
    strcpy(reflection->uniforms[0].name, "uniform_buffer");
    reflection->uniforms[0].binding = 0;
    reflection->uniforms[0].set = 0;
    reflection->uniforms[0].size = 64;
    reflection->uniforms[0].offset = 0;
    reflection->uniforms[0].type = 1;
    
    reflection->attribute_count = 1;
    strcpy(reflection->attributes[0].name, "position");
    reflection->attributes[0].location = 0;
    reflection->attributes[0].format = 44; // VK_FORMAT_R32G32B32_SFLOAT
    reflection->attributes[0].size = 12;
    
    return true;
}

bool shader_compiler_parse_spirv_reflection(const uint32_t* spirv_data,
                                            uint32_t word_count,
                                            shader_reflection_info_t* reflection) {
    if (!spirv_data || word_count == 0 || !reflection) return false;
    
    memset(reflection, 0, sizeof(shader_reflection_info_t));
    
    // TODO: Implement SPIR-V reflection parsing
    // For now, return dummy data
    reflection->uniform_count = 0;
    reflection->attribute_count = 0;
    reflection->sampler_count = 0;
    
    return true;
}

bool shader_compiler_get_uniform_info(const shader_reflection_info_t* reflection,
                                       const char* name,
                                       uint32_t* binding,
                                       uint32_t* set,
                                       uint32_t* size) {
    if (!reflection || !name) return false;
    
    for (uint32_t i = 0; i < reflection->uniform_count; i++) {
        if (strcmp(reflection->uniforms[i].name, name) == 0) {
            if (binding) *binding = reflection->uniforms[i].binding;
            if (set) *set = reflection->uniforms[i].set;
            if (size) *size = reflection->uniforms[i].size;
            return true;
        }
    }
    
    return false;
}

// ============================================================================
// SHADER OPTIMIZATION
// ============================================================================

shader_compilation_result_t shader_compiler_optimize(shader_compiler_handle_t handle,
                                                     const char* shader_code,
                                                     size_t code_size,
                                                     shader_language_e language,
                                                     shader_optimization_level_e level) {
    shader_compilation_result_t result = {0};
    
    if (!shader_code) {
        set_compiler_error(find_compiler_by_handle(handle), "Invalid shader code");
        return result;
    }
    
    shader_compiler_state_t* compiler = find_compiler_by_handle(handle);
    if (!compiler) {
        set_compiler_error(NULL, "Invalid compiler handle");
        return result;
    }
    
    if (compiler->verbose_logging) {
        printf("Optimizing shader at level %u\n", (uint32_t)level);
    }
    
    // TODO: Implement actual shader optimization
    // For now, just copy the input
    result.compiled_code = malloc(code_size);
    if (result.compiled_code) {
        memcpy(result.compiled_code, shader_code, code_size);
        result.compiled_size = code_size;
        result.success = true;
        result.output_language = language;
    } else {
        set_compiler_error(compiler, "Failed to allocate memory for optimization");
        result.success = false;
    }
    
    return result;
}

shader_compilation_result_t shader_compiler_eliminate_dead_code(shader_compiler_handle_t handle,
                                                               const char* shader_code,
                                                               size_t code_size,
                                                               shader_language_e language) {
    shader_compilation_result_t result = {0};
    
    if (!shader_code) {
        set_compiler_error(find_compiler_by_handle(handle), "Invalid shader code");
        return result;
    }
    
    shader_compiler_state_t* compiler = find_compiler_by_handle(handle);
    if (!compiler) {
        set_compiler_error(NULL, "Invalid compiler handle");
        return result;
    }
    
    if (compiler->verbose_logging) {
        printf("Eliminating dead code from shader\n");
    }
    
    // TODO: Implement actual dead code elimination
    // For now, just copy the input
    result.compiled_code = malloc(code_size);
    if (result.compiled_code) {
        memcpy(result.compiled_code, shader_code, code_size);
        result.compiled_size = code_size;
        result.success = true;
        result.output_language = language;
    } else {
        set_compiler_error(compiler, "Failed to allocate memory for dead code elimination");
        result.success = false;
    }
    
    return result;
}

shader_compilation_result_t shader_compiler_fold_constants(shader_compiler_handle_t handle,
                                                          const char* shader_code,
                                                          size_t code_size,
                                                          shader_language_e language) {
    shader_compilation_result_t result = {0};
    
    if (!shader_code) {
        set_compiler_error(find_compiler_by_handle(handle), "Invalid shader code");
        return result;
    }
    
    shader_compiler_state_t* compiler = find_compiler_by_handle(handle);
    if (!compiler) {
        set_compiler_error(NULL, "Invalid compiler handle");
        return result;
    }
    
    if (compiler->verbose_logging) {
        printf("Folding constants in shader\n");
    }
    
    // TODO: Implement actual constant folding
    // For now, just copy the input
    result.compiled_code = malloc(code_size);
    if (result.compiled_code) {
        memcpy(result.compiled_code, shader_code, code_size);
        result.compiled_size = code_size;
        result.success = true;
        result.output_language = language;
    } else {
        set_compiler_error(compiler, "Failed to allocate memory for constant folding");
        result.success = false;
    }
    
    return result;
}

// ============================================================================
// PREPROCESSING
// ============================================================================

bool shader_compiler_preprocess(shader_compiler_handle_t handle,
                                const char* source,
                                char* processed_output,
                                size_t output_size,
                                const shader_compilation_options_t* options) {
    if (!source || !processed_output) return false;
    
    shader_compiler_state_t* compiler = find_compiler_by_handle(handle);
    if (!compiler) return false;
    
    if (compiler->verbose_logging) {
        printf("Preprocessing shader source\n");
    }
    
    // TODO: Implement actual preprocessing (handle #include, #define, etc.)
    // For now, just copy the input
    strncpy(processed_output, source, output_size - 1);
    processed_output[output_size - 1] = '\0';
    
    return true;
}

void shader_compiler_add_include_path(shader_compilation_options_t* options, const char* path) {
    if (!options || !path || options->include_path_count >= 8) return;
    
    strncpy(options->include_paths[options->include_path_count], path, 255);
    options->include_paths[options->include_path_count][255] = '\0';
    options->include_path_count++;
}

void shader_compiler_add_define(shader_compilation_options_t* options, const char* define, const char* value) {
    if (!options || !define || options->define_count >= 32) return;
    
    if (value) {
        snprintf(options->defines[options->define_count], 127, "%s=%s", define, value);
    } else {
        strncpy(options->defines[options->define_count], define, 127);
    }
    options->defines[options->define_count][127] = '\0';
    options->define_count++;
}

// ============================================================================
// HOT RELOAD AND CACHING
// ============================================================================

void shader_compiler_set_cache_enabled(shader_compiler_handle_t handle, bool enabled) {
    shader_compiler_state_t* compiler = find_compiler_by_handle(handle);
    if (!compiler) return;
    
    compiler->cache_enabled = enabled;
    printf("Shader cache %s for compiler %u\n", enabled ? "enabled" : "disabled", handle.id);
}

void shader_compiler_clear_cache(shader_compiler_handle_t handle) {
    shader_compiler_state_t* compiler = find_compiler_by_handle(handle);
    if (!compiler) return;
    
    // Free all cache entries
    for (uint32_t i = 0; i < compiler->cache_count; i++) {
        free(compiler->cache[i].compiled_code);
    }
    
    compiler->cache_count = 0;
    memset(&compiler->cache_stats, 0, sizeof(compiler->cache_stats));
    
    printf("Cleared shader cache for compiler %u\n", handle.id);
}

bool shader_compiler_get_cache_stats(shader_compiler_handle_t handle, shader_cache_stats_t* stats) {
    if (!stats) return false;
    
    shader_compiler_state_t* compiler = find_compiler_by_handle(handle);
    if (!compiler) return false;
    
    *stats = compiler->cache_stats;
    return true;
}

bool shader_compiler_needs_recompilation(shader_compiler_handle_t handle,
                                          const char* file_path,
                                          uint64_t timestamp) {
    if (!file_path) return false;
    
    shader_compiler_state_t* compiler = find_compiler_by_handle(handle);
    if (!compiler) return false;
    
    uint64_t current_timestamp = get_file_timestamp(file_path);
    return current_timestamp > timestamp;
}

// ============================================================================
// DIAGNOSTICS AND DEBUGGING
// ============================================================================

const char* shader_compiler_get_last_error(shader_compiler_handle_t handle) {
    shader_compiler_state_t* compiler = find_compiler_by_handle(handle);
    if (!compiler) return "Invalid compiler handle";
    
    return compiler->last_error;
}

const char* shader_compiler_get_last_warnings(shader_compiler_handle_t handle) {
    shader_compiler_state_t* compiler = find_compiler_by_handle(handle);
    if (!compiler) return "Invalid compiler handle";
    
    return compiler->last_warnings;
}

void shader_compiler_set_verbose_logging(shader_compiler_handle_t handle, bool enabled) {
    shader_compiler_state_t* compiler = find_compiler_by_handle(handle);
    if (!compiler) return;
    
    compiler->verbose_logging = enabled;
    printf("Verbose logging %s for compiler %u\n", enabled ? "enabled" : "disabled", handle.id);
}

bool shader_compiler_generate_report(shader_compiler_handle_t handle,
                                      const shader_source_t* source,
                                      shader_compilation_report_t* report) {
    if (!source || !report) return false;
    
    shader_compiler_state_t* compiler = find_compiler_by_handle(handle);
    if (!compiler) return false;
    
    memset(report, 0, sizeof(shader_compilation_report_t));
    
    strncpy(report->shader_name, source->file_path[0] ? source->file_path : "<string>", 
            sizeof(report->shader_name) - 1);
    report->stage = source->stage;
    report->compilation_time_ms = compiler->current_compilation_time;
    
    // TODO: Generate actual report data
    report->success = true;
    report->instruction_count = 100; // Dummy value
    report->register_count = 32;     // Dummy value
    report->max_stack_depth = 16;    // Dummy value
    
    return true;
}

// ============================================================================
// UTILITIES
// ============================================================================

bool shader_compiler_get_supported_languages(shader_compiler_handle_t handle,
                                             char languages[16][32],
                                             uint32_t* language_count) {
    if (!languages || !language_count) return false;
    
    shader_compiler_state_t* compiler = find_compiler_by_handle(handle);
    if (!compiler) return false;
    
    *language_count = compiler->supported_language_count;
    for (uint32_t i = 0; i < compiler->supported_language_count; i++) {
        strcpy(languages[i], compiler->info.supported_languages[i]);
    }
    
    return true;
}

const char* shader_compiler_get_stage_name(shader_stage_e stage) {
    switch (stage) {
        case SHADER_STAGE_VERTEX: return "Vertex";
        case SHADER_STAGE_FRAGMENT: return "Fragment";
        case SHADER_STAGE_GEOMETRY: return "Geometry";
        case SHADER_STAGE_COMPUTE: return "Compute";
        case SHADER_STAGE_TESSELLATION_CONTROL: return "Tessellation Control";
        case SHADER_STAGE_TESSELLATION_EVALUATION: return "Tessellation Evaluation";
        default: return "Unknown";
    }
}

const char* shader_compiler_get_language_name(shader_language_e language) {
    switch (language) {
        case SHADER_LANGUAGE_GLSL: return "GLSL";
        case SHADER_LANGUAGE_HLSL: return "HLSL";
        case SHADER_LANGUAGE_SPIRV: return "SPIRV";
        case SHADER_LANGUAGE_MSL: return "MSL";
        default: return "Unknown";
    }
}

bool shader_compiler_can_translate(shader_compiler_handle_t handle,
                                    shader_language_e from,
                                    shader_language_e to) {
    shader_compiler_state_t* compiler = find_compiler_by_handle(handle);
    if (!compiler) return false;
    
    // TODO: Check if translation is supported
    // For now, assume all translations to/from SPIRV are supported
    return (from == SHADER_LANGUAGE_SPIRV || to == SHADER_LANGUAGE_SPIRV);
}

void shader_compiler_free_result(shader_compilation_result_t* result) {
    if (!result) return;
    
    free(result->compiled_code);
    free(result->error_log);
    free(result->warning_log);
    
    memset(result, 0, sizeof(shader_compilation_result_t));
}

void shader_compiler_free_reflection(shader_reflection_info_t* reflection) {
    if (!reflection) return;
    
    memset(reflection, 0, sizeof(shader_reflection_info_t));
}
