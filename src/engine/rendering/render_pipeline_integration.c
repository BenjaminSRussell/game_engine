// src/engine/rendering/render_pipeline_integration.c
// Render Pipeline Integration - Unified API for all rendering systems

#include "engine/include/core/logger.h"
#include <stdbool.h>
#include <stdint.h>
#include <stdlib.h>
#include <string.h>

#include "core/renderer.c"
#include "core/render_graph.c"
#include "core/culling.c"
#include "core/material.c"
#include "core/render_stats.c"
#include "gpu_driven/draw_command_gen.c"
#include "deferred/gbuffer_pass.c"
#include "lighting/lighting_system.c"
#include "shadows/shadow_mapping.c"
#include "post_process/post_process_pipeline.c"
#include "particles/particle_system.c"
#include "sky/sky_rendering.c"
#include "water/water_rendering.c"
#include "shader/shader_hot_reload.c"
#include "texture/texture_streaming.c"

// ============================================================================
// Unified Render Pipeline
// ============================================================================

typedef struct {
    // Core systems
    Renderer *renderer;
    RenderGraph *render_graph;
    CullingSystem *culling_system;
    
    // Advanced systems
    LightingSystem *lighting_system;
    ShadowMappingSystem *shadow_system;
    PostProcessPipeline *post_process;
    
    // Environmental systems
    SkySystem *sky_system;
    WaterSystem *water_system;
    ParticleSystem *particle_system;
    
    // GPU-driven systems
    DrawCommandSystem *draw_command_system;
    GBuffer *gbuffer;
    
    // Developer tools
    RenderStatsSystem *stats_system;
    ShaderHotReloadSystem *shader_hot_reload;
    TextureStreamingSystem *texture_streaming;
    
    // Configuration
    uint32_t width;
    uint32_t height;
    bool hdr_enabled;
    bool gpu_driven_enabled;
    bool clustered_shading_enabled;
    
    // Performance settings
    RenderQuality quality;
    uint32_t max_lights;
    uint32_t max_particles;
    uint64_t texture_memory_budget;
    
    // State
    bool initialized;
    bool frame_in_progress;
    
    // Callbacks
    void (*on_frame_start)(void);
    void (*on_frame_end)(void);
    void (*on_error)(const char *error);
} UnifiedRenderPipeline;

static UnifiedRenderPipeline g_pipeline = {0};

// ============================================================================
// Quality Settings
// ============================================================================

typedef enum {
    RENDER_QUALITY_LOW,
    RENDER_QUALITY_MEDIUM,
    RENDER_QUALITY_HIGH,
    RENDER_QUALITY_ULTRA
} RenderQuality;

typedef struct {
    uint32_t shadow_resolution;
    uint32_t shadow_cascades;
    uint32_t particle_count;
    uint32_t cloud_quality;
    uint32_t water_tessellation;
    bool enable_atmospheric_scattering;
    bool enable_volumetric_clouds;
    bool enable_volumetric_fog;
    bool enable_caustics;
    bool enable_reflections;
    bool enable_refractions;
    bool enable_foam;
    bool enable_bloom;
    bool enable_tone_mapping;
    bool enable_anti_aliasing;
    float lod_bias;
    float render_scale;
} QualitySettings;

