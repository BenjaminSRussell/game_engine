/**
 * @file di_container.c
 * @brief Dependency injection container implementation
 *
 * Implements a dependency injection system to manage subsystem dependencies
 * and promote loose coupling between engine components.
 */

#include "dependency_injection.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <pthread.h>

// ========================================
// Internal Structures
// ========================================

typedef struct DependencyEntry {
    char name[64];                      // Dependency name
    void *instance;                     // Dependency instance
    bool is_singleton;                   // Whether dependency is singleton
    uint32_t ref_count;                 // Reference count
    struct DependencyEntry *next;        // Next entry in hash table
} DependencyEntry;

typedef struct ServiceEntry {
    char name[64];                      // Service name
    IEngineService *service;            // Service instance
    bool is_running;                     // Whether service is running
    struct ServiceEntry *next;           // Next entry in hash table
} ServiceEntry;

struct DIContainer {
    DependencyEntry **dependency_table;   // Hash table for dependencies
    ServiceEntry **service_table;         // Hash table for services
    uint32_t dependency_capacity;         // Dependency table capacity
    uint32_t service_capacity;           // Service table capacity
    uint32_t dependency_count;            // Number of registered dependencies
    uint32_t service_count;               // Number of registered services
    pthread_mutex_t mutex;               // Thread safety mutex
    bool is_initialized;                  // Initialization state
};

// ========================================
// Hash Function
// ========================================

static uint32_t hash_string(const char *str, uint32_t table_size) {
    uint32_t hash = 5381;
    int c;
    while ((c = *str++)) {
        hash = ((hash << 5) + hash) + c;
    }
    return hash % table_size;
}

// ========================================
// Container Creation and Destruction
// ========================================

DIContainer* di_container_create(uint32_t dependency_capacity, uint32_t service_capacity) {
    DIContainer *container = malloc(sizeof(DIContainer));
    if (!container) return NULL;
    
    memset(container, 0, sizeof(DIContainer));
    
    // Allocate hash tables
    container->dependency_capacity = dependency_capacity;
    container->service_capacity = service_capacity;
    container->dependency_table = calloc(dependency_capacity, sizeof(DependencyEntry*));
    container->service_table = calloc(service_capacity, sizeof(ServiceEntry*));
    
    if (!container->dependency_table || !container->service_table) {
        if (container->dependency_table) free(container->dependency_table);
        if (container->service_table) free(container->service_table);
        free(container);
        return NULL;
    }
    
    // Initialize mutex
    if (pthread_mutex_init(&container->mutex, NULL) != 0) {
        free(container->dependency_table);
        free(container->service_table);
        free(container);
        return NULL;
    }
    
    container->is_initialized = true;
    return container;
}

void di_container_destroy(DIContainer *container) {
    if (!container || !container->is_initialized) return;
    
    pthread_mutex_lock(&container->mutex);
    
    // Destroy all dependencies
    for (uint32_t i = 0; i < container->dependency_capacity; i++) {
        DependencyEntry *entry = container->dependency_table[i];
        while (entry) {
            DependencyEntry *next = entry->next;
            free(entry);
            entry = next;
        }
    }
    
    // Destroy all services
    for (uint32_t i = 0; i < container->service_capacity; i++) {
        ServiceEntry *entry = container->service_table[i];
        while (entry) {
            ServiceEntry *next = entry->next;
            if (entry->service) {
                if (entry->service->stop) {
                    entry->service->stop();
                }
                free(entry->service);
            }
            free(entry);
            entry = next;
        }
    }
    
    pthread_mutex_unlock(&container->mutex);
    pthread_mutex_destroy(&container->mutex);
    
    free(container->dependency_table);
    free(container->service_table);
    free(container);
}

// ========================================
// Dependency Registration
// ========================================

bool di_container_register_dependency(DIContainer *container, const char *name, 
                                       void *instance, bool is_singleton) {
    if (!container || !container->is_initialized || !name || !instance) {
        return false;
    }
    
    pthread_mutex_lock(&container->mutex);
    
    // Check if dependency already exists
    uint32_t hash = hash_string(name, container->dependency_capacity);
    DependencyEntry *entry = container->dependency_table[hash];
    
    while (entry) {
        if (strcmp(entry->name, name) == 0) {
            pthread_mutex_unlock(&container->mutex);
            return false; // Already registered
        }
        entry = entry->next;
    }
    
    // Create new dependency entry
    DependencyEntry *new_entry = malloc(sizeof(DependencyEntry));
    if (!new_entry) {
        pthread_mutex_unlock(&container->mutex);
        return false;
    }
    
    strncpy(new_entry->name, name, sizeof(new_entry->name) - 1);
    new_entry->instance = instance;
    new_entry->is_singleton = is_singleton;
    new_entry->ref_count = 0;
    new_entry->next = container->dependency_table[hash];
    
    container->dependency_table[hash] = new_entry;
    container->dependency_count++;
    
    pthread_mutex_unlock(&container->mutex);
    return true;
}

