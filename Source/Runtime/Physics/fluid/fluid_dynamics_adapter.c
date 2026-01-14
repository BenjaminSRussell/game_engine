/**
 * @file fluid_dynamics_adapter.c
 * @brief Fluid dynamics subsystem interface adapter implementation
 *
 * Adapts the existing fluid dynamics system to conform to the standard
 * IEngineSubsystem interface, enabling dependency injection and standardized
 * communication patterns.
 */

#include "fluid_dynamics_interface.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <pthread.h>
#include <sys/time.h>

// ========================================
// Interface Constants
// ========================================

#define FLUID_DYNAMICS_NAME "fluid_dynamics"
#define FLUID_DYNAMICS_VERSION "1.0.0"
#define FLUID_DYNAMICS_ID 0x4C4C4C42

// ========================================
// Forward Declarations
// ========================================

extern FluidDynamicsSystem* fluid_dynamics_create(void);
extern void fluid_dynamics_destroy(FluidDynamicsSystem *system);

// ========================================
// Interface Implementation Functions
// ========================================

static InterfaceResult fluid_dynamics_initialize(const SubsystemConfig *config) {
    FluidDynamicsSubsystem *subsystem = (FluidDynamicsSubsystem*)g_current_interface;
    if (!subsystem) {
        return INTERFACE_ERROR_INVALID_PARAMETER;
    }
    
    // Validate configuration
    VALIDATE_INTERFACE_PARAMS(subsystem, config);
    
    // Copy configuration
    if (config) {
        subsystem->config = *(FluidDynamicsConfig*)config;
    } else {
        // Use default configuration
        memset(&subsystem->config, 0, sizeof(FluidDynamicsConfig));
        subsystem->config.base.version = 1;
        subsystem->config.max_bodies = 32;
        subsystem->config.max_particles_per_body = 1000;
        subsystem->config.enable_gpu_acceleration = true;
        subsystem->config.enable_ocean_waves = true;
        subsystem->config.enable_particle_effects = true;
        subsystem->config.enable_buoyancy_simulation = true;
        subsystem->config.default_gravity = -9.81f;
        subsystem->config.default_viscosity = 0.001f;
        subsystem->config.default_surface_tension = 0.0728f;
        subsystem->config.default_density = 1000.0f;
    }
    
    // Initialize dependency tracking
    subsystem->physics_dependency_available = false;
    subsystem->rendering_dependency_available = false;
    subsystem->audio_dependency_available = false;
    
    // Create the actual fluid dynamics system
    subsystem->system = fluid_dynamics_create();
    if (!subsystem->system) {
        snprintf(subsystem->error_message, sizeof(subsystem->error_message),
                "Failed to create fluid dynamics system");
        subsystem->current_status = SUBSYSTEM_STATUS_ERROR;
        return INTERFACE_ERROR_OPERATION_FAILED;
    }
    
    // Allocate bodies array
    subsystem->body_capacity = subsystem->config.max_bodies;
    subsystem->bodies = calloc(subsystem->body_capacity, sizeof(FluidBody*));
    if (!subsystem->bodies) {
        fluid_dynamics_destroy(subsystem->system);
        subsystem->system = NULL;
        snprintf(subsystem->error_message, sizeof(subsystem->error_message),
                "Failed to allocate bodies array");
        subsystem->current_status = SUBSYSTEM_STATUS_ERROR;
        return INTERFACE_ERROR_OUT_OF_MEMORY;
    }
    
    // Initialize performance monitoring
    subsystem->update_time_ns = 0;
    subsystem->update_count = 0;
    subsystem->render_time_ns = 0;
    subsystem->render_count = 0;
    
    // Set interface properties
    subsystem->base_interface.name = FLUID_DYNAMICS_NAME;
    subsystem->base_interface.version = FLUID_DYNAMICS_VERSION;
    subsystem->base_interface.interface_id = FLUID_DYNAMICS_ID;
    
    subsystem->is_initialized = true;
    subsystem->current_status = SUBSYSTEM_STATUS_READY;
    
    INTERFACE_LOG_INFO(FLUID_DYNAMICS_NAME, "Fluid dynamics subsystem initialized successfully");
    return INTERFACE_SUCCESS;
}

static void fluid_dynamics_shutdown(void) {
    FluidDynamicsSubsystem *subsystem = (FluidDynamicsSubsystem*)g_current_interface;
    if (!subsystem || !subsystem->is_initialized) {
        return;
    }
    
    INTERFACE_LOG_INFO(FLUID_DYNAMICS_NAME, "Shutting down fluid dynamics subsystem");
    
    // Destroy all fluid bodies
    if (subsystem->bodies) {
        for (uint32_t i = 0; i < subsystem->body_count; i++) {
            if (subsystem->bodies[i]) {
                fluid_reset(subsystem->bodies[i]);
            }
        }
        free(subsystem->bodies);
        subsystem->bodies = NULL;
    }
    
    // Destroy the fluid dynamics system
    if (subsystem->system) {
        fluid_dynamics_destroy(subsystem->system);
        subsystem->system = NULL;
    }
    
    subsystem->is_initialized = false;
    subsystem->current_status = SUBSYSTEM_STATUS_TERMINATED;
    
    INTERFACE_LOG_INFO(FLUID_DYNAMICS_NAME, "Fluid dynamics subsystem shutdown complete");
}

