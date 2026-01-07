/*
 * refraction.c
 * Refraction System Implementation
 */

#include "refraction.h"
#include <core/logger.h>
#include <core/memory.h>
#include <stdlib.h>
#include <math.h>

/* ============================================================================
 * INTERNAL STATE
 * ============================================================================ */

static struct {
    void* scene_color_texture; // Copy of backbuffer
    u32 width;
    u32 height;
    bool initialized;
} g_refraction_state;

/* ============================================================================
 * IMPLEMENTATION
 * ============================================================================ */

void refraction_init(void) {
    g_refraction_state.scene_color_texture = NULL;
    g_refraction_state.width = 0;
    g_refraction_state.height = 0;
    g_refraction_state.initialized = true;
    LOG_INFO("RefractionSystem: Initialized.");
}

void refraction_update_scene_color(void* command_buffer) {
    if (!g_refraction_state.initialized) return;
    
    // In a real implementation using Vulkan/Metal:
    // 1. Transition backbuffer (or current offscreen target) to TRANSFER_SRC
    // 2. Transition g_refraction_state.scene_color_texture to TRANSFER_DST
    // 3. Blit or Copy image
    // 4. Transition both back to original/shader_read layouts
    
    // If the texture hasn't been created or size changed, create/resize it
    // u32 current_width = renderer_get_width();
    // u32 current_height = renderer_get_height();
    
    // if (!g_refraction_state.scene_color_texture || 
    //     g_refraction_state.width != current_width || 
    //     g_refraction_state.height != current_height) {
        
    //     // recreate_texture(...)
    //     g_refraction_state.width = current_width;
    //     g_refraction_state.height = current_height;
    // }
    
    // texture_copy_from_framebuffer(g_refraction_state.scene_color_texture, command_buffer);
}

void* refraction_get_scene_color_texture(void) {
    return g_refraction_state.scene_color_texture;
}

f32 refraction_calculate_f0(f32 ior) {
    // Fresnel reflectance at normal incidence for dielectrics
    f32 f = (ior - 1.0f) / (ior + 1.0f);
    return f * f;
}

vec3 refraction_calculate_chromatic_aberration_offsets(f32 strength) {
    // Returns texture coordinate offsets for R, G, B channels
    // Typically G is center (0), R and B are offset in opposite directions
    // scaling by distance from center of screen usually happens in shader
    
    return (vec3){-strength * 0.01f, 0.0f, strength * 0.01f}; 
}
