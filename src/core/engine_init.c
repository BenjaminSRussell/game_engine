// Engine Initialization Pipeline Implementation
// Comprehensive system startup with dependency management and error handling

#include "engine_init.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>
#include <algorithm>

// Global initialization context
EngineInitContext g_engineInit = {};

// Static module registry
static EngineModule g_moduleRegistry[64];
static uint32_t g_moduleCount = 0;
static EngineEventCallback g_eventCallbacks[16];
static uint32_t g_eventCallbackCount = 0;

// Internal helper functions
static const char* get_init_phase_string(EngineInitPhase phase) {
    switch (phase) {
        case ENGINE_INIT_PHASE_PRE_INIT: return "Pre-Initialization";
        case ENGINE_INIT_PHASE_CORE_SYSTEMS: return "Core Systems";
        case ENGINE_INIT_PHASE_RENDERING_BACKEND: return "Rendering Backend";
        case ENGINE_INIT_PHASE_AUDIO_SYSTEM: return "Audio System";
        case ENGINE_INIT_PHASE_INPUT_SYSTEM: return "Input System";
        case ENGINE_INIT_PHASE_ASSET_SYSTEM: return "Asset System";
        case ENGINE_INIT_PHASE_SCRIPT_SYSTEM: return "Script System";
        case ENGINE_INIT_PHASE_NETWORK_SYSTEM: return "Network System";
        case ENGINE_INIT_PHASE_PHYSICS_SYSTEM: return "Physics System";
        case ENGINE_INIT_PHASE_UI_SYSTEM: return "UI System";
        case ENGINE_INIT_PHASE_GAME_SYSTEMS: return "Game Systems";
        case ENGINE_INIT_PHASE_POST_INIT: return "Post-Initialization";
        case ENGINE_INIT_PHASE_READY: return "Ready";
        case ENGINE_INIT_PHASE_ERROR: return "Error";
        default: return "Unknown";
    }
}

static const char* get_status_string(EngineInitStatus status) {
    switch (status) {
        case ENGINE_INIT_STATUS_NOT_STARTED: return "Not Started";
        case ENGINE_INIT_STATUS_IN_PROGRESS: return "In Progress";
        case ENGINE_INIT_STATUS_SUCCESS: return "Success";
        case ENGINE_INIT_STATUS_WARNING: return "Warning";
        case ENGINE_INIT_STATUS_ERROR: return "Error";
        default: return "Unknown";
    }
}

static bool check_dependencies(const EngineModule* module) {
    if (module->dependencies == 0) {
        return true; // No dependencies
    }
    
    for (uint32_t i = 0; i < g_moduleCount; i++) {
        if (g_moduleRegistry[i].initialized && 
            (module->dependencies & (1 << i))) {
            return true;
        }
    }
    
    return false;
}

static bool initialize_module(EngineModule* module) {
    if (!module || !module->init) {
        return false;
    }
    
    printf("Initializing module: %s\n", module->name);
    
    double startTime = (double)clock() / CLOCKS_PER_SEC * 1000.0;
    
    bool success = module->init(module->config);
    
    double endTime = (double)clock() / CLOCKS_PER_SEC * 1000.0;
    module->init_time = endTime - startTime;
    
    module->status = success ? ENGINE_INIT_STATUS_SUCCESS : ENGINE_INIT_STATUS_ERROR;
    module->initialized = success;
    
    if (success) {
        g_engineInit.successful_inits++;
        printf("Module %s initialized successfully in %.2f ms\n", 
               module->name, module->init_time);
        
        // Trigger success event
        EngineEventData event = {
            .type = ENGINE_EVENT_MODULE_INITIALIZED,
            .module_name = module->name,
            .status = ENGINE_INIT_STATUS_SUCCESS,
            .timestamp = endTime,
            .message = "Module initialized successfully"
        };
        engine_trigger_event(&event);
    } else {
        g_engineInit.failed_inits++;
        snprintf(g_engineInit.error_buffer, sizeof(g_engineInit.error_buffer),
                "Failed to initialize module: %s", module->name);
        printf("ERROR: Module %s failed to initialize\n", module->name);
        
        // Trigger failure event
        EngineEventData event = {
            .type = ENGINE_EVENT_MODULE_FAILED,
            .module_name = module->name,
            .status = ENGINE_INIT_STATUS_ERROR,
            .timestamp = endTime,
            .message = g_engineInit.error_buffer
        };
        engine_trigger_event(&event);
    }
    
    return success;
}

