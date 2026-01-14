// Engine Initialization Pipeline - Comprehensive System Startup
// ENGINE-INIT-001: System initialization order and dependencies
// ENGINE-INIT-002: Module registration and discovery
// ENGINE-INIT-003: Configuration loading and validation
// ENGINE-INIT-004: Resource initialization and allocation
// ENGINE-INIT-005: Service startup and health checking
// ENGINE-INIT-006: Error handling and recovery
// ENGINE-INIT-007: Performance monitoring and profiling
// ENGINE-INIT-008: Cleanup and shutdown procedures

#ifndef ENGINE_INIT_H
#define ENGINE_INIT_H

#include <common.h>
#include <stdbool.h>
#include <stdint.h>

#ifdef __cplusplus
extern "C" {
#endif

// System initialization phases
typedef enum {
    ENGINE_INIT_PHASE_PRE_INIT = 0,
    ENGINE_INIT_PHASE_CORE_SYSTEMS,
    ENGINE_INIT_PHASE_RENDERING_BACKEND,
    ENGINE_INIT_PHASE_AUDIO_SYSTEM,
    ENGINE_INIT_PHASE_INPUT_SYSTEM,
    ENGINE_INIT_PHASE_ASSET_SYSTEM,
    ENGINE_INIT_PHASE_SCRIPT_SYSTEM,
    ENGINE_INIT_PHASE_NETWORK_SYSTEM,
    ENGINE_INIT_PHASE_PHYSICS_SYSTEM,
    ENGINE_INIT_PHASE_UI_SYSTEM,
    ENGINE_INIT_PHASE_GAME_SYSTEMS,
    ENGINE_INIT_PHASE_POST_INIT,
    ENGINE_INIT_PHASE_READY,
    ENGINE_INIT_PHASE_ERROR,
    ENGINE_INIT_PHASE_COUNT
} EngineInitPhase;

// Initialization status
typedef enum {
    ENGINE_INIT_STATUS_NOT_STARTED = 0,
    ENGINE_INIT_STATUS_IN_PROGRESS,
    ENGINE_INIT_STATUS_SUCCESS,
    ENGINE_INIT_STATUS_WARNING,
    ENGINE_INIT_STATUS_ERROR,
    ENGINE_INIT_STATUS_COUNT
} EngineInitStatus;

// Module initialization callback
typedef bool (*ModuleInitFunc)(void* config);
typedef void (*ModuleShutdownFunc)(void);
typedef const char* (*ModuleGetNameFunc)(void);
typedef const char* (*ModuleGetVersionFunc)(void);

// Module descriptor
typedef struct {
    const char* name;
    const char* version;
    ModuleInitFunc init;
    ModuleShutdownFunc shutdown;
    ModuleGetNameFunc get_name;
    ModuleGetVersionFunc get_version;
    uint32_t priority;           // Initialization priority (0 = highest)
    bool required;               // Must succeed for engine to start
    bool initialized;            // Internal state
    void* config;               // Module-specific configuration
    EngineInitStatus status;      // Last initialization status
    double init_time;            // Time taken to initialize (ms)
    uint32_t dependencies;       // Bit mask of dependencies
    uint32_t dependents;        // Bit mask of modules that depend on this
} EngineModule;

// System configuration
typedef struct {
    // Engine settings
    const char* app_name;
    const char* app_version;
    uint32_t target_fps;
    bool enable_vsync;
    bool enable_multithreading;
    uint32_t worker_threads;
    
    // Rendering settings
    const char* preferred_renderer;
    uint32_t window_width;
    uint32_t window_height;
    bool fullscreen;
    uint32_t msaa_samples;
    
    // Audio settings
    uint32_t audio_sample_rate;
    uint32_t audio_buffer_size;
    uint32_t max_audio_channels;
    
    // Resource settings
    uint64_t texture_memory_budget;
    uint64_t mesh_memory_budget;
    uint64_t audio_memory_budget;
    bool enable_streaming;
    
    // Debug settings
    bool enable_validation;
    bool enable_profiling;
    bool enable_debug_rendering;
    const char* log_level;
} EngineConfig;

// Initialization context
typedef struct {
    EngineConfig config;
    EngineModule modules[64];
    uint32_t module_count;
    EngineInitPhase current_phase;
    EngineInitStatus overall_status;
    double start_time;
    double total_init_time;
    uint32_t successful_inits;
    uint32_t failed_inits;
    uint32_t warning_count;
    char error_buffer[1024];
    bool abort_requested;
} EngineInitContext;

// Initialization events
typedef enum {
    ENGINE_EVENT_MODULE_REGISTERED,
    ENGINE_EVENT_MODULE_INITIALIZED,
    ENGINE_EVENT_MODULE_FAILED,
    ENGINE_EVENT_PHASE_STARTED,
    ENGINE_EVENT_PHASE_COMPLETED,
    ENGINE_EVENT_SYSTEM_READY,
    ENGINE_EVENT_SYSTEM_ERROR,
    ENGINE_EVENT_SHUTDOWN_STARTED,
    ENGINE_EVENT_SHUTDOWN_COMPLETED
} EngineEvent;

// Event data
typedef struct {
    EngineEvent type;
    const char* module_name;
    EngineInitStatus status;
    double timestamp;
    const char* message;
    uint32_t error_code;
} EngineEventData;

// Event callback
typedef void (*EngineEventCallback)(const EngineEventData* event);

// Performance metrics
typedef struct {
    uint32_t total_modules;
    uint32_t initialized_modules;
    uint32_t failed_modules;
    uint32_t warning_modules;
    double total_init_time;
    double average_init_time;
    uint64_t memory_allocated;
    uint64_t memory_used;
    uint32_t cpu_usage_percent;
    uint32_t gpu_usage_percent;
} EngineMetrics;

// Global initialization context
extern EngineInitContext g_engineInit;

// Core initialization functions
bool engine_init(const EngineConfig* config);
void engine_shutdown(void);
bool engine_is_initialized(void);
bool engine_is_ready(void);

// Module management
bool engine_register_module(const EngineModule* module);
bool engine_unregister_module(const char* module_name);
EngineModule* engine_get_module(const char* module_name);
EngineModule* engine_get_all_modules(uint32_t* count);

// Phase management
EngineInitPhase engine_get_current_phase(void);
EngineInitStatus engine_get_phase_status(EngineInitPhase phase);
bool engine_advance_phase(void);
bool engine_wait_for_phase(EngineInitPhase phase, uint32_t timeout_ms);

// Configuration
const EngineConfig* engine_get_config(void);
bool engine_update_config(const EngineConfig* config);
bool engine_load_config_from_file(const char* config_path);
bool engine_save_config_to_file(const char* config_path);

// Event system
bool engine_register_event_callback(EngineEventCallback callback);
void engine_unregister_event_callback(EngineEventCallback callback);
void engine_trigger_event(const EngineEventData* event);
void engine_process_events(void);

// Metrics and monitoring
void engine_get_metrics(EngineMetrics* metrics);
void engine_reset_metrics(void);
bool engine_enable_profiling(bool enable);
bool engine_is_profiling_enabled(void);

// Error handling
void engine_set_error(const char* error);
const char* engine_get_last_error(void);
void engine_clear_error(void);

// Utilities
const char* engine_get_init_phase_string(EngineInitPhase phase);
const char* engine_get_status_string(EngineInitStatus status);
void engine_print_module_info(void);
void engine_print_init_summary(void);

// Validation and diagnostics
bool engine_validate_configuration(const EngineConfig* config);
bool engine_validate_module_dependencies(void);
bool engine_run_diagnostics(void);

// Module dependency flags
#define ENGINE_DEP_CORE_SYSTEMS      (1 << 0)
#define ENGINE_DEP_RENDERING_BACKEND  (1 << 1)
#define ENGINE_DEP_AUDIO_SYSTEM       (1 << 2)
#define ENGINE_DEP_INPUT_SYSTEM        (1 << 3)
#define ENGINE_DEP_ASSET_SYSTEM        (1 << 4)
#define ENGINE_DEP_SCRIPT_SYSTEM       (1 << 5)
#define ENGINE_DEP_NETWORK_SYSTEM      (1 << 6)
#define ENGINE_DEP_PHYSICS_SYSTEM     (1 << 7)
#define ENGINE_DEP_UI_SYSTEM           (1 << 8)
#define ENGINE_DEP_GAME_SYSTEMS       (1 << 9)

// Built-in module names
#define ENGINE_MODULE_CORE            "core"
#define ENGINE_MODULE_RENDERER         "renderer"
#define ENGINE_MODULE_AUDIO            "audio"
#define ENGINE_MODULE_INPUT            "input"
#define ENGINE_MODULE_ASSETS           "assets"
#define ENGINE_MODULE_SCRIPT           "script"
#define ENGINE_MODULE_NETWORK          "network"
#define ENGINE_MODULE_PHYSICS         "physics"
#define ENGINE_MODULE_UI               "ui"
#define ENGINE_MODULE_GAME             "game"

#ifdef __cplusplus
}
#endif

#endif // ENGINE_INIT_H
