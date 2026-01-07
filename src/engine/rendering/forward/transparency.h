/*
 * transparency.h
 * Transparency System for Forward Renderer
 *
 * Handles alpha blending configuration, sorting, and transparency modes.
 */

#ifndef RENDERING_FORWARD_TRANSPARENCY_H
#define RENDERING_FORWARD_TRANSPARENCY_H

#include <core/types.h>

/* ============================================================================
 * TYPES
 * ============================================================================ */

typedef enum {
    BLEND_MODE_OPAQUE = 0,
    BLEND_MODE_ALPHA,           // Standard Alpha Blending (SrcAlpha, OneMinusSrcAlpha)
    BLEND_MODE_PREMULTIPLIED,   // Premultiplied Alpha (One, OneMinusSrcAlpha)
    BLEND_MODE_ADDITIVE,        // Additive (SrcAlpha, One) or (One, One)
    BLEND_MODE_MULTIPLY,        // Multiplicative (DstColor, Zero)
    BLEND_MODE_SCREEN,          // Screen (One, OneMinusSrcColor)
    BLEND_MODE_CUSTOM
} BlendMode;

typedef struct {
    BlendMode mode;
    bool alpha_to_coverage;
    bool write_depth;       // Usually false for transparent, but maybe true for some cases
    float alpha_cutoff;     // For alpha testing
} TransparencyState;

/* ============================================================================
 * API
 * ============================================================================ */

/**
 * Initializes the transparency system constants/tables.
 */
void transparency_system_init(void);

/**
 * Configures the GPU blend state for the specified mode.
 */
void transparency_set_blend_state(BlendMode mode);

/**
 * Helper to determine if a material/object requires sorting.
 */
bool transparency_requires_sorting(BlendMode mode);

/**
 * Returns true if the blend mode is order-independent (e.g., additive often effectively is).
 */
bool transparency_is_order_independent(BlendMode mode);

/**
 * Enables or disables Alpha to Coverage (A2C) for MSAA.
 * This converts alpha values to coverage (subpixel masks) for smoother edges on foliage/hair.
 */
void transparency_set_alpha_to_coverage(bool enabled);

/**
 * Sets the alpha test cutoff threshold for alpha masking.
 * Pixels with alpha below this value are discarded (in shader).
 */
void transparency_set_alpha_test_cutoff(float cutoff);

/**
 * Gets the currently active blend mode.
 */
BlendMode transparency_get_current_mode(void);

#endif /* RENDERING_FORWARD_TRANSPARENCY_H */