static bool initialize_phase(EngineInitPhase phase) {
    printf("Starting phase: %s\n", get_init_phase_string(phase));
    
    g_engineInit.current_phase = phase;
    g_engineInit.overall_status = ENGINE_INIT_STATUS_IN_PROGRESS;
    
    // Trigger phase start event
    EngineEventData event = {
        .type = ENGINE_EVENT_PHASE_STARTED,
        .module_name = "",
        .status = ENGINE_INIT_STATUS_IN_PROGRESS,
        .timestamp = (double)clock() / CLOCKS_PER_SEC * 1000.0,
        .message = get_init_phase_string(phase)
    };
    engine_trigger_event(&event);
    
    // Initialize all modules for this phase
    bool phaseSuccess = true;
    uint32_t phaseModuleCount = 0;
    
    for (uint32_t i = 0; i < g_moduleCount; i++) {
        EngineModule* module = &g_moduleRegistry[i];
        
        // Check if module belongs to this phase
        bool belongsToPhase = false;
        switch (phase) {
            case ENGINE_INIT_PHASE_CORE_SYSTEMS:
                belongsToPhase = (module->dependencies & ENGINE_DEP_CORE_SYSTEMS) != 0;
                break;
            case ENGINE_INIT_PHASE_RENDERING_BACKEND:
                belongsToPhase = (module->dependencies & ENGINE_DEP_RENDERING_BACKEND) != 0;
                break;
            case ENGINE_INIT_PHASE_AUDIO_SYSTEM:
                belongsToPhase = (module->dependencies & ENGINE_DEP_AUDIO_SYSTEM) != 0;
                break;
            case ENGINE_INIT_PHASE_INPUT_SYSTEM:
                belongsToPhase = (module->dependencies & ENGINE_DEP_INPUT_SYSTEM) != 0;
                break;
            case ENGINE_INIT_PHASE_ASSET_SYSTEM:
                belongsToPhase = (module->dependencies & ENGINE_DEP_ASSET_SYSTEM) != 0;
                break;
            case ENGINE_INIT_PHASE_SCRIPT_SYSTEM:
                belongsToPhase = (module->dependencies & ENGINE_DEP_SCRIPT_SYSTEM) != 0;
                break;
            case ENGINE_INIT_PHASE_NETWORK_SYSTEM:
                belongsToPhase = (module->dependencies & ENGINE_DEP_NETWORK_SYSTEM) != 0;
                break;
            case ENGINE_INIT_PHASE_PHYSICS_SYSTEM:
                belongsToPhase = (module->dependencies & ENGINE_DEP_PHYSICS_SYSTEM) != 0;
                break;
            case ENGINE_INIT_PHASE_UI_SYSTEM:
                belongsToPhase = (module->dependencies & ENGINE_DEP_UI_SYSTEM) != 0;
                break;
            case ENGINE_INIT_PHASE_GAME_SYSTEMS:
                belongsToPhase = (module->dependencies & ENGINE_DEP_GAME_SYSTEMS) != 0;
                break;
            default:
                belongsToPhase = false;
                break;
        }
        
        if (belongsToPhase && !module->initialized) {
            if (check_dependencies(module)) {
                if (initialize_module(module)) {
                    phaseModuleCount++;
                } else if (module->required) {
                    phaseSuccess = false;
                    snprintf(g_engineInit.error_buffer, sizeof(g_engineInit.error_buffer),
                            "Required module %s failed to initialize", module->name);
                }
            } else {
                printf("Module %s waiting for dependencies\n", module->name);
            }
        }
    }
    
    if (phaseSuccess && phaseModuleCount > 0) {
        g_engineInit.overall_status = ENGINE_INIT_STATUS_SUCCESS;
        
        // Trigger phase completion event
        EngineEventData event = {
            .type = ENGINE_EVENT_PHASE_COMPLETED,
            .module_name = "",
            .status = ENGINE_INIT_STATUS_SUCCESS,
            .timestamp = (double)clock() / CLOCKS_PER_SEC * 1000.0,
            .message = get_init_phase_string(phase)
        };
        engine_trigger_event(&event);
        
        printf("Phase %s completed successfully (%d modules)\n", 
               get_init_phase_string(phase), phaseModuleCount);
    } else {
        g_engineInit.overall_status = ENGINE_INIT_STATUS_ERROR;
        
        // Trigger phase error event
        EngineEventData event = {
            .type = ENGINE_EVENT_PHASE_COMPLETED,
            .module_name = "",
            .status = ENGINE_INIT_STATUS_ERROR,
            .timestamp = (double)clock() / CLOCKS_PER_SEC * 1000.0,
            .message = get_init_phase_string(phase)
        };
        engine_trigger_event(&event);
        
        printf("ERROR: Phase %s failed\n", get_init_phase_string(phase));
    }
    
    return phaseSuccess;
}