static bool fluid_dynamics_is_initialized(void) {
    FluidDynamicsSubsystem *subsystem = (FluidDynamicsSubsystem*)g_current_interface;
    return subsystem && subsystem->is_initialized;
}

static void fluid_dynamics_update(float dt) {
    FluidDynamicsSubsystem *subsystem = (FluidDynamicsSubsystem*)g_current_interface;
    if (!subsystem || !subsystem->is_initialized || !subsystem->system) {
        return;
    }
    
    uint64_t start_time = get_timestamp_ns();
    
    // Update all fluid bodies
    for (uint32_t i = 0; i < subsystem->body_count; i++) {
        if (subsystem->bodies[i]) {
            fluid_update(subsystem->system, dt);
        }
    }
    
    subsystem->update_time_ns += get_timestamp_ns() - start_time;
    subsystem->update_count++;
    
    subsystem->current_status = SUBSYSTEM_STATUS_RUNNING;
}

static void fluid_dynamics_fixed_update(float fixed_dt) {
    FluidDynamicsSubsystem *subsystem = (FluidDynamicsSubsystem*)g_current_interface;
    if (!subsystem || !subsystem->is_initialized || !subsystem->system) {
        return;
    }
    
    // Fixed update with smaller time step for stability
    float sub_dt = fixed_dt / (float)subsystem->config.substeps;
    
    for (uint32_t substep = 0; substep < subsystem->config.substeps; substep++) {
        fluid_update(subsystem->system, sub_dt);
    }
    
    subsystem->current_status = SUBSYSTEM_STATUS_RUNNING;
}

static void fluid_dynamics_late_update(float dt) {
    FluidDynamicsSubsystem *subsystem = (FluidDynamicsSubsystem*)g_current_interface;
    if (!subsystem || !subsystem->is_initialized || !subsystem->system) {
        return;
    }
    
    // Late update for post-processing
    // This could include wave updates, particle cleanup, etc.
    subsystem->current_status = SUBSYSTEM_STATUS_RUNNING;
}

static InterfaceResult fluid_dynamics_set_config(const SubsystemConfig *config) {
    FluidDynamicsSubsystem *subsystem = (FluidDynamicsSubsystem*)g_current_interface;
    if (!subsystem || !config) {
        return INTERFACE_ERROR_INVALID_PARAMETER;
    }
    
    // Update configuration
    if (config->version == 1) {
        subsystem->config = *(FluidDynamicsConfig*)config;
    } else {
        INTERFACE_LOG_ERROR(FLUID_DYNAMICS_NAME, "Unsupported configuration version: %u", config->version);
        return INTERFACE_ERROR_INVALID_PARAMETER;
    }
    
    return INTERFACE_SUCCESS;
}

static SubsystemConfig fluid_dynamics_get_config(void) {
    FluidDynamicsSubsystem *subsystem = (FluidDynamicsSubsystem*)g_current_interface;
    if (!subsystem) {
        SubsystemConfig empty_config = {0};
        return empty_config;
    }
    
    return subsystem->config.base;
}

static bool fluid_dynamics_register_dependency(const char *dependency_name) {
    FluidDynamicsSubsystem *subsystem = (FluidDynamicsSubsystem*)g_current_interface;
    if (!subsystem || !dependency_name) {
        return false;
    }
    
    // Check for known dependencies
    if (strcmp(dependency_name, "physics") == 0) {
        subsystem->physics_dependency_available = true;
        return true;
    }
    if (strcmp(dependency_name, "rendering") == 0) {
        subsystem->rendering_dependency_available = true;
        return true;
    }
    if (strcmp(dependency_name, "audio") == 0) {
        subsystem->audio_dependency_available = true;
        return true;
    }
    
    INTERFACE_LOG_INFO(FLUID_DYNAMICS_NAME, "Unknown dependency: %s", dependency_name);
    return false;
}

static bool fluid_dynamics_unregister_dependency(const char *dependency_name) {
    FluidDynamicsSubsystem *subsystem = (FluidDynamicsSubsystem*)g_current_interface;
    if (!subsystem || !dependency_name) {
        return false;
    }
    
    if (strcmp(dependency_name, "physics") == 0) {
        subsystem->physics_dependency_available = false;
        return true;
    }
    if (strcmp(dependency_name, "rendering") == 0) {
        subsystem->rendering_dependency_available = false;
        return true;
    }
    if (strcmp(dependency_name, "audio") == 0) {
        subsystem->audio_dependency_available = false;
        return true;
    }
    
    return false;
}

