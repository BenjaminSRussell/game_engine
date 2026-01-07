/*
 * directional_lightmaps.c
 * Radiosity Normal Map (RNM) / Directional Lightmap support
 *
 * Part of the Lighting subsystem
 * Advanced 3D Rendering Engine
 */

#include "lighting/lightmaps/directional_lightmaps.h"
#include "include/math/vec3.h"

/*
 * Standard Lightmap: Stores RGB irradiance.
 * Directional Lightmap: Stores irradiance for 3 basis directions (or L1 SH).
 * This allows normal mapping to interact with baked lighting.
 */

typedef struct lightmap_ctx {
    void* texture_basis0;
    void* texture_basis1;
    void* texture_basis2;
    // or use a single texture array
} lightmap_ctx_t;

// Standard HL2 basis vectors
static const vec3_t basis[3] = {
    {0.81649658f, 0.0f, 0.57735027f},
    {-0.40824829f, 0.70710678f, 0.57735027f},
    {-0.40824829f, -0.70710678f, 0.57735027f}
};

void directional_lightmaps_init(void) {
    // Load default textures
}

void directional_lightmaps_bind(void) {
    // Bind the 3 textures to the shader slots
}
