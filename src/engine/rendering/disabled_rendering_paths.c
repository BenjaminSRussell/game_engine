// Disabled Rendering Paths Implementation
// Provides fallback and debugging rendering modes

#include "rendering/disabled_rendering_paths.h"
#include "rendering/post_processing/post_processing_pipeline.h"
#include "rendering/frame_graph/frame_graph.h"
#include "core/logger.h"
#include "core/memory.h"
#include <stdlib.h>
#include <string.h>

// Rendering path configuration
typedef struct {
    bool forward_rendering_enabled;
    bool deferred_rendering_enabled;
    bool post_processing_enabled;
    bool shadows_enabled;
    bool lighting_enabled;
    bool texturing_enabled;
    bool wireframe_mode;
    bool depth_only_mode;
    bool normal_visualization;
    bool uv_visualization;
    bool performance_mode;
} RenderingPathConfig;

// Disabled rendering context
typedef struct DisabledRenderingContext {
    RenderingPathConfig config;
    
    // Fallback resources
    TextureID fallback_color_texture;
    TextureID fallback_depth_texture;
    TextureID fallback_normal_texture;
    TextureID debug_texture;
    
    // Simple shaders for fallback rendering
    u32 basic_vertex_shader;
    u32 basic_fragment_shader;
    u32 wireframe_shader;
    u32 depth_only_shader;
    u32 normal_shader;
    u32 uv_shader;
    
    // Performance monitoring
    struct {
        u64 frames_rendered;
        f64 average_frame_time;
        f64 memory_usage_mb;
    } stats;
    
    bool initialized;
} DisabledRenderingContext;

// Initialize disabled rendering system
DisabledRenderingContext* disabled_rendering_init(u32 width, u32 height) {
    DisabledRenderingContext *ctx = malloc(sizeof(DisabledRenderingContext));
    if (!ctx) {
        LOG_ERROR("Failed to allocate disabled rendering context");
        return NULL;
    }
    
    memset(ctx, 0, sizeof(DisabledRenderingContext));
    
    // Set default configuration
    ctx->config.forward_rendering_enabled = true;
    ctx->config.deferred_rendering_enabled = false;
    ctx->config.post_processing_enabled = false;
    ctx->config.shadows_enabled = false;
    ctx->config.lighting_enabled = false;
    ctx->config.texturing_enabled = true;
    ctx->config.wireframe_mode = false;
    ctx->config.depth_only_mode = false;
    ctx->config.normal_visualization = false;
    ctx->config.uv_visualization = false;
    ctx->config.performance_mode = false;
    
    // Create fallback textures
    TextureDesc color_desc = {
        .width = width,
        .height = height,
        .depth = 1,
        .format = TEXTURE_FORMAT_RGBA8,
        .usage = TEXTURE_USAGE_RENDER_TARGET | TEXTURE_USAGE_SAMPLED,
        .name = "Fallback_Color"
    };
    
    TextureDesc depth_desc = {
        .width = width,
        .height = height,
        .depth = 1,
        .format = TEXTURE_FORMAT_DEPTH32F,
        .usage = TEXTURE_USAGE_DEPTH_STENCIL | TEXTURE_USAGE_SAMPLED,
        .name = "Fallback_Depth"
    };
    
    ctx->fallback_color_texture = texture_manager_create(&color_desc);
    ctx->fallback_depth_texture = texture_manager_create(&depth_desc);
    
    if (ctx->fallback_color_texture == INVALID_TEXTURE_ID || 
        ctx->fallback_depth_texture == INVALID_TEXTURE_ID) {
        LOG_ERROR("Failed to create fallback textures");
        free(ctx);
        return NULL;
    }
    
    // Load basic shaders
    ctx->basic_vertex_shader = shader_load_vertex("basic.vert");
    ctx->basic_fragment_shader = shader_load_fragment("basic.frag");
    ctx->wireframe_shader = shader_load("wireframe.shader");
    ctx->depth_only_shader = shader_load("depth_only.shader");
    ctx->normal_shader = shader_load("normal_visualization.shader");
    ctx->uv_shader = shader_load("uv_visualization.shader");
    
    ctx->initialized = true;
    
    LOG_INFO("Disabled rendering system initialized");
    return ctx;
}

