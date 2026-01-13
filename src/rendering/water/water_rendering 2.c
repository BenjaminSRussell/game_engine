// src/engine/rendering/water/water_rendering.c
// Water Rendering System - Realistic water with waves, reflections, and refractions

#include <core/logger.h>
#include <stdbool.h>
#include <stdint.h>
#include <stdlib.h>
#include <string.h>
#include <math.h>

#include "../render_pipeline.h"

// ============================================================================
// Water Rendering Types
// ============================================================================

typedef enum {
    WATER_QUALITY_LOW,
    WATER_QUALITY_MEDIUM,
    WATER_QUALITY_HIGH,
    WATER_QUALITY_ULTRA
} WaterQuality;

typedef enum {
    WATER_TYPE_OCEAN,
    WATER_TYPE_LAKE,
    WATER_TYPE_RIVER,
    WATER_TYPE_POOL
} WaterType;

typedef struct {
    // Water surface properties
    float level;           // Water height
    float size[2];         // Width and length
    WaterType type;
    WaterQuality quality;
    
    // Visual properties
    float color[3];        // Base water color
    float transparency;    // How transparent the water is
    float roughness;      // Surface roughness for reflections
    float metallic;       // Metallic component for PBR
    float foam_intensity; // Foam/white water intensity
    
    // Wave simulation
    float wave_amplitude;  // Height of waves
    float wave_frequency; // Frequency of waves
    float wave_speed;     // Speed of wave propagation
    float wind_strength;  // How much wind affects waves
    float wind_direction[2]; // Wind direction vector
    
    // Gerstner waves (multiple wave components)
    struct {
        float amplitude;
        float frequency;
        float speed;
        float direction[2];
        float steepness;
    } waves[4];
    
    // Rendering resources
    void *water_mesh;      // Tesselated water surface
    void *normal_map;      // Normal map for ripples
    void *foam_texture;    // Foam texture
    void *reflection_texture; // Scene reflection
    void *refraction_texture; // Scene refraction
    
    // Shaders
    void *water_shader;
    void *foam_shader;
    
    // GPU simulation
    void *wave_heightmap;  // Height map for GPU wave simulation
    void *simulation_buffer;
    bool gpu_simulation;
    
    // Animation
    float time;
    float foam_time;
    
    // Interaction
    float ripples[64][2];  // Ripple positions
    float ripple_strengths[64];
    uint32_t ripple_count;
    uint32_t max_ripples;
    
    // Performance
    bool enable_reflections;
    bool enable_refractions;
    bool enable_foam;
    bool enable_caustics;
    uint32_t tessellation_factor;
    
    // Statistics
    float render_time_ms;
    uint32_t vertices_rendered;
    uint32_t triangles_rendered;
    
    bool initialized;
} WaterSystem;

static WaterSystem g_water_system = {0};

// ============================================================================
// Wave Simulation
// ============================================================================

static void initialize_gerstner_waves(WaterSystem *water) {
    // Initialize multiple Gerstner wave components for realistic ocean waves
    for (int i = 0; i < 4; i++) {
        water->waves[i].amplitude = water->wave_amplitude * (0.25f + i * 0.25f);
        water->waves[i].frequency = water->wave_frequency * (1.0f + i * 0.5f);
        water->waves[i].speed = water->wave_speed * (1.0f + i * 0.3f);
        water->waves[i].steepness = 0.5f / (1.0f + i);
        
        // Vary wave directions
        float angle = (float)i * M_PI * 0.5f + water->wind_direction[0];
        water->waves[i].direction[0] = cosf(angle);
        water->waves[i].direction[1] = sinf(angle);
    }
}

