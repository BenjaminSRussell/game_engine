/*
 * refraction.c
 * Refraction System Implementation
 */

#include "refraction.h"
#include <core/logger.h>
#include <math.h>

/* ============================================================================
 * INTERNAL STATE
 * ============================================================================ */

static struct {
    void* scene_color_texture;
    bool initialized;
} g_refraction_state;

/* ============================================================================
 * IMPLEMENTATION
 * ============================================================================ */

void refraction_init(void) {
    g_refraction_state.scene_color_texture = NULL;
    g_refraction_state.initialized = true;
    LOG_INFO("RefractionSystem: Initialized.");
}

void refraction_update_scene_color(void* command_buffer) {
    if (!g_refraction_state.initialized) return;
    
    // TODO: Copy current framebuffer color attachment to g_refraction_state.scene_color_texture
    // This allows transparent objects to sample what's behind them.
    // Ideally, this texture should be a lower mip chain or blurred for rough refraction.
}

void* refraction_get_scene_color_texture(void) {
    return g_refraction_state.scene_color_texture;
}

f32 refraction_calculate_f0(f32 ior) {
    // Fresnel reflectance at normal incidence for dielectrics
    f32 f = (ior - 1.0f) / (ior + 1.0f);
    return f * f;
}
