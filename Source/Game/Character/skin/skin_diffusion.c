#include "character/skin/skin_rendering.h"
#include <include/math/math_all.h>
#include <stdlib.h>
#include <string.h>
#include <math.h>

/**
 * @file skin_diffusion.c
 * @brief Implementation of skin subsurface scattering diffusion
 * 
 * Implements pre-integrated SSS and separable screen-space blur.
 */

// Diffusion profile structure for pre-integrated SSS
typedef struct {
    float scatter_radius;        // Scattering radius in mm
    vec3_t scatter_color;        // RGB scatter color
    float scale;                 // Profile scale factor
    uint16_t* profile_data;      // Pre-computed profile data
    int profile_size;            // Size of profile data
} diffusion_profile_t;

// Multi-layer scattering support
typedef struct {
    diffusion_profile_t epidermis;  // Outer skin layer
    diffusion_profile_t dermis;     // Middle skin layer
    diffusion_profile_t subdermis;  // Deep tissue layer
    float layer_weights[3];          // Contribution weights for each layer
} multi_layer_scattering_t;

// Global state
static struct {
    diffusion_profile_t default_profile;
    multi_layer_scattering_t multi_layer;
    bool initialized;
    uint32_t shader_sss_blur_h;
    uint32_t shader_sss_blur_v;
    uint32_t shader_skin_gbuffer;
} g_skin_rendering = {0};

// CPU-side diffusion profile generation
static void generate_diffusion_profile(diffusion_profile_t* profile, float radius, vec3_t color) {
    if (!profile) return;
    
    profile->scatter_radius = radius;
    profile->scatter_color = color;
    profile->scale = 1.0f / radius;
    profile->profile_size = 256; // Standard profile size
    
    profile->profile_data = (uint16_t*)malloc(profile->profile_size * sizeof(uint16_t));
    if (!profile->profile_data) return;
    
    // Generate diffusion profile using dipole approximation
    for (int i = 0; i < profile->profile_size; i++) {
        float distance = (float)i / (float)(profile->profile_size - 1) * radius * 3.0f;
        
        // Normalized distance
        float r = distance * profile->scale;
        
        // Dipole diffusion approximation
        float r_d = sqrt(r * r + 1.0f);
        float diffusion = (1.0f / (4.0f * M_PI)) * (
            (1.0f / r_d) + 
            (1.0f / r_d * r_d * r_d)
        );
        
        // Apply material properties and normalize
        float profile_value = diffusion * exp(-r * 2.0f);
        
        // Convert to 16-bit normalized value
        profile->profile_data[i] = (uint16_t)(profile_value * 65535.0f);
    }
}

// Multi-layer scattering profile generation
static void generate_multi_layer_profile(multi_layer_scattering_t* mls) {
    if (!mls) return;
    
    // Epidermis (outer layer) - thin, high frequency scattering
    vec3_t epidermis_color = {0.8f, 0.3f, 0.2f}; // Reddish
    generate_diffusion_profile(&mls->epidermis, 0.5f, epidermis_color);
    
    // Dermis (middle layer) - medium scattering
    vec3_t dermis_color = {0.9f, 0.4f, 0.3f}; // More reddish
    generate_diffusion_profile(&mls->dermis, 1.2f, dermis_color);
    
    // Subdermis (deep tissue) - broad scattering
    vec3_t subdermis_color = {0.7f, 0.3f, 0.2f}; // Deep red
    generate_diffusion_profile(&mls->subdermis, 2.5f, subdermis_color);
    
    // Layer contribution weights (sum to 1.0)
    mls->layer_weights[0] = 0.3f; // Epidermis
    mls->layer_weights[1] = 0.5f; // Dermis
    mls->layer_weights[2] = 0.2f; // Subdermis
}

void sss_blur_horizontal(texture_handle_t skin_target, texture_handle_t profile) {
    if (!g_skin_rendering.initialized) return;
    
    // 1. Bind horizontal blur shader
    bind_shader(g_skin_rendering.shader_sss_blur_h);
    
    // 2. Set uniforms
    set_uniform_texture("u_SkinTarget", skin_target);
    set_uniform_texture("u_Profile", profile);
    set_uniform_float("u_BlurRadius", 1.0f); // Should come from skin_params
    
    // 3. Dispatch compute or draw full-screen quad
    dispatch_blur(skin_target.width, skin_target.height);
}