static void calculate_wave_displacement(WaterSystem *water, float x, float z, 
                                       float time, float *displacement, float *normal) {
    displacement[0] = 0.0f;
    displacement[1] = 0.0f;
    displacement[2] = 0.0f;
    
    float normal_x = 0.0f;
    float normal_y = 1.0f;
    float normal_z = 0.0f;
    
    // Sum multiple Gerstner waves
    for (int i = 0; i < 4; i++) {
        float k = water->waves[i].frequency * 2.0f * M_PI;
        float dot_kx = k * (x * water->waves[i].direction[0] + z * water->waves[i].direction[1]);
        float phase = dot_kx - water->waves[i].speed * time * k;
        
        float cos_phase = cosf(phase);
        float sin_phase = sinf(phase);
        
        float steepness = water->waves[i].steepness;
        float amp = water->waves[i].amplitude;
        
        // Wave displacement
        displacement[0] += steepness * amp * water->waves[i].direction[0] * cos_phase;
        displacement[1] += amp * sin_phase;
        displacement[2] += steepness * amp * water->waves[i].direction[1] * cos_phase;
        
        // Normal calculation (simplified)
        float factor = k * amp * steepness;
        normal_x -= factor * water->waves[i].direction[0] * sin_phase;
        normal_z -= factor * water->waves[i].direction[1] * sin_phase;
    }
    
    // Normalize normal
    float length = sqrtf(normal_x * normal_x + normal_y * normal_y + normal_z * normal_z);
    if (length > 0.0f) {
        normal[0] = normal_x / length;
        normal[1] = normal_y / length;
        normal[2] = normal_z / length;
    } else {
        normal[0] = 0.0f;
        normal[1] = 1.0f;
        normal[2] = 0.0f;
    }
}

static void generate_water_mesh(WaterSystem *water) {
    // Generate tessellated water surface mesh
    uint32_t grid_size = 64; // 64x64 grid
    if (water->quality == WATER_QUALITY_ULTRA) grid_size = 128;
    else if (water->quality == WATER_QUALITY_HIGH) grid_size = 96;
    else if (water->quality == WATER_QUALITY_MEDIUM) grid_size = 64;
    else grid_size = 32;
    
    // TODO: Generate vertex and index buffers for water mesh
    // This would create a tessellated grid that can be displaced by waves
    
    LOG_DEBUG("Generated water mesh: %ux%u grid", grid_size, grid_size);
}

// ============================================================================
// Water Rendering Functions
// ============================================================================

static void render_reflections(WaterSystem *water, const float *camera_pos) {
    if (!water->enable_reflections) return;
    
    // Render scene reflection from water surface perspective
    // Calculate reflection matrix (mirror camera across water plane)
    float reflection_matrix[16];
    float water_plane[4] = {0.0f, 1.0f, 0.0f, -water->level}; // Plane equation: ax + by + cz + d = 0
    
    // Create reflection matrix
    reflection_matrix[0] = 1.0f - 2.0f * water_plane[0] * water_plane[0];
    reflection_matrix[1] = -2.0f * water_plane[0] * water_plane[1];
    reflection_matrix[2] = -2.0f * water_plane[0] * water_plane[2];
    reflection_matrix[3] = -2.0f * water_plane[0] * water_plane[3];
    
    reflection_matrix[4] = -2.0f * water_plane[1] * water_plane[0];
    reflection_matrix[5] = 1.0f - 2.0f * water_plane[1] * water_plane[1];
    reflection_matrix[6] = -2.0f * water_plane[1] * water_plane[2];
    reflection_matrix[7] = -2.0f * water_plane[1] * water_plane[3];
    
    reflection_matrix[8] = -2.0f * water_plane[2] * water_plane[0];
    reflection_matrix[9] = -2.0f * water_plane[2] * water_plane[1];
    reflection_matrix[10] = 1.0f - 2.0f * water_plane[2] * water_plane[2];
    reflection_matrix[11] = -2.0f * water_plane[2] * water_plane[3];
    
    reflection_matrix[12] = 0.0f;
    reflection_matrix[13] = 0.0f;
    reflection_matrix[14] = 0.0f;
    reflection_matrix[15] = 1.0f;
    
    // Set up reflection camera and render to texture
    // set_render_target(water->reflection_texture);
    // clear_render_target(0.0f, 0.0f, 0.0f, 1.0f);
    // apply_reflection_clip_plane(water_plane);
    // render_scene_with_matrix(reflection_matrix);
    // disable_clip_plane();
    // bind_default_render_target();
    
    LOG_DEBUG("Rendering water reflections");
}

