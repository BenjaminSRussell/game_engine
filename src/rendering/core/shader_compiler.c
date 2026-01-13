// src/engine/rendering/core/shader_compiler.c
// Consolidated Shader System with Compilation and Validation

#include <core/logger.h>
#include <core/memory.h>
#include <core/vfs/vfs.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>
#include <sys/stat.h>

#ifdef __APPLE__
#include <Metal/Metal.h>
#endif

// ============================================================================
// Shader System Types
// ============================================================================

typedef struct {
    id<MTLLibrary> library;
    char path[256];
    time_t last_modified;
    bool is_compiled;
    uint64_t compilation_time;
} metal_shader_library_t;

typedef struct {
    id<MTLFunction> function;
    char name[64];
    MTLFunctionType type;
    bool is_valid;
    uint64_t validation_time;
} metal_shader_t;

typedef struct {
    metal_shader_library_t* libraries;
    uint32_t library_count;
    uint32_t library_capacity;
    metal_device_t* device;
    
    // Compilation state
    bool compilation_enabled;
    uint64_t total_compilations;
    uint64_t successful_compilations;
    uint64_t failed_compilations;
    uint64_t validation_errors;
    uint64_t last_compilation_time;
    
    // Hot reload state
    bool hot_reload_enabled;
    uint64_t hot_reload_checks;
    uint64_t hot_reload_reloads;
} shader_system_t;

// ============================================================================
// Internal Functions
// ============================================================================

static time_t shader_get_file_modified_time(const char *path);
static bool shader_validate_source(const char *source);
static bool shader_validate_library(metal_shader_library_t *library);
static void shader_log_compilation_error(shader_system_t *sys, const char *error);
static uint64_t shader_get_timestamp(void);
static bool shader_validate_shader_function(metal_shader_t *shader);

// ============================================================================
// Shader System API
// ============================================================================

shader_system_t* shader_system_create(metal_device_t* device) {
    if (!device) {
        LOG_ERROR("Cannot create shader system: device is NULL");
        return NULL;
    }
    
    shader_system_t* sys = calloc(1, sizeof(shader_system_t));
    if (!sys) {
        LOG_ERROR("Failed to allocate shader system");
        return NULL;
    }
    
    sys->device = device;
    sys->compilation_enabled = true;
    sys->hot_reload_enabled = true;
    sys->total_compilations = 0;
    sys->successful_compilations = 0;
    sys->failed_compilations = 0;
    sys->validation_errors = 0;
    sys->last_compilation_time = shader_get_timestamp();
    sys->hot_reload_checks = 0;
    sys->hot_reload_reloads = 0;
    
    // Initialize library array
    sys->library_capacity = 16;
    sys->libraries = calloc(sys->library_capacity, sizeof(metal_shader_library_t));
    if (!sys->libraries) {
        LOG_ERROR("Failed to allocate shader library array");
        free(sys);
        return NULL;
    }
    
    LOG_INFO("Shader system created with compilation and validation enabled");
    return sys;
}

void shader_system_destroy(shader_system_t* sys) {
    if (!sys) return;
    
    // Report statistics before destruction
    if (sys->compilation_enabled) {
        LOG_INFO("Shader System Statistics:");
        LOG_INFO("  Total compilations: %lu", sys->total_compilations);
        LOG_INFO("  Successful compilations: %lu", sys->successful_compilations);
        LOG_INFO("  Failed compilations: %lu", sys->failed_compilations);
        LOG_INFO("  Validation errors: %lu", sys->validation_errors);
        LOG_INFO("  Hot reload checks: %lu", sys->hot_reload_checks);
        LOG_INFO("  Hot reloads: %lu", sys->hot_reload_reloads);
        
        if (sys->total_compilations > 0) {
            double success_rate = (double)sys->successful_compilations / sys->total_compilations * 100.0;
            LOG_INFO("  Compilation success rate: %.2f%%", success_rate);
        }
    }
    
    // Destroy all libraries
    for (uint32_t i = 0; i < sys->library_count; i++) {
        if (sys->libraries[i].library) {
            [sys->libraries[i].library release];
        }
    }
    
    free(sys->libraries);
    free(sys);
    LOG_INFO("Shader system destroyed");
}

