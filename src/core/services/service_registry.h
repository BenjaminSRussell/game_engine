/**
 * @file service_registry.h
 * @brief Service registration and discovery system
 *
 * Provides centralized service registration, discovery, and lifecycle management
 * for all engine subsystems using the dependency injection container.
 */

#ifndef SERVICE_REGISTRY_H
#define SERVICE_REGISTRY_H

#include "include/engine/standards/interface_standards.h"
#include "src/engine/dependency_injection/di_container.h"

#ifdef __cplusplus
extern "C" {
#endif

// ========================================
// Service Registry Functions
// ========================================

/**
 * Initialize service registry
 * @param dependency_capacity Capacity for dependency table
 * @param service_capacity Capacity for service table
 * @return True if initialization successful
 */
bool service_registry_init(uint32_t dependency_capacity, uint32_t service_capacity);

/**
 * Shutdown service registry
 * Stops all services and cleans up resources
 */
void service_registry_shutdown(void);

/**
 * Register a service with the registry
 * @param service Service instance to register
 * @param auto_start Whether to auto-start the service
 * @param priority Service startup priority (lower = earlier)
 * @return True if registration successful
 */
bool service_registry_register_service(IEngineService *service, bool auto_start, uint32_t priority);

/**
 * Unregister a service from the registry
 * @param name Service name to unregister
 * @return True if unregistration successful
 */
bool service_registry_unregister_service(const char *name);

/**
 * Get a service by name
 * @param name Service name to retrieve
 * @return Service instance or NULL if not found
 */
IEngineService* service_registry_get_service(const char *name);

/**
 * Check if a service is registered
 * @param name Service name to check
 * @return True if service is registered
 */
bool service_registry_is_service_registered(const char *name);

/**
 * Check if a service is running
 * @param name Service name to check
 * @return True if service is running
 */
bool service_registry_is_service_running(const char *name);

// ========================================
// Service Lifecycle Management
// ========================================

/**
 * Start a specific service
 * @param name Service name to start
 * @return True if service started successfully
 */
bool service_registry_start_service(const char *name);

/**
 * Stop a specific service
 * @param name Service name to stop
 * @return True if service stopped successfully
 */
bool service_registry_stop_service(const char *name);

/**
 * Start all registered services
 * @return True if all services started successfully
 */
bool service_registry_start_all_services(void);

/**
 * Stop all registered services
 * @return True if all services stopped successfully
 */
bool service_registry_stop_all_services(void);

// ========================================
// Service Information
// ========================================

/**
 * Get total number of registered services
 * @return Number of registered services
 */
uint32_t service_registry_get_service_count(void);

/**
 * Get number of running services
 * @return Number of running services
 */
uint32_t service_registry_get_running_count(void);

/**
 * List all registered services
 */
void service_registry_list_services(void);

// ========================================
// Service Health Monitoring
// ========================================

/**
 * Check health of a specific service
 * @param name Service name to check
 * @param buffer Output buffer for health information
 * @param buffer_size Size of output buffer
 * @return True if service is healthy
 */
bool service_registry_check_service_health(const char *name, char *buffer, size_t buffer_size);

/**
 * Check health of all services
 * @param buffer Output buffer for health report
 * @param buffer_size Size of output buffer
 * @return True if all services are healthy
 */
bool service_registry_check_all_services_health(char *buffer, size_t buffer_size);

// ========================================
// Service Capabilities
// ========================================

/**
 * Get capabilities of a service
 * @param name Service name
 * @param buffer Output buffer for capabilities
 * @param buffer_size Size of output buffer
 * @return True if capabilities retrieved successfully
 */
bool service_registry_get_service_capabilities(const char *name, char *buffer, size_t buffer_size);

// ========================================
// Performance Monitoring
// ========================================

/**
 * Get performance statistics for the registry
 * @param buffer Output buffer for statistics
 * @param buffer_size Size of output buffer
 */
void service_registry_get_performance_stats(char *buffer, size_t buffer_size);

// ========================================
// Dependency Management
// ========================================

/**
 * Resolve all service dependencies
 * @return True if all dependencies are satisfied
 */
bool service_registry_resolve_dependencies(void);

// ========================================
// Registry Validation
// ========================================

/**
 * Validate the service registry
 * @return True if registry is valid
 */
bool service_registry_validate_registry(void);

// ========================================
// Registry Utilities
// ========================================

/**
 * Get the dependency injection container
 * @return DI container instance
 */
DIContainer* service_registry_get_container(void);

/**
 * Check if service registry is initialized
 * @return True if registry is initialized
 */
bool service_registry_is_initialized(void);

/**
 * Print registry statistics
 */
void service_registry_print_statistics(void);

// ========================================
// Service Registration Macros
// ========================================

#define REGISTER_SERVICE(service, auto_start, priority) \
    service_registry_register_service(service, auto_start, priority)

#define UNREGISTER_SERVICE(name) \
    service_registry_unregister_service(name)

#define GET_SERVICE(name) \
    service_registry_get_service(name)

#define IS_SERVICE_REGISTERED(name) \
    service_registry_is_service_registered(name)

#define IS_SERVICE_RUNNING(name) \
    service_registry_is_service_running(name)

#define START_SERVICE(name) \
    service_registry_start_service(name)

#define STOP_SERVICE(name) \
    service_registry_stop_service(name)

#define START_ALL_SERVICES() \
    service_registry_start_all_services()

#define STOP_ALL_SERVICES() \
    service_registry_stop_all_services()

#define CHECK_SERVICE_HEALTH(name, buffer, size) \
    service_registry_check_service_health(name, buffer, size)

#define GET_SERVICE_CAPABILITIES(name, buffer, size) \
    service_registry_get_service_capabilities(name, buffer, size)

// ========================================
// Service Lifecycle Macros
// ========================================

#define SERVICE_LIFECYCLE_START(name) \
    do { \
        if (!service_registry_start_service(name)) { \
            printf("Failed to start service: %s\n", name); \
        } \
    } while(0)

#define SERVICE_LIFECYCLE_STOP(name) \
    do { \
        if (!service_registry_stop_service(name)) { \
            printf("Failed to stop service: %s\n", name); \
        } \
    } while(0)

#define SERVICE_LIFECYCLE_AUTO_START(service, priority) \
    REGISTER_SERVICE(service, true, priority)

#define SERVICE_LIFECYCLE_MANUAL_START(service, priority) \
    REGISTER_SERVICE(service, false, priority)

// ========================================
// Service Health Macros
// ========================================

#define SERVICE_HEALTH_CHECK(name, buffer, size) \
    do { \
        if (!service_registry_check_service_health(name, buffer, size)) { \
            printf("Service health check failed: %s\n", name); \
        } \
    } while(0)

#define SERVICE_HEALTH_CHECK_ALL(buffer, size) \
    do { \
        if (!service_registry_check_all_services_health(buffer, size)) { \
            printf("Some services are unhealthy\n"); \
        } \
    } while(0)

// ========================================
// Service Discovery Macros
// ========================================

#define SERVICE_DISCOVER(type, name) \
    (type*)service_registry_get_service(name)

#define SERVICE_DISCOVER_SAFE(type, name) \
    ((type*)service_registry_get_service(name))

#define SERVICE_DISCOVER_OR_NULL(type, name) \
    ((type*)service_registry_get_service(name))

// ========================================
// Service Capability Macros
// ========================================

#define SERVICE_HAS_CAPABILITY(name, capability) \
    do { \
        char buffer[256]; \
        if (service_registry_get_service_capabilities(name, buffer, sizeof(buffer))) { \
            if (strstr(buffer, capability)) { \
                /* Service has capability */ \
            } \
        } \
    } while(0)

// ========================================
// Service Validation Macros
// ========================================

#define SERVICE_VALIDATE_REGISTRY() \
    do { \
        if (!service_registry_validate_registry()) { \
            printf("Service registry validation failed\n"); \
        } \
    } while(0)

#define SERVICE_VALIDATE_DEPENDENCIES() \
    do { \
        if (!service_registry_resolve_dependencies()) { \
            printf("Service dependency resolution failed\n"); \
        } \
    } while(0)

// ========================================
// Service Performance Macros
// ========================================

#define SERVICE_GET_PERFORMANCE_STATS(buffer, size) \
    service_registry_get_performance_stats(buffer, size)

#define SERVICE_PRINT_STATS() \
    service_registry_print_statistics()

// ========================================
// Service Registry Initialization Macros
// ========================================

#define SERVICE_REGISTRY_INIT(dep_capacity, service_capacity) \
    service_registry_init(dep_capacity, service_capacity)

#define SERVICE_REGISTRY_SHUTDOWN() \
    service_registry_shutdown()

#define SERVICE_REGISTRY_IS_READY() \
    service_registry_is_initialized()

// ========================================
// Service Registry Debugging Macros
// ========================================

#define SERVICE_REGISTRY_DEBUG_LIST() \
    service_registry_list_services()

#define SERVICE_REGISTRY_DEBUG_STATS() \
    service_registry_print_statistics()

#define SERVICE_REGISTRY_DEBUG_VALIDATE() \
    SERVICE_VALIDATE_REGISTRY()

// ========================================
// Service Registry Error Handling Macros
// ========================================

#define SERVICE_REGISTRY_ERROR_CHECK(operation) \
    do { \
        if (!(operation)) { \
            printf("Service registry operation failed: %s\n", #operation); \
        } \
    } while(0)

#define SERVICE_REGISTRY_ERROR_CHECK_RETURN(operation) \
    do { \
        if (!(operation)) { \
            printf("Service registry operation failed: %s\n", #operation); \
            return false; \
        } \
    } while(0)

// ========================================
// Service Registry Logging Macros
// ========================================

#define SERVICE_REGISTRY_LOG(level, format, ...) \
    do { \
        printf("[%s][SERVICE_REGISTRY] " format "\n", level, ##__VA_ARGS__); \
    } while(0)

#define SERVICE_REGISTRY_LOG_DEBUG(format, ...) \
    SERVICE_REGISTRY_LOG("DEBUG", format, ##__VA_ARGS__)

#define SERVICE_REGISTRY_LOG_INFO(format, ...) \
    SERVICE_REGISTRY_LOG("INFO", format, ##__VA_ARGS__)

#define SERVICE_REGISTRY_LOG_WARNING(format, ...) \
    SERVICE_REGISTRY_LOG("WARNING", format, ##__VA_ARGS__)

#define SERVICE_REGISTRY_LOG_ERROR(format, ...) \
    SERVICE_REGISTRY_LOG("ERROR", format, ##__VA_ARGS__)

#ifdef __cplusplus
}
#endif

#endif /* SERVICE_REGISTRY_H */