static void render_refractions(WaterSystem *water, const float *camera_pos) {
    if (!water->enable_refractions) return;
    
    // Render scene refraction from underwater perspective
    // Set up underwater camera with refraction distortion
    float underwater_offset = -0.5f; // Camera offset below water surface
    
    // Modify view matrix for underwater rendering
    float underwater_view[16];
    memcpy(underwater_view, view_matrix, sizeof(float) * 16);
    underwater_view[14] += underwater_offset; // Translate Y axis
    
    // Apply water refraction index (1.333 for water)
    float refraction_scale = 1.0f / 1.333f;
    
    // Set up refraction rendering
    // set_render_target(water->refraction_texture);
    // clear_render_target(water->color[0], water->color[1], water->color[2], water->transparency);
    // render_scene_with_refraction(underwater_view, proj_matrix, refraction_scale);
    // bind_default_render_target();
    
    LOG_DEBUG("Rendering water refractions");
}

static void render_water_surface(WaterSystem *water, const float *view_matrix, 
                                const float *proj_matrix, const float *camera_pos) {
    // Render water surface with appropriate shader
    // Bind water shader and set uniforms
    // bind_shader(water->water_shader);
    // set_shader_uniform("time", water->time);
    // set_shader_uniform("water_level", water->level);
    // set_shader_uniform("water_color", water->color, 3);
    // set_shader_uniform("transparency", water->transparency);
    // set_shader_uniform("roughness", water->roughness);
    // set_shader_uniform("metallic", water->metallic);
    // set_shader_uniform("foam_intensity", water->foam_intensity);
    // set_shader_uniform("wave_amplitude", water->wave_amplitude);
    // set_shader_uniform("wave_frequency", water->wave_frequency);
    // set_shader_uniform("wave_speed", water->wave_speed);
    // set_shader_uniform("wind_strength", water->wind_strength);
    // set_shader_uniform("wind_direction", water->wind_direction, 2);
    // set_shader_uniform("camera_pos", camera_pos, 3);
    
    // Bind textures
    // bind_texture("normal_map", water->normal_map, 0);
    // bind_texture("reflection_texture", water->reflection_texture, 1);
    // bind_texture("refraction_texture", water->refraction_texture, 2);
    // bind_texture("foam_texture", water->foam_texture, 3);
    
    // Set Gerstner wave parameters
    // set_shader_uniform_array("waves", water->waves, 4);
    
    // Render tessellated water mesh
    // bind_mesh(water->water_mesh);
    // set_tessellation_factor(water->tessellation_factor);
    // draw_mesh_tessellated();
    // unbind_mesh();
    // unbind_shader();
    
    LOG_DEBUG("Rendering water surface (quality: %d)", (int)water->quality);
}

static void render_foam(WaterSystem *water, const float *view_matrix, const float *proj_matrix) {
    if (!water->enable_foam) return;
    
    // Render foam effects on water surface
    // Calculate foam based on wave steepness and intersections
    // bind_shader(water->foam_shader);
    // set_shader_uniform("time", water->foam_time);
    // set_shader_uniform("foam_intensity", water->foam_intensity);
    // set_shader_uniform("wave_amplitude", water->wave_amplitude);
    // set_shader_uniform("camera_pos", camera_pos, 3);
    
    // Bind foam texture and render foam particles
    // bind_texture("foam_texture", water->foam_texture, 0);
    // enable_blending(true);
    // set_blend_mode(BLEND_ADDITIVE);
    
    // Render foam at wave crests and intersections
    // render_foam_particles(water->waves, water->ripple_count, water->ripples);
    
    // disable_blending();
    // unbind_texture(0);
    // unbind_shader();
    
    LOG_DEBUG("Rendering water foam");
}