static const QualitySettings quality_presets[] = {
    // Low
    {
        .shadow_resolution = 1024,
        .shadow_cascades = 2,
        .particle_count = 1000,
        .cloud_quality = 1,
        .water_tessellation = 1,
        .enable_atmospheric_scattering = false,
        .enable_volumetric_clouds = false,
        .enable_volumetric_fog = false,
        .enable_caustics = false,
        .enable_reflections = false,
        .enable_refractions = false,
        .enable_foam = false,
        .enable_bloom = false,
        .enable_tone_mapping = true,
        .enable_anti_aliasing = false,
        .lod_bias = 2.0f,
        .render_scale = 0.5f
    },
    // Medium
    {
        .shadow_resolution = 2048,
        .shadow_cascades = 3,
        .particle_count = 5000,
        .cloud_quality = 2,
        .water_tessellation = 2,
        .enable_atmospheric_scattering = true,
        .enable_volumetric_clouds = false,
        .enable_volumetric_fog = false,
        .enable_caustics = false,
        .enable_reflections = true,
        .enable_refractions = false,
        .enable_foam = false,
        .enable_bloom = true,
        .enable_tone_mapping = true,
        .enable_anti_aliasing = true,
        .lod_bias = 1.0f,
        .render_scale = 0.75f
    },
    // High
    {
        .shadow_resolution = 4096,
        .shadow_cascades = 4,
        .particle_count = 20000,
        .cloud_quality = 3,
        .water_tessellation = 4,
        .enable_atmospheric_scattering = true,
        .enable_volumetric_clouds = true,
        .enable_volumetric_fog = true,
        .enable_caustics = true,
        .enable_reflections = true,
        .enable_refractions = true,
        .enable_foam = true,
        .enable_bloom = true,
        .enable_tone_mapping = true,
        .enable_anti_aliasing = true,
        .lod_bias = 0.5f,
        .render_scale = 1.0f
    },
    // Ultra
    {
        .shadow_resolution = 8192,
        .shadow_cascades = 4,
        .particle_count = 100000,
        .cloud_quality = 4,
        .water_tessellation = 8,
        .enable_atmospheric_scattering = true,
        .enable_volumetric_clouds = true,
        .enable_volumetric_fog = true,
        .enable_caustics = true,
        .enable_reflections = true,
        .enable_refractions = true,
        .enable_foam = true,
        .enable_bloom = true,
        .enable_tone_mapping = true,
        .enable_anti_aliasing = true,
        .lod_bias = 0.0f,
        .render_scale = 1.0f
    }
};

// ============================================================================
// Pipeline Integration API
// ============================================================================

