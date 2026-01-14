/**
 * @file service_registry.c
 * @brief Service registration and discovery system
 *
 * Provides centralized service registration, discovery, and lifecycle management
 * for all engine subsystems using the dependency injection container.
 */

#include "service_registry.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <pthread.h>

// ========================================
// Internal Structures
// ========================================

typedef struct ServiceRegistration {
    char name[64];                      // Service name
    IEngineService *service;              // Service instance
    bool auto_start;                      // Whether to auto-start on registration
    uint32_t priority;                    // Service startup priority
    struct ServiceRegistration *next;      // Next registration in list
} ServiceRegistration;

typedef struct ServiceRegistry {
    ServiceRegistration *services;        // Linked list of registered services
    DIContainer *container;                // Dependency injection container
    pthread_mutex_t mutex;               // Thread safety mutex
    bool is_initialized;                  // Initialization state
    uint32_t service_count;               // Number of registered services
    uint32_t running_count;               // Number of running services
} ServiceRegistry;

// ========================================
// Global Registry Instance
// ========================================

static ServiceRegistry g_service_registry = {0};
static bool g_registry_initialized = false;

// ========================================
// Registry Initialization
// ========================================

bool service_registry_init(uint32_t dependency_capacity, uint32_t service_capacity) {
    if (g_registry_initialized) {
        return false;
    }
    
    // Initialize dependency injection container
    g_service_registry.container = di_container_create(dependency_capacity, service_capacity);
    if (!g_service_registry.container) {
        return false;
    }
    
    // Initialize mutex
    if (pthread_mutex_init(&g_service_registry.mutex, NULL) != 0) {
        di_container_destroy(g_service_registry.container);
        return false;
    }
    
    g_service_registry.is_initialized = true;
    
    printf("Service registry initialized\n");
    return true;
}

void service_registry_shutdown(void) {
    if (!g_registry_initialized) {
        return;
    }
    
    pthread_mutex_lock(&g_service_registry.mutex);
    
    // Stop all running services
    ServiceRegistration *current = g_service_registry.services;
    while (current) {
        if (current->service && current->service->is_running()) {
            current->service->stop();
        }
        current = current->next;
    }
    
    // Destroy all services
    current = g_service_registry.services;
    while (current) {
        ServiceRegistration *next = current->next;
        if (current->service) {
            free(current->service);
        }
        free(current);
        current = next;
    }
    
    pthread_mutex_unlock(&g_service_registry.mutex);
    
    // Destroy container and mutex
    di_container_destroy(g_service_registry.container);
    pthread_mutex_destroy(&g_service_registry.mutex);
    
    memset(&g_service_registry, 0, sizeof(g_service_registry));
    g_registry_initialized = false;
    
    printf("Service registry shutdown complete\n");
}

// ========================================
// Service Registration
// ========================================

bool service_registry_register_service(IEngineService *service, bool auto_start, uint32_t priority) {
    if (!g_registry_initialized || !service || !service->name) {
        return false;
    }
    
    pthread_mutex_lock(&g_service_registry.mutex);
    
    // Check if service already exists
    ServiceRegistration *current = g_service_registry.services;
    while (current) {
        if (strcmp(current->name, service->name) == 0) {
            pthread_mutex_unlock(&g_service_registry.mutex);
            printf("Service '%s' already registered\n", service->name);
            return false;
        }
        current = current->next;
    }
    
    // Register with DI container
    if (!di_container_register_service(g_service_registry.container, service)) {
        pthread_mutex_unlock(&g_service_registry.mutex);
        printf("Failed to register service '%s' with DI container\n", service->name);
        return false;
    }
    
    // Create registration entry
    ServiceRegistration *registration = malloc(sizeof(ServiceRegistration));
    if (!registration) {
        pthread_mutex_unlock(&g_service_registry.mutex);
        return false;
    }
    
    strncpy(registration->name, service->name, sizeof(registration->name) - 1);
    registration->service = service;
    registration->auto_start = auto_start;
    registration->priority = priority;
    
    // Insert into list (sorted by priority)
    if (!g_service_registry.services || priority < g_service_registry.services->priority) {
        registration->next = g_service_registry.services;
        g_service_registry.services = registration;
    } else {
        current = g_service_registry.services;
        while (current->next && current->next->priority <= priority) {
            current = current->next;
        }
        registration->next = current->next;
        current->next = registration;
    }
    
    g_service_registry.service_count++;
    
    // Auto-start if requested
    if (auto_start) {
        if (service->start && service->start() == INTERFACE_SUCCESS) {
            g_service_registry.running_count++;
            printf("Service '%s' started automatically\n", service->name);
        } else {
            printf("Failed to auto-start service '%s'\n", service->name);
        }
    }
    
    pthread_mutex_unlock(&g_service_registry.mutex);
    
    printf("Service '%s' registered successfully\n", service->name);
    return true;
}