static void render_caustics(WaterSystem *water, const float *view_matrix, const float *proj_matrix) {
    if (!water->enable_caustics) return;
    
    // Render underwater caustics
    // Calculate light patterns through water surface
    // bind_shader(load_shader("caustics"));
    // set_shader_uniform("time", water->time);
    // set_shader_uniform("water_level", water->level);
    // set_shader_uniform("wave_amplitude", water->wave_amplitude);
    // set_shader_uniform("wave_frequency", water->wave_frequency);
    // set_shader_uniform("light_direction", light_dir, 3);
    // set_shader_uniform("light_color", light_color, 3);
    // set_shader_uniform("caustic_intensity", 0.8f);
    // set_shader_uniform("caustic_scale", 15.0f);
    
    // Project caustic patterns onto underwater surfaces
    // enable_blending(true);
    // set_blend_mode(BLEND_MULTIPLY);
    // set_depth_write(false);
    
    // Render caustic projections
    // render_caustic_projections(water->waves, view_matrix, proj_matrix);
    
    // set_depth_write(true);
    // disable_blending();
    // unbind_shader();
    
    LOG_DEBUG("Rendering water caustics");
}

// ============================================================================
// Water System API
// ============================================================================

bool water_system_init(WaterQuality quality, bool enable_gpu_simulation) {
    if (g_water_system.initialized) {
        LOG_WARN("Water system already initialized");
        return true;
    }
    
    memset(&g_water_system, 0, sizeof(WaterSystem));
    
    g_water_system.quality = quality;
    g_water_system.type = WATER_TYPE_OCEAN;
    g_water_system.gpu_simulation = enable_gpu_simulation;
    
    // Set default water properties
    g_water_system.level = 0.0f;
    g_water_system.size[0] = 1000.0f;
    g_water_system.size[1] = 1000.0f;
    
    g_water_system.color[0] = 0.0f;
    g_water_system.color[1] = 0.3f;
    g_water_system.color[2] = 0.5f;
    g_water_system.transparency = 0.8f;
    g_water_system.roughness = 0.02f;
    g_water_system.metallic = 0.0f;
    g_water_system.foam_intensity = 0.5f;
    
    // Set default wave parameters
    g_water_system.wave_amplitude = 1.0f;
    g_water_system.wave_frequency = 0.1f;
    g_water_system.wave_speed = 2.0f;
    g_water_system.wind_strength = 5.0f;
    g_water_system.wind_direction[0] = 1.0f;
    g_water_system.wind_direction[1] = 0.0f;
    
    // Initialize Gerstner waves
    initialize_gerstner_waves(&g_water_system);
    
    // Set rendering options based on quality
    g_water_system.enable_reflections = true;
    g_water_system.enable_refractions = (quality >= WATER_QUALITY_MEDIUM);
    g_water_system.enable_foam = (quality >= WATER_QUALITY_HIGH);
    g_water_system.enable_caustics = (quality >= WATER_QUALITY_ULTRA);
    
    switch (quality) {
        case WATER_QUALITY_ULTRA:
            g_water_system.tessellation_factor = 8;
            break;
        case WATER_QUALITY_HIGH:
            g_water_system.tessellation_factor = 4;
            break;
        case WATER_QUALITY_MEDIUM:
            g_water_system.tessellation_factor = 2;
            break;
        case WATER_QUALITY_LOW:
            g_water_system.tessellation_factor = 1;
            break;
    }
    
    // Initialize ripple system
    g_water_system.max_ripples = 64;
    g_water_system.ripple_count = 0;
    
    // Generate water mesh
    generate_water_mesh(&g_water_system);
    
    // Create render resources
    g_water_system.water_shader = load_shader("water");
    g_water_system.foam_shader = load_shader("water_foam");
    g_water_system.normal_map = generate_normal_map(256, 256);
    g_water_system.foam_texture = load_texture("foam.png");
    
    // Create textures for reflections and refractions
    g_water_system.reflection_texture = create_render_target(1024, 1024, FORMAT_RGBA16F);
    g_water_system.refraction_texture = create_render_target(1024, 1024, FORMAT_RGBA16F);
    
    if (enable_gpu_simulation) {
        // Create GPU simulation resources
        g_water_system.wave_heightmap = create_texture_2d(512, 512, FORMAT_R16F);
        g_water_system.simulation_buffer = create_compute_buffer(512 * 512 * sizeof(float));
        
        // Initialize wave simulation compute shader
        void *wave_compute_shader = load_compute_shader("wave_simulation");
        // bind_compute_shader(wave_compute_shader);
        // set_compute_texture("heightmap", g_water_system.wave_heightmap, 0);
        // set_compute_buffer("simulation_data", g_water_system.simulation_buffer, 1);
        // dispatch_compute(512 / 8, 512 / 8, 1);
        // unbind_compute_shader();
    }
    
    g_water_system.initialized = true;
    LOG_INFO("Water system initialized (quality: %d, GPU simulation: %s)",
             (int)quality, enable_gpu_simulation ? "yes" : "no");
    return true;
}