static bool fluid_dynamics_check_dependencies(void) {
    FluidDynamicsSubsystem *subsystem = (FluidDynamicsSubsystem*)g_current_interface;
    if (!subsystem) {
        return false;
    }
    
    // All dependencies are optional for fluid dynamics
    return true;
}

static uint32_t fluid_dynamics_get_dependency_count(void) {
    FluidDynamicsSubsystem *subsystem = (FluidDynamicsSubsystem*)g_current_interface;
    if (!subsystem) {
        return 0;
    }
    
    uint32_t count = 0;
    if (subsystem->physics_dependency_available) count++;
    if (subsystem->rendering_dependency_available) count++;
    if (subsystem->audio_dependency_available) count++;
    
    return count;
}

static const char* fluid_dynamics_get_dependency_name(uint32_t index) {
    FluidDynamicsSubsystem *subsystem = (FluidDynamicsSubsystem*)g_current_interface;
    if (!subsystem || index >= fluid_dynamics_get_dependency_count()) {
        return NULL;
    }
    
    uint32_t count = 0;
    if (subsystem->physics_dependency_available && count++ == index) {
        return "physics";
    }
    if (subsystem->rendering_dependency_available && count++ == index) {
        return "rendering";
    }
    if (subsystem->audio_dependency_available && count++ == index) {
        return "audio";
    }
    
    return NULL;
}

static SubsystemStatus fluid_dynamics_get_status(void) {
    FluidDynamicsSubsystem *subsystem = (FluidDynamicsSubsystem*)g_current_interface;
    if (!subsystem) {
        return SUBSYSTEM_STATUS_UNINITIALIZED;
    }
    
    return subsystem->current_status;
}

static const char* fluid_dynamics_get_error_message(void) {
    FluidDynamicsSubsystem *subsystem = (FluidDynamicsSubsystem*)g_current_interface;
    if (!subsystem) {
        return "Subsystem not created";
    }
    
    return subsystem->error_message[0] ? subsystem->error_message : "No error";
}

static InterfaceResult fluid_diagnostics(char *buffer, size_t buffer_size) {
    FluidDynamicsSubsystem *subsystem = (FluidDynamicsSubsystem*)g_current_interface;
    if (!subsystem || !buffer || buffer_size == 0) {
        return INTERFACE_ERROR_INVALID_PARAMETER;
    }
    
    int written = snprintf(buffer, buffer_size,
            "=== Fluid Dynamics Diagnostics ===\n"
            "Status: %s\n"
            "Initialized: %s\n"
            "Bodies: %u / %u\n"
            "Update Count: %u\n"
            "Render Count: %u\n"
            "Update Time: %.3f ms\n"
            "Render Time: %.3f ms\n"
            "Physics Dependency: %s\n"
            "Rendering Dependency: %s\n"
            "Audio Dependency: %s\n"
            "Memory Usage: %zu bytes\n",
            subsystem->current_status == SUBSYSTEM_STATUS_READY ? "Ready" :
            subsystem->current_status == SUBSYSTEM_STATUS_RUNNING ? "Running" :
            subsystem->current_status == SUBSYSTEM_STATUS_ERROR ? "Error" : "Unknown",
            subsystem->is_initialized ? "Yes" : "No",
            subsystem->body_count,
            subsystem->body_capacity,
            subsystem->update_count,
            subsystem->render_count,
            (double)subsystem->update_time_ns / 1000000.0,
            (double)subsystem->render_time_ns / 1000000.0,
            subsystem->physics_dependency_available ? "Available" : "Not Available",
            subsystem->rendering_dependency_available ? "Available" : "Not Available",
            subsystem->audio_dependency_available ? "Available" : "Not Available",
            fluid_dynamics_get_memory_usage());
    
    return INTERFACE_SUCCESS;
}

// ========================================
// Resource Management
// ========================================

static InterfaceResult fluid_dynamics_acquire_resource(const char *resource_name) {
    // For now, all resources are always available
    return INTERFACE_SUCCESS;
}

static InterfaceResult fluid_dynamics_release_resource(const char *resource_name) {
    // For now, all resources are immediately available
    return INTERFACE_SUCCESS;
}

static bool fluid_dynamics_is_resource_available(const char *resource_name) {
    // For now, all resources are available
    return true;
}

