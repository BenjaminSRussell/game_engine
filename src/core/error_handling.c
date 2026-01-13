// error_handling.c - Standardized Error Handling Implementation
#include "error_handling.h"
#include <stdlib.h>
#include <string.h>
#include <time.h>

#ifdef _WIN32
#include <windows.h>
#define GET_THREAD_ID() GetCurrentThreadId()
#define GET_TIMESTAMP() GetTickCount64()
#else
#include <pthread.h>
#define GET_THREAD_ID() ((uint32_t)pthread_self())
#define GET_TIMESTAMP() ((uint64_t)time(NULL) * 1000)
#endif

/* ============================================================================
 * GLOBAL STATE
 * ============================================================================ */

static struct {
    bool initialized;
    error_context_t* current_error;
    error_strategy_t strategies[2000];  // Support up to custom error codes
    error_recovery_t recoveries[100];
    uint32_t recovery_count;
    error_stats_t stats;
    error_boundary_t* active_boundary;
    LOG_MUTEX_TYPE mutex;
} g_error_state = {0};

/* ============================================================================
 * ERROR CODE STRING MAPPINGS
 * ============================================================================ */

static const char* error_strings[] = {
    [ERROR_NONE] = "No error",
    [ERROR_SUCCESS] = "Success",
    [ERROR_OK] = "OK",
    
    [ERROR_INVALID_PARAMETER] = "Invalid parameter",
    [ERROR_NULL_POINTER] = "Null pointer",
    [ERROR_OUT_OF_MEMORY] = "Out of memory",
    [ERROR_BUFFER_OVERFLOW] = "Buffer overflow",
    [ERROR_BUFFER_UNDERFLOW] = "Buffer underflow",
    [ERROR_INVALID_OPERATION] = "Invalid operation",
    [ERROR_OPERATION_FAILED] = "Operation failed",
    [ERROR_TIMEOUT] = "Timeout",
    [ERROR_NOT_FOUND] = "Not found",
    [ERROR_ALREADY_EXISTS] = "Already exists",
    [ERROR_ACCESS_DENIED] = "Access denied",
    [ERROR_PERMISSION_DENIED] = "Permission denied",
    [ERROR_RESOURCE_BUSY] = "Resource busy",
    [ERROR_RESOURCE_EXHAUSTED] = "Resource exhausted",
    
    [ERROR_FILE_NOT_FOUND] = "File not found",
    [ERROR_FILE_ACCESS_DENIED] = "File access denied",
    [ERROR_FILE_ALREADY_EXISTS] = "File already exists",
    [ERROR_DIRECTORY_NOT_FOUND] = "Directory not found",
    [ERROR_PATH_TOO_LONG] = "Path too long",
    [ERROR_DISK_FULL] = "Disk full",
    [ERROR_IO_ERROR] = "I/O error",
    [ERROR_NETWORK_ERROR] = "Network error",
    [ERROR_NETWORK_UNREACHABLE] = "Network unreachable",
    [ERROR_CONNECTION_FAILED] = "Connection failed",
    [ERROR_CONNECTION_TIMEOUT] = "Connection timeout",
    [ERROR_CONNECTION_LOST] = "Connection lost",
    
    [ERROR_OPENGL_INIT_FAILED] = "OpenGL initialization failed",
    [ERROR_VULKAN_INIT_FAILED] = "Vulkan initialization failed",
    [ERROR_SHADER_COMPILATION_FAILED] = "Shader compilation failed",
    [ERROR_SHADER_LINK_FAILED] = "Shader link failed",
    [ERROR_TEXTURE_LOAD_FAILED] = "Texture load failed",
    [ERROR_MESH_LOAD_FAILED] = "Mesh load failed",
    [ERROR_BUFFER_CREATION_FAILED] = "Buffer creation failed",
    [ERROR_PIPELINE_CREATION_FAILED] = "Pipeline creation failed",
    [ERROR_SWAPCHAIN_CREATION_FAILED] = "Swapchain creation failed",
    [ERROR_DEVICE_LOST] = "Device lost",
    [ERROR_OUT_OF_VIDEO_MEMORY] = "Out of video memory",
    
    [ERROR_AUDIO_INIT_FAILED] = "Audio initialization failed",
    [ERROR_AUDIO_DEVICE_NOT_FOUND] = "Audio device not found",
    [ERROR_AUDIO_FORMAT_NOT_SUPPORTED] = "Audio format not supported",
    [ERROR_AUDIO_BUFFER_UNDERFLOW] = "Audio buffer underflow",
    [ERROR_AUDIO_BUFFER_OVERFLOW] = "Audio buffer overflow",
    [ERROR_AUDIO_STREAM_FAILED] = "Audio stream failed",
    
    [ERROR_PHYSICS_INIT_FAILED] = "Physics initialization failed",
    [ERROR_COLLISION_SHAPE_INVALID] = "Invalid collision shape",
    [ERROR_RIGID_BODY_CREATION_FAILED] = "Rigid body creation failed",
    [ERROR_CONSTRAINT_CREATION_FAILED] = "Constraint creation failed",
    [ERROR_SIMULATION_FAILED] = "Simulation failed",
    
    [ERROR_AI_INIT_FAILED] = "AI initialization failed",
    [ERROR_BEHAVIOR_TREE_INVALID] = "Invalid behavior tree",
    [ERROR_GOAP_PLANNING_FAILED] = "GOAP planning failed",
    [ERROR_PATHFINDING_FAILED] = "Pathfinding failed",
    [ERROR_NAVMESH_LOAD_FAILED] = "Navmesh load failed",
    
    [ERROR_ANIMATION_LOAD_FAILED] = "Animation load failed",
    [ERROR_SKELETON_INVALID] = "Invalid skeleton",
    [ERROR_ANIMATION_BLEND_FAILED] = "Animation blend failed",
    [ERROR_IK_SOLVER_FAILED] = "IK solver failed",
    
    [ERROR_ASSET_LOAD_FAILED] = "Asset load failed",
    [ERROR_ASSET_NOT_FOUND] = "Asset not found",
    [ERROR_ASSET_CORRUPTED] = "Asset corrupted",
    [ERROR_ASSET_VERSION_MISMATCH] = "Asset version mismatch",
    [ERROR_RESOURCE_NOT_FOUND] = "Resource not found",
    [ERROR_RESOURCE_LOAD_FAILED] = "Resource load failed",
    
    [ERROR_THREAD_CREATION_FAILED] = "Thread creation failed",
    [ERROR_MUTEX_INIT_FAILED] = "Mutex initialization failed",
    [ERROR_CONDITION_VARIABLE_FAILED] = "Condition variable failed",
    [ERROR_THREAD_JOIN_FAILED] = "Thread join failed",
    [ERROR_DEADLOCK_DETECTED] = "Deadlock detected"
};