static void shutdown_all_modules(void) {
    printf("Shutting down all modules...\n");
    
    // Shutdown in reverse dependency order
    for (int32_t i = (int32_t)g_moduleCount - 1; i >= 0; i--) {
        EngineModule* module = &g_moduleRegistry[i];
        if (module->initialized && module->shutdown) {
            printf("Shutting down module: %s\n", module->name);
            module->shutdown();
            module->initialized = false;
        }
    }
}

// Public API implementation
bool engine_init(const EngineConfig* config) {
    if (g_engineInit.current_phase != ENGINE_INIT_PHASE_NOT_STARTED) {
        printf("Engine already initialized or initialization in progress\n");
        return false;
    }
    
    printf("Starting engine initialization...\n");
    
    // Initialize context
    memset(&g_engineInit, 0, sizeof(g_engineInit));
    g_engineInit.config = *config;
    g_engineInit.start_time = (double)clock() / CLOCKS_PER_SEC * 1000.0;
    g_engineInit.current_phase = ENGINE_INIT_PHASE_PRE_INIT;
    g_engineInit.overall_status = ENGINE_INIT_STATUS_IN_PROGRESS;
    
    // Trigger pre-init event
    EngineEventData event = {
        .type = ENGINE_EVENT_SYSTEM_READY,
        .module_name = "engine",
        .status = ENGINE_INIT_STATUS_IN_PROGRESS,
        .timestamp = g_engineInit.start_time,
        .message = "Engine initialization started"
    };
    engine_trigger_event(&event);
    
    // Validate configuration
    if (!engine_validate_configuration(config)) {
        g_engineInit.overall_status = ENGINE_INIT_STATUS_ERROR;
        return false;
    }
    
    // Initialize phases in order
    EngineInitPhase phases[] = {
        ENGINE_INIT_PHASE_CORE_SYSTEMS,
        ENGINE_INIT_PHASE_RENDERING_BACKEND,
        ENGINE_INIT_PHASE_AUDIO_SYSTEM,
        ENGINE_INIT_PHASE_INPUT_SYSTEM,
        ENGINE_INIT_PHASE_ASSET_SYSTEM,
        ENGINE_INIT_PHASE_SCRIPT_SYSTEM,
        ENGINE_INIT_PHASE_NETWORK_SYSTEM,
        ENGINE_INIT_PHASE_PHYSICS_SYSTEM,
        ENGINE_INIT_PHASE_UI_SYSTEM,
        ENGINE_INIT_PHASE_GAME_SYSTEMS
    };
    
    for (size_t i = 0; i < sizeof(phases) / sizeof(phases[0]); i++) {
        if (!initialize_phase(phases[i])) {
            printf("ERROR: Phase %s failed, aborting initialization\n", 
                   get_init_phase_string(phases[i]));
            g_engineInit.current_phase = ENGINE_INIT_PHASE_ERROR;
            return false;
        }
        
        if (g_engineInit.abort_requested) {
            printf("Initialization aborted by user\n");
            break;
        }
    }
    
    // Post-initialization
    g_engineInit.current_phase = ENGINE_INIT_PHASE_POST_INIT;
    g_engineInit.total_init_time = (double)clock() / CLOCKS_PER_SEC * 1000.0 - g_engineInit.start_time;
    g_engineInit.overall_status = ENGINE_INIT_STATUS_SUCCESS;
    
    // Trigger system ready event
    EngineEventData readyEvent = {
        .type = ENGINE_EVENT_SYSTEM_READY,
        .module_name = "engine",
        .status = ENGINE_INIT_STATUS_SUCCESS,
        .timestamp = (double)clock() / CLOCKS_PER_SEC * 1000.0,
        .message = "Engine initialization completed"
    };
    engine_trigger_event(&readyEvent);
    
    printf("Engine initialization completed successfully in %.2f ms\n", g_engineInit.total_init_time);
    engine_print_init_summary();
    
    return true;
}