static void* fluid_dynamics_allocate_memory(size_t size, const char *tag) {
    void *ptr = malloc(size);
    if (ptr) {
        INTERFACE_LOG_DEBUG("FLUID_DYNAMICS_NAME, "Allocated %zu bytes for %s", size, tag);
    }
    return ptr;
}

static void fluid_dynamics_free_memory(void *ptr) {
    if (ptr) {
        free(ptr);
    }
}

static size_t fluid_dynamics_get_memory_usage(void) {
    // Calculate total memory usage
    FluidDynamicsSubsystem *subsystem = (FluidDynamics_t*)g_current_interface;
    if (!subsystem) {
        return 0;
    }
    
    size_t total = sizeof(FluidDynamicsSubsystem);
    if (subsystem->bodies) {
        total += subsystem->body_capacity * sizeof(FluidBody*);
        for (uint32_t i = 0; i < subsystem->body_count; i++) {
            total += fluid_calculate_total_volume(subsystem->bodies[i]);
        }
    }
    
    if (subsystem->system) {
        total += sizeof(FluidDynamicsSystem);
    }
    
    return total;
}

// ========================================
// Event Handling
// ========================================

static InterfaceResult fluid_dynamics_register_event_handler(uint32_t event_type, void (*handler)(void*, void*)) {
    // For now, no event handling is implemented
    return INTERFACE_ERROR_NOT_SUPPORTED;
}

static InterfaceResult fluid_dynamics_unregister_event_handler(uint32_t event_type) {
    // For now, no event handling is implemented
    return INTERFACE_ERROR_NOT_SUPPORTED;
}

static InterfaceResult fluid_dynamics_send_event(uint32_t event_type, void *event_data) {
    // For now, no event sending is implemented
    return INTERFACE_ERROR_NOT_SUPPORTED;
}

// ========================================
// Thread Safety
// ========================================

static bool fluid_dynamics_is_thread_safe(void) {
    // Fluid dynamics system is not thread-safe yet
    return false;
}

static uint32_t fluid_dynamics_get_thread_id(void) {
    return (uint32_t)pthread_self();
}

// ========================================
// Performance Monitoring
// ========================================

static void fluid_dynamics_start_performance_profile(const char *operation) {
    if (!g_performance_profiler_is_enabled()) {
        return;
    }
    
    char profile_name[128];
    snprintf(profile_name, sizeof(profile_name), "fluid_dynamics_%s", operation);
    performance_profiler_start_profile(profile_name);
}

static void fluid_dynamics_end_performance_profile(const char *operation) {
    if (!g_performance_profiler_is_enabled()) {
        return;
    }
    
    char profile_name[128];
    snprintf(profile_name, sizeof(profile_name), "fluid_dynamics_%s", operation);
    performance_profiler_end_profile(profile_name);
}

static void fluid_dynamics_get_performance_stats(char *buffer, size_t buffer_size) {
    if (!buffer || buffer_size == 0) {
        return;
    }
    
    FluidDynamicsSubsystem *subsystem = (FluidDynamicsSubsystem*)g_current_interface;
    if (!subsystem) {
        snprintf(buffer, buffer_size,
                "No performance stats available");
        return;
    }
    
    int written = snprintf(buffer, buffer_size,
            "=== Fluid Dynamics Performance Stats ===\n"
            "Update Count: %u\n"
            "Average Update Time: %.3f ms\n"
            "Total Update Time: %.3f ms\n"
            "Render Count: %u\n"
            "Average Render Time: %.3f ms\n"
            "Memory Usage: %zu bytes\n"
            "Bodies: %u / %u\n",
            subsystem->update_count,
            subsystem->update_count > 0 ? 
                (double)subsystem->update_time_ns / subsystem->update_count / 1000000.0 : 0.0,
            (double)subsystem->update_time_ns / 1000000.0,
            subsystem->render_count,
            subsystem->render_count > 0 ? 
                (double)subsystem->render_time_ns / subsystem->render_count / 1000000.0 : 0.0,
            fluid_dynamics_get_memory_usage());
    
    INTERFACE_LOG_DEBUG(FLUID_DYNAMICS_NAME, "Performance stats: %s", buffer);
}

// ========================================
// Validation
// ========================================

static bool fluid_dynamics_validate_state(void) {
    FluidDynamicsSubsystem *subsystem = (FluidDynamicsSubsystem*)g_current_interface;
    if (!subsystem || !subsystem->is_initialized || !subsystem->system) {
        return false;
    }
    
    // Validate all fluid bodies
    for (uint32_t i = 0; i < subsystem->body_count; i++) {
        if (!subsystem->bodies[i]) {
            fluid_validate_simulation(subsystem->bodies[i]);
        }
    }
    
    return true;
}

static InterfaceResult fluid_dynamics_run_self_tests(void) {
    FluidDynamicsSubsystem *subsystem = (FluidDynamicsSubsystem*)g_current_interface;
    if (!subsystem || !subsystem->is_initialized) {
        return INTERFACE_ERROR_NOT_INITIALIZED;
    }
    
    INTERFACE_LOG_INFO(FLUID_DYNAMICS_NAME, "Running self-tests...");
    
    // Test basic fluid body creation
    FluidBody *test_body = fluid_create_shallow_water(subsystem->system, 64, 64, 0.5f);
    if (!test_body) {
        return INTERFACE_ERROR_OPERATION_FAILED;
    }
    
    // Test basic fluid operations
    fluid_set_height(test_body, 32, 32, 1.0f);
    fluid_set_velocity(test_body, 32, 32, (Vec2){1.0f, 0.0f});
    
    // Validate the state
    bool valid = fluid_validate_simulation(test_body);
    
    // Cleanup
    fluid_reset(test_body);
    
    // Destroy test body
    fluid_destroy(test_body);
    
    INTERFACE_LOG_INFO(FLUID_DYNAMICS_NAME, "Self-tests %s", valid ? "PASSED" : "FAILED");
    
    return valid ? INTERFACE_SUCCESS : INTERFACE_ERROR_OPERATION_FAILED;
}

// ========================================
// Interface Creation and Destruction
// ========================================

IEngineSubsystem* create_fluid_dynamics_interface(const FluidDynamicsConfig *config) {
    // Allocate interface structure
    IEngineSubsystem *interface = malloc(sizeof(IEngineSubsystem));
    if (!interface) {
        return NULL;
    }
    
    // Allocate subsystem structure
    FluidDynamicsSubsystem *subsystem = malloc(sizeof(FluidDynamicsSubsystem));
    if (!subsystem) {
        free(interface);
        return NULL;
    }
    
    // Initialize interface
    memset(interface, 0, sizeof(IEngineSubsystem));
    
    // Set interface functions
    interface->name = FLUID_DYNAMICS_NAME;
    interface->version = FLUID_DYNAMICS_VERSION;
    interface->interface_id = FLUID_DYNAMICS_ID;
    
    // Set interface functions
    interface->initialize = fluid_dynamics_initialize;
    interface->shutdown = fluid_dynamics_shutdown;
    interface->is_initialized = fluid_dynamics_is_initialized;
    interface->update = fluid_dynamics_update;
    interface->fixed_update = fluid_dynamics_fixed_update;
    interface->late_update = fluid_dynamics_late_update;
    interface->set_config = fluid_dynamics_set_config;
    interface->get_config = fluid_dynamics_get_config;
    interface->register_dependency = fluid_dynamics_register_dependency;
    interface->unregister_dependency = fluid_dynamics_unregister_dependency;
    interface->check_dependencies = fluid_dynamics_check_dependencies;
    interface->get_dependency_count = fluid_dynamics_get_dependency_count;
    interface->get_dependency_name = fluid_dynamics_get_dependency_name;
    interface->get_status = fluid_dynamics_get_status;
    interface->get_error_message = fluid_dynamics_get_error_message;
    interface->diagnostics = fluid_diagnostics;
    interface->acquire_resource = fluid_dynamics_acquire_resource;
    interface->release_resource = fluid_dynamics_release_resource;
    interface->is_resource_available = fluid_dynamics_is_resource_available;
    interface->register_event_handler = fluid_dynamics_register_event_handler;
    interface->unregister_event_handler = fluid_dynamics_unregister_event_handler;
    interface->send_event = fluid_dynamics_send_event;
    interface->allocate_memory = fluid_dynamics_allocate_memory;
    interface->free_memory = fluid_dynamics_free_memory;
    interface->get_memory_usage = fluid_dynamics_get_memory_usage;
    interface->is_thread_safe = fluid_dynamics_is_thread_safe;
    interface->get_thread_id = fluid_dynamics_get_thread_id;
    interface->start_performance_profile = fluid_dynamics_start_performance_profile;
    interface->end_performance_profile = fluid_dynamics_end_performance_profile;
    interface->get_performance_stats = fluid_dynamics_get_performance_stats;
    interface->validate_state = fluid_dynamics_validate_state;
    interface->run_self_tests = fluid_dynamics_run_self_tests;
    
    // Set private data pointer
    interface->private_data = subsystem;
    
    // Set as current interface
    g_current_interface = interface;
    
    return interface;
}

void destroy_fluid_dynamics_interface(IEngineSubsystem *interface) {
    if (!interface) return;
    
    FluidDynamicsSubsystem *subsystem = (FluidDynamicsSubsystem*)interface->private_data;
    if (subsystem) {
        destroy_fluid_dynamics_interface(interface);
    }
    
    free(interface);
    g_current_interface = NULL;
}

// ========================================
// Service Registration
// ========================================

typedef struct FluidDynamicsService {
    IEngineService base_service;
    FluidDynamicsConfig config;
    DIContainer *container;
} FluidDynamicsService;

static InterfaceResult fluid_dynamics_service_start(void) {
    FluidDynamicsService *service = (FluidDynamicsService*)g_current_service;
    if (!service) {
        return INTERFACE_ERROR_NOT_INITIALIZED;
    }
    
    INTERFACE_LOG_INFO("fluid_dynamics_service", "Starting fluid dynamics service");
    
    // Initialize the subsystem
    InterfaceResult result = service->base_service.initialize(&service->config.base);
    if (result != INTERFACE_SUCCESS) {
        return result;
    }
    
    service->base_service.is_running = true;
    return INTERFACE_SUCCESS;
}

static InterfaceResult fluid_dynamics_service_stop(void) {
    FluidDynamicsService *service = (FluidDynamicsService*)g_current_service;
    if (!service || !service->base_service.is_running) {
        return INTERFACE_ERROR_INVALID_STATE;
    }
    
    INTERFACE_LOG_INFO("fluid_dynamics_service", "Stopping fluid dynamics service");
    
    InterfaceResult result = service->base_service.stop();
    if (result != INTERFACE_SUCCESS) {
        return result;
    }
    
    service->base_service.is_running = false;
    return INTERFACE_SUCCESS;
}

static bool fluid_dynamics_service_is_running(void) {
    FluidDynamicsService *service = (FluidDynamicsService*)g_current_service;
    return service && service->base_service.is_running;
}

static const char* fluid_dynamics_service_get_name(void) {
    FluidDynamicsService *service = (FluidDynamicsService*)g_current_service;
    return service ? service->base_service.name : NULL;
}

static const char* fluid_dynamics_service_get_version(void) {
    FluidDynamicsService *service = (FluidDynamicsService*)g_current_service;
    return service ? service->base_service.version : NULL;
}

static uint32_t fluid_dynamics_service_get_id(void) {
    FluidDynamicsService *service = (FluidDynamicsService*)g_current_service;
    return service ? service->base_service.service_id : 0;
}

static InterfaceResult fluid_dynamics_service_execute(const char *operation, void *input, void *output, size_t output_size) {
    FluidDynamicsService *service = (FluidDynamics_service*)g_current_service;
    if (!service || !service->base_service.is_running) {
        return INTERFACE_ERROR_INVALID_STATE;
    }
    
    // For now, no specific operations are supported
    return INTERFACE_ERROR_NOT_SUPPORTED;
}

static bool fluid_dynamics_service_supports_operation(const char *operation) {
    // For now, no specific operations are supported
    return false;
}

static const char* fluid_dynamics_service_get_capabilities(void) {
    return "fluid_simulation, wave_generation, particle_effects, buoyancy_calculation";
}

static uint32_t fluid_dynamics_service_get_capability_count(void) {
    return 4; // 4 capabilities listed above
}

static const char* fluid_dynamics_service_get_capability(uint32_t index) {
    switch (index) {
        case 0: return "fluid_simulation";
        case 1: return "wave_generation";
        case 2: return "particle_effects";
        case 3: return "buoyancy_calculation";
        default: return NULL;
    }
}

static bool fluid_dynamics_service_is_healthy(void) {
    FluidDynamicsService *service = (FluidDynamicsService*)g_current_service;
    return service && service->base_service.is_running;
}

static InterfaceResult fluid_dynamics_health_check(char *buffer, size_t buffer_size) {
    FluidDynamicsService *service = (FluidDynamicsService*)g_current_service;
    if (!service || !buffer || buffer_size == 0) {
        return INTERFACE_ERROR_INVALID_PARAMETER;
    }
    
    // Basic health check
    if (!service->base_service.is_running) {
        snprintf(buffer, buffer_size, "Service not running");
        return INTERFACE_ERROR_INVALID_STATE;
    }
    
    // Check if subsystem is healthy
    if (g_current_interface && !fluid_dynamics_validate_state()) {
        snprintf(buffer, buffer_size, "Subsystem validation failed");
        return INTERFACE_ERROR_INVALID_STATE;
    }
    
    snprintf(buffer, buffer_size, "Service is healthy");
    return INTERFACE_SUCCESS;
}

bool register_fluid_dynamics_service(DIContainer *container, const FluidDynamicsConfig *config) {
    if (!container || !config) {
        return false;
    }
    
    // Create service instance
    FluidDynamicsService *service = malloc(sizeof(FluidDynamicsService));
    if (!service) {
        return false;
    }
    
    // Initialize service
    memset(service, 0, sizeof(FluidDynamicsService));
    
    // Set service properties
    service->base_service.name = FLUID_DYNAMICS_NAME;
    service->base_service.version = FLUID_DYNAMICS_VERSION;
    service->base_service.service_id = FLUID_DYNAMICS_ID;
    service->container = container;
    
    // Copy configuration
    service->config = *config;
    
    // Set service functions
    service->base_service.start = fluid_dynamics_service_start;
    service->base_service.stop = fluid_dynamics_service_stop;
    service->base_service.is_running = false;
    service->base_service.is_initialized = false;
    service->base_service.get_name = fluid_dynamics_service_get_name;
    service->base_service.get_version = fluid_dynamics_service_get_version;
    service->base_service.get_id = fluid_dynamics_service_get_id;
    service->base_service.execute = fluid_dynamics_service_execute;
    service->base_service.supports_operation = fluid_dynamics_service_supports_operation;
    service->base_service.get_capabilities = fluid_dynamics_service_get_capabilities;
    service->base_service.get_capability_count = fluid_dynamics_service_get_capability_count;
    service->base_service.get_capability = fluid_dynamics_service_get_capability;
    service->base_service.is_healthy = fluid_dynamics_service_is_healthy;
    service->base_service.health_check = fluid_dynamics_health_check;
    
    // Register with container
    return di_container_register_service(container, (IEngineService*)service);
}

IEngineService* get_fluid_dynamics_service(DIContainer *container) {
    if (!container) {
        return NULL;
    }
    
    return di_container_get_service(container, FLUID_DYNAMICS_NAME);
}

// ========================================
// Performance Integration
// ========================================

bool fluid_dynamics_start_profiling(const char *operation) {
    return fluid_dynamics_start_performance_profile(operation);
}

bool fluid_dynamics_end_profiling(const char *operation) {
    return fluid_dynamics_end_performance_profile(operation);
}

bool fluid_dynamics_get_performance_stats(FluidDynamicsPerformanceStats *stats) {
    if (!stats) {
        return false;
    }
    
    FluidDynamicsSubsystem *subsystem = (FluidDynamicsSubsystem*)g_current_interface;
    if (!subsystem) {
        return false;
    }
    
    // Fill performance stats structure
    stats->update_count = subsystem->update_count;
    stats->average_update_time_ms = subsystem->update_count > 0 ? 
        (double)subsystem->update_time_ns / subsystem->update_count / 1000000.0 : 0.0;
    stats->total_update_time_ms = (double)subsystem->update_time_ns / 1000000.0;
    stats->render_count = subsystem->render_count;
    stats->average_render_time_ms = subsystem->render_count > 0 ? 
        (double)subsystem->render_time_ns / subsystem->render_count / 1000000.0 : 0.0;
    stats->total_render_time_ms = (double)subsystem->render_time_ns / 1000000.0;
    stats->memory_usage_bytes = fluid_dynamics_get_memory_usage();
    
    return true;
}

// ========================================
// Global Interface Instance
// ========================================

static IEngineSubsystem *g_current_interface = NULL;
static IEngineService *g_current_service = NULL;

// ========================================
// Memory Management
// ========================================

static uint64_t get_timestamp_ns(void) {
    struct timespec ts;
    clock_gettime(CLOCK_MONOTONIC, &ts);
    return (uint64_t)ts.tv_sec * 1000000000ULL + (uint64_t)ts.tv_nsec;
}

// ========================================
// Global Interface Registry Access
// ========================================

void set_current_fluid_dynamics_interface(IEngineSubsystem *interface) {
    g_current_interface = interface;
}

void set_current_fluid_dynamics_service(IEngineService *service) {
    g_current_service = service;
}

IEngineSubsystem* get_current_fluid_dynamics_interface(void) {
    return g_current_interface;
}

IEngineService* get_current_fluid_dynamics_service(void) {
    return g_current_service;
}

// ========================================
// Interface Standards Compliance
// ========================================

DECLARE_INTERFACE(fluid_dynamics, FLUID_DYNAMICS_ID);

IMPLEMENT_INTERFACE_BASE(fluid_dynamics);

// ========================================
// Performance Profiling Macros
// ========================================

#define FLUID_PROFILE_START(operation) \
    fluid_dynamics_start_performance_profile(operation)

#define FLUID_PROFILE_END(operation) \
    fluid_dynamics_end_performance_profile(operation)

#define FLUID_PROFILE_SCOPE_START(operation) \
    fluid_dynamics_start_performance_profile(operation)

#define FLUID_PROFILE_SCOPE_END(operation) \
    fluid_dynamics_end_performance_profile(operation)

#define FLUID_PROFILE_FRAME_START() \
    FLUID_PROFILE_START("frame_update")

#define FLUID_PROFILE_FRAME_END() \
    FLUID_PROFILE_END("frame_update")

#define FLUID_PROFILE_RENDER_START() \
    FLUID_PROFILE_START("rendering")

#define FLUID_PROFILE_RENDER_END() \
    FLUID_PROFILE_END("rendering")

// ========================================
// Service Registration Macros
// ========================================

#define REGISTER_FLUID_DYNAMICS_SERVICE(container, config) \
    register_fluid_dynamics_service(container, (const FluidDynamicsConfig*)config)

#define GET_FLUID_DYNAMICS_SERVICE(container) \
    get_fluid_dynamics_service(container)

// ========================================
// Thread Safety Macros
// ========================================

#define FLUID_ASSERT_THREAD_SAFE() \
    assert(fluid_dynamics_is_thread_safe())

// ========================================
// Initialization Macros
// ========================================

#define INITIALIZE_FLUID_DYNAMICS(config) \
    create_fluid_dynamics_interface(config)

#define SHUTDOWN_FLUID_DYNAMICS() \
    destroy_fluid_dynamics_interface(get_current_fluid_dynamics_interface())

#define IS_FLUID_DYNAMICS_INITIALIZED() \
    fluid_dynamics_is_initialized()

// ========================================
// Performance Monitoring Macros
// ========================================

#define FLUID_GET_PERFORMANCE_STATS(stats) \
    fluid_dynamics_get_performance_stats(stats)

// ========================================
// Validation Macros
// ========================================

#define VALIDATE_FLUID_DYNAMICS() \
    fluid_dynamics_validate_state()

#define RUN_FLUID_DYNAMICS_TESTS() \
    fluid_dynamics_run_self_tests()

// ========================================
// Dependency Macros
// ========================================

#define CHECK_FLUID_DEPENDENCIES() \
    fluid_dynamics_check_dependencies()

#define GET_FLUID_DEPENDENCY_COUNT() \
    fluid_dynamics_get_dependency_count()

#define GET_FLUID_DEPENDENCY_NAME(index) \
    fluid_dynamics_get_dependency_name(index)

// ========================================
// Status Macros
// ========================================

#define FLUID_GET_STATUS() \
    fluid_dynamics_get_status()

#define FLUID_GET_ERROR_MESSAGE() \
    fluid_dynamics_get_error_message()

// ========================================
// Resource Macros
// ========================================

#define FLUID_ACQUIRE_RESOURCE(name) \
    fluid_dynamics_acquire_resource(name)

#define FLUID_RELEASE_RESOURCE(name) \
    fluid_dynamics_release_resource(name)

#define FLUID_RESOURCE_AVAILABLE(name) \
    fluid_dynamics_is_resource_available(name)

// ========================================
// Memory Macros
// ========================================

#define FLUID_ALLOCATE(size, tag) \
    fluid_dynamics_allocate_memory(size, tag)

#define FLUID_FREE(ptr) \
    fluid_dynamics_free_memory(ptr)

#define FLUID_GET_MEMORY_USAGE() \
    fluid_dynamics_get_memory_usage()

// ========================================
// Thread Safety
// ========================================

#define FLUID_THREAD_ID() \
    fluid_dynamics_get_thread_id()

#define FLUID_THREAD_SAFE() \
    fluid_dynamics_is_thread_safe()

// ========================================
// Service Status
// ========================================

#define IS_FLUID_SERVICE_RUNNING() \
    fluid_dynamics_service_is_running()

// ========================================
// Diagnostics
// ========================================

#define FLUID_DIAGNOSTICS(buffer) \
    fluid_diagnostics(buffer, sizeof(buffer))

// ========================================
// Self Tests
// ========================================

#define RUN_FLUID_SELF_TESTS() \
    fluid_dynamics_run_self_tests()

// ========================================
// Error Handling
// ========================================

#define FLUID_SET_ERROR(message) \
    do { \
        FluidDynamicsSubsystem *subsystem = get_fluid_dynamics_subsystem(); \
        if (subsystem) { \
            strncpy(subsystem->error_message, message, sizeof(subsystem->error_message)); \
        } \
    } while(0)

#define FLUID_CLEAR_ERROR() \
    do { \
        FluidDynamicsSubsystem *subsystem = get_fluid_dynamics_subsystem(); \
        if (subsystem) { \
            subsystem->error_message[0] = '\0'; \
        } \
    } while(0)

// ========================================
// Logging Macros
// ========================================

#define FLUID_LOG(level, format, ...) \
    do { \
        char log_buffer[512]; \
        snprintf(log_buffer, sizeof(log_buffer), "[%s][FLUID_DYNAMICS_NAME] " format "\n", level, ##__VA_ARGS__); \
        printf("%s", log_buffer); \
    } while(0)

#define FLUID_LOG_DEBUG(format, ...) FLUID_LOG("DEBUG", format, ##__VA_ARGS__)
#define FLUID_LOG_INFO(format, ...) FLUID_LOG("INFO", format, ##__VA_ARGS__)
#define FLUID_LOG_WARNING(format, ...) FLUID_LOG("WARNING", format, ##__VA_ARGS__)
#define FLUID_LOG_ERROR(format, ...) FLUID_LOG("ERROR", format, ##__VA_ARGS__)

// ========================================
// Global Interface Registry
// ========================================

void register_fluid_dynamics_interfaces(void);
void unregister_fluid_dynamics_interfaces(void);

#endif /* FLUID_DYNAMICS_INTERFACE_H */