bool render_pipeline_init(uint32_t width, uint32_t height, RenderQuality quality) {
    if (g_pipeline.initialized) {
        LOG_WARN("Render pipeline already initialized");
        return true;
    }
    
    memset(&g_pipeline, 0, sizeof(UnifiedRenderPipeline));
    g_pipeline.width = width;
    g_pipeline.height = height;
    g_pipeline.quality = quality;
    g_pipeline.hdr_enabled = (quality >= RENDER_QUALITY_MEDIUM);
    g_pipeline.gpu_driven_enabled = (quality >= RENDER_QUALITY_HIGH);
    g_pipeline.clustered_shading_enabled = (quality >= RENDER_QUALITY_MEDIUM);
    
    const QualitySettings *settings = &quality_presets[quality];
    
    LOG_INFO("Initializing unified render pipeline at %ux%u (quality: %d)", width, height, (int)quality);
    
    // Initialize core systems
    g_pipeline.renderer = renderer_create(width, height);
    if (!g_pipeline.renderer) {
        LOG_ERROR("Failed to create renderer");
        return false;
    }
    
    g_pipeline.render_graph = render_graph_create();
    if (!g_pipeline.render_graph) {
        LOG_ERROR("Failed to create render graph");
        render_pipeline_shutdown();
        return false;
    }
    
    g_pipeline.culling_system = culling_system_create(10000);
    if (!g_pipeline.culling_system) {
        LOG_ERROR("Failed to create culling system");
        render_pipeline_shutdown();
        return false;
    }
    
    // Initialize GPU-driven systems
    if (g_pipeline.gpu_driven_enabled) {
        g_pipeline.draw_command_system = draw_command_system_create(10000, 10000);
        if (!g_pipeline.draw_command_system) {
            LOG_WARN("Failed to create GPU-driven system, falling back to CPU rendering");
            g_pipeline.gpu_driven_enabled = false;
        }
    }
    
    // Initialize deferred rendering
    g_pipeline.gbuffer = gbuffer_create(width, height);
    if (!g_pipeline.gbuffer) {
        LOG_ERROR("Failed to create G-buffer");
        render_pipeline_shutdown();
        return false;
    }
    
    // Initialize advanced systems
    g_pipeline.lighting_system = lighting_system_init(settings->shadow_cascades * 16, 
                                                   g_pipeline.clustered_shading_enabled);
    if (!g_pipeline.lighting_system) {
        LOG_ERROR("Failed to initialize lighting system");
        render_pipeline_shutdown();
        return false;
    }
    
    g_pipeline.shadow_system = shadow_mapping_init(settings->shadow_cascades, 
                                                   settings->shadow_resolution, 
                                                   settings->shadow_resolution * 2,
                                                   SHADOW_FILTER_PCF_3X3);
    if (!g_pipeline.shadow_system) {
        LOG_ERROR("Failed to initialize shadow system");
        render_pipeline_shutdown();
        return false;
    }
    
    g_pipeline.post_process = post_process_init(width, height, g_pipeline.hdr_enabled);
    if (!g_pipeline.post_process) {
        LOG_ERROR("Failed to initialize post-processing");
        render_pipeline_shutdown();
        return false;
    }
    
    // Initialize environmental systems
    g_pipeline.sky_system = sky_system_init(settings->enable_atmospheric_scattering,
                                             settings->enable_volumetric_clouds);
    if (!g_pipeline.sky_system) {
        LOG_ERROR("Failed to initialize sky system");
        render_pipeline_shutdown();
        return false;
    }
    
    g_pipeline.water_system = water_system_init((WaterQuality)settings->water_tessellation,
                                               g_pipeline.gpu_driven_enabled);
    if (!g_pipeline.water_system) {
        LOG_ERROR("Failed to initialize water system");
        render_pipeline_shutdown();
        return false;
    }
    
    g_pipeline.particle_system = particle_system_init(32, settings->particle_count);
    if (!g_pipeline.particle_system) {
        LOG_ERROR("Failed to initialize particle system");
        render_pipeline_shutdown();
        return false;
    }
    
    // Initialize developer tools
    g_pipeline.stats_system = render_stats_init(300);
    if (!g_pipeline.stats_system) {
        LOG_ERROR("Failed to initialize render statistics");
        render_pipeline_shutdown();
        return false;
    }
    
    g_pipeline.shader_hot_reload = shader_hot_reload_init();
    if (!g_pipeline.shader_hot_reload) {
        LOG_WARN("Failed to initialize shader hot-reloading");
    }
    
    g_pipeline.texture_streaming = texture_streaming_init(settings->shadow_resolution * 4); // 4x shadow resolution
    if (!g_pipeline.texture_streaming) {
        LOG_WARN("Failed to initialize texture streaming");
    }
    
    // Configure systems based on quality settings
    post_process_set_bloom_parameters(1.0f, 0.5f, 4);
    post_process_enable_effect(POST_EFFECT_BLOOM, settings->enable_bloom);
    post_process_enable_effect(POST_EFFECT_FXAA, settings->enable_anti_aliasing);
    
    water_system_enable_effects(settings->enable_reflections, settings->enable_refractions,
                               settings->enable_foam, settings->enable_caustics);
    
    g_pipeline.max_lights = settings->shadow_cascades * 16;
    g_pipeline.max_particles = settings->particle_count;
    g_pipeline.texture_memory_budget = settings->shadow_resolution * 4 * 1024 * 1024; // 4MB per 1024x1024
    
    g_pipeline.initialized = true;
    LOG_INFO("Unified render pipeline initialized successfully");
    return true;
}