metal_shader_library_t* shader_load_library(shader_system_t* sys, const char* path) {
    if (!sys || !path) {
        LOG_ERROR("Invalid parameters for shader library loading");
        return NULL;
    }
    
    if (!sys->compilation_enabled) {
        LOG_ERROR("Shader compilation is disabled");
        return NULL;
    }
    
    // Check if library already loaded
    for (uint32_t i = 0; i < sys->library_count; i++) {
        if (strcmp(sys->libraries[i].path, path) == 0) {
            LOG_INFO("Shader library already loaded: %s", path);
            return &sys->libraries[i];
        }
    }
    
    // Expand library array if needed
    if (sys->library_count >= sys->library_capacity) {
        sys->library_capacity *= 2;
        metal_shader_library_t* new_libraries = realloc(sys->libraries, 
            sys->library_capacity * sizeof(metal_shader_library_t));
        if (!new_libraries) {
            LOG_ERROR("Failed to expand shader library array");
            return NULL;
        }
        sys->libraries = new_libraries;
    }
    
    // Load shader source from file
    VFS* vfs = vfs_get_global_vfs();
    if (!vfs) {
        LOG_ERROR("VFS not available for shader loading");
        return NULL;
    }
    
    VFSFile* file = vfs_open(vfs, path, VFS_MODE_READ);
    if (!file) {
        LOG_ERROR("Failed to open shader file: %s", path);
        return NULL;
    }
    
    u64 file_size = vfs_size(file);
    char* source = malloc(file_size + 1);
    if (!source) {
        LOG_ERROR("Failed to allocate memory for shader source");
        vfs_close(file);
        return NULL;
    }
    
    u64 bytes_read = vfs_read(file, source, file_size);
    source[bytes_read] = '\0';
    vfs_close(file);
    
    if (bytes_read != file_size) {
        LOG_ERROR("Failed to read complete shader file: %s", path);
        free(source);
        return NULL;
    }
    
    // Validate shader source
    if (!shader_validate_source(source)) {
        LOG_ERROR("Invalid shader source: %s", path);
        free(source);
        return NULL;
    }
    
    // Compile shader library
    sys->total_compilations++;
    uint64_t compilation_start = shader_get_timestamp();
    
    NSError* error = nil;
    id<MTLLibrary> library = [sys->device->device newLibraryWithSource:[NSString stringWithUTF8String:source]
                                                                 options:nil
                                                                   error:&error];
    
    free(source);
    
    if (!library) {
        sys->failed_compilations++;
        LOG_ERROR("Failed to compile shader library %s: %s", path, 
                 [[error localizedDescription] UTF8String]);
        return NULL;
    }
    
    // Validate compiled library
    metal_shader_library_t* lib = &sys->libraries[sys->library_count];
    lib->library = library;
    strncpy(lib->path, path, sizeof(lib->path) - 1);
    lib->path[sizeof(lib->path) - 1] = '\0';
    lib->last_modified = shader_get_file_modified_time(path);
    lib->is_compiled = true;
    lib->compilation_time = shader_get_timestamp() - compilation_start;
    
    if (!shader_validate_library(lib)) {
        sys->validation_errors++;
        LOG_ERROR("Compiled library validation failed: %s", path);
        [library release];
        return NULL;
    }
    
    sys->successful_compilations++;
    sys->library_count++;
    
    LOG_INFO("Shader library compiled successfully: %s (%.2f ms)", 
             path, lib->compilation_time / 1000000.0);
    
    return lib;
}

