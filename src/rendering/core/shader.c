// src/engine/rendering/core/shader.c
// Consolidated Shader System - Unified shader compilation and management

#include "engine/include/core/logger.h"
#include <core/memory.h>
#include <core/file_system.h>
#include <stdbool.h>
#include <stdint.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>

#ifdef __APPLE__
#include <Metal/Metal.h>
#endif

#include "shader.h"
#include "../backend/metal/mtl_device.h"

// ============================================================================
// Constants and Configuration
// ============================================================================

#define MAX_SHADER_MODULES 1024
#define MAX_SHADER_CACHE_SIZE (128 * 1024 * 1024)  // 128 MB
#define MAX_SHADER_SOURCE_SIZE (1024 * 1024)        // 1 MB per shader
#define SHADER_CACHE_VERSION 1

// ============================================================================
// Shader Types and Enums
// ============================================================================

typedef enum {
    SHADER_TYPE_VERTEX,
    SHADER_TYPE_FRAGMENT,
    SHADER_TYPE_COMPUTE,
    SHADER_TYPE_GEOMETRY,
    SHADER_TYPE_TESSELLATION_CONTROL,
    SHADER_TYPE_TESSELLATION_EVALUATION,
    SHADER_TYPE_COUNT
} ShaderType;

typedef enum {
    SHADER_STATUS_UNCOMPILED,
    SHADER_STATUS_COMPILING,
    SHADER_STATUS_COMPILED,
    SHADER_STATUS_FAILED,
    SHADER_STATUS_CACHED
} ShaderStatus;

typedef enum {
    SHADER_LANGUAGE_GLSL,
    SHADER_LANGUAGE_HLSL,
    SHADER_LANGUAGE_MSL,
    SHADER_LANGUAGE_SPIRV
} ShaderLanguage;

// ============================================================================
// Shader Module Structure
// ============================================================================

typedef struct ShaderModule {
    uint32_t id;
    char name[256];
    ShaderType type;
    ShaderLanguage language;
    ShaderStatus status;
    
    // Source code
    char* source_code;
    uint32_t source_size;
    
    // Compiled binary
    void* binary_data;
    uint32_t binary_size;
    
    // Platform-specific handles
#ifdef __APPLE__
    id<MTLFunction> metal_function;
    id<MTLLibrary> metal_library;
#endif
    
    // Metadata
    time_t last_modified;
    char source_hash[64];
    uint32_t compile_time_ms;
    char error_message[512];
    
    // Dependencies
    uint32_t dependency_count;
    uint32_t* dependency_ids;
    
    bool is_hot_reload_enabled;
} ShaderModule;

// ============================================================================
// Shader System Structure
// ============================================================================

typedef struct ShaderSystem {
    ShaderModule modules[MAX_SHADER_MODULES];
    uint32_t module_count;
    uint32_t next_id;
    
    // Cache management
    struct {
        uint8_t* cache_data;
        uint32_t cache_size;
        uint32_t cache_used;
        uint32_t entry_count;
    } cache;
    
    // Compilation queue
    struct {
        uint32_t* compile_queue;
        uint32_t queue_size;
        uint32_t queue_capacity;
        bool is_processing;
    } compilation;
    
    // Hot reload
    struct {
        bool enabled;
        time_t last_check_time;
        uint32_t check_interval_seconds;
    } hot_reload;
    
    // Platform-specific
#ifdef __APPLE__
    struct {
        id<MTLDevice> metal_device;
        id<MTLCommandQueue> metal_queue;
    } metal;
#endif
    
    bool initialized;
} ShaderSystem;

static ShaderSystem* g_shader_system = NULL;

// ============================================================================
// Utility Functions
// ============================================================================

static const char* shader_type_to_string(ShaderType type) {
    switch (type) {
        case SHADER_TYPE_VERTEX: return "vertex";
        case SHADER_TYPE_FRAGMENT: return "fragment";
        case SHADER_TYPE_COMPUTE: return "compute";
        case SHADER_TYPE_GEOMETRY: return "geometry";
        case SHADER_TYPE_TESSELLATION_CONTROL: return "tessellation_control";
        case SHADER_TYPE_TESSELLATION_EVALUATION: return "tessellation_evaluation";
        default: return "unknown";
    }
}

