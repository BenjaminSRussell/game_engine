// src/engine/rendering/shader/shader_hot_reload.c
// Shader Hot-Reloading System - Automatic shader recompilation on file changes

#include <core/logger.h>
#include <stdbool.h>
#include <stdint.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>

#ifdef __APPLE__
#include <CoreFoundation/CoreFoundation.h>
#include <dispatch/dispatch.h>
#endif

#include "../render_pipeline.h"

// ============================================================================
// Shader Hot-Reload Types
// ============================================================================

typedef enum {
    SHADER_TYPE_VERTEX,
    SHADER_TYPE_FRAGMENT,
    SHADER_TYPE_COMPUTE,
    SHADER_TYPE_GEOMETRY
} ShaderType;

typedef struct {
    char name[256];
    char file_path[512];
    ShaderType type;
    uint32_t program_id;
    time_t last_modified;
    bool is_loaded;
    bool needs_reload;
} ShaderEntry;

typedef struct {
    ShaderEntry *shaders;
    uint32_t shader_count;
    uint32_t shader_capacity;
    
    bool hot_reload_enabled;
    float check_interval; // seconds
    time_t last_check_time;
    
    // File system monitoring
#ifdef __APPLE__
    dispatch_queue_t monitor_queue;
    CFArrayRef dir_paths;
    FSEventStreamRef event_stream;
#endif
    
    // Callbacks
    void (*on_shader_reloaded)(const char *name, uint32_t program_id);
    void (*on_shader_error)(const char *name, const char *error);
    
    bool initialized;
} ShaderHotReloadSystem;

static ShaderHotReloadSystem g_hot_reload_system = {0};

// ============================================================================
// File System Utilities
// ============================================================================

static time_t get_file_modified_time(const char *file_path) {
    struct stat file_stat;
    if (stat(file_path, &file_stat) == 0) {
        return file_stat.st_mtime;
    }
    return 0;
}

static bool file_exists(const char *file_path) {
    struct stat file_stat;
    return stat(file_path, &file_stat) == 0;
}

static char *load_file_content(const char *file_path) {
    FILE *file = fopen(file_path, "r");
    if (!file) {
        return NULL;
    }
    
    fseek(file, 0, SEEK_END);
    long file_size = ftell(file);
    fseek(file, 0, SEEK_SET);
    
    char *content = malloc(file_size + 1);
    if (!content) {
        fclose(file);
        return NULL;
    }
    
    size_t bytes_read = fread(content, 1, file_size, file);
    content[bytes_read] = '\0';
    
    fclose(file);
    return content;
}

// ============================================================================
// Shader Compilation
// ============================================================================

static uint32_t compile_shader_source(const char *source, ShaderType type) {
    // TODO: Implement actual shader compilation using Metal/Vulkan backend
    // This is a placeholder that would call the actual shader compiler
    
    LOG_DEBUG("Compiling shader of type %d", (int)type);
    
    // Return a fake program ID for now
    return (uint32_t)(rand() % 10000 + 1);
}

static bool reload_shader(ShaderEntry *entry) {
    if (!entry || !file_exists(entry->file_path)) {
        return false;
    }
    
    time_t current_time = get_file_modified_time(entry->file_path);
    if (current_time <= entry->last_modified) {
        return false; // No changes
    }
    
    char *source = load_file_content(entry->file_path);
    if (!source) {
        if (g_hot_reload_system.on_shader_error) {
            g_hot_reload_system.on_shader_error(entry->name, "Failed to load shader source");
        }
        return false;
    }
    
    // Compile new shader
    uint32_t new_program = compile_shader_source(source, entry->type);
    free(source);
    
    if (new_program == 0) {
        if (g_hot_reload_system.on_shader_error) {
            g_hot_reload_system.on_shader_error(entry->name, "Shader compilation failed");
        }
        return false;
    }
    
    // Replace old program
    uint32_t old_program = entry->program_id;
    entry->program_id = new_program;
    entry->last_modified = current_time;
    entry->needs_reload = false;
    
    // TODO: Destroy old shader program
    
    LOG_INFO("Shader reloaded: %s (program %u -> %u)", entry->name, old_program, new_program);
    
    if (g_hot_reload_system.on_shader_reloaded) {
        g_hot_reload_system.on_shader_reloaded(entry->name, new_program);
    }
    
    return true;
}

