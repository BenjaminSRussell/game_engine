/*
 * refraction.h
 * Refraction and Distortion System
 *
 * Handles screen-space refraction, IOR parameters, and distortion effects.
 */

#ifndef RENDERING_FORWARD_REFRACTION_H
#define RENDERING_FORWARD_REFRACTION_H

#include <core/types.h>

/* ============================================================================
 * TYPES
 * ============================================================================ */

typedef struct {
    f32 ior;                // Index of Refraction (e.g., 1.33 for water, 1.52 for glass)
    f32 thickness;          // Simulated thickness of the object
    f32 chromatic_aberration; // Strength of color dispersion
    f32 roughness;          // Surface roughness affecting refraction blur
    bool use_thickness_map;
    bool use_ior_map;
} RefractionParams;

/* ============================================================================
 * API
 * ============================================================================ */

/**
 * Initializes the refraction system resources.
 */
void refraction_init(void);

/**
 * Updates the scene color texture used for screen-space refraction.
 * This should be called before rendering refractive objects.
 * Copies the current backbuffer to a texture.
 */
void refraction_update_scene_color(void* command_buffer);

/**
 * Gets the current scene color texture.
 */
void* refraction_get_scene_color_texture(void);

/**
 * Helper: Calculates the Fresnel term for a given IOR.
 * F0 = ((ior - 1) / (ior + 1))^2
 */
f32 refraction_calculate_f0(f32 ior);

/**
 * Helper: Calculates base offsets for chromatic aberration.
 * Returns vec3 where x=Red offset, y=Green offset, z=Blue offset.
 */
vec3 refraction_calculate_chromatic_aberration_offsets(f32 strength);

#endif /* RENDERING_FORWARD_REFRACTION_H */