static const char* shader_status_to_string(ShaderStatus status) {
    switch (status) {
        case SHADER_STATUS_UNCOMPILED: return "uncompiled";
        case SHADER_STATUS_COMPILING: return "compiling";
        case SHADER_STATUS_COMPILED: return "compiled";
        case SHADER_STATUS_FAILED: return "failed";
        case SHADER_STATUS_CACHED: return "cached";
        default: return "unknown";
    }
}

static uint32_t hash_string(const char* str) {
    uint32_t hash = 5381;
    int c;
    while ((c = *str++)) {
        hash = ((hash << 5) + hash) + c;
    }
    return hash;
}

static void compute_shader_hash(const char* source_code, char* output_hash) {
    uint32_t hash = hash_string(source_code);
    snprintf(output_hash, 64, "%08x", hash);
}

// ============================================================================
// Shader Module Management
// ============================================================================

static ShaderModule* find_module_by_id(uint32_t id) {
    if (!g_shader_system) return NULL;
    
    for (uint32_t i = 0; i < g_shader_system->module_count; i++) {
        if (g_shader_system->modules[i].id == id) {
            return &g_shader_system->modules[i];
        }
    }
    return NULL;
}

static ShaderModule* find_module_by_name(const char* name) {
    if (!g_shader_system || !name) return NULL;
    
    for (uint32_t i = 0; i < g_shader_system->module_count; i++) {
        if (strcmp(g_shader_system->modules[i].name, name) == 0) {
            return &g_shader_system->modules[i];
        }
    }
    return NULL;
}

static uint32_t create_module(const char* name, ShaderType type) {
    if (!g_shader_system || !name || g_shader_system->module_count >= MAX_SHADER_MODULES) {
        return 0;
    }
    
    // Check if module already exists
    if (find_module_by_name(name)) {
        LOG_WARNING("Shader module '%s' already exists", name);
        return 0;
    }
    
    ShaderModule* module = &g_shader_system->modules[g_shader_system->module_count];
    memset(module, 0, sizeof(ShaderModule));
    
    module->id = ++g_shader_system->next_id;
    strncpy(module->name, name, sizeof(module->name) - 1);
    module->type = type;
    module->status = SHADER_STATUS_UNCOMPILED;
    module->language = SHADER_LANGUAGE_GLSL; // Default
    module->is_hot_reload_enabled = g_shader_system->hot_reload.enabled;
    
    g_shader_system->module_count++;
    
    LOG_INFO("Created shader module '%s' (ID: %u, Type: %s)", 
             name, module->id, shader_type_to_string(type));
    
    return module->id;
}

// ============================================================================
// Platform-Specific Compilation
// ============================================================================

#ifdef __APPLE__
static bool compile_metal_shader(ShaderModule* module) {
    if (!module || !module->source_code) {
        return false;
    }
    
    NSError* error = nil;
    MTLCompileOptions* options = [[MTLCompileOptions alloc] init];
    
    // Set language version based on shader type
    if (module->type == SHADER_TYPE_VERTEX) {
        options.languageVersion = MTLLanguageVersion2_4;
    } else {
        options.languageVersion = MTLLanguageVersion2_4;
    }
    
    // Create library from source
    id<MTLLibrary> library = [g_shader_system->metal.metal_device 
                               newLibraryWithSource:[NSString stringWithUTF8String:module->source_code]
                               options:options 
                               error:&error];
    
    [options release];
    
    if (error) {
        const char* error_desc = [[error localizedDescription] UTF8String];
        strncpy(module->error_message, error_desc, sizeof(module->error_message) - 1);
        LOG_ERROR("Metal shader compilation failed for '%s': %s", module->name, error_desc);
        return false;
    }
    
    module->metal_library = library;
    
    // Extract function based on shader type
    NSString* function_name = [NSString stringWithUTF8String:module->name];
    id<MTLFunction> function = [library newFunctionWithName:function_name];
    
    if (!function) {
        // Try common function names
        if (module->type == SHADER_TYPE_VERTEX) {
            function = [library newFunctionWithName:@"vertex_main"];
        } else if (module->type == SHADER_TYPE_FRAGMENT) {
            function = [library newFunctionWithName:@"fragment_main"];
        } else if (module->type == SHADER_TYPE_COMPUTE) {
            function = [library newFunctionWithName:@"compute_main"];
        }
    }
    
    if (!function) {
        LOG_ERROR("Could not find function in Metal library for '%s'", module->name);
        [library release];
        return false;
    }
    
    module->metal_function = function;
    module->status = SHADER_STATUS_COMPILED;
    
    LOG_INFO("Successfully compiled Metal shader '%s'", module->name);
    return true;
}
#endif