// Shutdown disabled rendering system
void disabled_rendering_shutdown(DisabledRenderingContext *ctx) {
    if (!ctx || !ctx->initialized) return;
    
    // Destroy textures
    if (ctx->fallback_color_texture != INVALID_TEXTURE_ID) {
        texture_manager_destroy(ctx->fallback_color_texture);
    }
    if (ctx->fallback_depth_texture != INVALID_TEXTURE_ID) {
        texture_manager_destroy(ctx->fallback_depth_texture);
    }
    if (ctx->fallback_normal_texture != INVALID_TEXTURE_ID) {
        texture_manager_destroy(ctx->fallback_normal_texture);
    }
    if (ctx->debug_texture != INVALID_TEXTURE_ID) {
        texture_manager_destroy(ctx->debug_texture);
    }
    
    // Destroy shaders
    if (ctx->basic_vertex_shader != 0) shader_destroy(ctx->basic_vertex_shader);
    if (ctx->basic_fragment_shader != 0) shader_destroy(ctx->basic_fragment_shader);
    if (ctx->wireframe_shader != 0) shader_destroy(ctx->wireframe_shader);
    if (ctx->depth_only_shader != 0) shader_destroy(ctx->depth_only_shader);
    if (ctx->normal_shader != 0) shader_destroy(ctx->normal_shader);
    if (ctx->uv_shader != 0) shader_destroy(ctx->uv_shader);
    
    free(ctx);
    LOG_INFO("Disabled rendering system shutdown");
}

// Configure rendering paths
void disabled_rendering_set_config(DisabledRenderingContext *ctx, const RenderingPathConfig *config) {
    if (!ctx || !ctx->initialized || !config) return;
    
    ctx->config = *config;
    
    LOG_INFO("Rendering paths configured:");
    LOG_INFO("  Forward rendering: %s", ctx->config.forward_rendering_enabled ? "enabled" : "disabled");
    LOG_INFO("  Deferred rendering: %s", ctx->config.deferred_rendering_enabled ? "enabled" : "disabled");
    LOG_INFO("  Post-processing: %s", ctx->config.post_processing_enabled ? "enabled" : "disabled");
    LOG_INFO("  Shadows: %s", ctx->config.shadows_enabled ? "enabled" : "disabled");
    LOG_INFO("  Lighting: %s", ctx->config.lighting_enabled ? "enabled" : "disabled");
    LOG_INFO("  Texturing: %s", ctx->config.texturing_enabled ? "enabled" : "disabled");
    LOG_INFO("  Wireframe mode: %s", ctx->config.wireframe_mode ? "enabled" : "disabled");
    LOG_INFO("  Depth only: %s", ctx->config.depth_only_mode ? "enabled" : "disabled");
}

// Render frame with disabled paths
void disabled_rendering_render_frame(DisabledRenderingContext *ctx, RenderGraph *render_graph) {
    if (!ctx || !ctx->initialized || !render_graph) return;
    
    Timer frame_timer;
    timer_start(&frame_timer);
    
    // Begin frame
    rg_begin_frame(render_graph);
    
    // Choose rendering mode based on configuration
    if (ctx->config.depth_only_mode) {
        render_depth_only_mode(ctx, render_graph);
    } else if (ctx->config.wireframe_mode) {
        render_wireframe_mode(ctx, render_graph);
    } else if (ctx->config.normal_visualization) {
        render_normal_visualization_mode(ctx, render_graph);
    } else if (ctx->config.uv_visualization) {
        render_uv_visualization_mode(ctx, render_graph);
    } else if (ctx->config.performance_mode) {
        render_performance_mode(ctx, render_graph);
    } else {
        render_basic_forward_mode(ctx, render_graph);
    }
    
    // Execute render graph
    rg_execute(render_graph);
    
    // End frame
    rg_end_frame(render_graph);
    
    // Update statistics
    f64 frame_time = timer_get_elapsed_seconds(&frame_timer) * 1000.0;
    ctx->stats.frames_rendered++;
    ctx->stats.average_frame_time = (ctx->stats.average_frame_time * (ctx->stats.frames_rendered - 1) + frame_time) / ctx->stats.frames_rendered;
    ctx->stats.memory_usage_mb = memory_get_usage() / (1024.0 * 1024.0);
}

