// Screen-Space Reflections (SSR) Implementation
// Efficient hierarchical ray marching with temporal reprojection
#include "ssr.h"
#include "core/logging/unified_logger.h"
#include <stdlib.h>
#include <string.h>
#include <math.h>

// SSR context for pipeline state
typedef struct SSRContext {
    u32 width;
    u32 height;
    f32 max_distance;        // Maximum ray march distance (screen space pixels)
    f32 thickness;           // Surface thickness for intersection
    u32 max_steps;           // Maximum ray march steps
    bool enable_temporal;    // Enable temporal reprojection
    bool enable_refinement;  // Enable binary search refinement
    f32 edge_fade_start;     // Screen edge fade start (0.85)
    f32 edge_fade_end;       // Screen edge fade end (1.0)
} SSRContext;

// Create SSR context
SSRContext *ssr_create(u32 width, u32 height) {
    SSRContext *ctx = malloc(sizeof(SSRContext));
    if (!ctx) {
        LOG_ERROR("GRAPHICS", "Failed to allocate SSR context");
        return NULL;
    }

    memset(ctx, 0, sizeof(SSRContext));

    ctx->width = width;
    ctx->height = height;
    ctx->max_distance = 512.0f;      // Screen space pixels
    ctx->thickness = 0.1f;           // World space
    ctx->max_steps = 64;             // Ray march steps
    ctx->enable_temporal = true;     // Temporal reprojection
    ctx->enable_refinement = true;   // Binary search refinement
    ctx->edge_fade_start = 0.85f;
    ctx->edge_fade_end = 1.0f;

    LOG_INFO("GRAPHICS", "SSR context created: %ux%u", width, height);
    return ctx;
}

// Destroy SSR context
void ssr_destroy(SSRContext *ctx) {
    if (!ctx) return;
    free(ctx);
    LOG_INFO("GRAPHICS", "SSR context destroyed");
}

// Configure maximum ray march distance
void ssr_set_max_distance(SSRContext *ctx, f32 distance) {
    if (!ctx) return;
    ctx->max_distance = distance;
}

// Configure surface thickness
void ssr_set_thickness(SSRContext *ctx, f32 thickness) {
    if (!ctx) return;
    ctx->thickness = thickness;
}

// Configure maximum ray march steps
void ssr_set_max_steps(SSRContext *ctx, u32 steps) {
    if (!ctx) return;
    if (steps < 1) steps = 1;
    if (steps > 256) steps = 256;
    ctx->max_steps = steps;
}

// SSR ray march pass execution callback
typedef struct {
    SSRContext *ctx;
    RGResourceHandle scene_color;
    RGResourceHandle normal_roughness;
    RGResourceHandle depth_buffer;
    RGResourceHandle output;
} SSRPassData;

static void ssr_execute_pass(RGPassContext *ctx, void *user_data) {
    SSRPassData *data = (SSRPassData *)user_data;
    if (!data || !data->ctx) return;

    TextureID scene_color_tex = rg_ctx_get_texture(ctx, data->scene_color);
    TextureID normal_roughness_tex = rg_ctx_get_texture(ctx, data->normal_roughness);
    TextureID depth_tex = rg_ctx_get_texture(ctx, data->depth_buffer);
    TextureID output_tex = rg_ctx_get_texture(ctx, data->output);

    if (scene_color_tex == INVALID_TEXTURE_ID || normal_roughness_tex == INVALID_TEXTURE_ID ||
        depth_tex == INVALID_TEXTURE_ID || output_tex == INVALID_TEXTURE_ID) {
        LOG_ERROR("GRAPHICS", "Invalid textures for SSR pass");
        return;
    }

    // Dispatch SSR compute shader with resources
    // Bind SSR shader resources
    texture_manager_bind(scene_color_tex, 0);
    texture_manager_bind(normal_roughness_tex, 1);
    texture_manager_bind(depth_tex, 2);
    texture_manager_bind(output_tex, 3);
    
    // Set SSR uniforms
    struct SSRUniforms {
        float max_distance;
        float max_steps;
        float thickness;
        float edge_fade;
        vec2 texel_size;
        uint frame_index;
        uint enable_temporal;
        float padding;
    } uniforms = {
        .max_distance = data->ctx->max_distance,
        .max_steps = float(data->ctx->max_steps),
        .thickness = data->ctx->thickness,
        .edge_fade = 0.1f,
        .texel_size = {1.0f / 1920.0f, 1.0f / 1080.0f}, // TODO: Get actual resolution
        .frame_index = data->ctx->frame_index,
        .enable_temporal = 1,
        .padding = 0.0f
    };
    
    shader_set_uniforms(&uniforms, sizeof(uniforms));
    
    // Get texture dimensions for compute dispatch
    u32 tex_width, tex_height;
    texture_manager_get_dimensions(output_tex, &tex_width, &tex_height);
    
    // Dispatch compute shader
    MTLSize gridSize = MTLSizeMake((tex_width + 15) / 16, (tex_height + 15) / 16, 1);
    MTLSize threadgroupSize = MTLSizeMake(16, 16, 1);
    compute_dispatch_threadgroups(gridSize, threadgroupSize);
    
    // Update frame index
    data->ctx->frame_index = (data->ctx->frame_index + 1) & 0xFFFF;

    LOG_DEBUG("GRAPHICS", "SSR shader executed");
}