/* ============================================================================
 * CORE ERROR HANDLING FUNCTIONS
 * ============================================================================ */

void error_init(void) {
    if (g_error_state.initialized) return;
    
    LOG_MUTEX_INIT(g_error_state.mutex);
    
    // Initialize default strategies
    for (int i = 0; i < 2000; i++) {
        if (error_is_fatal((error_code_t)i)) {
            g_error_state.strategies[i] = ERROR_STRATEGY_LOG_AND_ABORT;
        } else {
            g_error_state.strategies[i] = ERROR_STRATEGY_LOG_AND_CONTINUE;
        }
    }
    
    memset(&g_error_state.stats, 0, sizeof(g_error_state.stats));
    g_error_state.initialized = true;
    
    LOG_CORE_INFO("Error handling system initialized");
}

void error_shutdown(void) {
    if (!g_error_state.initialized) return;
    
    LOG_MUTEX_LOCK(g_error_state.mutex);
    
    // Clean up current error context
    if (g_error_state.current_error) {
        free(g_error_state.current_error);
        g_error_state.current_error = NULL;
    }
    
    // Clean up recoveries
    g_error_state.recovery_count = 0;
    
    g_error_state.initialized = false;
    
    LOG_MUTEX_UNLOCK(g_error_state.mutex);
    LOG_MUTEX_DESTROY(g_error_state.mutex);
    
    LOG_CORE_INFO("Error handling system shutdown");
}

