/*
 * shadow_caster.c
 * Shadow pass rendering
 *
 * Part of the Lighting subsystem
 * Advanced 3D Rendering Engine
 */

#include "shadow_caster.h"
#include <stdint.h>
#include <stdbool.h>
#include <stddef.h>
#include <string.h>
#include <stdlib.h>
#include <math.h>

/* ============================================================================
 * TYPES
 * ============================================================================ */

typedef struct mat4 {
    float m[16];
} mat4_t;

typedef struct vec3 {
    float x, y, z;
} vec3_t;

typedef struct vec4 {
    float x, y, z, w;
} vec4_t;

typedef struct shadow_view {
    mat4_t view_matrix;
    mat4_t proj_matrix;
    mat4_t view_proj_matrix;
} shadow_view_t;

typedef struct shadow_pass_params {
    uint32_t light_id;
    shadow_view_t view;
    uint32_t atlas_x;
    uint32_t atlas_y;
    uint32_t resolution;
    float depth_bias;
    float slope_bias;
} shadow_pass_params_t;

#define MAX_SHADOW_PASSES 64

typedef struct shadow_caster_context {
    shadow_pass_params_t passes[MAX_SHADOW_PASSES];
    uint32_t pass_count;
    bool initialized;
    
    // Global shadow system (singleton for now, or managed externally)
    shadow_map_system_t* active_system;
} shadow_caster_context_t;

static shadow_caster_context_t g_shadow_caster_ctx = {0};

/* ============================================================================
 * MATRIX HELPERS
 * ============================================================================ */

static void mat4_identity(mat4_t* mat) {
    memset(mat->m, 0, sizeof(mat->m));
    mat->m[0] = mat->m[5] = mat->m[10] = mat->m[15] = 1.0f;
}

static void mat4_multiply(const mat4_t* a, const mat4_t* b, mat4_t* out) {
    mat4_t result;
    for (int row = 0; row < 4; row++) {
        for (int col = 0; col < 4; col++) {
            float sum = 0.0f;
            for (int k = 0; k < 4; k++) {
                sum += a->m[row * 4 + k] * b->m[k * 4 + col];
            }
            result.m[row * 4 + col] = sum;
        }
    }
    *out = result;
}

static void mat4_look_at(const vec3_t* eye, const vec3_t* target, const vec3_t* up, mat4_t* out) {
    vec3_t f = {target->x - eye->x, target->y - eye->y, target->z - eye->z};
    float len = sqrtf(f.x * f.x + f.y * f.y + f.z * f.z);
    if (len > 0) { f.x /= len; f.y /= len; f.z /= len; }
    
    vec3_t s = {
        f.y * up->z - f.z * up->y,
        f.z * up->x - f.x * up->z,
        f.x * up->y - f.y * up->x
    };
    len = sqrtf(s.x * s.x + s.y * s.y + s.z * s.z);
    if (len > 0) { s.x /= len; s.y /= len; s.z /= len; }
    
    vec3_t u = {
        s.y * f.z - s.z * f.y,
        s.z * f.x - s.x * f.z,
        s.x * f.y - s.y * f.x
    };
    
    mat4_identity(out);
    out->m[0] = s.x;  out->m[4] = s.y;  out->m[8] = s.z;
    out->m[1] = u.x;  out->m[5] = u.y;  out->m[9] = u.z;
    out->m[2] = -f.x; out->m[6] = -f.y; out->m[10] = -f.z;
    out->m[12] = -(s.x * eye->x + s.y * eye->y + s.z * eye->z);
    out->m[13] = -(u.x * eye->x + u.y * eye->y + u.z * eye->z);
    out->m[14] = f.x * eye->x + f.y * eye->y + f.z * eye->z;
}

static void mat4_ortho(float left, float right, float bottom, float top, float near, float far, mat4_t* out) {
    mat4_identity(out);
    out->m[0] = 2.0f / (right - left);
    out->m[5] = 2.0f / (top - bottom);
    out->m[10] = -2.0f / (far - near);
    out->m[12] = -(right + left) / (right - left);
    out->m[13] = -(top + bottom) / (top - bottom);
    out->m[14] = -(far + near) / (far - near);
}

