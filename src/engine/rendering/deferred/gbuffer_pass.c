// src/engine/rendering/deferred/gbuffer_pass.c
// Deferred G-Buffer Pass - Geometry buffer generation for deferred rendering

#include <core/logger.h>
#include <stdbool.h>
#include <stdint.h>
#include <stdlib.h>
#include <string.h>

#include "backend/metal/mtl_buffer.h"
#include "backend/metal/mtl_device.h"
#include "backend/metal/mtl_render_command_encoder.h"
#include "backend/metal/mtl_render_pipeline.h"
#include "backend/metal/mtl_texture.h"
#include "../framebuffer.h"
#include "../render_pipeline.h"

// ============================================================================
// G-Buffer Types
// ============================================================================

typedef enum {
    GBUFFER_ALBEDO = 0,
    GBUFFER_NORMAL,
    GBUFFER_MATERIAL,    // metallic, roughness, ao
    GBUFFER_MOTION,      // motion vectors
    GBUFFER_COUNT
} GBufferTexture;

typedef struct {
    metal_texture_t *textures[GBUFFER_COUNT];
    metal_texture_t *depth_texture;
    Framebuffer *framebuffer;
    
    uint32_t width;
    uint32_t height;
    
    // Rendering pipeline
    metal_render_pipeline_t *pipeline;
    metal_depth_stencil_state_t *depth_state;
    
    // Statistics
    uint32_t triangles_rendered;
    uint32_t draw_calls;
    float render_time_ms;
} GBuffer;

// ============================================================================
// G-Buffer Shaders (simplified)
// ============================================================================

static const char *gbuffer_vertex_source = 
"#include <metal_stdlib>\n"
"using namespace metal;\n"
"\n"
"struct VertexIn {\n"
"    float3 position [[attribute(0)]];\n"
"    float3 normal [[attribute(1)]];\n"
"    float2 tex_coord [[attribute(2)]];\n"
"    float4 tangent [[attribute(3)]];\n"
"};\n"
"\n"
"struct VertexOut {\n"
"    float4 position [[position]];\n"
"    float3 world_pos;\n"
"    float3 normal;\n"
"    float2 tex_coord;\n"
"    float4 tangent;\n"
"};\n"
"\n"
"struct Uniforms {\n"
"    float4x4 model;\n"
"    float4x4 view;\n"
"    float4x4 proj;\n"
"    float4x4 model_view_proj;\n"
"    float4x4 prev_model_view_proj;\n"
"};\n"
"\n"
"vertex VertexOut gbuffer_vertex_main(VertexIn in [[stage_in]],\n"
"                                   constant Uniforms& uniforms [[buffer(1)]]) {\n"
"    VertexOut out;\n"
"    \n"
"    out.world_pos = (uniforms.model * float4(in.position, 1.0)).xyz;\n"
"    out.position = uniforms.model_view_proj * float4(in.position, 1.0);\n"
"    out.normal = (uniforms.model * float4(in.normal, 0.0)).xyz;\n"
"    out.tex_coord = in.tex_coord;\n"
"    out.tangent = uniforms.model * float4(in.tangent.xyz, 0.0);\n"
"    \n"
"    return out;\n"
"}\n";