error_context_t* error_create(error_code_t code, const char* message, 
                            const char* file, const char* function, int line) {
    error_context_t* error = malloc(sizeof(error_context_t));
    if (!error) return NULL;
    
    memset(error, 0, sizeof(error_context_t));
    error->code = code;
    error->message = message ? strdup(message) : strdup("Unknown error");
    error->file = file ? file : "unknown";
    error->function = function ? function : "unknown";
    error->line = line;
    error->timestamp = GET_TIMESTAMP();
    error->thread_id = GET_THREAD_ID();
    error->cause = NULL;
    
    return error;
}

void error_report(error_context_t* error) {
    if (!error || !g_error_state.initialized) return;
    
    LOG_MUTEX_LOCK(g_error_state.mutex);
    
    // Update statistics
    g_error_state.stats.total_errors++;
    
    if (error_is_fatal(error->code)) {
        g_error_state.stats.fatal_errors++;
    }
    
    // Update category statistics
    int category = error->code / 100;
    if (category < 10) {
        g_error_state.stats.errors_per_category[category]++;
    }
    
    // Update most common error
    // TODO: Track frequency of each error code
    
    // Log the error
    log_level_t level = error_is_fatal(error->code) ? LOG_LEVEL_FATAL : LOG_LEVEL_ERROR;
    log_channel_t channel = LOG_CHANNEL_CORE;
    
    // Determine channel based on error code
    if (error->code >= 300 && error->code < 400) channel = LOG_CHANNEL_RENDERING;
    else if (error->code >= 400 && error->code < 500) channel = LOG_CHANNEL_AUDIO;
    else if (error->code >= 500 && error->code < 600) channel = LOG_CHANNEL_PHYSICS;
    else if (error->code >= 600 && error->code < 700) channel = LOG_CHANNEL_AI;
    else if (error->code >= 800 && error->code < 900) channel = LOG_CHANNEL_ASSETS;
    else if (error->code >= 900 && error->code < 1000) channel = LOG_CHANNEL_THREADING;
    
    unified_log_message(level, channel, error->file, error->function, 
                      error->file, error->line, 
                      "Error %d (%s): %s", error->code, 
                      error_code_string(error->code), error->message);
    
    // Set as current error
    if (g_error_state.current_error) {
        free(g_error_state.current_error);
    }
    g_error_state.current_error = error;
    
    // Handle error boundary
    if (g_error_state.active_boundary) {
        g_error_state.active_boundary->last_error = error;
        g_error_state.active_boundary->error_count++;
        
        if (g_error_state.active_boundary->error_handler) {
            g_error_state.active_boundary->error_handler(error, 
                                                       g_error_state.active_boundary->user_data);
        }
    }
    
    // Execute error strategy
    error_execute_strategy(error);
    
    LOG_MUTEX_UNLOCK(g_error_state.mutex);
}

void error_report_with_cause(error_code_t code, const char* message,
                            const char* file, const char* function, int line,
                            error_context_t* cause) {
    error_context_t* error = error_create(code, message, file, function, line);
    if (error) {
        error->cause = cause;
        error_report(error);
    }
}

/* ============================================================================
 * ERROR UTILITIES
 * ============================================================================ */

const char* error_code_string(error_code_t code) {
    if (code >= sizeof(error_strings) / sizeof(error_strings[0])) {
        return "Unknown error code";
    }
    return error_strings[code] ? error_strings[code] : "Unknown error";
}

const char* error_category_string(error_code_t code) {
    if (code < 100) return "Success";
    if (code < 200) return "General";
    if (code < 300) return "System";
    if (code < 400) return "Graphics";
    if (code < 500) return "Audio";
    if (code < 600) return "Physics";
    if (code < 700) return "AI";
    if (code < 800) return "Animation";
    if (code < 900) return "Asset";
    if (code < 1000) return "Threading";
    return "Custom";
}

