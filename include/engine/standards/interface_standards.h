/**
 * @file interface_standards.h
 * @brief Standard interface definitions and conventions for Minecraft v2 Engine
 *
 * This file defines the standard interfaces, conventions, and patterns that all
 * engine subsystems must follow to ensure consistency, maintainability, and
 * proper decoupling between systems.
 */

#ifndef INTERFACE_STANDARDS_H
#define INTERFACE_STANDARDS_H

#include <stdint.h>
#include <stdbool.h>
#include <stddef.h>

#ifdef __cplusplus
extern "C" {
#endif

// ========================================
// Interface Version Information
// ========================================

#define INTERFACE_VERSION_MAJOR 1
#define INTERFACE_VERSION_MINOR 0
#define INTERFACE_VERSION_PATCH 0

#define INTERFACE_VERSION_STRING "1.0.0"

// ========================================
// Standard Return Codes
// ========================================

typedef enum InterfaceResult {
    INTERFACE_SUCCESS = 0,              // Operation completed successfully
    INTERFACE_ERROR_INVALID_PARAMETER = -1, // Invalid parameter provided
    INTERFACE_ERROR_NOT_INITIALIZED = -2,  // Interface not initialized
    INTERFACE_ERROR_ALREADY_INITIALIZED = -3, // Interface already initialized
    INTERFACE_ERROR_OUT_OF_MEMORY = -4,     // Out of memory
    INTERFACE_ERROR_DEPENDENCY_MISSING = -5, // Required dependency missing
    INTERFACE_ERROR_OPERATION_FAILED = -6,  // Operation failed
    INTERFACE_ERROR_TIMEOUT = -7,          // Operation timed out
    INTERFACE_ERROR_NOT_SUPPORTED = -8,     // Operation not supported
    INTERFACE_ERROR_INVALID_STATE = -9,     // Invalid state for operation
    INTERFACE_ERROR_RESOURCE_BUSY = -10     // Resource is busy
} InterfaceResult;

// ========================================
// Standard Configuration Structure
// ========================================

typedef struct SubsystemConfig {
    uint32_t version;                    // Configuration version
    bool enable_logging;                  // Enable logging for subsystem
    bool enable_profiling;                // Enable performance profiling
    bool enable_debug_mode;                // Enable debug features
    uint32_t worker_threads;              // Number of worker threads (0 = auto)
    void *user_data;                      // User-defined configuration data
    size_t user_data_size;                // Size of user data
} SubsystemConfig;

// ========================================
// Standard Status Enumeration
// ========================================

typedef enum SubsystemStatus {
    SUBSYSTEM_STATUS_UNINITIALIZED = 0,   // Subsystem not initialized
    SUBSYSTEM_STATUS_INITIALIZING = 1,     // Subsystem currently initializing
    SUBSYSTEM_STATUS_READY = 2,            // Subsystem ready for use
    SUBSYSTEM_STATUS_RUNNING = 3,           // Subsystem currently running
    SUBSYSTEM_STATUS_PAUSED = 4,           // Subsystem paused
    SUBSYSTEM_STATUS_ERROR = 5,            // Subsystem in error state
    SUBSYSTEM_STATUS_SHUTTING_DOWN = 6,    // Subsystem shutting down
    SUBSYSTEM_STATUS_TERMINATED = 7        // Subsystem terminated
} SubsystemStatus;

// ========================================
// Base Interface Definition
// ========================================

typedef struct IEngineSubsystem {
    // Interface identification
    const char *name;                    // Subsystem name
    const char *version;                 // Interface version
    uint32_t interface_id;               // Unique interface identifier
    
    // Lifecycle management
    InterfaceResult (*initialize)(const SubsystemConfig *config);
    void (*shutdown)(void);
    bool (*is_initialized)(void);
    
    // Update loop
    void (*update)(float dt);
    void (*fixed_update)(float fixed_dt);
    void (*late_update)(float dt);
    
    // Configuration
    InterfaceResult (*set_config)(const SubsystemConfig *config);
    SubsystemConfig (*get_config)(void);
    
    // Dependencies
    bool (*register_dependency)(const char *dependency_name);
    bool (*unregister_dependency)(const char *dependency_name);
    bool (*check_dependencies)(void);
    uint32_t (*get_dependency_count)(void);
    const char* (*get_dependency_name)(uint32_t index);
    
    // Status and diagnostics
    SubsystemStatus (*get_status)(void);
    const char* (*get_error_message)(void);
    InterfaceResult (*get_diagnostics)(char *buffer, size_t buffer_size);
    
    // Resource management
    InterfaceResult (*acquire_resource)(const char *resource_name);
    InterfaceResult (*release_resource)(const char *resource_name);
    bool (*is_resource_available)(const char *resource_name);
    
    // Event handling
    InterfaceResult (*register_event_handler)(uint32_t event_type, void (*handler)(void*, void*));
    InterfaceResult (*unregister_event_handler)(uint32_t event_type);
    InterfaceResult (*send_event)(uint32_t event_type, void *event_data);
    
    // Memory management
    void* (*allocate_memory)(size_t size, const char *tag);
    void (*free_memory)(void *ptr);
    size_t (*get_memory_usage)(void);
    
    // Thread safety
    bool (*is_thread_safe)(void);
    uint32_t (*get_thread_id)(void);
    
    // Performance
    void (*start_performance_profile)(const char *operation);
    void (*end_performance_profile)(const char *operation);
    void (*get_performance_stats)(char *buffer, size_t buffer_size);
    
    // Validation
    bool (*validate_state)(void);
    InterfaceResult (*run_self_tests)(void);
    
    // Private data (implementation-specific)
    void *private_data;
} IEngineSubsystem;

// ========================================
// Standard Service Interface
// ========================================

typedef struct IEngineService {
    const char *name;                    // Service name
    const char *version;                 // Service version
    uint32_t service_id;                  // Unique service identifier
    
    // Service lifecycle
    InterfaceResult (*start)(void);
    InterfaceResult (*stop)(void);
    bool (*is_running)(void);
    
    // Service operations
    InterfaceResult (*execute)(const char *operation, void *input, void *output, size_t output_size);
    bool (*supports_operation)(const char *operation);
    
    // Service discovery
    const char* (*get_capabilities)(void);
    uint32_t (*get_capability_count)(void);
    const char* (*get_capability)(uint32_t index);
    
    // Service health
    bool (*is_healthy)(void);
    InterfaceResult (*health_check)(char *buffer, size_t buffer_size);
    
    // Private data
    void *private_data;
} IEngineService;

// ========================================
// Standard Event Interface
// ========================================

typedef struct IEngineEvent {
    uint32_t event_type;                 // Event type identifier
    uint64_t timestamp;                  // Event timestamp
    uint32_t source_id;                   // Source subsystem ID
    uint32_t target_id;                   // Target subsystem ID (0 = broadcast)
    void *event_data;                     // Event-specific data
    size_t data_size;                     // Size of event data
    bool requires_response;               // Whether event requires response
} IEngineEvent;

typedef struct IEventHandler {
    uint32_t event_type;                 // Event type this handler processes
    InterfaceResult (*handle_event)(const IEngineEvent *event);
    bool (*can_handle)(uint32_t event_type);
    const char* (*get_handler_name)(void);
    void *user_data;
} IEventHandler;

// ========================================
// Standard Resource Interface
// ========================================

typedef struct IEngineResource {
    const char *name;                    // Resource name
    const char *type;                    // Resource type
    uint32_t resource_id;                 // Unique resource identifier
    size_t size;                         // Resource size in bytes
    uint32_t reference_count;             // Reference count
    
    // Resource operations
    InterfaceResult (*load)(const char *path);
    InterfaceResult (*unload)(void);
    InterfaceResult (*reload)(void);
    bool (*is_loaded)(void);
    
    // Resource access
    void* (*get_data)(void);
    const void* (*get_data_const)(void);
    size_t (*get_size)(void);
    
    // Reference counting
    void (*add_reference)(void);
    void (*remove_reference)(void);
    uint32_t (*get_reference_count)(void);
    
    // Resource metadata
    const char* (*get_metadata)(const char *key);
    InterfaceResult (*set_metadata)(const char *key, const char *value);
    
    // Private data
    void *private_data;
} IEngineResource;

// ========================================
// Interface Registration and Discovery
// ========================================

typedef struct InterfaceRegistry {
    // Subsystem registration
    InterfaceResult (*register_subsystem)(IEngineSubsystem *subsystem);
    InterfaceResult (*unregister_subsystem)(const char *name);
    IEngineSubsystem* (*get_subsystem)(const char *name);
    IEngineSubsystem* (*get_subsystem_by_id)(uint32_t id);
    
    // Service registration
    InterfaceResult (*register_service)(IEngineService *service);
    InterfaceResult (*unregister_service)(const char *name);
    IEngineService* (*get_service)(const char *name);
    
    // Resource registration
    InterfaceResult (*register_resource)(IEngineResource *resource);
    InterfaceResult (*unregister_resource)(const char *name);
    IEngineResource* (*get_resource)(const char *name);
    
    // Discovery
    uint32_t (*get_subsystem_count)(void);
    uint32_t (*get_service_count)(void);
    uint32_t (*get_resource_count)(void);
    
    // Validation
    InterfaceResult (*validate_all_interfaces)(void);
    InterfaceResult (*check_dependencies)(void);
    
    // Private data
    void *private_data;
} InterfaceRegistry;

// ========================================
// Interface Validation Functions
// ========================================

/**
 * Validate interface compliance
 * @param interface Pointer to interface to validate
 * @return True if interface complies with standards
 */
bool validate_interface_compliance(const IEngineSubsystem *interface);

/**
 * Validate interface naming conventions
 * @param name Interface name to validate
 * @return True if name follows conventions
 */
bool validate_interface_name(const char *name);

/**
 * Validate interface version compatibility
 * @param version Version string to validate
 * @return True if version is compatible
 */
bool validate_interface_version(const char *version);

/**
 * Generate interface documentation
 * @param interface Pointer to interface
 * @param buffer Output buffer for documentation
 * @param buffer_size Size of output buffer
 * @return True if documentation generated successfully
 */
bool generate_interface_documentation(const IEngineSubsystem *interface, 
                                       char *buffer, 
                                       size_t buffer_size);

// ========================================
// Global Interface Registry Access
// ========================================

/**
 * Get global interface registry
 * @return Pointer to global interface registry
 */
InterfaceRegistry* get_interface_registry(void);

/**
 * Initialize interface registry
 * @return True if initialization successful
 */
bool initialize_interface_registry(void);

/**
 * Shutdown interface registry
 */
void shutdown_interface_registry(void);

// ========================================
// Interface Macros for Standards Compliance
// ========================================

#define DECLARE_INTERFACE(InterfaceName, InterfaceID) \
    static const char* InterfaceName##_NAME = #InterfaceName; \
    static const char* InterfaceName##_VERSION = INTERFACE_VERSION_STRING; \
    static const uint32_t InterfaceName##_ID = InterfaceID;

#define IMPLEMENT_INTERFACE_BASE(InterfaceName) \
    .name = InterfaceName##_NAME, \
    .version = InterfaceName##_VERSION, \
    .interface_id = InterfaceName##_ID

#define VALIDATE_INTERFACE_PARAMS(interface, config) \
    do { \
        if (!interface) return INTERFACE_ERROR_INVALID_PARAMETER; \
        if (!validate_interface_compliance(interface)) return INTERFACE_ERROR_INVALID_STATE; \
        if (config && config->version != 1) return INTERFACE_ERROR_INVALID_PARAMETER; \
    } while(0)

#define INTERFACE_LOG_DEBUG(subsystem, format, ...) \
    do { \
        if (g_interface_debug_enabled) { \
            printf("[DEBUG] [%s] " format "\n", subsystem, ##__VA_ARGS__); \
        } \
    } while(0)

#define INTERFACE_LOG_ERROR(subsystem, format, ...) \
    printf("[ERROR] [%s] " format "\n", subsystem, ##__VA_ARGS__)

#define INTERFACE_LOG_INFO(subsystem, format, ...) \
    printf("[INFO] [%s] " format "\n", subsystem, ##__VA_ARGS__)

// ========================================
// Standard Interface Creation Macros
// ========================================

#define CREATE_INTERFACE(InterfaceName, PrivateData) \
    static IEngineSubsystem* create_##InterfaceName##_interface(void) { \
        IEngineSubsystem *interface = calloc(1, sizeof(IEngineSubsystem)); \
        if (!interface) return NULL; \
        PrivateData *private_data = calloc(1, sizeof(PrivateData)); \
        if (!private_data) { \
            free(interface); \
            return NULL; \
        } \
        interface->private_data = private_data; \
        return interface; \
    }

#define DESTROY_INTERFACE(InterfaceName) \
    static void destroy_##InterfaceName##_interface(IEngineSubsystem *interface) { \
        if (interface && interface->private_data) { \
            free(interface->private_data); \
            free(interface); \
        } \
    }

#ifdef __cplusplus
}
#endif

#endif /* INTERFACE_STANDARDS_H */
