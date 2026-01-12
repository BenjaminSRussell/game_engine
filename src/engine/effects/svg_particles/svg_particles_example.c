/**
 * SVG Particles Example - Usage and Integration Examples
 *
 * This file demonstrates how to use the SVG particle system.
 * For actual game integration, adapt these patterns to your needs.
 */

#include "svg_particle_system.h"
#include "svg_particle_renderer.h"
#include "editor/importer/svg_importer.h"

/* ============================================================================
   Example 1: Basic SVG Circle Particles
   ============================================================================ */

void example_svg_circle_particles(void)
{
    /* Create particle system */
    SVGParticleSystem *system = svg_particle_system_create(16);
    if (!system) return;

    /* Create emitter configuration */
    SVGParticleEmitterConfig config = {
        .max_particles = 1024,
        .spawn_rate = 100.0f,  /* 100 particles/sec */
        .lifetime_min = 0.5f,
        .lifetime_max = 2.0f,
        .scale_min = 0.1f,
        .scale_max = 0.5f,
        .opacity = 0.8f,
        .gravity = 9.81f,
        .drag = 0.5f,
        .svg_shape = NULL,  /* Can be set to custom SVG element */
        .svg_element_count = 1
    };

    /* Create emitter */
    SVGParticleEmitter *emitter = svg_particle_emitter_create(config);
    if (!emitter) {
        svg_particle_system_destroy(system);
        return;
    }

    /* Add emitter to system */
    svg_particle_system_add_emitter(system, emitter);

    /* Set emitter position (e.g., at block position) */
    svg_particle_emitter_set_position(emitter, 10.0f, 5.0f, 10.0f);

    /* Start emission */
    svg_particle_emitter_start(emitter);

    /* In main loop: */
    /* svg_particle_system_update(system, delta_time); */
    /* svg_particle_renderer_render_system(system); */

    /* Cleanup */
    svg_particle_emitter_destroy(emitter);
    svg_particle_system_destroy(system);
}

/* ============================================================================
   Example 2: Burst/Explosion Particles
   ============================================================================ */

void example_svg_explosion_particles(float x, float y, float z)
{
    SVGParticleSystem *system = svg_particle_system_create(8);
    if (!system) return;

    /* Configure for explosion effect */
    SVGParticleEmitterConfig config = {
        .max_particles = 256,
        .spawn_rate = 0.0f,  /* No continuous emission */
        .lifetime_min = 0.3f,
        .lifetime_max = 1.0f,
        .scale_min = 0.2f,
        .scale_max = 0.4f,
        .opacity = 1.0f,
        .gravity = 9.81f,
        .drag = 1.5f,  /* High drag for explosion */
        .svg_shape = NULL,
        .svg_element_count = 1
    };

    SVGParticleEmitter *emitter = svg_particle_emitter_create(config);
    svg_particle_system_add_emitter(system, emitter);
    svg_particle_emitter_set_position(emitter, x, y, z);

    /* Burst emission - instant particles */
    svg_particle_emitter_burst(emitter, 64);  /* 64 particles at once */

    /* Update and render until particles die */
    /* while (svg_particle_emitter_get_active_count(emitter) > 0) {
       svg_particle_system_update(system, delta_time);
       svg_particle_renderer_render_system(system);
    } */

    svg_particle_emitter_destroy(emitter);
    svg_particle_system_destroy(system);
}

/* ============================================================================
   Example 3: Weather Particles (Rain/Snow with SVG)
   ============================================================================ */

