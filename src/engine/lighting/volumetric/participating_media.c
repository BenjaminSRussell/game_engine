/*
 * participating_media.c
 * Volumetric media properties (scattering, absorption)
 *
 * Part of the Lighting subsystem
 * Advanced 3D Rendering Engine
 */

#include "lighting/volumetric/participating_media.h"
#include "include/math/vec3.h"
#include <stdint.h>
#include <stdbool.h>
#include <stddef.h>
#include <string.h>
#include <stdlib.h>
#include <include/math/math.h>

/* ============================================================================
 * TYPES
 * ============================================================================ */

typedef struct media_coefficients {
    vec3_t sigma_s; // Scattering coefficient
    vec3_t sigma_a; // Absorption coefficient
    vec3_t sigma_t; // Extinction coefficient (s + a)
    float g;        // Phase function anisotropy
} media_coefficients_t;

typedef struct participating_media_context {
    media_coefficients_t rayleigh;
    media_coefficients_t mie;
    media_coefficients_t ozone;
    bool initialized;
} participating_media_context_t;

static participating_media_context_t g_media_ctx = {0};

/* ============================================================================
 * PUBLIC API
 * ============================================================================ */

int participating_media_init(void) {
    if (g_media_ctx.initialized) return 0;
    
    // Default Earth Atmosphere values (approximate)
    // Rayleigh (Air molecules) - strong blue scattering
    g_media_ctx.rayleigh.sigma_s = vec3_set(5.802e-6f, 13.558e-6f, 33.100e-6f);
    g_media_ctx.rayleigh.sigma_a = vec3_set(0.0f, 0.0f, 0.0f);
    g_media_ctx.rayleigh.sigma_t = g_media_ctx.rayleigh.sigma_s;
    g_media_ctx.rayleigh.g = 0.0f; // Isotropic-ish (actually Rayleigh phase)

    // Mie (Aerosols) - white scattering, forward bias
    g_media_ctx.mie.sigma_s = vec3_set(3.996e-6f, 3.996e-6f, 3.996e-6f);
    g_media_ctx.mie.sigma_a = vec3_set(4.40e-6f, 4.40e-6f, 4.40e-6f);
    g_media_ctx.mie.sigma_t = vec3_add(g_media_ctx.mie.sigma_s, g_media_ctx.mie.sigma_a);
    g_media_ctx.mie.g = 0.8f; // Strong forward scattering

    // Ozone (Absorption only)
    g_media_ctx.ozone.sigma_s = vec3_set(0.0f, 0.0f, 0.0f);
    g_media_ctx.ozone.sigma_a = vec3_set(0.650e-6f, 1.881e-6f, 0.085e-6f);
    g_media_ctx.ozone.sigma_t = g_media_ctx.ozone.sigma_a;
    g_media_ctx.ozone.g = 0.0f;
    
    g_media_ctx.initialized = true;
    return 0;
}

void participating_media_shutdown(void) {
    g_media_ctx.initialized = false;
}

void participating_media_get_coefficients(
    vec3_t* out_sigma_s, 
    vec3_t* out_sigma_a, 
    vec3_t* out_sigma_t, 
    const vec3_t* wavelengths
) {
    if (!g_media_ctx.initialized) participating_media_init();
    
    // For standard rendering we use RGB, ignoring specific wavelengths logic for now
    // But we combine Rayleigh + Mie for lower atmosphere
    
    if (out_sigma_s) {
        *out_sigma_s = vec3_add(g_media_ctx.rayleigh.sigma_s, g_media_ctx.mie.sigma_s);
    }
    
    if (out_sigma_a) {
        vec3_t combined_a = vec3_add(g_media_ctx.rayleigh.sigma_a, g_media_ctx.mie.sigma_a);
        combined_a = vec3_add(combined_a, g_media_ctx.ozone.sigma_a);
        *out_sigma_a = combined_a;
    }
    
    if (out_sigma_t) {
        // Extinction = Scattering + Absorption
        vec3_t sigma_s, sigma_a;
        if (out_sigma_s) sigma_s = *out_sigma_s;
        else sigma_s = vec3_add(g_media_ctx.rayleigh.sigma_s, g_media_ctx.mie.sigma_s);
        
        if (out_sigma_a) sigma_a = *out_sigma_a;
        else {
             vec3_t combined_a = vec3_add(g_media_ctx.rayleigh.sigma_a, g_media_ctx.mie.sigma_a);
             sigma_a = vec3_add(combined_a, g_media_ctx.ozone.sigma_a);
        }
        
        *out_sigma_t = vec3_add(sigma_s, sigma_a);
    }
}

// Calculate transmittance over distance d
// T = exp(-sigma_t * d)
vec3_t participating_media_evaluate_transmittance(vec3_t sigma_t, float distance) {
   vec3_t t;
   t.x = expf(-sigma_t.x * distance);
   t.y = expf(-sigma_t.y * distance);
   t.z = expf(-sigma_t.z * distance);
   // t.w = 1.0f; // vec3 doesn't have w usually, handled by vec3_set if needed
   return t;
}
