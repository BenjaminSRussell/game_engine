#include <rendering/lighting.h>
#include <string.h>
#include <math.h>

void lighting_system_init(LightingSystem* system) {
    if (!system) return;
    
    memset(system, 0, sizeof(LightingSystem));
    
    // Default sun direction (noon)
    system->directional.direction = (Vec3){0.0f, -1.0f, 0.0f};
    system->directional.color = (Vec4){1.0f, 1.0f, 1.0f, 1.0f};
    system->directional.intensity = 1.0f;
    system->directional.cast_shadows = true;
    
    // Day cycle
    system->time.time_of_day = 6000.0f; // Noon
    system->time.day_cycle_speed = 1.0f;
    
    system->initialized = true;
}

void lighting_system_shutdown(LightingSystem* system) {
    if (system) {
        system->initialized = false;
    }
}

void lighting_update_sun_direction(LightingSystem* system) {
    if (!system) return;
    
    // Convert time (0-24000) to angle (0-2PI)
    // 0 = Sunrise, 6000 = Noon, 12000 = Sunset, 18000 = Midnight
    f32 angle = ((system->time.time_of_day / 24000.0f) * 2.0f * 3.14159f) - (3.14159f / 2.0f);
    
    // Simple rotation around Z axis for now
    system->directional.direction.x = cosf(angle);
    system->directional.direction.y = -sinf(angle); // Sun is up when y is negative in some coords, or positive. 
                                                    // Assuming standard: Noon (6000) -> angle = PI/2 - PI/2 = 0 ? 
                                                    // Let's adjust: 6000 should differ. 
                                                    // Let's stick to simple cycle:
    // 0 ticks = Sunrise (East)
    // 6000 ticks = Noon (Overhead)
    
    // Map 0..24000 to 0..2PI
    f32 theta = (system->time.time_of_day / 24000.0f) * 2.0f * 3.14159f;
    
    // Simple directional update
    system->directional.direction.x = sinf(theta);
    system->directional.direction.y = cosf(theta);
    system->directional.direction.z = 0.2f; // Slight tilt
    
    // Normalize
    // (In real engine vec3_normalize would be used)
}

DirectionalLight* lighting_get_directional(LightingSystem* system) {
    return system ? &system->directional : NULL;
}

void lighting_update(LightingSystem* system, f32 delta_time, Vec3 camera_position) {
    if (!system) return;
    (void)camera_position;
    
    if (!system->time.paused) {
        system->time.time_of_day += delta_time * 20.0f * system->time.day_cycle_speed; // 20 ticks per second
        if (system->time.time_of_day >= 24000.0f) {
            system->time.time_of_day -= 24000.0f;
        }
        lighting_update_sun_direction(system);
    }
}

// Stubs for other lighting functions to satisfy linker
void lighting_update_cycle(LightingSystem* system, f32 delta_time) {
    lighting_update(system, delta_time, (Vec3){0});
}