// Basic forward rendering mode
static void render_basic_forward_mode(DisabledRenderingContext *ctx, RenderGraph *render_graph) {
    // Create basic render target
    RGResourceHandle color_target = rg_get_texture_handle(render_graph, ctx->fallback_color_texture);
    RGResourceHandle depth_target = rg_get_texture_handle(render_graph, ctx->fallback_depth_texture);
    
    // Add basic render pass
    RGPassDesc pass_desc = {
        .name = "BasicForward",
        .color_attachments = &color_target,
        .color_attachment_count = 1,
        .depth_stencil_attachment = depth_target,
        .execute = basic_forward_execute,
        .user_data = ctx,
        .queue_type = RG_QUEUE_GRAPHICS
    };
    
    rg_add_pass(render_graph, &pass_desc);
}

// Wireframe rendering mode
static void render_wireframe_mode(DisabledRenderingContext *ctx, RenderGraph *render_graph) {
    RGResourceHandle color_target = rg_get_texture_handle(render_graph, ctx->fallback_color_texture);
    RGResourceHandle depth_target = rg_get_texture_handle(render_graph, ctx->fallback_depth_texture);
    
    RGPassDesc pass_desc = {
        .name = "Wireframe",
        .color_attachments = &color_target,
        .color_attachment_count = 1,
        .depth_stencil_attachment = depth_target,
        .execute = wireframe_execute,
        .user_data = ctx,
        .queue_type = RG_QUEUE_GRAPHICS
    };
    
    rg_add_pass(render_graph, &pass_desc);
}

// Depth only rendering mode
static void render_depth_only_mode(DisabledRenderingContext *ctx, RenderGraph *render_graph) {
    RGResourceHandle depth_target = rg_get_texture_handle(render_graph, ctx->fallback_depth_texture);
    
    RGPassDesc pass_desc = {
        .name = "DepthOnly",
        .depth_stencil_attachment = depth_target,
        .execute = depth_only_execute,
        .user_data = ctx,
        .queue_type = RG_QUEUE_GRAPHICS
    };
    
    rg_add_pass(render_graph, &pass_desc);
}

// Normal visualization mode
static void render_normal_visualization_mode(DisabledRenderingContext *ctx, RenderGraph *render_graph) {
    RGResourceHandle color_target = rg_get_texture_handle(render_graph, ctx->fallback_color_texture);
    RGResourceHandle depth_target = rg_get_texture_handle(render_graph, ctx->fallback_depth_texture);
    
    RGPassDesc pass_desc = {
        .name = "NormalVisualization",
        .color_attachments = &color_target,
        .color_attachment_count = 1,
        .depth_stencil_attachment = depth_target,
        .execute = normal_visualization_execute,
        .user_data = ctx,
        .queue_type = RG_QUEUE_GRAPHICS
    };
    
    rg_add_pass(render_graph, &pass_desc);
}

// UV visualization mode
static void render_uv_visualization_mode(DisabledRenderingContext *ctx, RenderGraph *render_graph) {
    RGResourceHandle color_target = rg_get_texture_handle(render_graph, ctx->fallback_color_texture);
    RGResourceHandle depth_target = rg_get_texture_handle(render_graph, ctx->fallback_depth_texture);
    
    RGPassDesc pass_desc = {
        .name = "UVVisualization",
        .color_attachments = &color_target,
        .color_attachment_count = 1,
        .depth_stencil_attachment = depth_target,
        .execute = uv_visualization_execute,
        .user_data = ctx,
        .queue_type = RG_QUEUE_GRAPHICS
    };
    
    rg_add_pass(render_graph, &pass_desc);
}

// Performance mode (minimal rendering)
static void render_performance_mode(DisabledRenderingContext *ctx, RenderGraph *render_graph) {
    RGResourceHandle color_target = rg_get_texture_handle(render_graph, ctx->fallback_color_texture);
    
    RGPassDesc pass_desc = {
        .name = "PerformanceMode",
        .color_attachments = &color_target,
        .color_attachment_count = 1,
        .execute = performance_mode_execute,
        .user_data = ctx,
        .queue_type = RG_QUEUE_GRAPHICS
    };
    
    rg_add_pass(render_graph, &pass_desc);
}