void sss_blur_vertical(texture_handle_t skin_target, texture_handle_t profile) {
    if (!g_skin_rendering.initialized) return;
    
    // 1. Bind vertical blur shader
    bind_shader(g_skin_rendering.shader_sss_blur_v);
    
    // 2. Set uniforms
    set_uniform_texture("u_SkinTarget", skin_target);
    set_uniform_texture("u_Profile", profile);
    
    // 3. Dispatch
    dispatch_blur(skin_target.width, skin_target.height);
}

// Beckmann distribution for specular reflection
static float beckmann_distribution(float ndoth, float roughness) {
    if (ndoth <= 0.0f) return 0.0f;
    
    float alpha = roughness * roughness;
    float alpha2 = alpha * alpha;
    float ndoth2 = ndoth * ndoth;
    
    float denom = ndoth2 * (alpha2 - 1.0f) + 1.0f;
    float beckmann = (alpha2) / (M_PI * denom * denom);
    
    return beckmann;
}

// Kelemen-Szirmay-Kalos specular correction
static float ksk_specular_correction(float ndoth, float vdoth, float roughness) {
    if (vdoth <= 0.0f) return 0.0f;
    
    // KSZ correction factor for energy conservation
    float alpha = roughness * roughness;
    float correction = (vdoth) / (alpha * ndoth + (1.0f - alpha) * vdoth);
    
    return correction;
}

void composite_skin(texture_handle_t final_target, texture_handle_t skin_target) {
    if (!g_skin_rendering.initialized) return;
    
    // Bind compositing shader
    bind_shader(g_skin_rendering.shader_skin_gbuffer);
    
    // Set uniforms for compositing
    set_uniform_texture("u_SkinTarget", skin_target);
    set_uniform_vec3("u_ScatterColor", g_skin_rendering.default_profile.scatter_color);
    set_uniform_float("u_ScatterRadius", g_skin_rendering.default_profile.scatter_radius);
    
    // Apply Beckmann distribution and KSZ correction for specular
    set_uniform_float("u_Roughness", 0.8f); // Skin roughness parameter
    set_uniform_float("u_SpecularStrength", 0.3f); // Skin specular strength
    
    // Draw full-screen quad for compositing
    draw_fullscreen_quad();
}

bool skin_rendering_init(void) {
    if (g_skin_rendering.initialized) return true;
    
    // Load SSS shaders
    g_skin_rendering.shader_sss_blur_h = load_shader("shaders/skin/sss_blur_h.glsl");
    g_skin_rendering.shader_sss_blur_v = load_shader("shaders/skin/sss_blur_v.glsl");
    g_skin_rendering.shader_skin_gbuffer = load_shader("shaders/skin/skin_gbuffer.glsl");
    
    if (!g_skin_rendering.shader_sss_blur_h || 
        !g_skin_rendering.shader_sss_blur_v || 
        !g_skin_rendering.shader_skin_gbuffer) {
        return false;
    }
    
    // Create default diffusion profiles
    vec3_t default_color = {0.8f, 0.4f, 0.3f}; // Skin-like reddish color
    generate_diffusion_profile(&g_skin_rendering.default_profile, 1.0f, default_color);
    
    // Generate multi-layer scattering profiles
    generate_multi_layer_profile(&g_skin_rendering.multi_layer);
    
    g_skin_rendering.initialized = true;
    return true;
}

void skin_rendering_shutdown(void) {
    if (!g_skin_rendering.initialized) return;
    
    // Release diffusion profile data
    if (g_skin_rendering.default_profile.profile_data) {
        free(g_skin_rendering.default_profile.profile_data);
        g_skin_rendering.default_profile.profile_data = NULL;
    }
    
    // Release multi-layer profile data
    if (g_skin_rendering.multi_layer.epidermis.profile_data) {
        free(g_skin_rendering.multi_layer.epidermis.profile_data);
        g_skin_rendering.multi_layer.epidermis.profile_data = NULL;
    }
    if (g_skin_rendering.multi_layer.dermis.profile_data) {
        free(g_skin_rendering.multi_layer.dermis.profile_data);
        g_skin_rendering.multi_layer.dermis.profile_data = NULL;
    }
    if (g_skin_rendering.multi_layer.subdermis.profile_data) {
        free(g_skin_rendering.multi_layer.subdermis.profile_data);
        g_skin_rendering.multi_layer.subdermis.profile_data = NULL;
    }
    
    // Release shaders
    unload_shader(g_skin_rendering.shader_sss_blur_h);
    unload_shader(g_skin_rendering.shader_sss_blur_v);
    unload_shader(g_skin_rendering.shader_skin_gbuffer);
    
    g_skin_rendering.initialized = false;
}