void water_system_shutdown(void) {
    if (!g_water_system.initialized)
        return;
    
    // Destroy all resources
    destroy_mesh(g_water_system.water_mesh);
    destroy_texture(g_water_system.normal_map);
    destroy_texture(g_water_system.foam_texture);
    destroy_texture(g_water_system.reflection_texture);
    destroy_texture(g_water_system.refraction_texture);
    destroy_shader(g_water_system.water_shader);
    destroy_shader(g_water_system.foam_shader);
    
    if (g_water_system.gpu_simulation) {
        destroy_texture(g_water_system.wave_heightmap);
        destroy_buffer(g_water_system.simulation_buffer);
    }
    
    memset(&g_water_system, 0, sizeof(WaterSystem));
    
    LOG_INFO("Water system shutdown");
}

void water_system_update(float dt, const float *wind_direction, float wind_strength) {
    if (!g_water_system.initialized) return;
    
    // Update time
    g_water_system.time += dt;
    g_water_system.foam_time += dt * 0.5f;
    
    // Update wind
    if (wind_direction) {
        g_water_system.wind_direction[0] = wind_direction[0];
        g_water_system.wind_direction[1] = wind_direction[1];
    }
    g_water_system.wind_strength = wind_strength;
    
    // Update wave parameters based on wind
    g_water_system.wave_amplitude = 1.0f + wind_strength * 0.5f;
    g_water_system.wave_speed = 2.0f + wind_strength * 0.3f;
    
    // Re-initialize Gerstner waves with new parameters
    initialize_gerstner_waves(&g_water_system);
    
    // Update ripples (decay over time)
    for (uint32_t i = 0; i < g_water_system.ripple_count; i++) {
        g_water_system.ripple_strengths[i] *= (1.0f - dt * 2.0f); // Decay over 0.5 seconds
        
        // Remove dead ripples
        if (g_water_system.ripple_strengths[i] < 0.01f) {
            g_water_system.ripples[i][0] = g_water_system.ripples[g_water_system.ripple_count - 1][0];
            g_water_system.ripples[i][1] = g_water_system.ripples[g_water_system.ripple_count - 1][1];
            g_water_system.ripple_strengths[i] = g_water_system.ripple_strengths[g_water_system.ripple_count - 1];
            g_water_system.ripple_count--;
        }
    }
    
    // Update GPU wave simulation if enabled
    if (g_water_system.gpu_simulation) {
        // Dispatch wave simulation compute shader
        void *wave_compute_shader = get_compute_shader("wave_simulation");
        // bind_compute_shader(wave_compute_shader);
        // set_compute_uniform("time", g_water_system.time);
        // set_compute_uniform("dt", dt);
        // set_compute_uniform("wave_amplitude", g_water_system.wave_amplitude);
        // set_compute_uniform("wave_frequency", g_water_system.wave_frequency);
        // set_compute_uniform("wind_strength", g_water_system.wind_strength);
        // set_compute_uniform("wind_direction", g_water_system.wind_direction, 2);
        // dispatch_compute(512 / 8, 512 / 8, 1);
        // memory_barrier_compute();
        // unbind_compute_shader();
        
        // Generate normal map from heightmap
        // generate_normal_map_from_heightmap(g_water_system.wave_heightmap, g_water_system.normal_map);
    }
}