// ============================================================================
// File System Monitoring (macOS implementation)
// ============================================================================

#ifdef __APPLE__
static void file_system_callback(ConstFSEventStreamRef streamRef,
                                void *clientCallBackInfo,
                                size_t numEvents,
                                void *eventPaths,
                                const FSEventStreamEventFlags eventFlags[],
                                const FSEventStreamEventId *eventIds) {
    (void)streamRef;
    (void)clientCallBackInfo;
    (void)eventIds;
    
    char **paths = (char **)eventPaths;
    
    for (size_t i = 0; i < numEvents; i++) {
        if (eventFlags[i] & kFSEventStreamEventFlagItemModified) {
            // Check if this is a shader file we're monitoring
            for (uint32_t j = 0; j < g_hot_reload_system.shader_count; j++) {
                ShaderEntry *entry = &g_hot_reload_system.shaders[j];
                if (strstr(paths[i], entry->file_path) != NULL) {
                    entry->needs_reload = true;
                    LOG_DEBUG("Shader file modified: %s", entry->file_path);
                }
            }
        }
    }
}
#endif

// ============================================================================
// Shader Hot-Reload API
// ============================================================================

bool shader_hot_reload_init(void) {
    if (g_hot_reload_system.initialized) {
        LOG_WARN("Shader hot-reload system already initialized");
        return true;
    }
    
    memset(&g_hot_reload_system, 0, sizeof(ShaderHotReloadSystem));
    g_hot_reload_system.hot_reload_enabled = true;
    g_hot_reload_system.check_interval = 0.5f; // Check every 0.5 seconds
    g_hot_reload_system.shader_capacity = 256;
    
    g_hot_reload_system.shaders = calloc(g_hot_reload_system.shader_capacity, sizeof(ShaderEntry));
    if (!g_hot_reload_system.shaders) {
        LOG_ERROR("Failed to allocate shader array");
        return false;
    }
    
#ifdef __APPLE__
    // Create file system monitoring
    g_hot_reload_system.monitor_queue = dispatch_queue_create("shader.hot_reload", DISPATCH_QUEUE_SERIAL);
    
    // TODO: Set up directory monitoring for shader directories
    // This would require setting up FSEventStream for shader directories
#endif
    
    g_hot_reload_system.initialized = true;
    LOG_INFO("Shader hot-reload system initialized");
    return true;
}

void shader_hot_reload_shutdown(void) {
    if (!g_hot_reload_system.initialized)
        return;
    
#ifdef __APPLE__
    if (g_hot_reload_system.event_stream) {
        FSEventStreamStop(g_hot_reload_system.event_stream);
        FSEventStreamInvalidate(g_hot_reload_system.event_stream);
        FSEventStreamRelease(g_hot_reload_system.event_stream);
    }
    
    if (g_hot_reload_system.monitor_queue) {
        dispatch_release(g_hot_reload_system.monitor_queue);
    }
    
    if (g_hot_reload_system.dir_paths) {
        CFRelease(g_hot_reload_system.dir_paths);
    }
#endif
    
    // Clean up shader entries
    for (uint32_t i = 0; i < g_hot_reload_system.shader_count; i++) {
        // TODO: Destroy shader programs
    }
    
    free(g_hot_reload_system.shaders);
    memset(&g_hot_reload_system, 0, sizeof(ShaderHotReloadSystem));
    
    LOG_INFO("Shader hot-reload system shutdown");
}