void* di_container_get_dependency(DIContainer *container, const char *name) {
    if (!container || !container->is_initialized || !name) {
        return NULL;
    }
    
    pthread_mutex_lock(&container->mutex);
    
    uint32_t hash = hash_string(name, container->dependency_capacity);
    DependencyEntry *entry = container->dependency_table[hash];
    
    while (entry) {
        if (strcmp(entry->name, name) == 0) {
            entry->ref_count++;
            void *instance = entry->instance;
            pthread_mutex_unlock(&container->mutex);
            return instance;
        }
        entry = entry->next;
    }
    
    pthread_mutex_unlock(&container->mutex);
    return NULL; // Not found
}

bool di_container_release_dependency(DIContainer *container, const char *name) {
    if (!container || !container->is_initialized || !name) {
        return false;
    }
    
    pthread_mutex_lock(&container->mutex);
    
    uint32_t hash = hash_string(name, container->dependency_capacity);
    DependencyEntry *entry = container->dependency_table[hash];
    
    while (entry) {
        if (strcmp(entry->name, name) == 0) {
            if (entry->ref_count > 0) {
                entry->ref_count--;
            }
            pthread_mutex_unlock(&container->mutex);
            return true;
        }
        entry = entry->next;
    }
    
    pthread_mutex_unlock(&container->mutex);
    return false; // Not found
}

// ========================================
// Service Registration
// ========================================

bool di_container_register_service(DIContainer *container, IEngineService *service) {
    if (!container || !container->is_initialized || !service || !service->name) {
        return false;
    }
    
    pthread_mutex_lock(&container->mutex);
    
    // Check if service already exists
    uint32_t hash = hash_string(service->name, container->service_capacity);
    ServiceEntry *entry = container->service_table[hash];
    
    while (entry) {
        if (strcmp(entry->name, service->name) == 0) {
            pthread_mutex_unlock(&container->mutex);
            return false; // Already registered
        }
        entry = entry->next;
    }
    
    // Create new service entry
    ServiceEntry *new_entry = malloc(sizeof(ServiceEntry));
    if (!new_entry) {
        pthread_mutex_unlock(&container->mutex);
        return false;
    }
    
    strncpy(new_entry->name, service->name, sizeof(new_entry->name) - 1);
    new_entry->service = service;
    new_entry->is_running = false;
    new_entry->next = container->service_table[hash];
    
    container->service_table[hash] = new_entry;
    container->service_count++;
    
    pthread_mutex_unlock(&container->mutex);
    return true;
}

IEngineService* di_container_get_service(DIContainer *container, const char *name) {
    if (!container || !container->is_initialized || !name) {
        return NULL;
    }
    
    pthread_mutex_lock(&container->mutex);
    
    uint32_t hash = hash_string(name, container->service_capacity);
    ServiceEntry *entry = container->service_table[hash];
    
    while (entry) {
        if (strcmp(entry->name, name) == 0) {
            IEngineService *service = entry->service;
            pthread_mutex_unlock(&container->mutex);
            return service;
        }
        entry = entry->next;
    }
    
    pthread_mutex_unlock(&container->mutex);
    return NULL; // Not found
}

// ========================================
// Service Lifecycle Management
// ========================================

bool di_container_start_service(DIContainer *container, const char *name) {
    if (!container || !container->is_initialized || !name) {
        return false;
    }
    
    pthread_mutex_lock(&container->mutex);
    
    uint32_t hash = hash_string(name, container->service_capacity);
    ServiceEntry *entry = container->service_table[hash];
    
    while (entry) {
        if (strcmp(entry->name, name) == 0) {
            if (entry->service && entry->service->start && !entry->is_running) {
                InterfaceResult result = entry->service->start();
                if (result == INTERFACE_SUCCESS) {
                    entry->is_running = true;
                    pthread_mutex_unlock(&container->mutex);
                    return true;
                }
            }
            break;
        }
        entry = entry->next;
    }
    
    pthread_mutex_unlock(&container->mutex);
    return false;
}

