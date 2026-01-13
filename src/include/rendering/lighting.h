// include/render/lighting.h
//
// Purpose: Dynamic lighting system with directional light, point lights, and day/night cycles.
// Integrates with the block lighting system for seamless light propagation.
//
#ifndef RENDERER_LIGHTING_H
#define RENDERER_LIGHTING_H

#include "engine/include/common.h"
#include <math/vec3.h>
#include <math/vec4.h>
#include <math/mat4.h>

// Light types
typedef enum {
    LIGHT_TYPE_DIRECTIONAL,  // Sun/moon light
    LIGHT_TYPE_POINT,        // Lamp, torch, glowstone
    LIGHT_TYPE_SPOT,         // Focused light (flashlight)
    LIGHT_TYPE_AREA,         // Rectangular light
} LightType;

// Day/night cycle phases
typedef enum {
    PHASE_DAWN,              // 4:30 - 6:00 (sunrise)
    PHASE_DAY,               // 6:00 - 18:00 (daylight)
    PHASE_DUSK,              // 18:00 - 19:30 (sunset)
    PHASE_NIGHT,             // 19:30 - 4:30 (nighttime)
} DayPhase;

// Directional light (sun/moon)
typedef struct {
    Vec3 direction;          // Light direction (normalized)
    Vec4 color;              // RGB + intensity
    f32 intensity;           // 0.0-2.0
    f32 ambient_intensity;   // Ambient contribution
    Mat4 view_projection;    // For shadow mapping
    bool cast_shadows;       // Enable shadow mapping
    u32 shadow_map_size;     // Shadow map resolution (512/1024/2048)
} DirectionalLight;

// Point light (lamp, torch, etc)
typedef struct {
    Vec3 position;
    Vec4 color;              // RGB + intensity
    f32 intensity;           // 0.0-4.0
    f32 radius;              // Light radius in meters
    f32 falloff;             // Attenuation curve (0.0-1.0)
    bool cast_shadows;       // Enable shadow mapping
    bool enabled;
} PointLight;

// Spot light (flashlight)
typedef struct {
    Vec3 position;
    Vec3 direction;
    Vec4 color;
    f32 intensity;
    f32 radius;
    f32 inner_angle;         // Inner cone angle in degrees
    f32 outer_angle;         // Outer cone angle in degrees
    f32 falloff;
    bool cast_shadows;
    bool enabled;
} SpotLight;

// Area light (rectangular)
typedef struct {
    Vec3 position;
    Mat4 transform;          // Orientation and size
    Vec4 color;
    f32 intensity;
    f32 width, height;       // Dimensions
    bool cast_shadows;
    bool enabled;
} AreaLight;

// Dynamic lighting system
#define MAX_POINT_LIGHTS 256
#define MAX_SPOT_LIGHTS 32
#define MAX_AREA_LIGHTS 16

typedef struct {
    // Directional light (sun/moon)
    DirectionalLight directional;

    // Dynamic lights
    PointLight point_lights[MAX_POINT_LIGHTS];
    u32 point_light_count;

    SpotLight spot_lights[MAX_SPOT_LIGHTS];
    u32 spot_light_count;

    AreaLight area_lights[MAX_AREA_LIGHTS];
    u32 area_light_count;

    // Day/night cycle
    struct {
        f32 time_of_day;        // 0.0-24000 (ticks in Minecraft)
        f32 day_cycle_speed;    // Relative speed (1.0 = normal)
        DayPhase current_phase;
        bool paused;
    } time;

    // Lighting properties
    struct {
        Vec4 ambient_color;     // Ambient light color
        f32 ambient_intensity;  // Ambient light intensity
        f32 sky_brightness;     // Sky brightness (0.0-1.0)
        Vec4 fog_color;         // Atmospheric fog color
        f32 fog_density;        // Fog density
    } environment;

    // Shadow mapping
    struct {
        bool enabled;
        f32 shadow_bias;        // Bias to prevent shadow acne
        f32 shadow_softness;    // PCF kernel size
        u32 max_resolution;     // Max shadow map resolution
    } shadows;

    bool initialized;
} LightingSystem;

// ==============================================================================
// Lighting System Lifecycle
// ==============================================================================

// Initialize lighting system
void lighting_system_init(LightingSystem* system);

// Shutdown lighting system
void lighting_system_shutdown(LightingSystem* system);

// ==============================================================================
// Directional Light (Sun/Moon)
// ==============================================================================

// Set directional light properties
void lighting_set_directional(LightingSystem* system, DirectionalLight light);

// Get directional light
DirectionalLight* lighting_get_directional(LightingSystem* system);

// Update sun position based on time of day
void lighting_update_sun_direction(LightingSystem* system);

