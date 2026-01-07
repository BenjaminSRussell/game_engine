/*
 * transparency.c
 * Transparency System Implementation
 */

#include "transparency.h"
#include <core/logger.h>

/* ============================================================================
 * INTERNAL STATE
 * ============================================================================ */

// Mocking backend state calls for now. In a real engine, this would interface with 
// the RHI (Render Hardware Interface) like Vulkan or Metal backend.

static struct {
    BlendMode current_mode;
    bool initialized;
} g_transparency_state;

/* ============================================================================
 * IMPLEMENTATION
 * ============================================================================ */

void transparency_system_init(void) {
    g_transparency_state.current_mode = BLEND_MODE_OPAQUE;
    g_transparency_state.initialized = true;
    LOG_INFO("TransparencySystem: Initialized.");
}

void transparency_set_blend_state(BlendMode mode) {
    if (g_transparency_state.current_mode == mode) return;
    
    // In a real implementation:
    // switch (mode) {
    //     case BLEND_MODE_ALPHA: 
    //         rhi_set_blend_func(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA); 
    //         break;
    //     ...
    // }
    
    // For now dealing with logic only
    g_transparency_state.current_mode = mode;
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
    
    return false;
}