void render_pipeline_shutdown(void) {
    if (!g_pipeline.initialized)
        return;
    
    LOG_INFO("Shutting down unified render pipeline");
    
    // Shutdown in reverse order
    if (g_pipeline.texture_streaming) {
        texture_streaming_shutdown();
        g_pipeline.texture_streaming = NULL;
    }
    
    if (g_pipeline.shader_hot_reload) {
        shader_hot_reload_shutdown();
        g_pipeline.shader_hot_reload = NULL;
    }
    
    if (g_pipeline.stats_system) {
        render_stats_shutdown();
        g_pipeline.stats_system = NULL;
    }
    
    if (g_pipeline.particle_system) {
        particle_system_shutdown();
        g_pipeline.particle_system = NULL;
    }
    
    if (g_pipeline.water_system) {
        water_system_shutdown();
        g_pipeline.water_system = NULL;
    }
    
    if (g_pipeline.sky_system) {
        sky_system_shutdown();
        g_pipeline.sky_system = NULL;
    }
    
    if (g_pipeline.post_process) {
        post_process_shutdown();
        g_pipeline.post_process = NULL;
    }
    
    if (g_pipeline.shadow_system) {
        shadow_mapping_shutdown();
        g_pipeline.shadow_system = NULL;
    }
    
    if (g_pipeline.lighting_system) {
        lighting_system_shutdown();
        g_pipeline.lighting_system = NULL;
    }
    
    if (g_pipeline.gbuffer) {
        gbuffer_destroy(g_pipeline.gbuffer);
        g_pipeline.gbuffer = NULL;
    }
    
    if (g_pipeline.draw_command_system) {
        draw_command_system_destroy(g_pipeline.draw_command_system);
        g_pipeline.draw_command_system = NULL;
    }
    
    if (g_pipeline.culling_system) {
        culling_system_destroy(g_pipeline.culling_system);
        g_pipeline.culling_system = NULL;
    }
    
    if (g_pipeline.render_graph) {
        render_graph_destroy(g_pipeline.render_graph);
        g_pipeline.render_graph = NULL;
    }
    
    if (g_pipeline.renderer) {
        renderer_destroy(g_pipeline.renderer);
        g_pipeline.renderer = NULL;
    }
    
    memset(&g_pipeline, 0, sizeof(UnifiedRenderPipeline));
    
    LOG_INFO("Unified render pipeline shutdown complete");
}

void render_pipeline_begin_frame(void) {
    if (!g_pipeline.initialized || g_pipeline.frame_in_progress) {
        return;
    }
    
    g_pipeline.frame_in_progress = true;
    
    // Call frame start callback
    if (g_pipeline.on_frame_start) {
        g_pipeline.on_frame_start();
    }
    
    // Begin frame tracking
    renderer_begin_frame(g_pipeline.renderer);
    render_stats_begin_frame();
    
    // Update developer tools
    if (g_pipeline.shader_hot_reload) {
        shader_hot_reload_update();
    }
}

void render_pipeline_end_frame(void) {
    if (!g_pipeline.initialized || !g_pipeline.frame_in_progress) {
        return;
    }
    
    // End frame tracking
    render_stats_end_frame();
    renderer_end_frame(g_pipeline.renderer);
    
    // Call frame end callback
    if (g_pipeline.on_frame_end) {
        g_pipeline.on_frame_end();
    }
    
    g_pipeline.frame_in_progress = false;
}

void render_pipeline_render_scene(const Scene *scene, const Camera *camera) {
    if (!g_pipeline.initialized || !g_pipeline.frame_in_progress) {
        return;
    }
    
    uint64_t start_time = get_time_nanos();
    
    // Update culling
    culling_system_update_frustum(g_pipeline.culling_system, camera->view, camera->proj);
    
    // Update and render shadows
    shadow_mapping_render_shadows(scene->lights, scene->light_count, 
                                  camera->view, camera->proj, 
                                  camera->near_plane, camera->far_plane);
    
    // Update lighting
    lighting_update_lights(scene->lights, scene->light_count, camera->view, camera->proj);
    
    // Update environmental systems
    sky_system_update(1.0f / 60.0f); // 60 FPS
    water_system_update(1.0f / 60.0f, scene->wind_direction, scene->wind_strength);
    particle_system_update(1.0f / 60.0f);
    
    // Begin G-buffer pass
    gbuffer_begin_pass(g_pipeline.gbuffer, camera->command_encoder);
    
    // Render main geometry
    if (g_pipeline.gpu_driven_enabled && g_pipeline.draw_command_system) {
        // GPU-driven rendering
        draw_command_system_perform_culling(g_pipeline.draw_command_system, 
                                           camera->view_proj, camera->position);
        draw_command_system_generate_commands(g_pipeline.draw_command_system);
        draw_command_system_execute_indirect(g_pipeline.draw_command_system, camera->command_encoder);
    } else {
        // CPU-driven rendering
        // TODO: Implement CPU rendering path
    }
    
    gbuffer_end_pass(g_pipeline.gbuffer);
    
    // Bind G-buffer for lighting
    gbuffer_bind_for_lighting(g_pipeline.gbuffer, camera->command_encoder);
    
    // Render environmental effects
    sky_system_render(camera->view, camera->proj, camera->position);
    water_system_render(camera->view, camera->proj, camera->position);
    particle_system_render(camera->view, camera->proj);
    
    // Post-processing
    post_process_set_input_texture(gbuffer_get_texture(g_pipeline.gbuffer, GBUFFER_ALBEDO));
    post_process_execute();
    
    uint64_t end_time = get_time_nanos();
    float frame_time = nanos_to_ms(end_time - start_time);
    
    LOG_DEBUG("Frame rendered in %.2f ms", frame_time);
}