void engine_shutdown(void) {
    if (g_engineInit.current_phase == ENGINE_INIT_PHASE_NOT_STARTED) {
        return;
    }
    
    printf("Starting engine shutdown...\n");
    
    // Trigger shutdown start event
    EngineEventData event = {
        .type = ENGINE_EVENT_SHUTDOWN_STARTED,
        .module_name = "engine",
        .status = ENGINE_INIT_STATUS_IN_PROGRESS,
        .timestamp = (double)clock() / CLOCKS_PER_SEC * 1000.0,
        .message = "Engine shutdown started"
    };
    engine_trigger_event(&event);
    
    shutdown_all_modules();
    
    // Clear context
    memset(&g_engineInit, 0, sizeof(g_engineInit));
    g_engineInit.current_phase = ENGINE_INIT_PHASE_NOT_STARTED;
    
    // Trigger shutdown completed event
    EngineEventData completedEvent = {
        .type = ENGINE_EVENT_SHUTDOWN_COMPLETED,
        .module_name = "engine",
        .status = ENGINE_INIT_STATUS_SUCCESS,
        .timestamp = (double)clock() / CLOCKS_PER_SEC * 1000.0,
        .message = "Engine shutdown completed"
    };
    engine_trigger_event(&completedEvent);
    
    printf("Engine shutdown completed\n");
}

bool engine_is_initialized(void) {
    return g_engineInit.overall_status == ENGINE_INIT_STATUS_SUCCESS && 
           g_engineInit.current_phase == ENGINE_INIT_PHASE_READY;
}

bool engine_is_ready(void) {
    return g_engineInit.overall_status == ENGINE_INIT_STATUS_SUCCESS;
}

bool engine_register_module(const EngineModule* module) {
    if (g_moduleCount >= 64) {
        printf("ERROR: Module registry full\n");
        return false;
    }
    
    // Check for duplicate module names
    for (uint32_t i = 0; i < g_moduleCount; i++) {
        if (strcmp(g_moduleRegistry[i].name, module->name) == 0) {
            printf("ERROR: Module %s already registered\n", module->name);
            return false;
        }
    }
    
    // Add module to registry
    g_moduleRegistry[g_moduleCount] = *module;
    g_moduleCount++;
    
    printf("Registered module: %s (priority: %u)\n", module->name, module->priority);
    
    // Trigger registration event
    EngineEventData event = {
        .type = ENGINE_EVENT_MODULE_REGISTERED,
        .module_name = module->name,
        .status = ENGINE_INIT_STATUS_SUCCESS,
        .timestamp = (double)clock() / CLOCKS_PER_SEC * 1000.0,
        .message = "Module registered"
    };
    engine_trigger_event(&event);
    
    return true;
}

bool engine_unregister_module(const char* module_name) {
    for (uint32_t i = 0; i < g_moduleCount; i++) {
        if (strcmp(g_moduleRegistry[i].name, module_name) == 0) {
            if (g_moduleRegistry[i].initialized && g_moduleRegistry[i].shutdown) {
                g_moduleRegistry[i].shutdown();
            }
            
            // Remove module from registry
            for (uint32_t j = i; j < g_moduleCount - 1; j++) {
                g_moduleRegistry[j] = g_moduleRegistry[j + 1];
            }
            g_moduleCount--;
            
            printf("Unregistered module: %s\n", module_name);
            return true;
        }
    }
    
    printf("WARNING: Module %s not found\n", module_name);
    return false;
}