static void mat4_perspective(float fov, float aspect, float near, float far, mat4_t* out) {
    memset(out->m, 0, sizeof(out->m));
    float tan_half_fov = tanf(fov / 2.0f);
    out->m[0] = 1.0f / (aspect * tan_half_fov);
    out->m[5] = 1.0f / tan_half_fov;
    out->m[10] = -(far + near) / (far - near);
    out->m[11] = -1.0f;
    out->m[14] = -(2.0f * far * near) / (far - near);
}

/* ============================================================================
 * PUBLIC API
 * ============================================================================ */

int lighting_shadow_caster_init(void) {
    if (g_shadow_caster_ctx.initialized) {
        return 0;
    }
    
    g_shadow_caster_ctx.pass_count = 0;
    memset(g_shadow_caster_ctx.passes, 0, sizeof(g_shadow_caster_ctx.passes));
    g_shadow_caster_ctx.initialized = true;
    
    return 0;
}

void lighting_shadow_caster_shutdown(void) {
    if (!g_shadow_caster_ctx.initialized) {
        return;
    }
    
    g_shadow_caster_ctx.pass_count = 0;
    g_shadow_caster_ctx.initialized = false;
}

int lighting_shadow_caster_begin_pass(uint32_t light_id, uint32_t atlas_x, uint32_t atlas_y, 
                                      uint32_t resolution) {
    if (!g_shadow_caster_ctx.initialized) {
        return -1;
    }
    
    if (g_shadow_caster_ctx.pass_count >= MAX_SHADOW_PASSES) {
        return -2;
    }
    
    shadow_pass_params_t* pass = &g_shadow_caster_ctx.passes[g_shadow_caster_ctx.pass_count++];
    pass->light_id = light_id;
    pass->atlas_x = atlas_x;
    pass->atlas_y = atlas_y;
    pass->resolution = resolution;
    pass->depth_bias = 0.005f;  // Default bias
    pass->slope_bias = 1.75f;   // Default slope bias
    
    return 0;
}

void lighting_shadow_caster_set_view_matrix(const float* view_matrix) {
    if (!g_shadow_caster_ctx.initialized || g_shadow_caster_ctx.pass_count == 0) {
        return;
    }
    
    shadow_pass_params_t* pass = &g_shadow_caster_ctx.passes[g_shadow_caster_ctx.pass_count - 1];
    memcpy(pass->view.view_matrix.m, view_matrix, sizeof(float) * 16);
}

void lighting_shadow_caster_set_proj_matrix(const float* proj_matrix) {
    if (!g_shadow_caster_ctx.initialized || g_shadow_caster_ctx.pass_count == 0) {
        return;
    }
    
    shadow_pass_params_t* pass = &g_shadow_caster_ctx.passes[g_shadow_caster_ctx.pass_count - 1];
    memcpy(pass->view.proj_matrix.m, proj_matrix, sizeof(float) * 16);
    
    // Compute view-proj matrix
    mat4_multiply(&pass->view.proj_matrix, &pass->view.view_matrix, &pass->view.view_proj_matrix);
}

void lighting_shadow_caster_set_directional_light(const float* light_dir, const float* scene_center,
                                                   float scene_radius) {
    if (!g_shadow_caster_ctx.initialized || g_shadow_caster_ctx.pass_count == 0) {
        return;
    }
    
    shadow_pass_params_t* pass = &g_shadow_caster_ctx.passes[g_shadow_caster_ctx.pass_count - 1];
    
    // Create view matrix for directional light
    vec3_t light_pos = {
        scene_center[0] - light_dir[0] * scene_radius,
        scene_center[1] - light_dir[1] * scene_radius,
        scene_center[2] - light_dir[2] * scene_radius
    };
    vec3_t target = {scene_center[0], scene_center[1], scene_center[2]};
    vec3_t up = {0.0f, 1.0f, 0.0f};
    
    mat4_look_at(&light_pos, &target, &up, &pass->view.view_matrix);
    
    // Create orthographic projection
    mat4_ortho(-scene_radius, scene_radius, -scene_radius, scene_radius, 
               0.1f, scene_radius * 2.0f, &pass->view.proj_matrix);
    
    // Compute view-proj
    mat4_multiply(&pass->view.proj_matrix, &pass->view.view_matrix, &pass->view.view_proj_matrix);
}