// ==============================================================================
// Point Lights
// ==============================================================================

// Add point light (returns light ID or 0xFFFFFFFF on failure)
u32 lighting_add_point_light(LightingSystem* system, Vec3 position, Vec4 color, f32 radius);

// Update point light properties
void lighting_set_point_light(LightingSystem* system, u32 light_id, PointLight* light);

// Get point light
PointLight* lighting_get_point_light(LightingSystem* system, u32 light_id);

// Remove point light
void lighting_remove_point_light(LightingSystem* system, u32 light_id);

// Enable/disable point light
void lighting_set_point_light_enabled(LightingSystem* system, u32 light_id, bool enabled);

// Get visible point lights for a position (returns count and fills array)
u32 lighting_get_visible_point_lights(LightingSystem* system, Vec3 position,
                                      f32 search_radius, PointLight* out_lights,
                                      u32 max_lights);

// ==============================================================================
// Spot Lights
// ==============================================================================

// Add spot light
u32 lighting_add_spot_light(LightingSystem* system, Vec3 position, Vec3 direction,
                            Vec4 color, f32 radius, f32 angle);

// Update spot light
void lighting_set_spot_light(LightingSystem* system, u32 light_id, SpotLight* light);

// Get spot light
SpotLight* lighting_get_spot_light(LightingSystem* system, u32 light_id);

// Remove spot light
void lighting_remove_spot_light(LightingSystem* system, u32 light_id);

// ==============================================================================
// Area Lights
// ==============================================================================

// Add area light
u32 lighting_add_area_light(LightingSystem* system, Vec3 position,
                            f32 width, f32 height, Vec4 color);

// Remove area light
void lighting_remove_area_light(LightingSystem* system, u32 light_id);

// ==============================================================================
// Day/Night Cycle
// ==============================================================================

// Start day/night cycle
void lighting_cycle_start(LightingSystem* system);

// Stop day/night cycle
void lighting_cycle_stop(LightingSystem* system);

// Set time of day (0.0-24000, Minecraft ticks)
void lighting_set_time(LightingSystem* system, f32 ticks);

// Get current time of day
f32 lighting_get_time(LightingSystem* system);

// Set cycle speed (1.0 = normal)
void lighting_set_cycle_speed(LightingSystem* system, f32 speed);

// Get current day phase
DayPhase lighting_get_phase(LightingSystem* system);

// Update day/night cycle (call once per frame)
void lighting_update_cycle(LightingSystem* system, f32 delta_time);

// ==============================================================================
// Ambient Lighting & Atmosphere
// ==============================================================================

// Set ambient light color and intensity
void lighting_set_ambient(LightingSystem* system, Vec4 color, f32 intensity);

// Set sky brightness (affects overall scene brightness)
void lighting_set_sky_brightness(LightingSystem* system, f32 brightness);

// Set atmospheric fog
void lighting_set_fog(LightingSystem* system, Vec4 color, f32 density);

// Get sky color for current time
Vec4 lighting_get_sky_color(LightingSystem* system);

// Get ambient light for current time
Vec4 lighting_get_ambient_color(LightingSystem* system);

// ==============================================================================
// Shadow Mapping
// ==============================================================================

// Enable/disable shadow mapping
void lighting_set_shadows_enabled(LightingSystem* system, bool enabled);

// Set shadow bias (prevents shadow acne)
void lighting_set_shadow_bias(LightingSystem* system, f32 bias);

// Set shadow softness (PCF kernel size)
void lighting_set_shadow_softness(LightingSystem* system, f32 softness);

// ==============================================================================
// Per-Frame Update
// ==============================================================================

// Update lighting system (call every frame)
void lighting_update(LightingSystem* system, f32 delta_time, Vec3 camera_position);

// ==============================================================================
// Query Functions
// ==============================================================================

// Get total light contribution at position
Vec4 lighting_get_total_light(LightingSystem* system, Vec3 position);

// Get light influence for a specific point light
f32 lighting_get_point_light_influence(PointLight* light, Vec3 position);

// Check if position is in shadow
bool lighting_is_position_shadowed(LightingSystem* system, Vec3 position, Vec3 normal);

// Get dominant light direction at position
Vec3 lighting_get_dominant_direction(LightingSystem* system, Vec3 position);

// ==============================================================================
// Serialization
// ==============================================================================

// Export lighting configuration to bytes
bool lighting_serialize(LightingSystem* system, u8* buffer, u32 buffer_size, u32* out_size);

// Import lighting configuration from bytes
bool lighting_deserialize(u8* buffer, u32 size, LightingSystem* out_system);

#endif // RENDERER_LIGHTING_H