EngineModule* engine_get_module(const char* module_name) {
    for (uint32_t i = 0; i < g_moduleCount; i++) {
        if (strcmp(g_moduleRegistry[i].name, module_name) == 0) {
            return &g_moduleRegistry[i];
        }
    }
    return NULL;
}

EngineModule* engine_get_all_modules(uint32_t* count) {
    *count = g_moduleCount;
    return g_moduleRegistry;
}

EngineInitPhase engine_get_current_phase(void) {
    return g_engineInit.current_phase;
}

EngineInitStatus engine_get_phase_status(EngineInitPhase phase) {
    // This is a simplified implementation - in a real system,
    // you'd track phase-specific status
    return g_engineInit.overall_status;
}

bool engine_advance_phase(void) {
    switch (g_engineInit.current_phase) {
        case ENGINE_INIT_PHASE_PRE_INIT:
            return initialize_phase(ENGINE_INIT_PHASE_CORE_SYSTEMS);
        case ENGINE_INIT_PHASE_CORE_SYSTEMS:
            return initialize_phase(ENGINE_INIT_PHASE_RENDERING_BACKEND);
        case ENGINE_INIT_PHASE_RENDERING_BACKEND:
            return initialize_phase(ENGINE_INIT_PHASE_AUDIO_SYSTEM);
        case ENGINE_INIT_PHASE_AUDIO_SYSTEM:
            return initialize_phase(ENGINE_INIT_PHASE_INPUT_SYSTEM);
        case ENGINE_INIT_PHASE_INPUT_SYSTEM:
            return initialize_phase(ENGINE_INIT_PHASE_ASSET_SYSTEM);
        case ENGINE_INIT_PHASE_ASSET_SYSTEM:
            return initialize_phase(ENGINE_INIT_PHASE_SCRIPT_SYSTEM);
        case ENGINE_INIT_PHASE_SCRIPT_SYSTEM:
            return initialize_phase(ENGINE_INIT_PHASE_NETWORK_SYSTEM);
        case ENGINE_INIT_PHASE_NETWORK_SYSTEM:
            return initialize_phase(ENGINE_INIT_PHASE_PHYSICS_SYSTEM);
        case ENGINE_INIT_PHASE_PHYSICS_SYSTEM:
            return initialize_phase(ENGINE_INIT_PHASE_UI_SYSTEM);
        case ENGINE_INIT_PHASE_UI_SYSTEM:
            return initialize_phase(ENGINE_INIT_PHASE_GAME_SYSTEMS);
        case ENGINE_INIT_PHASE_GAME_SYSTEMS:
            g_engineInit.current_phase = ENGINE_INIT_PHASE_POST_INIT;
            g_engineInit.total_init_time = (double)clock() / CLOCKS_PER_SEC * 1000.0 - g_engineInit.start_time;
            return true;
        default:
            return false;
    }
}

bool engine_wait_for_phase(EngineInitPhase phase, uint32_t timeout_ms) {
    uint32_t startTime = (uint32_t)(clock() / CLOCKS_PER_SEC * 1000);
    
    while (g_engineInit.current_phase != phase && 
           g_engineInit.current_phase != ENGINE_INIT_PHASE_ERROR &&
           g_engineInit.current_phase != ENGINE_INIT_PHASE_NOT_STARTED) {
        
        uint32_t currentTime = (uint32_t)(clock() / CLOCKS_PER_SEC * 1000);
        if (currentTime - startTime > timeout_ms) {
            printf("Timeout waiting for phase %s\n", get_init_phase_string(phase));
            return false;
        }
        
        // Small delay to prevent busy waiting
        #ifdef _WIN32
        Sleep(10);
        #else
        usleep(10000);
        #endif
    }
    
    return g_engineInit.current_phase == phase;
}

const EngineConfig* engine_get_config(void) {
    return &g_engineInit.config;
}