void render_pipeline_set_quality(RenderQuality quality) {
    if (!g_pipeline.initialized) return;
    
    // TODO: Implement quality switching
    // This would involve reconfiguring all systems with new quality settings
    LOG_INFO("Render quality set to %d", (int)quality);
}

void render_pipeline_set_callbacks(void (*on_frame_start)(void), void (*on_frame_end)(void), 
                                 void (*on_error)(const char*)) {
    if (!g_pipeline.initialized) return;
    
    g_pipeline.on_frame_start = on_frame_start;
    g_pipeline.on_frame_end = on_frame_end;
    g_pipeline.on_error = on_error;
}

void render_pipeline_get_stats(RenderPipelineStats *stats) {
    if (!g_pipeline.initialized || !stats) return;
    
    // Collect statistics from all systems
    RenderStats renderer_stats = renderer_get_stats(g_pipeline.renderer);
    RenderStats culling_stats = {0}; // TODO: Get from culling system
    RenderStats lighting_stats = {0}; // TODO: Get from lighting system
    RenderStats shadow_stats = {0}; // TODO: Get from shadow system
    RenderStats post_process_stats = {0}; // TODO: Get from post-process system
    
    // Aggregate statistics
    stats->frame_time_ms = renderer_stats.frame_time_ms;
    stats->draw_calls = renderer_stats.draw_calls;
    stats->triangles_drawn = renderer_stats.triangles_drawn;
    stats->cpu_time_ms = renderer_stats.cpu_time_ms;
    stats->gpu_time_ms = renderer_stats.gpu_time_ms;
    stats->frames_rendered = renderer_stats.frames_rendered;
    
    // Add system-specific stats
    stats->lights_count = g_pipeline.max_lights;
    stats->particles_count = g_pipeline.max_particles;
    stats->texture_memory_used = g_pipeline.texture_memory_budget;
}

bool render_pipeline_is_initialized(void) {
    return g_pipeline.initialized;
}

// ============================================================================
// Convenience Functions
// ============================================================================

Renderer* render_pipeline_get_renderer(void) {
    return g_pipeline.renderer;
}

LightingSystem* render_pipeline_get_lighting_system(void) {
    return g_pipeline.lighting_system;
}

SkySystem* render_pipeline_get_sky_system(void) {
    return g_pipeline.sky_system;
}

WaterSystem* render_pipeline_get_water_system(void) {
    return g_pipeline.water_system;
}

ParticleSystem* render_pipeline_get_particle_system(void) {
    return g_pipeline.particle_system;
}

void render_pipeline_resize(uint32_t width, uint32_t height) {
    if (!g_pipeline.initialized) return;
    
    g_pipeline.width = width;
    g_pipeline.height = height;
    
    // Resize all systems
    renderer_resize(g_pipeline.renderer, width, height);
    gbuffer_resize(g_pipeline.gbuffer, width, height);
    post_process_resize(width, height);
    
    LOG_INFO("Render pipeline resized to %ux%u", width, height);
}