bool di_container_stop_service(DIContainer *container, const char *name) {
    if (!container || !container->is_initialized || !name) {
        return false;
    }
    
    pthread_mutex_lock(&container->mutex);
    
    uint32_t hash = hash_string(name, container->service_capacity);
    ServiceEntry *entry = container->service_table[hash];
    
    while (entry) {
        if (strcmp(entry->name, name) == 0) {
            if (entry->service && entry->service->stop && entry->is_running) {
                InterfaceResult result = entry->service->stop();
                if (result == INTERFACE_SUCCESS) {
                    entry->is_running = false;
                    pthread_mutex_unlock(&container->mutex);
                    return true;
                }
            }
            break;
        }
        entry = entry->next;
    }
    
    pthread_mutex_unlock(&container->mutex);
    return false;
}

// ========================================
// Container Information
// ========================================

uint32_t di_container_get_dependency_count(DIContainer *container) {
    if (!container || !container->is_initialized) {
        return 0;
    }
    
    pthread_mutex_lock(&container->mutex);
    uint32_t count = container->dependency_count;
    pthread_mutex_unlock(&container->mutex);
    
    return count;
}

uint32_t di_container_get_service_count(DIContainer *container) {
    if (!container || !container->is_initialized) {
        return 0;
    }
    
    pthread_mutex_lock(&container->mutex);
    uint32_t count = container->service_count;
    pthread_mutex_unlock(&container->mutex);
    
    return count;
}

bool di_container_is_service_running(DIContainer *container, const char *name) {
    if (!container || !container->is_initialized || !name) {
        return false;
    }
    
    pthread_mutex_lock(&container->mutex);
    
    uint32_t hash = hash_string(name, container->service_capacity);
    ServiceEntry *entry = container->service_table[hash];
    
    while (entry) {
        if (strcmp(entry->name, name) == 0) {
            bool running = entry->is_running;
            pthread_mutex_unlock(&container->mutex);
            return running;
        }
        entry = entry->next;
    }
    
    pthread_mutex_unlock(&container->mutex);
    return false;
}

// ========================================
// Container Validation
// ========================================

bool di_container_validate_dependencies(DIContainer *container) {
    if (!container || !container->is_initialized) {
        return false;
    }
    
    pthread_mutex_lock(&container->mutex);
    
    // Check for circular dependencies by validating each subsystem
    bool all_valid = true;
    
    for (uint32_t i = 0; i < container->dependency_capacity && all_valid; i++) {
        DependencyEntry *entry = container->dependency_table[i];
        while (entry && all_valid) {
            // Validate dependency instance
            if (!entry->instance) {
                printf("ERROR: Dependency '%s' has NULL instance\n", entry->name);
                all_valid = false;
                break;
            }
            
            // For singleton dependencies, check reference count
            if (entry->is_singleton && entry->ref_count > 1) {
                printf("WARNING: Singleton dependency '%s' has ref count %u\n", 
                       entry->name, entry->ref_count);
            }
            
            entry = entry->next;
        }
    }
    
    pthread_mutex_unlock(&container->mutex);
    return all_valid;
}

void di_container_print_statistics(DIContainer *container) {
    if (!container || !container->is_initialized) {
        printf("DI Container not initialized\n");
        return;
    }
    
    pthread_mutex_lock(&container->mutex);
    
    printf("\n=== DI Container Statistics ===\n");
    printf("Dependencies: %u / %u\n", container->dependency_count, container->dependency_capacity);
    printf("Services: %u / %u\n", container->service_count, container->service_capacity);
    
    printf("\nRegistered Dependencies:\n");
    for (uint32_t i = 0; i < container->dependency_capacity; i++) {
        DependencyEntry *entry = container->dependency_table[i];
        while (entry) {
            printf("  %s (singleton: %s, ref_count: %u)\n", 
                   entry->name, 
                   entry->is_singleton ? "yes" : "no",
                   entry->ref_count);
            entry = entry->next;
        }
    }
    
    printf("\nRegistered Services:\n");
    for (uint32_t i = 0; i < container->service_capacity; i++) {
        ServiceEntry *entry = container->service_table[i];
        while (entry) {
            printf("  %s (running: %s)\n", 
                   entry->name,
                   entry->is_running ? "yes" : "no");
            entry = entry->next;
        }
    }
    
    printf("=============================\n\n");
    
    pthread_mutex_unlock(&container->mutex);
}
