/*
 * transparency.c
 * Transparency System Implementation
 */

#include "rendering/forward/transparency.h"
#include <core/logger.h>
#include <stdbool.h>

/* ============================================================================
 * INTERNAL STATE
 * ============================================================================ */

// Mocking backend state calls for now. In a real engine, this would interface with 
// the RHI (Render Hardware Interface) like Vulkan or Metal backend.

static struct {
    BlendMode current_mode;
    bool alpha_to_coverage_enabled;
    bool initialized;
} g_transparency_state;

/* ============================================================================
 * PRIVATE FUNCTIONS
 * ============================================================================ */

static void apply_blend_state_opengl(BlendMode mode) {
   // In a real implementation, this would call OpenGL/Vulkan/Metal APIs
    // Example OpenGL mapping:
    switch (mode) {
        case BLEND_MODE_OPAQUE:
            // glDisable(GL_BLEND);
            break;
            
        case BLEND_MODE_ALPHA:
            // glEnable(GL_BLEND);
            // glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
            // glBlendEquation(GL_FUNC_ADD);
            break;
            
        case BLEND_MODE_PREMULTIPLIED:
            // glEnable(GL_BLEND);
            // glBlendFunc(GL_ONE, GL_ONE_MINUS_SRC_ALPHA);
            // glBlendEquation(GL_FUNC_ADD);
            break;
            
        case BLEND_MODE_ADDITIVE:
            // glEnable(GL_BLEND);
            // glBlendFunc(GL_SRC_ALPHA, GL_ONE); // or (GL_ONE, GL_ONE)
            // glBlendEquation(GL_FUNC_ADD);
            break;
            
        case BLEND_MODE_MULTIPLY:
            // glEnable(GL_BLEND);
            // glBlendFunc(GL_DST_COLOR, GL_ZERO);
            // glBlendEquation(GL_FUNC_ADD);
            break;
            
        case BLEND_MODE_SCREEN:
            // glEnable(GL_BLEND);
            // glBlendFunc(GL_ONE, GL_ONE_MINUS_SRC_COLOR);
            // glBlendEquation(GL_FUNC_ADD);
            break;
            
        case BLEND_MODE_CUSTOM:
            // Application-specific custom blend mode
            break;
            
        default:
            LOG_WARN("TransparencySystem: Unknown blend mode: %d", mode);
            break;
    }
}

/* ============================================================================
 * IMPLEMENTATION
 * ============================================================================ */

void transparency_system_init(void) {
    g_transparency_state.current_mode = BLEND_MODE_OPAQUE;
    g_transparency_state.alpha_to_coverage_enabled = false;
    g_transparency_state.initialized = true;
    LOG_INFO("TransparencySystem: Initialized.");
}

void transparency_set_blend_state(BlendMode mode) {
    if (g_transparency_state.current_mode == mode) return;
    
    apply_blend_state_opengl(mode);
    g_transparency_state.current_mode = mode;
}

void transparency_set_alpha_to_coverage(bool enabled) {
    if (g_transparency_state.alpha_to_coverage_enabled == enabled) return;
    
    // TODO: Enable/disable Alpha to Coverage MSAA feature
    // if (enabled) {
    //     glEnable(GL_SAMPLE_ALPHA_TO_COVERAGE);
    // } else {
    //     glDisable(GL_SAMPLE_ALPHA_TO_COVERAGE);
    // }
    
    g_transparency_state.alpha_to_coverage_enabled = enabled;
    LOG_DEBUG("TransparencySystem: Alpha-to-Coverage %s", enabled ? "enabled" : "disabled");
}

bool transparency_requires_sorting(BlendMode mode) {
    // Most transparency modes require back-to-front sorting
    if (mode == BLEND_MODE_OPAQUE) return false;
    
    // Additive can sometimes be order-independent for color accumulation, 
    // but depth testing interacting with it might differ. 
    // Generally, standard alpha blending strictly needs sorting.
    return true;
}

bool transparency_is_order_independent(BlendMode mode) {
    // Additive blending is mathematically commutative (A + B = B + A),
    // so strictly speaking, the color result is the same regardless of order.
    // However, if depth writing is off and testing is on, they still need to be drawn 
    // after opaque objects.
    if (mode == BLEND_MODE_ADDITIVE) return true;
    if (mode == BLEND_MODE_SCREEN) return true; // Also commutative
    
    return false;
}

void transparency_set_alpha_test_cutoff(float cutoff) {
    // For alpha testing/masking (discard pixels below threshold)
    // This is typically done in the shader itself in modern graphics
    // But we can store global cutoff value for shader uniform binding
    
    // TODO: Set uniform for alpha cutoff in current shader
    // shader_set_uniform_float("u_alpha_cutoff", cutoff);
    
    LOG_DEBUG("TransparencySystem: Alpha test cutoff set to %.3f", cutoff);
}

BlendMode transparency_get_current_mode(void) {
    return g_transparency_state.current_mode;
}