void lighting_shadow_caster_set_spot_light(const float* light_pos, const float* light_dir,
                                           float fov, float near, float far) {
    if (!g_shadow_caster_ctx.initialized || g_shadow_caster_ctx.pass_count == 0) {
        return;
    }
    
    shadow_pass_params_t* pass = &g_shadow_caster_ctx.passes[g_shadow_caster_ctx.pass_count - 1];
    
    // Create view matrix
    vec3_t pos = {light_pos[0], light_pos[1], light_pos[2]};
    vec3_t target = {
        light_pos[0] + light_dir[0],
        light_pos[1] + light_dir[1],
        light_pos[2] + light_dir[2]
    };
    vec3_t up = {0.0f, 1.0f, 0.0f};
    
    mat4_look_at(&pos, &target, &up, &pass->view.view_matrix);
    
    // Create perspective projection
    mat4_perspective(fov, 1.0f, near, far, &pass->view.proj_matrix);
    
    // Compute view-proj
    mat4_multiply(&pass->view.proj_matrix, &pass->view.view_matrix, &pass->view.view_proj_matrix);
}

void lighting_shadow_caster_set_bias(float depth_bias, float slope_bias) {
    if (!g_shadow_caster_ctx.initialized || g_shadow_caster_ctx.pass_count == 0) {
        return;
    }
    
    shadow_pass_params_t* pass = &g_shadow_caster_ctx.passes[g_shadow_caster_ctx.pass_count - 1];
    pass->depth_bias = depth_bias;
    pass->slope_bias = slope_bias;
}

void lighting_shadow_caster_end_pass(void) {
    // Pass is already stored, nothing to do
}

void lighting_shadow_caster_render_all(void) {
    if (!g_shadow_caster_ctx.initialized) {
        return;
    }
    
    // This would integrate with the actual rendering backend
    // For each pass, set viewport to atlas region and render shadow casters
    for (uint32_t i = 0; i < g_shadow_caster_ctx.pass_count; i++) {
        shadow_pass_params_t* pass = &g_shadow_caster_ctx.passes[i];
        (void)pass; // Suppress unused warning
    }
}

void lighting_shadow_caster_clear_passes(void) {
    g_shadow_caster_ctx.pass_count = 0;
}

uint32_t lighting_shadow_caster_get_pass_count(void) {
    return g_shadow_caster_ctx.pass_count;
}

int lighting_shadow_caster_get_pass_matrix(uint32_t pass_index, float* out_matrix) {
    if (!out_matrix || pass_index >= g_shadow_caster_ctx.pass_count) {
        return -1;
    }
    
    memcpy(out_matrix, g_shadow_caster_ctx.passes[pass_index].view.view_proj_matrix.m, sizeof(float) * 16);
    return 0;
}

/* ============================================================================
 * CASCADED SHADOW SYSTEM IMPLEMENTATION
 * ============================================================================ */

/* Implementation Note: 
 * These functions are C implementations of the Metal logic requested.
 * The actual Metal API calls are commented or abstracted since this is compiled as C.
 * In a real Metal project, these would be in .m/.mm files or use a C wrapper.
 */

shadow_map_system_t* shadow_system_create(void* device, uint32_t resolution, uint32_t cascades) {
    shadow_map_system_t* sys = (shadow_map_system_t*)calloc(1, sizeof(shadow_map_system_t));
    if (!sys) return NULL;
    
    sys->resolution = resolution;
    sys->cascade_count = cascades;
    
    /* 
    MTLTextureDescriptor* desc = [MTLTextureDescriptor 
        texture2DDescriptorWithPixelFormat:MTLPixelFormatDepth32Float 
                                     width:resolution 
                                    height:resolution 
                                 mipmapped:NO];
    desc.usage = MTLTextureUsageRenderTarget | MTLTextureUsageShaderRead;
    desc.storageMode = MTLStorageModePrivate;

    for (uint32_t i = 0; i < cascades; i++) {
        sys->cascades[i].depth_texture = [device newTextureWithDescriptor:desc];
    }
    */
   
    // Placeholder allocation for C compilation
    (void)device;
    
    return sys;
}