static const char *gbuffer_fragment_source = 
"#include <metal_stdlib>\n"
"using namespace metal;\n"
"\n"
"struct VertexOut {\n"
"    float4 position [[position]];\n"
"    float3 world_pos;\n"
"    float3 normal;\n"
"    float2 tex_coord;\n"
"    float4 tangent;\n"
"};\n"
"\n"
"struct MaterialData {\n"
"    float3 albedo;\n"
"    float metallic;\n"
"    float roughness;\n"
"    float ao;\n"
"    float3 emissive;\n"
"    float alpha;\n"
"};\n"
"\n"
"struct GBufferOut {\n"
"    float4 albedo [[color(0)]];\n"
"    float4 normal [[color(1)]];\n"
"    float4 material [[color(2)]];\n"
"    float4 motion [[color(3)]];\n"
"};\n"
"\n"
"fragment GBufferOut gbuffer_fragment_main(VertexOut in [[stage_in]],\n"
"                                         constant MaterialData& material [[buffer(2)]],\n"
"                                         texture2d<float> albedo_tex [[texture(0)]],\n"
"                                         texture2d<float> normal_tex [[texture(1)]],\n"
"                                         texture2d<float> material_tex [[texture(2)]],\n"
"                                         sampler texture_sampler [[sampler(0)]]) {\n"
"    GBufferOut out;\n"
"    \n"
"    // Sample textures\n"
"    float3 albedo = albedo_tex.sample(texture_sampler, in.tex_coord).rgb * material.albedo;\n"
"    float ao = material.ao;\n"
"    float roughness = material.roughness;\n"
"    float metallic = material.metallic;\n"
"    \n"
"    if (material_tex.is_valid()) {\n"
"        float4 mat_sample = material_tex.sample(texture_sampler, in.tex_coord);\n"
"        ao *= mat_sample.r;\n"
"        roughness *= mat_sample.g;\n"
"        metallic *= mat_sample.b;\n"
"    }\n"
"    \n"
"    // Pack normal\n"
"    float3 normal = normalize(in.normal);\n"
"    if (normal_tex.is_valid()) {\n"
"        float3 normal_map = normal_tex.sample(texture_sampler, in.tex_coord).rgb * 2.0 - 1.0;\n"
"        float3 T = normalize(in.tangent.xyz);\n"
"        float3 N = normal;\n"
"        float3 B = cross(N, T) * in.tangent.w;\n"
"        mat3 TBN = mat3(T, B, N);\n"
"        normal = normalize(TBN * normal_map);\n"
"    }\n"
"    \n"
"    // Output to G-buffer\n"
"    out.albedo = float4(albedo, material.alpha);\n"
"    out.normal = float4(normal * 0.5 + 0.5, 1.0); // Pack to [0,1]\n"
"    out.material = float4(metallic, roughness, ao, 0.0);\n"
"    out.motion = float4(0.0); // TODO: Implement motion vectors\n"
"    \n"
"    return out;\n"
"}\n";

// ============================================================================
// G-Buffer API
// ============================================================================

GBuffer *gbuffer_create(uint32_t width, uint32_t height) {
    GBuffer *gbuffer = calloc(1, sizeof(GBuffer));
    if (!gbuffer) {
        LOG_ERROR("Failed to allocate G-buffer");
        return NULL;
    }
    
    gbuffer->width = width;
    gbuffer->height = height;
    
    // Create Metal device
    metal_device_t *device = metal_device_create_system_default();
    if (!device) {
        LOG_ERROR("Failed to create Metal device");
        free(gbuffer);
        return NULL;
    }
    
    // Create G-buffer textures
    metal_texture_desc_t tex_desc = {
        .width = width,
        .height = height,
        .depth = 1,
        .pixel_format = METAL_PIXEL_FORMAT_BGRA8_UNORM,
        .texture_type = METAL_TEXTURE_TYPE_2D,
        .usage = METAL_TEXTURE_USAGE_RENDER_TARGET | METAL_TEXTURE_USAGE_SHADER_READ,
        .storage_mode = METAL_STORAGE_PRIVATE
    };
    
    // Albedo texture
    gbuffer->textures[GBUFFER_ALBEDO] = metal_texture_create(device, &tex_desc);
    
    // Normal texture (RGB10A2 for better precision)
    tex_desc.pixel_format = METAL_PIXEL_FORMAT_RGB10A2_UNORM;
    gbuffer->textures[GBUFFER_NORMAL] = metal_texture_create(device, &tex_desc);
    
    // Material texture (metallic, roughness, AO)
    tex_desc.pixel_format = METAL_PIXEL_FORMAT_BGRA8_UNORM;
    gbuffer->textures[GBUFFER_MATERIAL] = metal_texture_create(device, &tex_desc);
    
    // Motion vectors texture
    gbuffer->textures[GBUFFER_MOTION] = metal_texture_create(device, &tex_desc);
    
    // Depth texture
    tex_desc.pixel_format = METAL_PIXEL_FORMAT_DEPTH32_FLOAT;
    tex_desc.usage = METAL_TEXTURE_USAGE_RENDER_TARGET | METAL_TEXTURE_USAGE_SHADER_READ;
    gbuffer->depth_texture = metal_texture_create(device, &tex_desc);
    
    // Verify all textures were created
    for (int i = 0; i < GBUFFER_COUNT; i++) {
        if (!gbuffer->textures[i]) {
            LOG_ERROR("Failed to create G-buffer texture %d", i);
            gbuffer_destroy(gbuffer);
            return NULL;
        }
    }
    
    if (!gbuffer->depth_texture) {
        LOG_ERROR("Failed to create depth texture");
        gbuffer_destroy(gbuffer);
        return NULL;
    }
    
    // Create framebuffer
    gbuffer->framebuffer = framebuffer_create(width, height);
    if (!gbuffer->framebuffer) {
        LOG_ERROR("Failed to create G-buffer framebuffer");
        gbuffer_destroy(gbuffer);
        return NULL;
    }
    
    // Attach textures to framebuffer
    for (int i = 0; i < GBUFFER_COUNT; i++) {
        framebuffer_attach_color(gbuffer->framebuffer, i, gbuffer->textures[i]);
    }
    framebuffer_attach_depth(gbuffer->framebuffer, gbuffer->depth_texture);
    
    // Validate framebuffer
    if (!framebuffer_validate(gbuffer->framebuffer)) {
        LOG_ERROR("G-buffer framebuffer validation failed");
        gbuffer_destroy(gbuffer);
        return NULL;
    }
    
    // TODO: Create rendering pipeline
    // gbuffer->pipeline = create_gbuffer_pipeline(gbuffer_vertex_source, gbuffer_fragment_source);
    
    LOG_INFO("G-buffer created: %ux%u", width, height);
    return gbuffer;
}

