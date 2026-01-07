#include "../include/vfx/vfx_manager.h"
#include "../core/logger.h"
#include "../core/memory.h"
#include "../core/time.h"
#include <string.h>
#include <math.h>

/**
 * =================================================================================================
 *                                   VFX MANAGER - IMPLEMENTATION
 * =================================================================================================
 */

// Internal helper functions
static void vfx_manager_update_performance_settings(VFXManager* manager);
static void vfx_manager_update_statistics(VFXManager* manager, f32 delta_time);

void vfx_manager_init(VFXManager* manager) {
    if (!manager) return;
    
    memset(manager, 0, sizeof(VFXManager));
    
    // Set initial states
    manager->fluid_state = VFX_SYSTEM_INACTIVE;
    manager->destruction_state = VFX_SYSTEM_INACTIVE;
    manager->weather_state = VFX_SYSTEM_INACTIVE;
    manager->terrain_state = VFX_SYSTEM_INACTIVE;
    
    // Set default settings
    manager->current_profile = VFX_PROFILE_HIGH;
    manager->enable_vfx = true;
    manager->enable_gpu_acceleration = true;
    manager->global_time_scale = 1.0f;
    
    // Set update frequencies
    manager->fluid_update_frequency = 1;      // Every frame
    manager->destruction_update_frequency = 1; // Every frame
    manager->weather_update_frequency = 1;     // Every frame
    manager->terrain_update_frequency = 1;     // Every frame
    
    // Initialize camera
    manager->camera_position = vec3_zero();
    manager->camera_direction = vec3_forward();
    manager->viewer_position = vec3_zero();
    manager->view_matrix = mat4_identity();
    manager->projection_matrix = mat4_identity();
    
    // Initialize timing
    manager->last_update_time = time_get_current_ms();
    manager->accumulated_delta_time = 0.0f;
    
    // Initialize systems based on profile
    vfx_manager_update_performance_settings(manager);
    
    LOG_INFO(\"VFX Manager initialized\");
    LOG_INFO(\"  Profile: %d\", manager->current_profile);
    LOG_INFO(\"  GPU acceleration: %s\", manager->enable_gpu_acceleration ? \"enabled\" : \"disabled\");
}

void vfx_manager_shutdown(VFXManager* manager) {
    if (!manager) return;
    
    // Shutdown all systems
    for (u32 i = 0; i < manager->fluid_simulation_count; i++) {
        fluid_simulation_shutdown(&manager->fluid_simulations[i]);
    }
    manager->fluid_simulation_count = 0;
    manager->fluid_state = VFX_SYSTEM_SHUTTING_DOWN;
    
    for (u32 i = 0; i < manager->destruction_system_count; i++) {
        destruction_system_shutdown(&manager->destruction_systems[i]);
    }
    manager->destruction_system_count = 0;
    manager->destruction_state = VFX_SYSTEM_SHUTTING_DOWN;
    
    weather_system_shutdown(&manager->weather_system);
    manager->weather_state = VFX_SYSTEM_SHUTTING_DOWN;
    
    terrain_clipmap_shutdown(&manager->terrain_clipmap);
    manager->terrain_state = VFX_SYSTEM_SHUTTING_DOWN;
    
    // Log final statistics
    LOG_INFO(\"VFX Manager shutdown\");
    LOG_INFO(\"  Total fluid simulations: %u\", manager->statistics.active_fluid_simulations);
    LOG_INFO(\"  Total fracture pieces: %u\", manager->statistics.total_fracture_pieces);
    LOG_INFO(\"  Total VFX time: %llu ms\", manager->statistics.total_vfx_time_ms);
    
    memset(manager, 0, sizeof(VFXManager));
}

void vfx_manager_update(VFXManager* manager, f32 delta_time) {
    if (!manager || !manager->enable_vfx) return;
    
    u64 start_time = time_get_current_ms();
    
    // Apply time scale
    f32 scaled_delta_time = delta_time * manager->global_time_scale;
    manager->accumulated_delta_time += scaled_delta_time;
    
    // Update frame counter
    manager->update_frame_counter++;
    
    // Update fluid simulations
    if (manager->fluid_state == VFX_SYSTEM_ACTIVE && 
        (manager->update_frame_counter % manager->fluid_update_frequency) == 0) {
        
        u64 fluid_start = time_get_current_ms();
        
        for (u32 i = 0; i < manager->fluid_simulation_count; i++) {
            fluid_simulation_update(&manager->fluid_simulations[i], scaled_delta_time);
        }
        
        u64 fluid_end = time_get_current_ms();
        manager->statistics.fluid_simulation_time_ms += (fluid_end - fluid_start);
    }
    
    // Update destruction systems
    if (manager->destruction_state == VFX_SYSTEM_ACTIVE && 
        (manager->update_frame_counter % manager->destruction_update_frequency) == 0) {
        
        u64 destruction_start = time_get_current_ms();
        
        for (u32 i = 0; i < manager->destruction_system_count; i++) {
            destruction_system_update(&manager->destruction_systems[i], scaled_delta_time);
        }
        
        u64 destruction_end = time_get_current_ms();
        manager->statistics.destruction_time_ms += (destruction_end - destruction_start);
    }
    
    // Update weather system
    if (manager->weather_state == VFX_SYSTEM_ACTIVE && 
        (manager->update_frame_counter % manager->weather_update_frequency) == 0) {
        
        u64 weather_start = time_get_current_ms();
        
        weather_system_update(&manager->weather_system, scaled_delta_time);
        
        u64 weather_end = time_get_current_ms();
        manager->statistics.weather_update_time_ms += (weather_end - weather_start);
    }
    
    // Update terrain clipmap
    if (manager->terrain_state == VFX_SYSTEM_ACTIVE && 
        (manager->update_frame_counter % manager->terrain_update_frequency) == 0) {
        
        u64 terrain_start = time_get_current_ms();
        
        terrain_clipmap_update(&manager->terrain_clipmap, manager->viewer_position, scaled_delta_time);
        
        u64 terrain_end = time_get_current_ms();
        manager->statistics.terrain_update_time_ms += (terrain_end - terrain_start);
    }
    
    // Update statistics
    vfx_manager_update_statistics(manager, scaled_delta_time);
    
    u64 end_time = time_get_current_ms();
    manager->statistics.total_vfx_time_ms += (end_time - start_time);
    manager->last_update_time = end_time;
}

void vfx_manager_render(VFXManager* manager) {
    if (!manager || !manager->enable_vfx) return;
    
    u64 start_time = time_get_current_ms();
    
    // Render terrain clipmap (render first as background)
    if (manager->terrain_state == VFX_SYSTEM_ACTIVE) {
        terrain_clipmap_render(&manager->terrain_clipmap);
    }
    
    // Render weather effects
    if (manager->weather_state == VFX_SYSTEM_ACTIVE) {
        // Render precipitation particles
        weather_render_rain_particles(&manager->weather_system);
        weather_render_snow_particles(&manager->weather_system);
        
        // Render volumetric clouds
        weather_render_volumetric_clouds(&manager->weather_system);
        
        // Render celestial bodies
        weather_render_star_map(&manager->weather_system);
    }
    
    // Render destruction pieces
    if (manager->destruction_state == VFX_SYSTEM_ACTIVE) {
        // In a real implementation, would render fracture pieces
        for (u32 i = 0; i < manager->destruction_system_count; i++) {
            // Render pieces from destruction system i
        }
    }
    
    // Render fluid simulation
    if (manager->fluid_state == VFX_SYSTEM_ACTIVE) {
        for (u32 i = 0; i < manager->fluid_simulation_count; i++) {
            // Render fluid particles from simulation i
            // In a real implementation, would use screen-space fluid rendering or marching cubes
        }
    }
    
    u64 end_time = time_get_current_ms();
    manager->statistics.total_vfx_time_ms += (end_time - start_time);
}

void vfx_manager_enable_systems(VFXManager* manager, bool enable) {
    if (!manager) return;
    
    manager->enable_vfx = enable;
    
    if (enable) {
        // Activate systems if they're initialized
        if (manager->fluid_simulation_count > 0) {
            manager->fluid_state = VFX_SYSTEM_ACTIVE;
        }
        if (manager->destruction_system_count > 0) {
            manager->destruction_state = VFX_SYSTEM_ACTIVE;
        }
        if (manager->weather_state == VFX_SYSTEM_INACTIVE) {
            manager->weather_state = VFX_SYSTEM_ACTIVE;
        }
        if (manager->terrain_state == VFX_SYSTEM_INACTIVE) {
            manager->terrain_state = VFX_SYSTEM_ACTIVE;
        }
    } else {
        // Deactivate all systems
        manager->fluid_state = VFX_SYSTEM_INACTIVE;
        manager->destruction_state = VFX_SYSTEM_INACTIVE;
        manager->weather_state = VFX_SYSTEM_INACTIVE;
        manager->terrain_state = VFX_SYSTEM_INACTIVE;
    }
    
    LOG_INFO(\"VFX systems %s\", enable ? \"enabled\" : \"disabled\");
}

void vfx_manager_set_profile(VFXManager* manager, VFXProfile profile) {
    if (!manager) return;
    
    manager->current_profile = profile;
    vfx_manager_update_performance_settings(manager);
    
    LOG_INFO(\"VFX profile set to: %d\", profile);
}

void vfx_manager_set_time_scale(VFXManager* manager, f32 time_scale) {
    if (!manager) return;
    
    manager->global_time_scale = fmaxf(0.0f, time_scale);
}

void vfx_manager_set_camera(VFXManager* manager, Vec3 position, Vec3 direction, Mat4 view_matrix, Mat4 projection_matrix) {
    if (!manager) return;
    
    manager->camera_position = position;
    manager->camera_direction = direction;
    manager->view_matrix = view_matrix;
    manager->projection_matrix = projection_matrix;
    
    // Update systems that need camera information
    if (manager->terrain_state == VFX_SYSTEM_ACTIVE) {
        terrain_clipmap_set_view_matrix(&manager->terrain_clipmap, view_matrix);
        terrain_clipmap_set_projection_matrix(&manager->terrain_clipmap, projection_matrix);
    }
}

void vfx_manager_set_viewer_position(VFXManager* manager, Vec3 position) {
    if (!manager) return;
    
    manager->viewer_position = position;
    
    // Update systems that need viewer position
    if (manager->weather_state == VFX_SYSTEM_ACTIVE) {
        // Weather system might use viewer position for local weather effects
    }
}

u32 vfx_manager_create_fluid_simulation(VFXManager* manager) {
    if (!manager || manager->fluid_simulation_count >= VFX_MAX_FLUID_SIMULATIONS) {
        return 0;
    }
    
    u32 simulation_id = manager->fluid_simulation_count + 1;
    FluidSimulationSystem* simulation = &manager->fluid_simulations[manager->fluid_simulation_count];
    
    fluid_simulation_init(simulation);
    
    // Configure based on current profile
    if (manager->current_profile >= VFX_PROFILE_HIGH) {
        simulation->use_gpu_simulation = manager->enable_gpu_acceleration;
    }
    
    manager->fluid_simulation_count++;
    manager->fluid_state = VFX_SYSTEM_ACTIVE;
    
    LOG_INFO(\"Created fluid simulation %u\", simulation_id);
    return simulation_id;
}

void vfx_manager_destroy_fluid_simulation(VFXManager* manager, u32 simulation_id) {
    if (!manager || simulation_id == 0 || simulation_id > manager->fluid_simulation_count) {
        return;
    }
    
    FluidSimulationSystem* simulation = &manager->fluid_simulations[simulation_id - 1];
    fluid_simulation_shutdown(simulation);
    
    // Move last simulation to this position
    if (simulation_id < manager->fluid_simulation_count) {
        manager->fluid_simulations[simulation_id - 1] = manager->fluid_simulations[manager->fluid_simulation_count - 1];
    }
    
    manager->fluid_simulation_count--;
    
    if (manager->fluid_simulation_count == 0) {
        manager->fluid_state = VFX_SYSTEM_INACTIVE;
    }
    
    LOG_INFO(\"Destroyed fluid simulation %u\", simulation_id);
}

FluidSimulationSystem* vfx_manager_get_fluid_simulation(VFXManager* manager, u32 simulation_id) {
    if (!manager || simulation_id == 0 || simulation_id > manager->fluid_simulation_count) {
        return NULL;
    }
    
    return &manager->fluid_simulations[simulation_id - 1];
}

void vfx_manager_add_fluid_particle(VFXManager* manager, u32 simulation_id, Vec3 position, Vec3 velocity) {
    FluidSimulationSystem* simulation = vfx_manager_get_fluid_simulation(manager, simulation_id);
    if (simulation) {
        fluid_simulation_add_particle(simulation, position, velocity);
    }
}

void vfx_manager_trigger_destruction(VFXManager* manager, Vec3 impact_point, Vec3 impact_direction, f32 impact_force) {
    if (!manager || manager->destruction_system_count == 0) return;
    
    // Use the first available destruction system
    DestructionSystem* destruction = &manager->destruction_systems[0];
    destruction_generate_voronoi_fracture(destruction, impact_point, impact_direction, impact_force);
    
    if (manager->destruction_state == VFX_SYSTEM_INACTIVE) {
        manager->destruction_state = VFX_SYSTEM_ACTIVE;
    }
}

void vfx_manager_trigger_recursive_fracture(VFXManager* manager, u32 piece_id, Vec3 fracture_point, f32 energy) {
    if (!manager || manager->destruction_system_count == 0) return;
    
    DestructionSystem* destruction = &manager->destruction_systems[0];
    destruction_generate_recursive_fracture(destruction, piece_id, fracture_point, energy);
}

void vfx_manager_set_weather_state(VFXManager* manager, WeatherState state) {
    if (!manager) return;
    
    if (manager->weather_state == VFX_SYSTEM_INACTIVE) {
        weather_system_init(&manager->weather_system);
        manager->weather_state = VFX_SYSTEM_ACTIVE;
    }
    
    weather_set_weather_state(&manager->weather_system, state);
}

void vfx_manager_set_time_of_day(VFXManager* manager, f32 hours) {
    if (!manager || manager->weather_state == VFX_SYSTEM_INACTIVE) return;
    
    weather_set_time_of_day(&manager->weather_system, hours);
}

void vfx_manager_set_precipitation_intensity(VFXManager* manager, f32 rain_intensity, f32 snow_intensity) {
    if (!manager || manager->weather_state == VFX_SYSTEM_INACTIVE) return;
    
    weather_set_rain_intensity(&manager->weather_system, rain_intensity);
    weather_set_snow_intensity(&manager->weather_system, snow_intensity);
}

void vfx_manager_load_terrain_heightmap(VFXManager* manager, const char* heightmap_path) {
    if (!manager) return;
    
    if (manager->terrain_state == VFX_SYSTEM_INACTIVE) {
        terrain_clipmap_init(&manager->terrain_clipmap);
        manager->terrain_state = VFX_SYSTEM_ACTIVE;
    }
    
    terrain_clipmap_load_heightmap(&manager->terrain_clipmap, heightmap_path);
}

f32 vfx_manager_get_terrain_height(VFXManager* manager, f32 world_x, f32 world_z) {
    if (!manager || manager->terrain_state == VFX_SYSTEM_INACTIVE) return 0.0f;
    
    return terrain_clipmap_get_height_at(&manager->terrain_clipmap, world_x, world_z);
}

Vec3 vfx_manager_get_terrain_normal(VFXManager* manager, f32 world_x, f32 world_z) {
    if (!manager || manager->terrain_state == VFX_SYSTEM_INACTIVE) return vec3_up();
    
    return terrain_clipmap_get_normal_at(&manager->terrain_clipmap, world_x, world_z);
}

void vfx_manager_get_statistics(VFXManager* manager, VFXStatistics* stats) {
    if (!manager || !stats) return;
    
    *stats = manager->statistics;
    
    // Update current values
    stats->active_fluid_simulations = manager->fluid_simulation_count;
    stats->active_destruction_events = manager->destruction_system_count;
    stats->active_precipitation_zones = manager->weather_system.precipitation_zone_count;
    stats->current_visibility = weather_get_current_visibility(&manager->weather_system);
    
    // Get terrain statistics
    u32 vertices, triangles;
    u64 update_time, render_time;
    terrain_clipmap_get_statistics(&manager->terrain_clipmap, &vertices, &triangles, &update_time, &render_time);
    stats->terrain_vertices_rendered = vertices;
    stats->active_clipmap_levels = manager->terrain_clipmap.active_levels;
    
    // Calculate total fluid particles
    stats->total_fluid_particles = 0;
    for (u32 i = 0; i < manager->fluid_simulation_count; i++) {
        stats->total_fluid_particles += manager->fluid_simulations[i].particle_count;
    }
    
    // Calculate total fracture pieces
    stats->total_fracture_pieces = 0;
    for (u32 i = 0; i < manager->destruction_system_count; i++) {
        stats->total_fracture_pieces += manager->destruction_systems[i].piece_count;
    }
}

void vfx_manager_reset_statistics(VFXManager* manager) {
    if (!manager) return;
    
    memset(&manager->statistics, 0, sizeof(VFXStatistics));
}

// Internal helper functions
static void vfx_manager_update_performance_settings(VFXManager* manager) {
    if (!manager) return;
    
    switch (manager->current_profile) {
        case VFX_PROFILE_LOW:
            manager->fluid_update_frequency = 2;      // Every 2 frames
            manager->destruction_update_frequency = 2; // Every 2 frames
            manager->weather_update_frequency = 2;     // Every 2 frames
            manager->terrain_update_frequency = 2;     // Every 2 frames
            manager->enable_gpu_acceleration = false;
            break;
            
        case VFX_PROFILE_MEDIUM:
            manager->fluid_update_frequency = 1;
            manager->destruction_update_frequency = 1;
            manager->weather_update_frequency = 1;
            manager->terrain_update_frequency = 1;
            manager->enable_gpu_acceleration = true;
            break;
            
        case VFX_PROFILE_HIGH:
            manager->fluid_update_frequency = 1;
            manager->destruction_update_frequency = 1;
            manager->weather_update_frequency = 1;
            manager->terrain_update_frequency = 1;
            manager->enable_gpu_acceleration = true;
            break;
            
        case VFX_PROFILE_ULTRA:
            manager->fluid_update_frequency = 1;
            manager->destruction_update_frequency = 1;
            manager->weather_update_frequency = 1;
            manager->terrain_update_frequency = 1;
            manager->enable_gpu_acceleration = true;
            // Additional ultra settings would be applied here
            break;
    }
    
    // Update existing systems with new settings
    for (u32 i = 0; i < manager->fluid_simulation_count; i++) {
        manager->fluid_simulations[i].use_gpu_simulation = manager->enable_gpu_acceleration;
    }
}

static void vfx_manager_update_statistics(VFXManager* manager, f32 delta_time) {
    if (!manager) return;
    
    // Update CPU usage (simplified calculation)
    static f32 cpu_usage_accumulator = 0.0f;
    static u32 cpu_usage_samples = 0;
    
    if (delta_time > 0.0f) {
        f32 frame_time = (f32)manager->statistics.total_vfx_time_ms / 1000.0f;
        f32 cpu_usage = (frame_time / delta_time) * 100.0f;
        
        cpu_usage_accumulator += cpu_usage;
        cpu_usage_samples++;
        
        if (cpu_usage_samples >= 60) {  // Average over 60 frames
            manager->statistics.vfx_cpu_usage = cpu_usage_accumulator / cpu_usage_samples;
            cpu_usage_accumulator = 0.0f;
            cpu_usage_samples = 0;
        }
    }
    
    // Update memory usage (simplified estimation)
    f32 memory_usage = 0.0f;
    
    // Fluid simulation memory
    for (u32 i = 0; i < manager->fluid_simulation_count; i++) {
        memory_usage += manager->fluid_simulations[i].particle_count * sizeof(FluidParticle) / (1024.0f * 1024.0f);
    }
    
    // Destruction system memory
    for (u32 i = 0; i < manager->destruction_system_count; i++) {
        memory_usage += manager->destruction_systems[i].piece_count * sizeof(FracturePiece) / (1024.0f * 1024.0f);
    }
    
    // Weather system memory
    memory_usage += manager->weather_system.precipitation_zone_count * sizeof(PrecipitationZone) / (1024.0f * 1024.0f);
    
    // Terrain clipmap memory
    memory_usage += manager->terrain_clipmap.active_levels * TERRAIN_CLIPMAP_SIZE * TERRAIN_CLIPMAP_SIZE * sizeof(f32) / (1024.0f * 1024.0f);
    
    manager->statistics.vfx_memory_usage_mb = memory_usage;
}

// Public utility functions
bool vfx_manager_is_system_active(VFXManager* manager) {
    return manager ? manager->enable_vfx : false;
}

VFXProfile vfx_manager_get_current_profile(VFXManager* manager) {
    return manager ? manager->current_profile : VFX_PROFILE_MEDIUM;
}

f32 vfx_manager_get_global_time_scale(VFXManager* manager) {
    return manager ? manager->global_time_scale : 1.0f;
}
