#include "svg_particle_renderer.h"
#include "engine/include/core/logger.h"
#include <string.h>
#include <stdlib.h>
#include <math.h>

/* ============================================================================
   SVG Particle Renderer - Minimum Viable Implementation

   This renderer provides basic particle rendering using billboards with
   SVG-rasterized textures. For production use, expand with:
   - GPU compute shader batching
   - Instancing
   - LOD systems
   - Frustum culling
   ============================================================================ */

static struct {
    bool initialized;
    uint32_t *rasterization_cache;
    uint32_t cache_size;
} _renderer_state = {
    .initialized = false,
    .rasterization_cache = NULL,
    .cache_size = 0
};

/* ============================================================================
   Renderer Initialization
   ============================================================================ */

void svg_particle_renderer_init(void)
{
    if (_renderer_state.initialized) return;

    /* Initialize rasterization cache */
    _renderer_state.cache_size = 16;  /* Initial capacity */
    _renderer_state.rasterization_cache = (uint32_t *)calloc(
        _renderer_state.cache_size,
        sizeof(uint32_t)
    );

    _renderer_state.initialized = true;
    LOG_INFO("SVG Particle Renderer initialized");
}

void svg_particle_renderer_cleanup(void)
{
    if (!_renderer_state.initialized) return;

    free(_renderer_state.rasterization_cache);
    _renderer_state.rasterization_cache = NULL;
    _renderer_state.cache_size = 0;
    _renderer_state.initialized = false;

    LOG_INFO("SVG Particle Renderer cleaned up");
}

/* ============================================================================
   SVG Rasterization

   Converts SVG path data to a texture for billboard rendering.
   For minimum viable: creates a simple circle or rectangle based on shape.
   ============================================================================ */

static void _rasterize_circle(
    uint8_t *pixels, uint32_t width, uint32_t height,
    float center_x, float center_y, float radius
)
{
    for (uint32_t y = 0; y < height; y++) {
        for (uint32_t x = 0; x < width; x++) {
            float dx = (float)x - center_x;
            float dy = (float)y - center_y;
            float dist = sqrtf(dx * dx + dy * dy);

            uint32_t idx = (y * width + x) * 4;
            if (dist <= radius) {
                pixels[idx + 0] = 255;      /* R */
                pixels[idx + 1] = 255;      /* G */
                pixels[idx + 2] = 255;      /* B */
                pixels[idx + 3] = 255;      /* A */
            } else {
                pixels[idx + 0] = 0;
                pixels[idx + 1] = 0;
                pixels[idx + 2] = 0;
                pixels[idx + 3] = 0;
            }
        }
    }
}

static void _rasterize_rect(
    uint8_t *pixels, uint32_t width, uint32_t height,
    float x1, float y1, float x2, float y2
)
{
    for (uint32_t y = 0; y < height; y++) {
        for (uint32_t x = 0; x < width; x++) {
            uint32_t idx = (y * width + x) * 4;

            if ((float)x >= x1 && (float)x <= x2 &&
                (float)y >= y1 && (float)y <= y2) {
                pixels[idx + 0] = 255;
                pixels[idx + 1] = 255;
                pixels[idx + 2] = 255;
                pixels[idx + 3] = 255;
            } else {
                pixels[idx + 0] = 0;
                pixels[idx + 1] = 0;
                pixels[idx + 2] = 0;
                pixels[idx + 3] = 0;
            }
        }
    }
}

bool svg_particle_renderer_rasterize_shape(
    SVGElement *shape,
    uint32_t width, uint32_t height,
    uint32_t *out_texture
)
{
    if (!shape || !out_texture) return false;

    /* Allocate pixel buffer */
    uint8_t *pixels = (uint8_t *)calloc(width * height * 4, sizeof(uint8_t));
    if (!pixels) {
        LOG_ERROR("Failed to allocate rasterization buffer");
        return false;
    }

    /* Rasterize based on SVG element type */
    /* For now, support basic shapes */
    if (shape->type == SVG_ELEMENT_CIRCLE) {
        float center_x = width / 2.0f;
        float center_y = height / 2.0f;
        float radius = (width < height ? width : height) / 2.0f - 2.0f;
        _rasterize_circle(pixels, width, height, center_x, center_y, radius);
    } else if (shape->type == SVG_ELEMENT_RECT) {
        float x1 = 5.0f;
        float y1 = 5.0f;
        float x2 = width - 5.0f;
        float y2 = height - 5.0f;
        _rasterize_rect(pixels, width, height, x1, y1, x2, y2);
    } else {
        /* Default: create a circle for unknown shapes */
        float center_x = width / 2.0f;
        float center_y = height / 2.0f;
        float radius = (width < height ? width : height) / 2.0f - 2.0f;
        _rasterize_circle(pixels, width, height, center_x, center_y, radius);
    }

    /* TODO: Actual texture upload to GPU
       This is a placeholder texture handle.
       In production, use graphics API (OpenGL/Vulkan/Metal) to:
       1. Create texture with pixel data
       2. Set filtering and wrapping modes
       3. Return GPU texture handle
    */
    *out_texture = (uint32_t)(uintptr_t)pixels;

    LOG_DEBUG("Rasterized SVG shape: %ux%u", width, height);
    return true;
}

/* ============================================================================
   Particle Rendering
   ============================================================================ */

void svg_particle_renderer_render(
    const SVGParticleEmitter *emitter,
    SVGParticleRenderState *render_state
)
{
    if (!emitter || !render_state) return;

    uint32_t particle_count = 0;
    const SVGParticle *particles = svg_particle_emitter_get_particles(
        emitter,
        &particle_count
    );

    if (!particles || particle_count == 0) return;

    /* Render each active particle as a billboard */
    for (uint32_t i = 0; i < particle_count; i++) {
        const SVGParticle *p = &particles[i];
        if (!p->active) continue;

        /* TODO: Actual billboard rendering
           For minimum viable, this queues particles for rendering.
           In production, use:
           1. GPU instancing for efficient batching
           2. Billboard vertex generation (2 triangles per particle)
           3. Texture blending with particle color/opacity
           4. Depth sorting for transparency
        */

        /* Render batch data:
           Position: (x, y, z)
           Scale: scale
           Rotation: rotation
           Color: (r, g, b, a)
           Opacity: opacity
           Texture: render_state->texture
        */
    }

    /* TODO: Actual GPU rendering call
       glDrawInstanced() or equivalent for platform
    */
}

void svg_particle_renderer_render_system(
    const SVGParticleSystem *system
)
{
    if (!system) return;

    for (uint32_t i = 0; i < system->emitter_count; i++) {
        SVGParticleEmitter *emitter = (SVGParticleEmitter *)&system->emitters[i];
        svg_particle_renderer_render(emitter, NULL);
    }
}