bool error_is_fatal(error_code_t code) {
    return (code == ERROR_OUT_OF_MEMORY ||
            code == ERROR_DEVICE_LOST ||
            code == ERROR_DEADLOCK_DETECTED ||
            code == ERROR_FATAL);
}

bool error_is_recoverable(error_code_t code) {
    return !error_is_fatal(code) && 
           code != ERROR_NULL_POINTER &&
           code != ERROR_INVALID_PARAMETER;
}

bool error_is_network_related(error_code_t code) {
    return (code >= 207 && code <= 211);  // Network error range
}

bool error_is_file_related(error_code_t code) {
    return (code >= 200 && code <= 206);  // File error range
}

/* ============================================================================
 * ERROR STRATEGY SYSTEM
 * ============================================================================ */

void error_set_strategy(error_code_t code, error_strategy_t strategy) {
    if (code < 2000) {
        g_error_state.strategies[code] = strategy;
    }
}

error_strategy_t error_get_strategy(error_code_t code) {
    if (code < 2000) {
        return g_error_state.strategies[code];
    }
    return ERROR_STRATEGY_LOG_AND_CONTINUE;
}

void error_execute_strategy(error_context_t* error) {
    if (!error) return;
    
    error_strategy_t strategy = error_get_strategy(error->code);
    
    switch (strategy) {
        case ERROR_STRATEGY_IGNORE:
            // Do nothing
            break;
            
        case ERROR_STRATEGY_LOG_ONLY:
            // Already logged above
            break;
            
        case ERROR_STRATEGY_LOG_AND_CONTINUE:
            // Already logged, continue execution
            break;
            
        case ERROR_STRATEGY_LOG_AND_RETURN:
            // Already logged, caller should return
            break;
            
        case ERROR_STRATEGY_LOG_AND_ABORT:
            // Fatal error, abort
            LOG_CORE_FATAL("Fatal error encountered, aborting: %s", error->message);
            abort();
            break;
            
        case ERROR_STRATEGY_CUSTOM:
            // Attempt recovery
            error_attempt_recovery(error);
            break;
    }
}

/* ============================================================================
 * ERROR RECOVERY SYSTEM
 * ============================================================================ */

void error_register_recovery(error_code_t code, error_code_t (*recover_func)(error_context_t*), 
                           const char* description) {
    if (g_error_state.recovery_count >= 100) return;
    
    error_recovery_t* recovery = &g_error_state.recoveries[g_error_state.recovery_count++];
    recovery->code = code;
    recovery->recover_func = recover_func;
    recovery->description = description;
}

error_code_t error_attempt_recovery(error_context_t* error) {
    if (!error) return ERROR_INVALID_PARAMETER;
    
    for (uint32_t i = 0; i < g_error_state.recovery_count; i++) {
        error_recovery_t* recovery = &g_error_state.recoveries[i];
        if (recovery->code == error->code && recovery->recover_func) {
            LOG_CORE_INFO("Attempting recovery for error %d: %s", 
                         error->code, recovery->description);
            
            uint64_t start_time = GET_TIMESTAMP();
            error_code_t result = recovery->recover_func(error);
            uint64_t end_time = GET_TIMESTAMP();
            
            if (result == ERROR_NONE) {
                g_error_state.stats.recovered_errors++;
                LOG_CORE_INFO("Error recovery successful for error %d", error->code);
            } else {
                g_error_state.stats.unrecovered_errors++;
                LOG_CORE_WARNING("Error recovery failed for error %d: %s", 
                                error->code, error_code_string(result));
            }
            
            // Update recovery time statistics
            float recovery_time = (float)(end_time - start_time);
            g_error_state.stats.average_recovery_time_ms = 
                (g_error_state.stats.average_recovery_time_ms * (g_error_state.stats.recovered_errors - 1) + 
                 recovery_time) / g_error_state.stats.recovered_errors;
            
            return result;
        }
    }
    
    return ERROR_NOT_FOUND;
}

/* ============================================================================
 * ERROR BOUNDARY SYSTEM
 * ============================================================================ */

