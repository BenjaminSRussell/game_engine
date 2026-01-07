#ifndef VFX_MANAGER_H
#define VFX_MANAGER_H

#include "include/core/types.h"
#include "math/vec3.h"
#include "include/core/memory.h"
#include "include/effects/vfx/fluid_simulation.h"
#include "include/effects/vfx/destruction_system.h"
#include "include/effects/vfx/weather_system.h"
#include "include/environment/terrain/terrain_clipmap.h"

// VFX Manager configuration
#define VFX_MAX_FLUID_SIMULATIONS 4
#define VFX_MAX_DESTRUCTION_EVENTS 8
#define VFX_UPDATE_PRIORITY_HIGH 0
#define VFX_UPDATE_PRIORITY_MEDIUM 1
#define VFX_UPDATE_PRIORITY_LOW 2

// VFX system states
typedef enum {
    VFX_SYSTEM_INACTIVE,
    VFX_SYSTEM_INITIALIZING,
    VFX_SYSTEM_ACTIVE,
    VFX_SYSTEM_SHUTTING_DOWN
} VFXSystemState;

// VFX performance profile
typedef enum {
    VFX_PROFILE_LOW,
    VFX_PROFILE_MEDIUM,
    VFX_PROFILE_HIGH,
    VFX_PROFILE_ULTRA
} VFXProfile;

// VFX manager statistics
typedef struct {
    // Fluid simulation stats
    u32 active_fluid_simulations;
    u64 fluid_simulation_time_ms;
    u32 total_fluid_particles;
    
    // Destruction system stats
    u32 active_destruction_events;
    u64 destruction_time_ms;
    u32 total_fracture_pieces;
    
    // Weather system stats
    u64 weather_update_time_ms;
    u32 active_precipitation_zones;
    f32 current_visibility;
    
    // Terrain clipmap stats
    u64 terrain_update_time_ms;
    u32 active_clipmap_levels;
    u32 terrain_vertices_rendered;
    
    // Overall performance
    u64 total_vfx_time_ms;
    f32 vfx_cpu_usage;
    f32 vfx_memory_usage_mb;
} VFXStatistics;

// VFX manager
typedef struct {
    // System states
    VFXSystemState fluid_state;
    VFXSystemState destruction_state;
    VFXSystemState weather_state;
    VFXSystemState terrain_state;
    
    // System instances
    FluidSimulationSystem fluid_simulations[VFX_MAX_FLUID_SIMULATIONS];
    u32 fluid_simulation_count;
    
    DestructionSystem destruction_systems[VFX_MAX_DESTRUCTION_EVENTS];
    u32 destruction_system_count;
    
    WeatherSystem weather_system;
    TerrainClipmapSystem terrain_clipmap;
    
    // Performance settings
    VFXProfile current_profile;
    bool enable_vfx;
    bool enable_gpu_acceleration;
    f32 global_time_scale;
    
    // Update scheduling
    u32 update_frame_counter;
    u32 fluid_update_frequency;
    u32 destruction_update_frequency;
    u32 weather_update_frequency;
    u32 terrain_update_frequency;
    
    // Camera and viewer
    Vec3 camera_position;
    Vec3 camera_direction;
    Vec3 viewer_position;
    Mat4 view_matrix;
    Mat4 projection_matrix;
    
    // Statistics
    VFXStatistics statistics;
    
    // Timing
    f64 last_update_time;
    f32 accumulated_delta_time;
} VFXManager;

// Core API functions
void vfx_manager_init(VFXManager* manager);
void vfx_manager_shutdown(VFXManager* manager);
void vfx_manager_update(VFXManager* manager, f32 delta_time);
void vfx_manager_render(VFXManager* manager);

// System management
void vfx_manager_enable_systems(VFXManager* manager, bool enable);
void vfx_manager_set_profile(VFXManager* manager, VFXProfile profile);
void vfx_manager_set_time_scale(VFXManager* manager, f32 time_scale);

// Camera management
void vfx_manager_set_camera(VFXManager* manager, Vec3 position, Vec3 direction, Mat4 view_matrix, Mat4 projection_matrix);
void vfx_manager_set_viewer_position(VFXManager* manager, Vec3 position);

// Fluid simulation management
u32 vfx_manager_create_fluid_simulation(VFXManager* manager);
void vfx_manager_destroy_fluid_simulation(VFXManager* manager, u32 simulation_id);
FluidSimulationSystem* vfx_manager_get_fluid_simulation(VFXManager* manager, u32 simulation_id);
void vfx_manager_add_fluid_particle(VFXManager* manager, u32 simulation_id, Vec3 position, Vec3 velocity);

// Destruction system management
void vfx_manager_trigger_destruction(VFXManager* manager, Vec3 impact_point, Vec3 impact_direction, f32 impact_force);
void vfx_manager_trigger_recursive_fracture(VFXManager* manager, u32 piece_id, Vec3 fracture_point, f32 energy);

// Weather system management
void vfx_manager_set_weather_state(VFXManager* manager, WeatherState state);
void vfx_manager_set_time_of_day(VFXManager* manager, f32 hours);
void vfx_manager_set_precipitation_intensity(VFXManager* manager, f32 rain_intensity, f32 snow_intensity);

// Terrain management
void vfx_manager_load_terrain_heightmap(VFXManager* manager, const char* heightmap_path);
f32 vfx_manager_get_terrain_height(VFXManager* manager, f32 world_x, f32 world_z);
Vec3 vfx_manager_get_terrain_normal(VFXManager* manager, f32 world_x, f32 world_z);

// Performance and statistics
void vfx_manager_get_statistics(VFXManager* manager, VFXStatistics* stats);
void vfx_manager_reset_statistics(VFXManager* manager);

// Utility functions
bool vfx_manager_is_system_active(VFXManager* manager);
VFXProfile vfx_manager_get_current_profile(VFXManager* manager);
f32 vfx_manager_get_global_time_scale(VFXManager* manager);

#endif // VFX_MANAGER_H