bool service_registry_unregister_service(const char *name) {
    if (!g_registry_initialized || !name) {
        return false;
    }
    
    pthread_mutex_lock(&g_service_registry.mutex);
    
    ServiceRegistration *current = g_service_registry.services;
    ServiceRegistration *prev = NULL;
    
    while (current) {
        if (strcmp(current->name, name) == 0) {
            // Stop service if running
            if (current->service && current->service->is_running()) {
                current->service->stop();
                g_service_registry.running_count--;
            }
            
            // Remove from list
            if (prev) {
                prev->next = current->next;
            } else {
                g_service_registry.services = current->next;
            }
            
            // Unregister from DI container
            di_container_unregister_service(g_service_registry.container, name);
            
            // Free registration
            free(current);
            g_service_registry.service_count--;
            
            pthread_mutex_unlock(&g_service_registry.mutex);
            printf("Service '%s' unregistered successfully\n", name);
            return true;
        }
        prev = current;
        current = current->next;
    }
    
    pthread_mutex_unlock(&g_service_registry.mutex);
    printf("Service '%s' not found for unregistration\n", name);
    return false;
}

// ========================================
// Service Discovery
// ========================================

IEngineService* service_registry_get_service(const char *name) {
    if (!g_registry_initialized || !name) {
        return NULL;
    }
    
    return di_container_get_service(g_service_registry.container, name);
}

bool service_registry_is_service_registered(const char *name) {
    if (!g_registry_initialized || !name) {
        return false;
    }
    
    return service_registry_get_service(name) != NULL;
}

bool service_registry_is_service_running(const char *name) {
    if (!g_registry_initialized || !name) {
        return false;
    }
    
    IEngineService *service = service_registry_get_service(name);
    return service ? service->is_running() : false;
}

// ========================================
// Service Lifecycle Management
// ========================================

bool service_registry_start_service(const char *name) {
    if (!g_registry_initialized || !name) {
        return false;
    }
    
    pthread_mutex_lock(&g_service_registry.mutex);
    
    IEngineService *service = service_registry_get_service(name);
    if (!service) {
        pthread_mutex_unlock(&g_service_registry.mutex);
        return false;
    }
    
    if (service->is_running()) {
        pthread_mutex_unlock(&g_service_registry.mutex);
        return true; // Already running
    }
    
    bool success = (service->start && service->start() == INTERFACE_SUCCESS);
    if (success) {
        g_service_registry.running_count++;
    }
    
    pthread_mutex_unlock(&g_service_registry.mutex);
    
    printf("Service '%s' %s\n", name, success ? "started" : "failed to start");
    return success;
}

bool service_registry_stop_service(const char *name) {
    if (!g_registry_initialized || !name) {
        return false;
    }
    
    pthread_mutex_lock(&g_service_registry.mutex);
    
    IEngineService *service = service_registry_get_service(name);
    if (!service) {
        pthread_mutex_unlock(&g_service_registry.mutex);
        return false;
    }
    
    if (!service->is_running()) {
        pthread_mutex_unlock(&g_service_registry.mutex);
        return true; // Already stopped
    }
    
    bool success = (service->stop && service->stop() == INTERFACE_SUCCESS);
    if (success) {
        g_service_registry.running_count--;
    }
    
    pthread_mutex_unlock(&g_service_registry.mutex);
    
    printf("Service '%s' %s\n", name, success ? "stopped" : "failed to stop");
    return success;
}

bool service_registry_start_all_services(void) {
    if (!g_registry_initialized) {
        return false;
    }
    
    pthread_mutex_lock(&g_service_registry.mutex);
    
    bool all_success = true;
    ServiceRegistration *current = g_service_registry.services;
    
    while (current) {
        if (current->service && !current->service->is_running()) {
            if (current->service->start && current->service->start() == INTERFACE_SUCCESS) {
                g_service_registry.running_count++;
                printf("Service '%s' started\n", current->name);
            } else {
                printf("Failed to start service '%s'\n", current->name);
                all_success = false;
            }
        }
        current = current->next;
    }
    
    pthread_mutex_unlock(&g_service_registry.mutex);
    
    printf("All services start %s\n", all_success ? "successful" : "completed with failures");
    return all_success;
}