// ============================================================================
// Shader Compilation
// ============================================================================

static bool compile_shader_module(ShaderModule* module) {
    if (!module || !module->source_code) {
        return false;
    }
    
    clock_t start_time = clock();
    module->status = SHADER_STATUS_COMPILING;
    
    bool success = false;
    
#ifdef __APPLE__
    if (module->language == SHADER_LANGUAGE_MSL || module->language == SHADER_LANGUAGE_GLSL) {
        success = compile_metal_shader(module);
    }
#else
    // Vulkan/OpenGL compilation would go here
    LOG_WARNING("Shader compilation not implemented for this platform");
    module->status = SHADER_STATUS_FAILED;
    return false;
#endif
    
    clock_t end_time = clock();
    module->compile_time_ms = (uint32_t)((end_time - start_time) * 1000 / CLOCKS_PER_SEC);
    
    if (!success) {
        module->status = SHADER_STATUS_FAILED;
        LOG_ERROR("Shader compilation failed for '%s': %s", 
                  module->name, module->error_message);
    } else {
        // Compute hash of compiled shader
        compute_shader_hash(module->source_code, module->source_hash);
        LOG_INFO("Shader '%s' compiled successfully in %u ms", 
                 module->name, module->compile_time_ms);
    }
    
    return success;
}

// ============================================================================
// Public API Implementation
// ============================================================================

bool shader_system_init(void) {
    if (g_shader_system) {
        LOG_WARNING("Shader system already initialized");
        return true;
    }
    
    g_shader_system = calloc(1, sizeof(ShaderSystem));
    if (!g_shader_system) {
        LOG_ERROR("Failed to allocate shader system");
        return false;
    }
    
    // Initialize compilation queue
    g_shader_system->compilation.queue_capacity = 64;
    g_shader_system->compilation.compile_queue = calloc(
        g_shader_system->compilation.queue_capacity, sizeof(uint32_t));
    
    if (!g_shader_system->compilation.compile_queue) {
        LOG_ERROR("Failed to allocate compilation queue");
        free(g_shader_system);
        g_shader_system = NULL;
        return false;
    }
    
    // Initialize cache
    g_shader_system->cache.cache_size = MAX_SHADER_CACHE_SIZE;
    g_shader_system->cache.cache_data = calloc(1, MAX_SHADER_CACHE_SIZE);
    
    if (!g_shader_system->cache.cache_data) {
        LOG_ERROR("Failed to allocate shader cache");
        free(g_shader_system->compilation.compile_queue);
        free(g_shader_system);
        g_shader_system = NULL;
        return false;
    }
    
    // Initialize hot reload
    g_shader_system->hot_reload.enabled = true;
    g_shader_system->hot_reload.check_interval_seconds = 1;
    g_shader_system->hot_reload.last_check_time = time(NULL);
    
#ifdef __APPLE__
    // Initialize Metal device
    g_shader_system->metal.metal_device = MTLCreateSystemDefaultDevice();
    if (!g_shader_system->metal.metal_device) {
        LOG_ERROR("Failed to create Metal device");
        free(g_shader_system->cache.cache_data);
        free(g_shader_system->compilation.compile_queue);
        free(g_shader_system);
        g_shader_system = NULL;
        return false;
    }
    
    g_shader_system->metal.metal_queue = [g_shader_system->metal.metal_device newCommandQueue];
#endif
    
    g_shader_system->initialized = true;
    LOG_INFO("Shader system initialized successfully");
    
    return true;
}