void gbuffer_destroy(GBuffer *gbuffer) {
    if (!gbuffer)
        return;
    
    // Destroy textures
    for (int i = 0; i < GBUFFER_COUNT; i++) {
        if (gbuffer->textures[i]) {
            metal_texture_destroy(gbuffer->textures[i]);
        }
    }
    
    if (gbuffer->depth_texture) {
        metal_texture_destroy(gbuffer->depth_texture);
    }
    
    // Destroy framebuffer
    if (gbuffer->framebuffer) {
        framebuffer_destroy(gbuffer->framebuffer);
    }
    
    // TODO: Destroy pipeline and depth state
    
    free(gbuffer);
    LOG_INFO("G-buffer destroyed");
}

void gbuffer_resize(GBuffer *gbuffer, uint32_t width, uint32_t height) {
    if (!gbuffer || width == 0 || height == 0)
        return;
    
    // TODO: Implement texture resizing
    // For now, just update dimensions
    gbuffer->width = width;
    gbuffer->height = height;
    
    LOG_INFO("G-buffer resized to %ux%u", width, height);
}

void gbuffer_begin_pass(GBuffer *gbuffer, void *command_encoder) {
    if (!gbuffer || !command_encoder)
        return;
    
    // Bind framebuffer
    framebuffer_bind(gbuffer->framebuffer);
    
    // Set viewport
    // metal_render_encoder_set_viewport(command_encoder, 0, 0, gbuffer->width, gbuffer->height);
    
    // Clear G-buffer
    framebuffer_clear_color(gbuffer->framebuffer, 0.0f, 0.0f, 0.0f, 0.0f);
    framebuffer_clear_depth(gbuffer->framebuffer, 1.0f);
    
    // Reset statistics
    gbuffer->triangles_rendered = 0;
    gbuffer->draw_calls = 0;
    
    LOG_DEBUG("G-buffer pass begun");
}

void gbuffer_end_pass(GBuffer *gbuffer) {
    if (!gbuffer)
        return;
    
    framebuffer_unbind();
    
    LOG_DEBUG("G-buffer pass ended: %u draw calls, %u triangles", 
             gbuffer->draw_calls, gbuffer->triangles_rendered);
}

void gbuffer_bind_for_lighting(GBuffer *gbuffer, void *command_encoder) {
    if (!gbuffer || !command_encoder)
        return;
    
    // Bind G-buffer textures for lighting pass
    for (int i = 0; i < GBUFFER_COUNT; i++) {
        // TODO: Bind textures to shader slots
        // metal_render_encoder_set_fragment_texture(command_encoder, gbuffer->textures[i], i);
    }
    
    LOG_DEBUG("G-buffer bound for lighting pass");
}

void gbuffer_get_texture(GBuffer *gbuffer, GBufferTexture type, void **texture) {
    if (!gbuffer || !texture || type >= GBUFFER_COUNT)
        return;
    
    *texture = gbuffer->textures[type];
}

void gbuffer_get_depth_texture(GBuffer *gbuffer, void **texture) {
    if (!gbuffer || !texture)
        return;
    
    *texture = gbuffer->depth_texture;
}

void gbuffer_get_dimensions(GBuffer *gbuffer, uint32_t *width, uint32_t *height) {
    if (!gbuffer)
        return;
    
    if (width) *width = gbuffer->width;
    if (height) *height = gbuffer->height;
}

void gbuffer_get_stats(GBuffer *gbuffer, uint32_t *draw_calls, uint32_t *triangles, float *time_ms) {
    if (!gbuffer)
        return;
    
    if (draw_calls) *draw_calls = gbuffer->draw_calls;
    if (triangles) *triangles = gbuffer->triangles_rendered;
    if (time_ms) *time_ms = gbuffer->render_time_ms;
}
