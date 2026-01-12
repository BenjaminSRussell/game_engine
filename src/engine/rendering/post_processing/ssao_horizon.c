// Screen-Space Ambient Occlusion (SSAO) Implementation
// Horizon-based SSAO (HBAO+) - fast and high-quality ambient occlusion
#include "rendering/frame_graph/frame_graph.h"
#include "core/logger.h"
#include <stdlib.h>
#include <string.h>
#include <math.h>

typedef struct SSAOContext {
    u32 width;
    u32 height;
    f32 radius;           // Occlusion radius (screen space)
    f32 bias;             // Angle bias to reduce false occlusion
    f32 intensity;        // AO intensity multiplier
    u32 num_directions;   // Number of sample directions (typically 8)
    u32 num_samples;      // Samples per direction (typically 4)
    bool enable_blur;     // Enable bilateral blur post-processing
} SSAOContext;

// Create SSAO context
SSAOContext *ssao_create(u32 width, u32 height) {
    SSAOContext *ctx = malloc(sizeof(SSAOContext));
    if (!ctx) {
        LOG_ERROR("Failed to allocate SSAO context");
        return NULL;
    }

    memset(ctx, 0, sizeof(SSAOContext));

    ctx->width = width;
    ctx->height = height;
    ctx->radius = 32.0f;        // Pixels in screen space
    ctx->bias = 0.1f;           // Angle bias
    ctx->intensity = 1.0f;      // Full intensity
    ctx->num_directions = 8;    // 8 directions
    ctx->num_samples = 4;       // 4 samples per direction
    ctx->enable_blur = true;    // Enable bilateral blur

    LOG_INFO("SSAO context created: %ux%u", width, height);
    return ctx;
}

// Destroy SSAO context
void ssao_destroy(SSAOContext *ctx) {
    if (!ctx) return;
    free(ctx);
    LOG_INFO("SSAO context destroyed");
}

// Add SSAO pass to render graph
RGResourceHandle ssao_add_to_graph(RenderGraph *rg,
                                  SSAOContext *ctx,
                                  RGResourceHandle depth_buffer,
                                  RGResourceHandle normal_buffer) {
    if (!rg || !ctx) {
        LOG_ERROR("Invalid render graph or SSAO context");
        return RG_INVALID_RESOURCE;
    }

    if (ctx->radius < 1.0f) ctx->radius = 1.0f;
    if (ctx->radius > 256.0f) ctx->radius = 256.0f;
    if (ctx->intensity < 0.0f) ctx->intensity = 0.0f;
    if (ctx->intensity > 2.0f) ctx->intensity = 2.0f;

    LOG_DEBUG("SSAO pass added: radius=%.2f, intensity=%.2f", ctx->radius, ctx->intensity);

    // SSAO (Horizon-Based AO) shader implementation:
    // 1. For each pixel, reconstruct world position from depth
    // 2. In multiple directions, scan horizon angle
    // 3. Compute occlusion from angle difference
    // 4. Apply bias and intensity
    // 5. Optional: Apply bilateral blur for denoising
    //
    // Algorithm benefits:
    // - Cache-friendly (linear memory access patterns)
    // - Works well with screen-space data
    // - No precomputed data required
    // - Temporal coherent results

    // For now, return pass-through
    // Real implementation would add compute shader to render graph

    return depth_buffer;  // TODO: Return SSAO output
}

// Update SSAO radius
void ssao_set_radius(SSAOContext *ctx, f32 radius) {
    if (!ctx) return;
    if (radius < 1.0f) radius = 1.0f;
    if (radius > 256.0f) radius = 256.0f;
    ctx->radius = radius;
}

// Update SSAO intensity
void ssao_set_intensity(SSAOContext *ctx, f32 intensity) {
    if (!ctx) return;
    if (intensity < 0.0f) intensity = 0.0f;
    if (intensity > 2.0f) intensity = 2.0f;
    ctx->intensity = intensity;
}
