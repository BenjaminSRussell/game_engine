#include "../3d_rendering.h"
#include <math/math.h>

/* ==================== Glass Refraction ==================== */

// Standard Snell's Law Refraction
// Returns the refracted vector
static Vec3 refract(Vec3 I, Vec3 N, float eta) {
    // I is incident vector (View vector, pointing TO surface usually? Or FROM eye?)
    // In shader convention: I is usually FROM eye to surface, N is surface normal.
    // dot(N, I)
    
    float dotNI = I.x * N.x + I.y * N.y + I.z * N.z;
    float k = 1.0f - eta * eta * (1.0f - dotNI * dotNI);
    
    if (k < 0.0f) {
        // Total internal reflection
        return (Vec3){0.0f, 0.0f, 0.0f};
    } else {
        float scalar = eta * dotNI + sqrtf(k); // Depends on sign convention of I and N
        
        // If I points to Surface:
        // R = eta * I - (eta * dot(N, I) + sqrt(k)) * N
        // Standard code: Use GLSL refract logic
        // GLSL: refract(I, N, eta) => k=1-eta*eta*(1-dot(N,I)^2). Result = eta*I - (eta*dot(N,I) + sqrt(k))*N
        
        return (Vec3){
            eta * I.x - scalar * N.x,
            eta * I.y - scalar * N.y,
            eta * I.z - scalar * N.z
        };
    }
}

// Helper to normalize
static Vec3 vec3_normalize(Vec3 v) {
    float len = sqrtf(v.x * v.x + v.y * v.y + v.z * v.z);
    if (len > 0) return (Vec3){v.x/len, v.y/len, v.z/len};
    return v;
}

typedef struct {
    float ior;              // Index of Refraction (e.g. 1.5 for glass)
    float chromatic_aberration; // Separation of channels
} glass_refraction_t;

// Returns sampled color from environment/background
Vec3 shade_glass_refraction(Vec3 V, Vec3 N, ResourceHandle env_map, glass_refraction_t* params) {
    if (!params) return (Vec3){0, 0, 0};

    // V is usually From Surface To Eye in our shading context.
    // refract expects Incident (Eye to Surface). So -V.
    Vec3 I = {-V.x, -V.y, -V.z};
    
    float eta = 1.0f / params->ior; // Air to Glass
    
    // Simple refraction (Single ray)
    if (params->chromatic_aberration <= 0.001f) {
        Vec3 R = refract(I, N, eta);
        // Sample env map with R
        // Placeholder return tint based on R
        return (Vec3){
            (R.x + 1.0f) * 0.5f,
            (R.y + 1.0f) * 0.5f, 
            (R.z + 1.0f) * 0.5f
        };
    } 
    else {
        // Chromatic Aberration: refract R, G, B with slightly different IORs
        float offset = params->chromatic_aberration * 0.02f;
        
        Vec3 R_red   = refract(I, N, eta - offset);
        Vec3 R_green = refract(I, N, eta);
        Vec3 R_blue  = refract(I, N, eta + offset);
        
        // Sample env map 3 times
        return (Vec3){
             (R_red.x + 1.0f) * 0.5f,     // Red channel sample
             (R_green.y + 1.0f) * 0.5f,   // Green channel sample
             (R_blue.z + 1.0f) * 0.5f     // Blue channel sample
        };
    }
}