// Pass execution functions
static void basic_forward_execute(RGPassContext *ctx, void *user_data) {
    DisabledRenderingContext *render_ctx = (DisabledRenderingContext*)user_data;
    
    // Bind basic shaders
    shader_bind(render_ctx->basic_vertex_shader);
    shader_bind(render_ctx->basic_fragment_shader);
    
    // Set basic uniforms
    vec4 clear_color = {0.2f, 0.3f, 0.8f, 1.0f}; // Sky blue
    shader_set_uniform("uClearColor", &clear_color, sizeof(clear_color));
    
    // Render simple geometry
    render_simple_geometry();
    
    LOG_DEBUG("Basic forward rendering executed");
}

static void wireframe_execute(RGPassContext *ctx, void *user_data) {
    DisabledRenderingContext *render_ctx = (DisabledRenderingContext*)user_data;
    
    // Enable wireframe rendering
    renderer_set_wireframe(true);
    
    // Bind wireframe shader
    shader_bind(render_ctx->wireframe_shader);
    
    // Render geometry as wireframe
    render_simple_geometry();
    
    // Disable wireframe
    renderer_set_wireframe(false);
    
    LOG_DEBUG("Wireframe rendering executed");
}

static void depth_only_execute(RGPassContext *ctx, void *user_data) {
    DisabledRenderingContext *render_ctx = (DisabledRenderingContext*)user_data;
    
    // Bind depth-only shader
    shader_bind(render_ctx->depth_only_shader);
    
    // Set depth write only
    renderer_set_color_write_mask(false, false, false, false);
    renderer_set_depth_write_mask(true);
    
    // Render geometry for depth
    render_simple_geometry();
    
    // Restore color write
    renderer_set_color_write_mask(true, true, true, true);
    
    LOG_DEBUG("Depth-only rendering executed");
}

static void normal_visualization_execute(RGPassContext *ctx, void *user_data) {
    DisabledRenderingContext *render_ctx = (DisabledRenderingContext*)user_data;
    
    // Bind normal visualization shader
    shader_bind(render_ctx->normal_shader);
    
    // Render geometry with normal colors
    render_simple_geometry();
    
    LOG_DEBUG("Normal visualization executed");
}

static void uv_visualization_execute(RGPassContext *ctx, void *user_data) {
    DisabledRenderingContext *render_ctx = (DisabledRenderingContext*)user_data;
    
    // Bind UV visualization shader
    shader_bind(render_ctx->uv_shader);
    
    // Render geometry with UV colors
    render_simple_geometry();
    
    LOG_DEBUG("UV visualization executed");
}

static void performance_mode_execute(RGPassContext *ctx, void *user_data) {
    DisabledRenderingContext *render_ctx = (DisabledRenderingContext*)user_data;
    
    // Minimal rendering - just clear to a solid color
    vec4 clear_color = {0.1f, 0.1f, 0.1f, 1.0f}; // Dark gray
    renderer_clear_color(clear_color);
    
    LOG_DEBUG("Performance mode executed");
}

// Helper function to render simple geometry
static void render_simple_geometry(void) {
    // Render a simple test scene with basic shapes
    // This would normally render the actual scene geometry
    // For now, just render a simple triangle or quad
    
    static const float vertices[] = {
        // Position (x, y, z), Normal (nx, ny, nz), UV (u, v)
        -0.5f, -0.5f, 0.0f,  0.0f, 0.0f, 1.0f,  0.0f, 0.0f,
         0.5f, -0.5f, 0.0f,  0.0f, 0.0f, 1.0f,  1.0f, 0.0f,
         0.0f,  0.5f, 0.0f,  0.0f, 0.0f, 1.0f,  0.5f, 1.0f
    };
    
    renderer_draw_triangles(vertices, sizeof(vertices), 3);
}

// Get rendering statistics
void disabled_rendering_get_stats(DisabledRenderingContext *ctx, 
                                u64 *frames_rendered, 
                                f64 *average_frame_time, 
                                f64 *memory_usage_mb) {
    if (!ctx || !ctx->initialized) return;
    
    if (frames_rendered) *frames_rendered = ctx->stats.frames_rendered;
    if (average_frame_time) *average_frame_time = ctx->stats.average_frame_time;
    if (memory_usage_mb) *memory_usage_mb = ctx->stats.memory_usage_mb;
}

// Reset statistics
void disabled_rendering_reset_stats(DisabledRenderingContext *ctx) {
    if (!ctx || !ctx->initialized) return;
    
    memset(&ctx->stats, 0, sizeof(ctx->stats));
}