metal_shader_t* shader_get_function(metal_shader_library_t* lib, const char* name) {
    if (!lib || !name || !lib->is_compiled) {
        return NULL;
    }
    
    id<MTLFunction> function = [lib->library newFunctionWithName:[NSString stringWithUTF8String:name]];
    if (!function) {
        LOG_ERROR("Failed to get shader function: %s", name);
        return NULL;
    }
    
    metal_shader_t* shader = malloc(sizeof(metal_shader_t));
    if (!shader) {
        [function release];
        return NULL;
    }
    
    shader->function = function;
    strncpy(shader->name, name, sizeof(shader->name) - 1);
    shader->name[sizeof(shader->name) - 1] = '\0';
    shader->type = [function functionType];
    shader->is_valid = false;
    shader->validation_time = 0;
    
    // Validate shader function
    if (shader_validate_shader_function(shader)) {
        shader->is_valid = true;
        LOG_DEBUG("Shader function validated: %s", name);
    } else {
        LOG_ERROR("Shader function validation failed: %s", name);
        [function release];
        free(shader);
        return NULL;
    }
    
    return shader;
}

metal_shader_t* shader_get_function_with_constants(metal_shader_library_t* lib, const char* name, MTLFunctionConstantValues* constants) {
    if (!lib || !name || !lib->is_compiled) {
        return NULL;
    }
    
    id<MTLFunction> function = [lib->library newFunctionWithName:[NSString stringWithUTF8String:name]
                                                        constantValues:constants
                                                               error:nil];
    if (!function) {
        LOG_ERROR("Failed to get shader function with constants: %s", name);
        return NULL;
    }
    
    metal_shader_t* shader = malloc(sizeof(metal_shader_t));
    if (!shader) {
        [function release];
        return NULL;
    }
    
    shader->function = function;
    strncpy(shader->name, name, sizeof(shader->name) - 1);
    shader->name[sizeof(shader->name) - 1] = '\0';
    shader->type = [function functionType];
    shader->is_valid = false;
    shader->validation_time = 0;
    
    // Validate shader function
    if (shader_validate_shader_function(shader)) {
        shader->is_valid = true;
        LOG_DEBUG("Shader function with constants validated: %s", name);
    } else {
        LOG_ERROR("Shader function with constants validation failed: %s", name);
        [function release];
        free(shader);
        return NULL;
    }
    
    return shader;
}

void shader_system_check_reload(shader_system_t* sys) {
    if (!sys || !sys->hot_reload_enabled) {
        return;
    }
    
    sys->hot_reload_checks++;
    
    for (uint32_t i = 0; i < sys->library_count; i++) {
        metal_shader_library_t* lib = &sys->libraries[i];
        time_t current_modified = shader_get_file_modified_time(lib->path);
        
        if (current_modified > lib->last_modified) {
            LOG_INFO("Detected shader change, reloading: %s", lib->path);
            
            // Release old library
            if (lib->library) {
                [lib->library release];
            }
            
            // Reload library
            metal_shader_library_t* new_lib = shader_load_library(sys, lib->path);
            if (new_lib) {
                // Copy new library state to existing slot
                lib->library = new_lib->library;
                lib->last_modified = new_lib->last_modified;
                lib->is_compiled = new_lib->is_compiled;
                lib->compilation_time = new_lib->compilation_time;
                
                sys->hot_reload_reloads++;
                LOG_INFO("Shader library reloaded successfully: %s", lib->path);
            } else {
                LOG_ERROR("Failed to reload shader library: %s", lib->path);
                // Mark library as invalid
                lib->is_compiled = false;
            }
        }
    }
}

// ============================================================================
// Validation Functions
// ============================================================================

static time_t shader_get_file_modified_time(const char *path) {
    struct stat attr;
    if (stat(path, &attr) == 0) {
        return attr.st_mtime;
    }
    return 0;
}

static bool shader_validate_source(const char *source) {
    if (!source) return false;
    
    // Basic validation checks
    size_t len = strlen(source);
    if (len == 0 || len > 1024 * 1024) { // Max 1MB
        return false;
    }
    
    // Check for Metal shader keywords
    bool has_vertex = strstr(source, "vertex") != NULL;
    bool has_fragment = strstr(source, "fragment") != NULL;
    bool has_kernel = strstr(source, "kernel") != NULL;
    
    if (!has_vertex && !has_fragment && !has_kernel) {
        return false; // No shader type found
    }
    
    return true;
}