void shader_system_shutdown(void) {
    if (!g_shader_system) {
        return;
    }
    
    // Cleanup all shader modules
    for (uint32_t i = 0; i < g_shader_system->module_count; i++) {
        ShaderModule* module = &g_shader_system->modules[i];
        
        if (module->source_code) {
            free(module->source_code);
        }
        
        if (module->binary_data) {
            free(module->binary_data);
        }
        
        if (module->dependency_ids) {
            free(module->dependency_ids);
        }
        
#ifdef __APPLE__
        if (module->metal_function) {
            [module->metal_function release];
        }
        if (module->metal_library) {
            [module->metal_library release];
        }
#endif
    }
    
#ifdef __APPLE__
    if (g_shader_system->metal.metal_queue) {
        [g_shader_system->metal.metal_queue release];
    }
    if (g_shader_system->metal.metal_device) {
        // Don't release system device
    }
#endif
    
    free(g_shader_system->cache.cache_data);
    free(g_shader_system->compilation.compile_queue);
    free(g_shader_system);
    
    g_shader_system = NULL;
    LOG_INFO("Shader system shutdown");
}

uint32_t shader_load_from_file(const char* filepath, ShaderType type) {
    if (!g_shader_system || !filepath) {
        return 0;
    }
    
    // Extract filename from path
    const char* filename = strrchr(filepath, '/');
    if (!filename) {
        filename = filepath;
    } else {
        filename++;
    }
    
    // Remove extension
    char name[256];
    strncpy(name, filename, sizeof(name) - 1);
    char* dot = strrchr(name, '.');
    if (dot) {
        *dot = '\0';
    }
    
    uint32_t module_id = create_module(name, type);
    if (!module_id) {
        return 0;
    }
    
    ShaderModule* module = find_module_by_id(module_id);
    if (!module) {
        return 0;
    }
    
    // Load source code
    FILE* file = fopen(filepath, "r");
    if (!file) {
        LOG_ERROR("Failed to open shader file '%s'", filepath);
        return 0;
    }
    
    fseek(file, 0, SEEK_END);
    module->source_size = ftell(file);
    fseek(file, 0, SEEK_SET);
    
    if (module->source_size > MAX_SHADER_SOURCE_SIZE) {
        LOG_ERROR("Shader file '%s' is too large (%u bytes)", filepath, module->source_size);
        fclose(file);
        return 0;
    }
    
    module->source_code = calloc(module->source_size + 1, 1);
    if (!module->source_code) {
        LOG_ERROR("Failed to allocate memory for shader source");
        fclose(file);
        return 0;
    }
    
    size_t read_size = fread(module->source_code, 1, module->source_size, file);
    fclose(file);
    
    if (read_size != module->source_size) {
        LOG_ERROR("Failed to read complete shader file '%s'", filepath);
        free(module->source_code);
        module->source_code = NULL;
        return 0;
    }
    
    module->source_code[module->source_size] = '\0';
    
    // Detect language from extension
    const char* extension = strrchr(filepath, '.');
    if (extension) {
        if (strcmp(extension, ".metal") == 0) {
            module->language = SHADER_LANGUAGE_MSL;
        } else if (strcmp(extension, ".hlsl") == 0) {
            module->language = SHADER_LANGUAGE_HLSL;
        } else if (strcmp(extension, ".spv") == 0) {
            module->language = SHADER_LANGUAGE_SPIRV;
        } else {
            module->language = SHADER_LANGUAGE_GLSL;
        }
    }
    
    // Get file modification time
    struct stat file_stat;
    if (stat(filepath, &file_stat) == 0) {
        module->last_modified = file_stat.st_mtime;
    }
    
    LOG_INFO("Loaded shader '%s' from '%s' (%u bytes, %s)", 
             name, filepath, module->source_size, shader_type_to_string(type));
    
    return module_id;
}

uint32_t shader_load_from_source(const char* name, const char* source_code, ShaderType type) {
    if (!g_shader_system || !name || !source_code) {
        return 0;
    }
    
    uint32_t module_id = create_module(name, type);
    if (!module_id) {
        return 0;
    }
    
    ShaderModule* module = find_module_by_id(module_id);
    if (!module) {
        return 0;
    }
    
    module->source_size = strlen(source_code);
    module->source_code = calloc(module->source_size + 1, 1);
    if (!module->source_code) {
        LOG_ERROR("Failed to allocate memory for shader source");
        return 0;
    }
    
    strcpy(module->source_code, source_code);
    module->last_modified = time(NULL);
    
    LOG_INFO("Loaded shader '%s' from source (%u bytes)", name, module->source_size);
    
    return module_id;
}