// SSR temporal reprojection pass
typedef struct {
    SSRContext *ctx;
    RGResourceHandle current_ssr;
    RGResourceHandle previous_ssr;
    RGResourceHandle velocity_buffer;
    RGResourceHandle output;
} SSRTemporalPassData;

static void ssr_temporal_execute(RGPassContext *ctx, void *user_data) {
    SSRTemporalPassData *data = (SSRTemporalPassData *)user_data;
    if (!data || !data->ctx) return;

    TextureID current_tex = rg_ctx_get_texture(ctx, data->current_ssr);
    TextureID previous_tex = rg_ctx_get_texture(ctx, data->previous_ssr);
    TextureID velocity_tex = rg_ctx_get_texture(ctx, data->velocity_buffer);
    TextureID output_tex = rg_ctx_get_texture(ctx, data->output);

    if (current_tex == INVALID_TEXTURE_ID || previous_tex == INVALID_TEXTURE_ID ||
        velocity_tex == INVALID_TEXTURE_ID || output_tex == INVALID_TEXTURE_ID) {
        LOG_ERROR("GRAPHICS", "Invalid textures for SSR temporal pass");
        return;
    }

    // Dispatch SSR temporal reprojection shader
    // Bind temporal reprojection resources
    texture_manager_bind(current_tex, 0);
    texture_manager_bind(previous_tex, 1);
    texture_manager_bind(velocity_tex, 2);
    texture_manager_bind(output_tex, 3);
    
    // Set temporal reprojection uniforms
    struct SSRTemporalUniforms {
        float temporal_weight;
        float spatial_weight;
        uint frame_index;
        uint enable_variance_clamping;
        vec2 texel_size;
        float velocity_scale;
        float padding;
    } uniforms = {
        .temporal_weight = 0.9f,
        .spatial_weight = 0.1f,
        .frame_index = data->ctx->frame_index,
        .enable_variance_clamping = 1,
        .texel_size = {1.0f / 1920.0f, 1.0f / 1080.0f}, // TODO: Get actual resolution
        .velocity_scale = 1.0f,
        .padding = 0.0f
    };
    
    shader_set_uniforms(&uniforms, sizeof(uniforms));
    
    // Get texture dimensions for compute dispatch
    u32 tex_width, tex_height;
    texture_manager_get_dimensions(output_tex, &tex_width, &tex_height);
    
    // Dispatch temporal reprojection compute shader
    MTLSize gridSize = MTLSizeMake((tex_width + 15) / 16, (tex_height + 15) / 16, 1);
    MTLSize threadgroupSize = MTLSizeMake(16, 16, 1);
    compute_dispatch_threadgroups(gridSize, threadgroupSize);
    // 2. Sample neighborhood for variance clipping
    // 3. Blend with current frame using adaptive blend factor
    // 4. Reduce noise while preserving temporal coherence

    LOG_DEBUG("GRAPHICS", "SSR temporal reprojection executed");
}

// Add SSR pass to render graph
RGResourceHandle ssr_add_to_graph(RenderGraph *rg,
                                 SSRContext *ctx,
                                 RGResourceHandle scene_color,
                                 RGResourceHandle normal_roughness,
                                 RGResourceHandle depth_buffer) {
    if (!rg || !ctx) {
        LOG_ERROR("GRAPHICS", "Invalid render graph or SSR context");
        return RG_INVALID_RESOURCE;
    }

    // Create output texture for SSR result
    RGTextureDesc output_desc = {
        .width = ctx->width,
        .height = ctx->height,
        .depth = 1,
        .format = TEXTURE_FORMAT_RGBA16F,
        .usage = TEXTURE_USAGE_STORAGE | TEXTURE_USAGE_SAMPLED,
        .name = "SSR_Output"
    };
    RGResourceHandle ssr_output = rg_create_texture(rg, &output_desc);

    // Add SSR ray march pass
    SSRPassData *pass_data = malloc(sizeof(SSRPassData));
    if (pass_data) {
        pass_data->ctx = ctx;
        pass_data->scene_color = scene_color;
        pass_data->normal_roughness = normal_roughness;
        pass_data->depth_buffer = depth_buffer;
        pass_data->output = ssr_output;

        RGPassDesc pass_desc = {
            .name = "SSR_RayMarch",
            .execute = ssr_execute_pass,
            .user_data = pass_data,
            .queue_type = RG_QUEUE_COMPUTE_ASYNC,
            .priority = 85
        };
        RGPassHandle pass = rg_add_pass(rg, &pass_desc);
        rg_pass_read(rg, pass, scene_color);
        rg_pass_read(rg, pass, normal_roughness);
        rg_pass_read(rg, pass, depth_buffer);
        rg_pass_write(rg, pass, ssr_output);
    }

    LOG_INFO("GRAPHICS", "SSR pass added to graph (max_distance=%.0f, max_steps=%u)",
             ctx->max_distance, ctx->max_steps);

    return ssr_output;
}