static bool shader_validate_library(metal_shader_library_t *library) {
    if (!library || !library->is_compiled || !library->library) {
        return false;
    }
    
    // Check if library has any functions
    NSArray<NSString*>* function_names = [library->library functionNames];
    if ([functionNames count] == 0) {
        return false;
    }
    
    return true;
}

static void shader_log_compilation_error(shader_system_t *sys, const char *error) {
    if (!sys || !error) return;
    
    sys->validation_errors++;
    LOG_ERROR("Shader Compilation Error [%lu]: %s", 
             sys->validation_errors, error);
}

static uint64_t shader_get_timestamp(void) {
    struct timespec ts;
    clock_gettime(CLOCK_MONOTONIC, &ts);
    return (uint64_t)ts.tv_sec * 1000000000ULL + (uint64_t)ts.tv_nsec;
}

static bool shader_validate_shader_function(metal_shader_t *shader) {
    if (!shader || !shader->function) {
        return false;
    }
    
    // Basic validation - check function name
    if (strlen(shader->name) == 0) {
        return false;
    }
    
    // Check function type
    MTLFunctionType type = [shader->function functionType];
    if (type != MTLFunctionTypeVertex && 
        type != MTLFunctionTypeFragment && 
        type != MTLFunctionTypeKernel) {
        return false;
    }
    
    return true;
}

// ============================================================================
// Public Validation API
// ============================================================================

bool shader_system_validate_state(shader_system_t* sys) {
    if (!sys || !sys->compilation_enabled) {
        return true;
    }
    
    bool valid = true;
    uint64_t current_time = shader_get_timestamp();
    
    // Check compilation success rate
    if (sys->total_compilations > 0) {
        double success_rate = (double)sys->successful_compilations / sys->total_compilations;
        if (success_rate < 0.8) { // Less than 80% success rate
            shader_log_compilation_error(sys, "Low shader compilation success rate detected");
            valid = false;
        }
    }
    
    // Check for excessive validation errors
    if (sys->validation_errors > 50) {
        shader_log_compilation_error(sys, "Excessive shader validation errors detected");
        valid = false;
    }
    
    // Validate all loaded libraries
    for (uint32_t i = 0; i < sys->library_count; i++) {
        if (!shader_validate_library(&sys->libraries[i])) {
            shader_log_compilation_error(sys, "Invalid shader library detected");
            valid = false;
        }
    }
    
    sys->last_compilation_time = current_time;
    return valid;
}

void shader_system_enable_compilation(shader_system_t* sys, bool enabled) {
    if (!sys) return;
    
    sys->compilation_enabled = enabled;
    LOG_INFO("Shader compilation %s", enabled ? "enabled" : "disabled");
}

void shader_system_enable_hot_reload(shader_system_t* sys, bool enabled) {
    if (!sys) return;
    
    sys->hot_reload_enabled = enabled;
    LOG_INFO("Shader hot reload %s", enabled ? "enabled" : "disabled");
}

void shader_system_get_statistics(shader_system_t* sys, uint64_t* total_compilations,
                                 uint64_t* successful_compilations, uint64_t* failed_compilations,
                                 uint64_t* validation_errors, uint64_t* hot_reload_reloads) {
    if (!sys) return;
    
    if (total_compilations) *total_compilations = sys->total_compilations;
    if (successful_compilations) *successful_compilations = sys->successful_compilations;
    if (failed_compilations) *failed_compilations = sys->failed_compilations;
    if (validation_errors) *validation_errors = sys->validation_errors;
    if (hot_reload_reloads) *hot_reload_reloads = sys->hot_reload_reloads;
}

void shader_system_reset_statistics(shader_system_t* sys) {
    if (!sys) return;
    
    sys->total_compilations = 0;
    sys->successful_compilations = 0;
    sys->failed_compilations = 0;
    sys->validation_errors = 0;
    sys->hot_reload_checks = 0;
    sys->hot_reload_reloads = 0;
    sys->last_compilation_time = shader_get_timestamp();
    
    LOG_INFO("Shader system statistics reset");
}