void example_svg_weather_particles(void)
{
    SVGParticleSystem *system = svg_particle_system_create(32);
    if (!system) return;

    /* Rain droplet configuration */
    SVGParticleEmitterConfig rain_config = {
        .max_particles = 5000,
        .spawn_rate = 2000.0f,  /* Many particles for weather */
        .lifetime_min = 1.0f,
        .lifetime_max = 3.0f,
        .scale_min = 0.05f,
        .scale_max = 0.1f,
        .opacity = 0.6f,
        .gravity = 15.0f,  /* Higher gravity for rain */
        .drag = 0.2f,
        .svg_shape = NULL,
        .svg_element_count = 1
    };

    SVGParticleEmitter *rain_emitter = svg_particle_emitter_create(rain_config);
    svg_particle_system_add_emitter(system, rain_emitter);

    /* Position high in sky */
    svg_particle_emitter_set_position(rain_emitter, 0.0f, 100.0f, 0.0f);
    svg_particle_emitter_start(rain_emitter);

    /* Cleanup */
    svg_particle_emitter_destroy(rain_emitter);
    svg_particle_system_destroy(system);
}

/* ============================================================================
   Example 4: Custom SVG Shape Particles
   ============================================================================ */

void example_custom_svg_particles(SVGElement *custom_shape)
{
    if (!custom_shape) return;

    SVGParticleSystem *system = svg_particle_system_create(8);
    SVGParticleEmitterConfig config = {
        .max_particles = 512,
        .spawn_rate = 50.0f,
        .lifetime_min = 0.5f,
        .lifetime_max = 2.0f,
        .scale_min = 0.1f,
        .scale_max = 0.3f,
        .opacity = 0.9f,
        .gravity = 5.0f,
        .drag = 0.3f,
        .svg_shape = custom_shape,  /* Use custom SVG shape */
        .svg_element_count = 1
    };

    SVGParticleEmitter *emitter = svg_particle_emitter_create(config);
    svg_particle_system_add_emitter(system, emitter);
    svg_particle_emitter_set_position(emitter, 0.0f, 0.0f, 0.0f);
    svg_particle_emitter_start(emitter);

    svg_particle_emitter_destroy(emitter);
    svg_particle_system_destroy(system);
}

/* ============================================================================
   Example 5: Integration with Game Events
   ============================================================================ */

typedef struct {
    SVGParticleSystem *system;
    SVGParticleEmitter *emitter;
} GameParticleContext;

GameParticleContext* create_block_break_particles(
    float x, float y, float z
)
{
    GameParticleContext *ctx = (GameParticleContext *)malloc(
        sizeof(GameParticleContext)
    );
    if (!ctx) return NULL;

    ctx->system = svg_particle_system_create(4);
    if (!ctx->system) {
        free(ctx);
        return NULL;
    }

    /* Block break effect: quick burst with rapid fade */
    SVGParticleEmitterConfig config = {
        .max_particles = 128,
        .spawn_rate = 0.0f,
        .lifetime_min = 0.2f,
        .lifetime_max = 0.5f,
        .scale_min = 0.15f,
        .scale_max = 0.25f,
        .opacity = 1.0f,
        .gravity = 9.81f,
        .drag = 2.0f,
        .svg_shape = NULL,
        .svg_element_count = 1
    };

    ctx->emitter = svg_particle_emitter_create(config);
    svg_particle_system_add_emitter(ctx->system, ctx->emitter);
    svg_particle_emitter_set_position(ctx->emitter, x, y, z);

    /* Immediate burst */
    svg_particle_emitter_burst(ctx->emitter, 32);

    return ctx;
}

void update_and_render_particles(GameParticleContext *ctx, float delta_time)
{
    if (!ctx) return;

    svg_particle_system_update(ctx->system, delta_time);

    if (svg_particle_emitter_get_active_count(ctx->emitter) > 0) {
        svg_particle_renderer_render_system(ctx->system);
    }
}

void cleanup_particles(GameParticleContext *ctx)
{
    if (!ctx) return;

    svg_particle_emitter_destroy(ctx->emitter);
    svg_particle_system_destroy(ctx->system);
    free(ctx);
}

/* ============================================================================
   Renderer Initialization Template
   ============================================================================ */

void init_svg_particles_for_game(void)
{
    /* Call this in your game initialization */
    svg_particle_renderer_init();
}

void cleanup_svg_particles_for_game(void)
{
    /* Call this during game shutdown */
    svg_particle_renderer_cleanup();
}
