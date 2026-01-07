#include "../3d_rendering.h"
#include <math/math.h>

/* ==================== Brushed Metal Anisotropy ==================== */

// TODO: Move to a shared math header if available
static Vec3 vec3_normalize(Vec3 v) {
    float len = sqrtf(v.x * v.x + v.y * v.y + v.z * v.z);
    if (len > 0.0f) {
        return (Vec3){v.x / len, v.y / len, v.z / len};
    }
    return (Vec3){0, 0, 0};
}

static Vec3 vec3_cross(Vec3 a, Vec3 b) {
    return (Vec3){
        a.y * b.z - a.z * b.y,
        a.z * b.x - a.x * b.z,
        a.x * b.y - a.y * b.x
    };
}

static float vec3_dot(Vec3 a, Vec3 b) {
    return a.x * b.x + a.y * b.y + a.z * b.z;
}

typedef struct brushed_metal {
    float roughness_along;      // Along brush direction
    float roughness_across;     // Perpendicular
    Vec3 brush_direction;       // Tangent space
} brushed_metal_t;

// Placeholder using standard GGX for now as basis for anisotropic
// Real anisotropic GGX requires more complex distribution function
static float distribution_ggx_anisotropic(float NdotH, float HdotX, float HdotY, float ax, float ay) {
    float PI = 3.14159265359f;
    float mx = ax * ax;
    float my = ay * ay;
    
    float x_term = (HdotX * HdotX) / mx;
    float y_term = (HdotY * HdotY) / my;
    float z_term = NdotH * NdotH;
    
    float denom = x_term + y_term + z_term;
    return 1.0f / (PI * ax * ay * denom * denom);
}

// Simplified shadowing term
static float geometry_schlick_ggx(float NdotV, float roughness) {
    float r = (roughness + 1.0f);
    float k = (r*r) / 8.0f;

    float num   = NdotV;
    float denom = NdotV * (1.0f - k) + k;

    return num / denom;
}

static float geometry_smith(float NdotV, float NdotL, float roughness) {
    float ggx2  = geometry_schlick_ggx(NdotV, roughness);
    float ggx1  = geometry_schlick_ggx(NdotL, roughness);

    return ggx1 * ggx2;
}

static Vec3 fresnel_schlick(float cosTheta, Vec3 F0) {
    float one_minus_cos = 1.0f - cosTheta;
    float pow5 = one_minus_cos * one_minus_cos * one_minus_cos * one_minus_cos * one_minus_cos;
    
    return (Vec3){
        F0.x + (1.0f - F0.x) * pow5,
        F0.y + (1.0f - F0.y) * pow5,
        F0.z + (1.0f - F0.z) * pow5
    };
}

// IMPORTANT: Anisotropic shading function
Vec3 anisotropic_ggx(Vec3 N, Vec3 V, Vec3 L, Vec3 T, Vec3 B, float at, float ab) {
    Vec3 H = vec3_normalize((Vec3){V.x + L.x, V.y + L.y, V.z + L.z});
    
    float NdotL = fmaxf(vec3_dot(N, L), 0.0f);
    float NdotV = fmaxf(vec3_dot(N, V), 0.0f);
    
    if (NdotL <= 0.0f || NdotV <= 0.0f) {
        return (Vec3){0, 0, 0};
    }
    
    float NdotH = fmaxf(vec3_dot(N, H), 0.0f);
    float HdotT = vec3_dot(H, T);
    float HdotB = vec3_dot(H, B);
    float VdotH = fmaxf(vec3_dot(V, H), 0.0f);
    
    // Anisotropic NDF
    // Use average roughness for Smith shadowing approximation
    float avg_roughness = (at + ab) * 0.5f;
    
    float D = distribution_ggx_anisotropic(NdotH, HdotT, HdotB, at, ab);
    float G = geometry_smith(NdotV, NdotL, avg_roughness);
    
    // F0 assumed for generic metal (e.g. Iron/Steel)
    Vec3 F0 = {0.56f, 0.57f, 0.58f}; 
    Vec3 F = fresnel_schlick(VdotH, F0);
    
    Vec3 numerator = {
        D * G * F.x,
        D * G * F.y,
        D * G * F.z
    };
    
    float denominator = 4.0f * NdotV * NdotL;
    
    // Specular contribution
    Vec3 specular = {
        numerator.x / fmaxf(denominator, 0.001f),
        numerator.y / fmaxf(denominator, 0.001f),
        numerator.z / fmaxf(denominator, 0.001f)
    };
    
    // Metal has very low/no diffuse, so we mostly return specular
    // Add NdotL scaling
    return (Vec3){
        specular.x * NdotL,
        specular.y * NdotL,
        specular.z * NdotL
    };
}

Vec3 shade_brushed_metal(Vec3 N, Vec3 V, Vec3 L, brushed_metal_t* params) {
    if (!params) return (Vec3){0, 0, 0};

    // Build anisotropic TBN
    Vec3 T = vec3_normalize(params->brush_direction);
    // Ideally ensure T is orthogonal to N, re-orthogonalize
    float dotNT = vec3_dot(N, T);
    Vec3 corrected_T = {
        T.x - N.x * dotNT,
        T.y - N.y * dotNT,
        T.z - N.z * dotNT
    };
    T = vec3_normalize(corrected_T);
    
    Vec3 B = vec3_cross(N, T);

    // Anisotropic GGX
    float roughness_t = params->roughness_along;
    float roughness_b = params->roughness_across;

    return anisotropic_ggx(N, V, L, T, B, roughness_t, roughness_b);
}