bool service_registry_stop_all_services(void) {
    if (!g_registry_initialized) {
        return false;
    }
    
    pthread_mutex_lock(&g_service_registry.mutex);
    
    bool all_success = true;
    ServiceRegistration *current = g_service_registry.services;
    
    while (current) {
        if (current->service && current->service->is_running()) {
            if (current->service->stop && current->service->stop() == INTERFACE_SUCCESS) {
                g_service_registry.running_count--;
                printf("Service '%s' stopped\n", current->name);
            } else {
                printf("Failed to stop service '%s'\n", current->name);
                all_success = false;
            }
        }
        current = current->next;
    }
    
    pthread_mutex_unlock(&g_service_registry.mutex);
    
    printf("All services stop %s\n", all_success ? "successful" : "completed with failures");
    return all_success;
}

// ========================================
// Service Information
// ========================================

uint32_t service_registry_get_service_count(void) {
    if (!g_registry_initialized) {
        return 0;
    }
    
    pthread_mutex_lock(&g_service_registry.mutex);
    uint32_t count = g_service_registry.service_count;
    pthread_mutex_unlock(&g_service_registry.mutex);
    
    return count;
}

uint32_t service_registry_get_running_count(void) {
    if (!g_registry_initialized) {
        return 0;
    }
    
    pthread_mutex_lock(&g_service_registry.mutex);
    uint32_t count = g_service_registry.running_count;
    pthread_mutex_unlock(&g_service_registry.mutex);
    
    return count;
}

void service_registry_list_services(void) {
    if (!g_registry_initialized) {
        printf("Service registry not initialized\n");
        return;
    }
    
    pthread_mutex_lock(&g_service_registry.mutex);
    
    printf("\n=== Registered Services ===\n");
    printf("Total Services: %u\n", g_service_registry.service_count);
    printf("Running Services: %u\n", g_service_registry.running_count);
    printf("\n");
    
    ServiceRegistration *current = g_service_registry.services;
    while (current) {
        printf("  %-20s %-10s %-10s %-10u\n",
               current->name,
               current->service ? "Valid" : "Invalid",
               current->service && current->service->is_running() ? "Running" : "Stopped",
               current->priority);
        current = current->next;
    }
    
    printf("============================\n\n");
    
    pthread_mutex_unlock(&g_service_registry.mutex);
}

// ========================================
// Service Health Monitoring
// ========================================

bool service_registry_check_service_health(const char *name, char *buffer, size_t buffer_size) {
    if (!g_registry_initialized || !name || !buffer || buffer_size == 0) {
        return false;
    }
    
    IEngineService *service = service_registry_get_service(name);
    if (!service) {
        snprintf(buffer, buffer_size, "Service '%s' not found", name);
        return false;
    }
    
    if (!service->is_running()) {
        snprintf(buffer, buffer_size, "Service '%s' is not running", name);
        return false;
    }
    
    if (!service->is_healthy()) {
        snprintf(buffer, buffer_size, "Service '%s' is unhealthy", name);
        return false;
    }
    
    if (service->health_check) {
        InterfaceResult result = service->health_check(buffer, buffer_size);
        return result == INTERFACE_SUCCESS;
    }
    
    snprintf(buffer, buffer_size, "Service '%s' is healthy", name);
    return true;
}

bool service_registry_check_all_services_health(char *buffer, size_t buffer_size) {
    if (!g_registry_initialized || !buffer || buffer_size == 0) {
        return false;
    }
    
    pthread_mutex_lock(&g_service_registry.mutex);
    
    bool all_healthy = true;
    int written = snprintf(buffer, buffer_size, "=== Service Health Report ===\n");
    
    ServiceRegistration *current = g_service_registry.services;
    while (current && written < buffer_size - 1) {
        char health_buffer[256];
        bool is_healthy = service_registry_check_service_health(current->name, health_buffer, sizeof(health_buffer));
        
        written += snprintf(buffer + written, buffer_size - written,
                          "%-20s: %s\n",
                          current->name,
                          is_healthy ? " Healthy" : " Unhealthy");
        
        if (!is_healthy) {
            all_healthy = false;
        }
        
        current = current->next;
    }
    
    written += snprintf(buffer + written, buffer_size - written,
                      "Overall Health: %s\n",
                      all_healthy ? "HEALTHY" : "UNHEALTHY");
    
    pthread_mutex_unlock(&g_service_registry.mutex);
    
    return all_healthy;
}