void shadow_system_destroy(shadow_map_system_t* sys) {
    if (!sys) return;
    free(sys);
}

void shadow_system_update_cascades(shadow_map_system_t* sys, 
                                  const cascade_camera_t* camera, 
                                  const float* light_dir, 
                                  float shadow_distance) {
    if (!sys || !camera || !light_dir) return;

    cascade_split_info_t split_info[4];
    uint32_t count = sys->cascade_count > 4 ? 4 : sys->cascade_count;
    
    // lambda = 0.5f (Practical split scheme blend)
    cascade_splits_calculate(camera, light_dir, count, shadow_distance, 0.5f, split_info);
    
    for (uint32_t i = 0; i < count; i++) {
        sys->cascades[i].split_near = split_info[i].split_near;
        sys->cascades[i].split_far = split_info[i].split_far;
        memcpy(sys->cascades[i].view_proj, split_info[i].view_proj, sizeof(float) * 16);
    }
}

void shadow_system_render_cascade(shadow_map_system_t* sys, uint32_t cascade_index, 
                                  void* cmd_buffer, void* shadow_casters) {
    if (!sys || cascade_index >= sys->cascade_count) return;
    
    shadow_cascade_t* cascade = &sys->cascades[cascade_index];
    (void)cascade;
    (void)cmd_buffer;
    (void)shadow_casters;
    
    /*
    MTLRenderPassDescriptor* desc = [MTLRenderPassDescriptor renderPassDescriptor];
    desc.depthAttachment.texture = cascade->depth_texture;
    desc.depthAttachment.loadAction = MTLLoadActionClear;
    desc.depthAttachment.storeAction = MTLStoreActionStore;
    desc.depthAttachment.clearDepth = 1.0;

    id<MTLRenderCommandEncoder> encoder = [cmd_buffer renderCommandEncoderWithDescriptor:desc];
    [encoder setRenderPipelineState:sys->shadow_pipeline];
    [encoder setDepthStencilState:sys->shadow_depth_state];
    [encoder setCullMode:MTLCullModeFront]; 

    // Set cascade view-proj
    [encoder setVertexBytes:&cascade->view_proj length:sizeof(float)*16 atIndex:1];

    // Loop through shadow_casters...
    */
}

/* Placeholder implementations for compatibility */
int lighting_shadow_caster_create(lighting_shadow_caster_handle_t* out_handle, 
                                  const lighting_shadow_caster_desc_t* desc) {
    if (!out_handle || !desc) return -1;
    out_handle->id = 0;
    return 0;
}

void lighting_shadow_caster_destroy(lighting_shadow_caster_handle_t handle) {
    (void)handle;
}

int lighting_shadow_caster_update(lighting_shadow_caster_handle_t handle, const void* data, size_t size) {
    (void)handle; (void)data; (void)size;
    return 0;
}

bool lighting_shadow_caster_is_valid(lighting_shadow_caster_handle_t handle) {
    (void)handle;
    return g_shadow_caster_ctx.initialized;
}

int lighting_shadow_caster_get_info(lighting_shadow_caster_handle_t handle, 
                                    lighting_shadow_caster_info_t* out_info) {
    if (!out_info) return -1;
    out_info->id = handle.id;
    out_info->flags = 0;
    out_info->initialized = g_shadow_caster_ctx.initialized;
    return 0;
}

void lighting_shadow_caster_mark_dirty(lighting_shadow_caster_handle_t handle) {
    (void)handle;
}

int lighting_shadow_caster_process_pending(void) {
    return 0;
}

uint32_t lighting_shadow_caster_get_count(void) {
    return g_shadow_caster_ctx.pass_count;
}

size_t lighting_shadow_caster_get_memory_usage(void) {
    return sizeof(shadow_caster_context_t);
}

void lighting_shadow_caster_debug_print(void) {
    // Debug output
}