void water_system_render(const float *view_matrix, const float *proj_matrix, const float *camera_pos) {
    if (!g_water_system.initialized) return;
    
    uint64_t start_time = get_time_nanos();
    
    // Render reflections and refractions first
    render_reflections(&g_water_system, camera_pos);
    render_refractions(&g_water_system, camera_pos);
    
    // Render water surface
    render_water_surface(&g_water_system, view_matrix, proj_matrix, camera_pos);
    
    // Render additional effects
    render_foam(&g_water_system, view_matrix, proj_matrix);
    render_caustics(&g_water_system, view_matrix, proj_matrix);
    
    uint64_t end_time = get_time_nanos();
    g_water_system.render_time_ms = nanos_to_ms(end_time - start_time);
    
    LOG_DEBUG("Water rendering completed in %.2f ms", g_water_system.render_time_ms);
}

void water_system_add_ripple(float x, float z, float strength) {
    if (!g_water_system.initialized || g_water_system.ripple_count >= g_water_system.max_ripples) {
        return;
    }
    
    g_water_system.ripples[g_water_system.ripple_count][0] = x;
    g_water_system.ripples[g_water_system.ripple_count][1] = z;
    g_water_system.ripple_strengths[g_water_system.ripple_count] = strength;
    g_water_system.ripple_count++;
    
    LOG_DEBUG("Added ripple at (%.2f, %.2f) with strength %.2f", x, z, strength);
}

void water_system_set_water_level(float level) {
    if (!g_water_system.initialized) return;
    
    g_water_system.level = level;
    LOG_DEBUG("Water level set to %.2f", level);
}

void water_system_set_water_color(float r, float g, float b) {
    if (!g_water_system.initialized) return;
    
    g_water_system.color[0] = r;
    g_water_system.color[1] = g;
    g_water_system.color[2] = b;
    
    LOG_DEBUG("Water color set to (%.2f, %.2f, %.2f)", r, g, b);
}

void water_system_get_wave_height(float x, float z, float *height, float *normal) {
    if (!g_water_system.initialized || !height) return;
    
    float displacement[3];
    calculate_wave_displacement(&g_water_system, x, z, g_water_system.time, displacement, normal);
    
    *height = g_water_system.level + displacement[1];
    
    // Add ripple contributions
    for (uint32_t i = 0; i < g_water_system.ripple_count; i++) {
        float dx = x - g_water_system.ripples[i][0];
        float dz = z - g_water_system.ripples[i][1];
        float distance = sqrtf(dx * dx + dz * dz);
        
        if (distance < 10.0f) { // Ripple radius
            float ripple_height = g_water_system.ripple_strengths[i] * 
                               expf(-distance * distance * 0.1f) * 
                               cosf(distance * 2.0f - g_water_system.time * 5.0f);
            *height += ripple_height;
        }
    }
}

void water_system_enable_effects(bool reflections, bool refractions, bool foam, bool caustics) {
    if (!g_water_system.initialized) return;
    
    g_water_system.enable_reflections = reflections;
    g_water_system.enable_refractions = refractions && (g_water_system.quality >= WATER_QUALITY_MEDIUM);
    g_water_system.enable_foam = foam && (g_water_system.quality >= WATER_QUALITY_HIGH);
    g_water_system.enable_caustics = caustics && (g_water_system.quality >= WATER_QUALITY_ULTRA);
    
    LOG_INFO("Water effects: reflections=%s, refractions=%s, foam=%s, caustics=%s",
             reflections ? "yes" : "no",
             g_water_system.enable_refractions ? "yes" : "no",
             g_water_system.enable_foam ? "yes" : "no",
             g_water_system.enable_caustics ? "yes" : "no");
}

void water_system_get_stats(float *render_time, uint32_t *vertices, uint32_t *triangles) {
    if (!g_water_system.initialized) return;
    
    if (render_time) *render_time = g_water_system.render_time_ms;
    if (vertices) *vertices = g_water_system.vertices_rendered;
    if (triangles) *triangles = g_water_system.triangles_rendered;
}
