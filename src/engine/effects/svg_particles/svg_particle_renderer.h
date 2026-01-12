#ifndef SVG_PARTICLE_RENDERER_H
#define SVG_PARTICLE_RENDERER_H

#include "svg_particle_system.h"

/**
 * SVG Particle Renderer - Handles rendering of SVG particles
 * Supports both CPU (rasterization) and GPU rendering paths
 */

typedef struct {
    uint32_t vertex_buffer;     /* GPU vertex buffer */
    uint32_t index_buffer;      /* GPU index buffer */
    uint32_t vertex_array;      /* GPU vertex array object */
    uint32_t shader_program;    /* GPU shader program */
    uint32_t texture;           /* Rasterized SVG texture */
} SVGParticleRenderState;

typedef struct {
    SVGParticleEmitter *emitter;
    SVGParticleRenderState render_state;
} SVGParticleRenderBatch;

/**
 * Initialize SVG particle renderer
 */
void svg_particle_renderer_init(void);

/**
 * Cleanup SVG particle renderer
 */
void svg_particle_renderer_cleanup(void);

/**
 * Rasterize SVG shape to texture for fast rendering
 */
bool svg_particle_renderer_rasterize_shape(
    SVGElement *shape,
    uint32_t width, uint32_t height,
    uint32_t *out_texture
);

/**
 * Render SVG particles for given emitter
 */
void svg_particle_renderer_render(
    const SVGParticleEmitter *emitter,
    SVGParticleRenderState *render_state
);

/**
 * Render all particles in system
 */
void svg_particle_renderer_render_system(
    const SVGParticleSystem *system
);

#endif /* SVG_PARTICLE_RENDERER_H */