bool engine_update_config(const EngineConfig* config) {
    g_engineInit.config = *config;
    return engine_validate_configuration(config);
}

bool engine_load_config_from_file(const char* config_path) {
    FILE* file = fopen(config_path, "r");
    if (!file) {
        printf("ERROR: Could not open config file: %s\n", config_path);
        return false;
    }
    
    // Simple config parsing (in a real implementation, use a proper config library)
    char line[256];
    while (fgets(line, sizeof(line), file)) {
        // Parse key=value pairs
        char* key = strtok(line, "=");
        char* value = strtok(NULL, "\n");
        
        if (key && value) {
            if (strcmp(key, "app_name") == 0) {
                strncpy((char*)g_engineInit.config.app_name, value, 63);
            } else if (strcmp(key, "window_width") == 0) {
                g_engineInit.config.window_width = (uint32_t)atoi(value);
            }
            // Add more config options as needed
        }
    }
    
    fclose(file);
    return engine_validate_configuration(&g_engineInit.config);
}

bool engine_save_config_to_file(const char* config_path) {
    FILE* file = fopen(config_path, "w");
    if (!file) {
        printf("ERROR: Could not create config file: %s\n", config_path);
        return false;
    }
    
    fprintf(file, "app_name=%s\n", g_engineInit.config.app_name);
    fprintf(file, "window_width=%u\n", g_engineInit.config.window_width);
    // Add more config options as needed
    
    fclose(file);
    return true;
}

bool engine_register_event_callback(EngineEventCallback callback) {
    if (g_eventCallbackCount >= 16) {
        printf("ERROR: Event callback registry full\n");
        return false;
    }
    
    g_eventCallbacks[g_eventCallbackCount++] = callback;
    return true;
}

void engine_unregister_event_callback(EngineEventCallback callback) {
    for (uint32_t i = 0; i < g_eventCallbackCount; i++) {
        if (g_eventCallbacks[i] == callback) {
            // Remove callback by shifting remaining callbacks
            for (uint32_t j = i; j < g_eventCallbackCount - 1; j++) {
                g_eventCallbacks[j] = g_eventCallbacks[j + 1];
            }
            g_eventCallbackCount--;
            break;
        }
    }
}

void engine_trigger_event(const EngineEventData* event) {
    for (uint32_t i = 0; i < g_eventCallbackCount; i++) {
        g_eventCallbacks[i](event);
    }
}

void engine_process_events(void) {
    // Process queued events (simplified implementation)
}

void engine_get_metrics(EngineMetrics* metrics) {
    metrics->total_modules = g_moduleCount;
    metrics->initialized_modules = g_engineInit.successful_inits;
    metrics->failed_modules = g_engineInit.failed_inits;
    metrics->warning_modules = g_engineInit.warning_count;
    metrics->total_init_time = g_engineInit.total_init_time;
    
    // Calculate average init time
    if (g_engineInit.successful_inits > 0) {
        metrics->average_init_time = g_engineInit.total_init_time / g_engineInit.successful_inits;
    } else {
        metrics->average_init_time = 0.0;
    }
    
    // Memory usage would be calculated from actual memory allocators
    metrics->memory_allocated = 0;
    metrics->memory_used = 0;
    metrics->cpu_usage_percent = 0;
    metrics->gpu_usage_percent = 0;
}

void engine_reset_metrics(void) {
    g_engineInit.successful_inits = 0;
    g_engineInit.failed_inits = 0;
    g_engineInit.warning_count = 0;
    g_engineInit.total_init_time = 0.0;
}

bool engine_enable_profiling(bool enable) {
    // Implementation would depend on the profiling system
    return true;
}

bool engine_is_profiling_enabled(void) {
    // Implementation would depend on the profiling system
    return false;
}

void engine_set_error(const char* error) {
    strncpy(g_engineInit.error_buffer, error, sizeof(g_engineInit.error_buffer) - 1);
    g_engineInit.error_buffer[sizeof(g_engineInit.error_buffer) - 1] = '\0';
}

const char* engine_get_last_error(void) {
    return g_engineInit.error_buffer;
}

void engine_clear_error(void) {
    memset(g_engineInit.error_buffer, 0, sizeof(g_engineInit.error_buffer));
}