error_boundary_t* error_boundary_create(void) {
    error_boundary_t* boundary = malloc(sizeof(error_boundary_t));
    if (!boundary) return NULL;
    
    memset(boundary, 0, sizeof(error_boundary_t));
    boundary->enabled = true;
    
    return boundary;
}

void error_boundary_destroy(error_boundary_t* boundary) {
    if (!boundary) return;
    
    if (boundary->last_error) {
        free(boundary->last_error);
    }
    
    free(boundary);
}

void error_boundary_enter(error_boundary_t* boundary) {
    if (!boundary || !boundary->enabled) return;
    
    LOG_MUTEX_LOCK(g_error_state.mutex);
    g_error_state.active_boundary = boundary;
    boundary->error_count = 0;
    boundary->last_error = NULL;
    LOG_MUTEX_UNLOCK(g_error_state.mutex);
}

void error_boundary_exit(error_boundary_t* boundary) {
    if (!boundary) return;
    
    LOG_MUTEX_LOCK(g_error_state.mutex);
    if (g_error_state.active_boundary == boundary) {
        g_error_state.active_boundary = NULL;
    }
    LOG_MUTEX_UNLOCK(g_error_state.mutex);
}

error_context_t* error_boundary_get_last_error(error_boundary_t* boundary) {
    return boundary ? boundary->last_error : NULL;
}

void error_boundary_set_handler(error_boundary_t* boundary, 
                              void (*handler)(error_context_t*, void*), void* user_data) {
    if (boundary) {
        boundary->error_handler = handler;
        boundary->user_data = user_data;
    }
}

/* ============================================================================
 * STATISTICS AND MONITORING
 * ============================================================================ */

void error_get_stats(error_stats_t* stats) {
    if (!stats) return;
    
    LOG_MUTEX_LOCK(g_error_state.mutex);
    *stats = g_error_state.stats;
    LOG_MUTEX_UNLOCK(g_error_state.mutex);
}

void error_reset_stats(void) {
    LOG_MUTEX_LOCK(g_error_state.mutex);
    memset(&g_error_state.stats, 0, sizeof(g_error_state.stats));
    LOG_MUTEX_UNLOCK(g_error_state.mutex);
}

void error_dump_stats(void) {
    error_stats_t stats;
    error_get_stats(&stats);
    
    LOG_CORE_INFO("=== Error Statistics ===");
    LOG_CORE_INFO("Total errors: %llu", stats.total_errors);
    LOG_CORE_INFO("Fatal errors: %llu", stats.fatal_errors);
    LOG_CORE_INFO("Recovered errors: %llu", stats.recovered_errors);
    LOG_CORE_INFO("Unrecovered errors: %llu", stats.unrecovered_errors);
    LOG_CORE_INFO("Average recovery time: %.2f ms", stats.average_recovery_time_ms);
    
    LOG_CORE_INFO("Errors by category:");
    for (int i = 0; i < 10; i++) {
        if (stats.errors_per_category[i] > 0) {
            LOG_CORE_INFO("  %s: %llu", error_category_string((error_code_t)(i * 100)), 
                         stats.errors_per_category[i]);
        }
    }
}

/* ============================================================================
 * CONTEXT MANAGEMENT
 * ============================================================================ */

void error_set_context(error_context_t* context) {
    LOG_MUTEX_LOCK(g_error_state.mutex);
    if (g_error_state.current_error) {
        free(g_error_state.current_error);
    }
    g_error_state.current_error = context;
    LOG_MUTEX_UNLOCK(g_error_state.mutex);
}

error_context_t* error_get_context(void) {
    LOG_MUTEX_LOCK(g_error_state.mutex);
    error_context_t* context = g_error_state.current_error;
    LOG_MUTEX_UNLOCK(g_error_state.mutex);
    return context;
}

void error_clear_context(void) {
    LOG_MUTEX_LOCK(g_error_state.mutex);
    if (g_error_state.current_error) {
        free(g_error_state.current_error);
        g_error_state.current_error = NULL;
    }
    LOG_MUTEX_UNLOCK(g_error_state.mutex);
}