bool shader_hot_reload_add_shader(const char *name, const char *file_path, ShaderType type) {
    if (!g_hot_reload_system.initialized || !name || !file_path) {
        LOG_ERROR("Hot-reload system not initialized or invalid parameters");
        return false;
    }
    
    if (g_hot_reload_system.shader_count >= g_hot_reload_system.shader_capacity) {
        LOG_ERROR("Too many shaders in hot-reload system");
        return false;
    }
    
    if (!file_exists(file_path)) {
        LOG_ERROR("Shader file does not exist: %s", file_path);
        return false;
    }
    
    ShaderEntry *entry = &g_hot_reload_system.shaders[g_hot_reload_system.shader_count++];
    strncpy(entry->name, name, sizeof(entry->name) - 1);
    strncpy(entry->file_path, file_path, sizeof(entry->file_path) - 1);
    entry->type = type;
    entry->last_modified = get_file_modified_time(file_path);
    entry->needs_reload = false;
    
    // Initial compilation
    char *source = load_file_content(file_path);
    if (source) {
        entry->program_id = compile_shader_source(source, type);
        entry->is_loaded = (entry->program_id != 0);
        free(source);
    }
    
    if (!entry->is_loaded) {
        LOG_ERROR("Failed to compile initial shader: %s", name);
        return false;
    }
    
    LOG_INFO("Added shader to hot-reload: %s (%s)", name, file_path);
    return true;
}

void shader_hot_reload_update(void) {
    if (!g_hot_reload_system.initialized || !g_hot_reload_system.hot_reload_enabled) {
        return;
    }
    
    time_t current_time = time(NULL);
    if (difftime(current_time, g_hot_reload_system.last_check_time) < g_hot_reload_system.check_interval) {
        return;
    }
    
    g_hot_reload_system.last_check_time = current_time;
    
    // Check for file modifications
    for (uint32_t i = 0; i < g_hot_reload_system.shader_count; i++) {
        ShaderEntry *entry = &g_hot_reload_system.shaders[i];
        
        if (entry->needs_reload || get_file_modified_time(entry->file_path) > entry->last_modified) {
            reload_shader(entry);
        }
    }
}

void shader_hot_reload_set_enabled(bool enabled) {
    g_hot_reload_system.hot_reload_enabled = enabled;
    LOG_INFO("Shader hot-reload %s", enabled ? "enabled" : "disabled");
}

void shader_hot_reload_set_check_interval(float interval) {
    g_hot_reload_system.check_interval = interval;
    LOG_DEBUG("Shader hot-reload check interval set to %.2f seconds", interval);
}

void shader_hot_reload_set_callbacks(void (*on_reloaded)(const char *, uint32_t),
                                    void (*on_error)(const char *, const char *)) {
    g_hot_reload_system.on_shader_reloaded = on_reloaded;
    g_hot_reload_system.on_shader_error = on_error;
}

uint32_t shader_hot_reload_get_program_id(const char *name) {
    if (!g_hot_reload_system.initialized || !name) {
        return 0;
    }
    
    for (uint32_t i = 0; i < g_hot_reload_system.shader_count; i++) {
        if (strcmp(g_hot_reload_system.shaders[i].name, name) == 0) {
            return g_hot_reload_system.shaders[i].program_id;
        }
    }
    
    return 0;
}

bool shader_hot_reload_is_enabled(void) {
    return g_hot_reload_system.hot_reload_enabled;
}

void shader_hot_reload_get_stats(uint32_t *total_shaders, uint32_t *loaded_shaders, uint32_t *pending_reloads) {
    if (!g_hot_reload_system.initialized) {
        if (total_shaders) *total_shaders = 0;
        if (loaded_shaders) *loaded_shaders = 0;
        if (pending_reloads) *pending_reloads = 0;
        return;
    }
    
    uint32_t loaded = 0;
    uint32_t pending = 0;
    
    for (uint32_t i = 0; i < g_hot_reload_system.shader_count; i++) {
        if (g_hot_reload_system.shaders[i].is_loaded) loaded++;
        if (g_hot_reload_system.shaders[i].needs_reload) pending++;
    }
    
    if (total_shaders) *total_shaders = g_hot_reload_system.shader_count;
    if (loaded_shaders) *loaded_shaders = loaded;
    if (pending_reloads) *pending_reloads = pending;
}