void engine_print_module_info(void) {
    printf("\n=== Engine Module Registry ===\n");
    printf("Total modules: %u\n", g_moduleCount);
    printf("Successful inits: %u\n", g_engineInit.successful_inits);
    printf("Failed inits: %u\n", g_engineInit.failed_inits);
    printf("Warnings: %u\n", g_engineInit.warning_count);
    
    for (uint32_t i = 0; i < g_moduleCount; i++) {
        const EngineModule* module = &g_moduleRegistry[i];
        printf("  %s v%s - %s - %s\n", 
               module->name,
               module->version ? module->version : "unknown",
               module->initialized ? "initialized" : "not initialized",
               get_status_string(module->status));
        if (module->init_time > 0) {
            printf("    Init time: %.2f ms\n", module->init_time);
        }
    }
    printf("================================\n\n");
}

void engine_print_init_summary(void) {
    printf("\n=== Engine Initialization Summary ===\n");
    printf("Total init time: %.2f ms\n", g_engineInit.total_init_time);
    printf("Final status: %s\n", get_status_string(g_engineInit.overall_status));
    printf("Modules initialized: %u/%u\n", g_engineInit.successful_inits, g_moduleCount);
    
    if (g_engineInit.failed_inits > 0) {
        printf("Failed modules: %u\n", g_engineInit.failed_inits);
    }
    
    if (g_engineInit.warning_count > 0) {
        printf("Warnings: %u\n", g_engineInit.warning_count);
    }
    
    if (g_engineInit.error_buffer[0] != '\0') {
        printf("Last error: %s\n", g_engineInit.error_buffer);
    }
    
    printf("================================\n\n");
}

bool engine_validate_configuration(const EngineConfig* config) {
    // Basic validation
    if (!config) {
        engine_set_error("Configuration is null");
        return false;
    }
    
    if (!config->app_name || strlen(config->app_name) == 0) {
        engine_set_error("Application name is required");
        return false;
    }
    
    if (config->window_width == 0 || config->window_height == 0) {
        engine_set_error("Window dimensions must be greater than 0");
        return false;
    }
    
    if (config->target_fps == 0 || config->target_fps > 240) {
        engine_set_error("Target FPS must be between 1 and 240");
        return false;
    }
    
    return true;
}

bool engine_validate_module_dependencies(void) {
    // Check for circular dependencies
    for (uint32_t i = 0; i < g_moduleCount; i++) {
        uint32_t visited = 0;
        uint32_t current = i;
        
        while (current < g_moduleCount) {
            if (visited & (1 << current)) {
                printf("ERROR: Circular dependency detected involving module %s\n", 
                       g_moduleRegistry[current].name);
                return false;
            }
            
            visited |= (1 << current);
            current = __builtin_ctz(g_moduleRegistry[current].dependencies);
        }
    }
    
    return true;
}

bool engine_run_diagnostics(void) {
    printf("Running engine diagnostics...\n");
    
    // Check module registry
    if (g_moduleCount == 0) {
        printf("WARNING: No modules registered\n");
    }
    
    // Check for missing required modules
    bool hasCore = false;
    bool hasRenderer = false;
    
    for (uint32_t i = 0; i < g_moduleCount; i++) {
        if (g_moduleRegistry[i].dependencies & ENGINE_DEP_CORE_SYSTEMS) {
            hasCore = true;
        }
        if (g_moduleRegistry[i].dependencies & ENGINE_DEP_RENDERING_BACKEND) {
            hasRenderer = true;
        }
    }
    
    if (!hasCore) {
        printf("ERROR: No core system module found\n");
    }
    
    if (!hasRenderer) {
        printf("ERROR: No rendering backend module found\n");
    }
    
    // Check initialization status
    uint32_t notInitialized = 0;
    for (uint32_t i = 0; i < g_moduleCount; i++) {
        if (!g_moduleRegistry[i].initialized) {
            notInitialized++;
        }
    }
    
    if (notInitialized > 0) {
        printf("WARNING: %u modules not initialized\n", notInitialized);
    }
    
    return true;
}