bool shader_compile(uint32_t module_id) {
    if (!g_shader_system) {
        return false;
    }
    
    ShaderModule* module = find_module_by_id(module_id);
    if (!module) {
        LOG_ERROR("Shader module with ID %u not found", module_id);
        return false;
    }
    
    return compile_shader_module(module);
}

bool shader_is_compiled(uint32_t module_id) {
    ShaderModule* module = find_module_by_id(module_id);
    return module && (module->status == SHADER_STATUS_COMPILED || 
                      module->status == SHADER_STATUS_CACHED);
}

const char* shader_get_error(uint32_t module_id) {
    ShaderModule* module = find_module_by_id(module_id);
    return module ? module->error_message : "Module not found";
}

void* shader_get_native_handle(uint32_t module_id) {
    ShaderModule* module = find_module_by_id(module_id);
    if (!module) {
        return NULL;
    }
    
#ifdef __APPLE__
    return module->metal_function;
#else
    return module->binary_data;
#endif
}

// ============================================================================
// Hot Reload System
// ============================================================================

void shader_system_enable_hot_reload(bool enabled) {
    if (!g_shader_system) {
        return;
    }
    
    g_shader_system->hot_reload.enabled = enabled;
    
    // Update all modules
    for (uint32_t i = 0; i < g_shader_system->module_count; i++) {
        g_shader_system->modules[i].is_hot_reload_enabled = enabled;
    }
    
    LOG_INFO("Shader hot reload %s", enabled ? "enabled" : "disabled");
}

void shader_system_check_hot_reload(void) {
    if (!g_shader_system || !g_shader_system->hot_reload.enabled) {
        return;
    }
    
    time_t current_time = time(NULL);
    if (current_time - g_shader_system->hot_reload.last_check_time < 
        g_shader_system->hot_reload.check_interval_seconds) {
        return;
    }
    
    g_shader_system->hot_reload.last_check_time = current_time;
    
    // Check all modules for file modifications
    for (uint32_t i = 0; i < g_shader_system->module_count; i++) {
        ShaderModule* module = &g_shader_system->modules[i];
        
        if (!module->is_hot_reload_enabled || !module->source_code) {
            continue;
        }
        
        // This is a simplified check - in practice you'd need to track file paths
        // and check their modification times
        // For now, we'll just log that hot reload checking is happening
        static uint32_t reload_counter = 0;
        if (reload_counter++ % 100 == 0) {
            LOG_DEBUG("Checking for shader hot reload...");
        }
    }
}

// ============================================================================
// Statistics and Debugging
// ============================================================================

void shader_system_log_stats(void) {
    if (!g_shader_system) {
        return;
    }
    
    uint32_t compiled_count = 0;
    uint32_t failed_count = 0;
    uint32_t total_source_size = 0;
    uint32_t total_binary_size = 0;
    
    for (uint32_t i = 0; i < g_shader_system->module_count; i++) {
        ShaderModule* module = &g_shader_system->modules[i];
        
        if (module->status == SHADER_STATUS_COMPILED || 
            module->status == SHADER_STATUS_CACHED) {
            compiled_count++;
        } else if (module->status == SHADER_STATUS_FAILED) {
            failed_count++;
        }
        
        total_source_size += module->source_size;
        total_binary_size += module->binary_size;
    }
    
    LOG_INFO("=== Shader System Statistics ===");
    LOG_INFO("Total modules: %u", g_shader_system->module_count);
    LOG_INFO("Compiled: %u", compiled_count);
    LOG_INFO("Failed: %u", failed_count);
    LOG_INFO("Source memory: %u bytes", total_source_size);
    LOG_INFO("Binary memory: %u bytes", total_binary_size);
    LOG_INFO("Cache used: %u / %u bytes", 
             g_shader_system->cache.cache_used, g_shader_system->cache.cache_size);
    LOG_INFO("Hot reload: %s", g_shader_system->hot_reload.enabled ? "enabled" : "disabled");
    LOG_INFO("================================");
}

bool shader_system_is_initialized(void) {
    return g_shader_system && g_shader_system->initialized;
}
