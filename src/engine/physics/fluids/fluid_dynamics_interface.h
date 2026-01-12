/**
 * @file fluid_dynamics_interface.h
 * @brief Fluid dynamics subsystem interface implementation
 *
 * Implements the standard IEngineSubsystem interface for the fluid dynamics
 * system, providing clean separation of concerns and dependency management.
 */

#ifndef FLUID_DYNAMICS_INTERFACE_H
#define FLUID_DYNAMICS_INTERFACE_H

#include "include/engine/standards/interface_standards.h"
#include "src/engine/physics/fluids/fluid_dynamics.h"

#ifdef __cplusplus
extern "C" {
#endif

// ========================================
// Fluid Dynamics Subsystem Configuration
// ========================================

typedef struct FluidDynamicsConfig {
    SubsystemConfig base;                 // Base configuration
    uint32_t max_bodies;                  // Maximum number of fluid bodies
    uint32_t max_particles_per_body;         // Maximum particles per body
    bool enable_gpu_acceleration;          // Enable GPU compute
    bool enable_ocean_waves;              // Enable ocean wave simulation
    bool enable_particle_effects;          // Enable particle effects
    bool enable_buoyancy_simulation;         // Enable buoyancy calculations
    f32 default_gravity;                   // Default gravity force
    f32 default_viscosity;                // Default fluid viscosity
    f32 default_surface_tension;           // Default surface tension
    f32 default_density;                   // Default fluid density
} FluidDynamicsConfig;

// ========================================
// Fluid Dynamics Subsystem Private Data
// ========================================

typedef struct FluidDynamicsSubsystem {
    // Interface implementation
    IEngineSubsystem base_interface;
    
    // Subsystem-specific data
    FluidDynamicsSystem *system;           // Actual fluid dynamics system
    FluidDynamicsConfig config;            // Configuration
    FluidBody **bodies;                    // Array of fluid bodies
    uint32_t body_capacity;                // Capacity of bodies array
    uint32_t body_count;                    // Current number of bodies
    
    // Performance monitoring
    uint64_t update_time_ns;               // Time spent in updates
    uint32_t update_count;                 // Number of updates
    uint64_t render_time_ns;               // Time spent in rendering
    uint32_t render_count;                 // Number of renders
    
    // State management
    bool is_initialized;                  // Initialization state
    SubsystemStatus current_status;          // Current subsystem status
    char error_message[256];              // Last error message
    
    // Dependencies
    bool physics_dependency_available;       // Whether physics system is available
    bool rendering_dependency_available;     // Whether rendering system is available
    bool audio_dependency_available;        // Whether audio system is available
} FluidDynamicsSubsystem;

// ========================================
// Interface Implementation Functions
// ========================================

// Lifecycle management
static InterfaceResult fluid_dynamics_initialize(const SubsystemConfig *config);
static void fluid_dynamics_shutdown(void);
static bool fluid_dynamics_is_initialized(void);

// Update loop
static void fluid_dynamics_update(float dt);
static void fluid_dynamics_fixed_update(float fixed_dt);
static void fluid_dynamics_late_update(float dt);

// Configuration
static InterfaceResult fluid_dynamics_set_config(const SubsystemConfig *config);
static SubsystemConfig fluid_dynamics_get_config(void);

// Dependencies
static bool fluid_dynamics_register_dependency(const char *dependency_name);
static bool fluid_dynamics_unregister_dependency(const char *dependency_name);
static bool fluid_dynamics_check_dependencies(void);
static uint32_t fluid_dynamics_get_dependency_count(void);
static const char* fluid_dynamics_get_dependency_name(uint32_t index);

// Status and diagnostics
static SubsystemStatus fluid_dynamics_get_status(void);
static const char* fluid_dynamics_get_error_message(void);
static InterfaceResult fluid_diagnostics(char *buffer, size_t buffer_size);

// Resource management
static InterfaceResult fluid_dynamics_acquire_resource(const char *resource_name);
static InterfaceResult fluid_dynamics_release_resource(const char *resource_name);
static bool fluid_dynamics_is_resource_available(const char *resource_name);

// Event handling
static InterfaceResult fluid_dynamics_register_event_handler(uint32_t event_type, void (*handler)(void*, void*));
static InterfaceResult fluid_dynamics_unregister_event_handler(uint32_t event_type);
static InterfaceResult fluid_dynamics_send_event(uint32_t event_type, void *event_data);

// Memory management
static void* fluid_dynamics_allocate_memory(size_t size, const char *tag);
static void fluid_dynamics_free_memory(void *ptr);
static size_t fluid_dynamics_get_memory_usage(void);

// Thread safety
static bool fluid_dynamics_is_thread_safe(void);
static uint32_t fluid_dynamics_get_thread_id(void);

// Performance monitoring
static void fluid_dynamics_start_performance_profile(const char *operation);
static void fluid_dynamics_end_performance_profile(const char *operation);
static void fluid_dynamics_get_performance_stats(char *buffer, size_t buffer_size);

// Validation
static bool fluid_dynamics_validate_state(void);
static InterfaceResult fluid_dynamics_run_self_tests(void);

// ========================================
// Interface Creation and Destruction
// ========================================

/**
 * Create fluid dynamics subsystem interface
 * @param config Configuration for the subsystem
 * @return Interface instance or NULL on failure
 */
IEngineSubsystem* create_fluid_dynamics_interface(const FluidDynamicsConfig *config);

/**
 * Destroy fluid dynamics subsystem interface
 * @param interface Interface instance to destroy
 */
void destroy_fluid_dynamics_interface(IEngineSubsystem *interface);

// ========================================
// Helper Functions
// ========================================

/**
 * Get fluid dynamics subsystem from interface
 * @param interface Interface instance
 * @return Fluid dynamics subsystem pointer
 */
static inline FluidDynamicsSubsystem* get_fluid_dynamics_subsystem(IEngineSubsystem *interface) {
    return (FluidDynamicsSubsystem*)interface;
}

/**
 * Get fluid dynamics system from interface
 * @param interface Interface instance
 * @return Fluid dynamics system pointer
 */
static inline FluidDynamicsSystem* get_fluid_dynamics_system(IEngineSubsystem *interface) {
    FluidDynamicsSubsystem *subsystem = get_fluid_dynamics_subsystem(interface);
    return subsystem ? subsystem->system : NULL;
}

// ========================================
// Service Registration
// ========================================

/**
 * Register fluid dynamics service with dependency injection container
 * @param container DI container
 * @param config Configuration for the service
 * @return True if registration successful
 */
bool register_fluid_dynamics_service(DIContainer *container, const FluidDynamicsConfig *config);

/**
 * Get fluid dynamics service from dependency injection container
 * @param container DI container
 * @return Service instance or NULL if not found
 */
IEngineService* get_fluid_dynamics_service(DIContainer *container);

// ========================================
// Performance Integration
// ========================================

/**
 * Add performance profiling to fluid dynamics operations
 * @param operation Name of the operation being profiled
 * @return True if profiling started successfully
 */
bool fluid_dynamics_start_profiling(const char *operation);

/**
 * End performance profiling for fluid dynamics operations
 * @param operation Name of the operation being profiled
 * @return True if profiling ended successfully
 */
bool fluid_dynamics_end_profiling(const char *operation);

/**
 * Get fluid dynamics performance statistics
 * @param stats Output statistics structure
 * @return True if statistics retrieved successfully
 */
bool fluid_dynamics_get_performance_stats(FluidDynamicsPerformanceStats *stats);

#ifdef __cplusplus
}
#endif

#endif /* FLUID_DYNAMICS_INTERFACE_H */