// ========================================
// Service Capabilities
// ========================================

bool service_registry_get_service_capabilities(const char *name, char *buffer, size_t buffer_size) {
    if (!g_registry_initialized || !name || !buffer || buffer_size == 0) {
        return false;
    }
    
    IEngineService *service = service_registry_get_service(name);
    if (!service || !service->get_capabilities) {
        return false;
    }
    
    const char *capabilities = service->get_capabilities();
    if (!capabilities) {
        return false;
    }
    
    strncpy(buffer, capabilities, buffer_size - 1);
    buffer[buffer_size - 1] = '\0';
    
    return true;
}

// ========================================
// Performance Monitoring
// ========================================

void service_registry_get_performance_stats(char *buffer, size_t buffer_size) {
    if (!g_registry_initialized || !buffer || buffer_size == 0) {
        return;
    }
    
    pthread_mutex_lock(&g_service_registry.mutex);
    
    int written = snprintf(buffer, buffer_size,
            "=== Service Registry Performance ===\n"
            "Total Services: %u\n"
            "Running Services: %u\n"
            "Stopped Services: %u\n"
            "Registry Uptime: %.2f seconds\n",
            g_service_registry.service_count,
            g_service_registry.running_count,
            g_service_registry.service_count - g_service_registry.running_count,
            0.0); // TODO: Add uptime tracking
    
    // Add individual service performance if available
    ServiceRegistration *current = g_service_registry.services;
    while (current && written < buffer_size - 1) {
        if (current->service && current->service->is_running()) {
            written += snprintf(buffer + written, buffer_size - written,
                              "%-20s: Running\n", current->name);
        }
        current = current->next;
    }
    
    pthread_mutex_unlock(&g_service_registry.mutex);
}

// ========================================
// Dependency Management
// ========================================

bool service_registry_resolve_dependencies(void) {
    if (!g_registry_initialized) {
        return false;
    }
    
    // Check all services for dependency satisfaction
    pthread_mutex_lock(&g_service_registry.mutex);
    
    bool all_dependencies_satisfied = true;
    ServiceRegistration *current = g_service_registry.services;
    
    while (current) {
        if (current->service && current->service->is_running()) {
            // For now, assume all dependencies are satisfied
            // In a real implementation, this would check actual dependencies
        }
        current = current->next;
    }
    
    pthread_mutex_unlock(&g_service_registry.mutex);
    
    return all_dependencies_satisfied;
}

// ========================================
// Registry Validation
// ========================================

bool service_registry_validate_registry(void) {
    if (!g_registry_initialized) {
        return false;
    }
    
    pthread_mutex_lock(&g_service_registry.mutex);
    
    bool is_valid = true;
    ServiceRegistration *current = g_service_registry.services;
    
    while (current) {
        if (!current->service) {
            printf("Service '%s' has NULL service pointer\n", current->name);
            is_valid = false;
        } else if (!current->service->name) {
            printf("Service at %p has NULL name\n", (void*)current->service);
            is_valid = false;
        }
        current = current->next;
    }
    
    pthread_mutex_unlock(&g_service_registry.mutex);
    
    return is_valid;
}

// ========================================
// Service Registry Utilities
// ========================================

DIContainer* service_registry_get_container(void) {
    if (!g_registry_initialized) {
        return NULL;
    }
    
    return g_service_registry.container;
}

bool service_registry_is_initialized(void) {
    return g_registry_initialized;
}

void service_registry_print_statistics(void) {
    if (!g_registry_initialized) {
        printf("Service registry not initialized\n");
        return;
    }
    
    pthread_mutex_lock(&g_service_registry.mutex);
    
    printf("\n=== Service Registry Statistics ===\n");
    printf("Initialized: %s\n", g_registry_initialized ? "Yes" : "No");
    printf("Total Services: %u\n", g_service_registry.service_count);
    printf("Running Services: %u\n", g_service_registry.running_count);
    printf("Stopped Services: %u\n", g_service_registry.service_count - g_service_registry.running_count);
    
    if (g_service_registry.container) {
        printf("DI Container Dependencies: %u\n", di_container_get_dependency_count(g_service_registry.container));
        printf("DI Container Services: %u\n", di_container_get_service_count(g_service_registry.container));
    }
    
    printf("===================================\n\n");
    
    pthread_mutex_unlock(&g_service_registry.mutex);
}
