#include "character/skin/skin_rendering.h"
#include <stdlib.h>
#include <string.h>
#include <math.h>

/**
 * @file skin_detail.c
 * @brief Implementation of skin pore detail and wrinkles
 */

// Microfiber fuzz layer parameters
typedef struct {
    float fuzz_density;          // Density of fine hair fibers
    float fuzz_length;           // Average length of fibers
    vec3_t fuzz_color;           // Color of fine hair
    float fuzz_roughness;        // Roughness of fiber surface
    float fuzz_strength;          // Overall fuzz contribution
} microfiber_fuzz_t;

// Dual-lobe specular parameters
typedef struct {
    float primary_roughness;     // Primary specular roughness (skin surface)
    float secondary_roughness;   // Secondary specular roughness (sweat/oil)
    float primary_strength;      // Primary specular strength
    float secondary_strength;    // Secondary specular strength
    vec3_t secondary_color;      // Secondary specular tint (for sweat/oil)
} dual_lobe_specular_t;

// Sweat/oily skin parameters
typedef struct {
    float sweat_level;           // Amount of sweat on skin (0-1)
    float oil_level;             // Oiliness of skin (0-1)
    float wetness_factor;        // Overall wetness affecting specular
    vec3_t sweat_tint;           // Color tint for sweat (slightly blue/clear)
    float evaporation_rate;      // How quickly sweat evaporates
} sweat_oil_params_t;

// Global state for skin detail rendering
static struct {
    microfiber_fuzz_t fuzz_params;
    dual_lobe_specular_t specular_params;
    sweat_oil_params_t sweat_params;
    bool initialized;
    uint32_t shader_skin_gbuffer;
} g_skin_detail = {0};

// Microfiber fuzz layer implementation
static void apply_microfiber_fuzz(skin_params_t* params) {
    if (!params) return;
    
    // Generate fuzz pattern based on skin surface normals
    // This creates fine hair-like detail that catches light
    
    // Fuzz density varies across body parts (higher on arms, lower on face)
    float density_variation = sin(params->scatter_radius * 0.5f) * 0.3f + 0.7f;
    g_skin_detail.fuzz_params.fuzz_density = density_variation * 0.8f;
    
    // Fuzz length is very short for skin (0.1-0.3mm)
    g_skin_detail.fuzz_params.fuzz_length = 0.2f;
    
    // Fuzz color is slightly darker than skin tone
    g_skin_detail.fuzz_params.fuzz_color.x = params->scatter_color.x * 0.7f;
    g_skin_detail.fuzz_params.fuzz_color.y = params->scatter_color.y * 0.6f;
    g_skin_detail.fuzz_params.fuzz_color.z = params->scatter_color.z * 0.5f;
    
    // Fuzz roughness is high (diffuse scattering)
    g_skin_detail.fuzz_params.fuzz_roughness = 0.9f;
    
    // Overall fuzz strength is subtle
    g_skin_detail.fuzz_params.fuzz_strength = 0.15f;
}

void render_gbuffer_skin(void* mesh, skin_params_t* params) {
    if (!g_skin_detail.initialized || !mesh || !params) return;
    
    // 1. Bind skin G-Buffer shader
    bind_shader(g_skin_detail.shader_skin_gbuffer);
    
    // 2. Set material parameters
    set_uniform_vec3("u_ScatterColor", params->scatter_color);
    set_uniform_float("u_DetailStrength", params->detail_normal_strength);
    
    // 3. Apply microfiber fuzz layer
    apply_microfiber_fuzz(params);
    set_uniform_float("u_FuzzDensity", g_skin_detail.fuzz_params.fuzz_density);
    set_uniform_float("u_FuzzLength", g_skin_detail.fuzz_params.fuzz_length);
    set_uniform_vec3("u_FuzzColor", g_skin_detail.fuzz_params.fuzz_color);
    set_uniform_float("u_FuzzRoughness", g_skin_detail.fuzz_params.fuzz_roughness);
    set_uniform_float("u_FuzzStrength", g_skin_detail.fuzz_params.fuzz_strength);
    
    // 4. Set dual-lobe specular parameters
    set_uniform_float("u_PrimaryRoughness", g_skin_detail.specular_params.primary_roughness);
    set_uniform_float("u_SecondaryRoughness", g_skin_detail.specular_params.secondary_roughness);
    set_uniform_float("u_PrimarySpecularStrength", g_skin_detail.specular_params.primary_strength);
    set_uniform_float("u_SecondarySpecularStrength", g_skin_detail.specular_params.secondary_strength);
    set_uniform_vec3("u_SecondarySpecularColor", g_skin_detail.specular_params.secondary_color);
    
    // 5. Set sweat/oil parameters
    set_uniform_float("u_SweatLevel", g_skin_detail.sweat_params.sweat_level);
    set_uniform_float("u_OilLevel", g_skin_detail.sweat_params.oil_level);
    set_uniform_float("u_WetnessFactor", g_skin_detail.sweat_params.wetness_factor);
    set_uniform_vec3("u_SweatTint", g_skin_detail.sweat_params.sweat_tint);
    
    // 6. Bind texture variants
    bind_texture(0, params->detail_normal);
    bind_texture(1, params->wrinkle_map);
    
    // 7. Draw mesh
    draw_mesh(mesh);
}

// Dual-lobe specular implementation for skin
static void setup_dual_lobe_specular(void) {
    // Primary lobe: skin surface specular (wider, softer)
    g_skin_detail.specular_params.primary_roughness = 0.8f;
    g_skin_detail.specular_params.primary_strength = 0.25f;
    
    // Secondary lobe: sweat/oil specular (tighter, brighter)
    g_skin_detail.specular_params.secondary_roughness = 0.2f;
    g_skin_detail.specular_params.secondary_strength = 0.15f;
    
    // Secondary specular has slight blue/clear tint for sweat
    g_skin_detail.specular_params.secondary_color.x = 0.9f;
    g_skin_detail.specular_params.secondary_color.y = 0.95f;
    g_skin_detail.specular_params.secondary_color.z = 1.0f;
}

// Sweat/oily skin parameters implementation
static void setup_sweat_oil_params(void) {
    // Default sweat level (can be modified based on character state)
    g_skin_detail.sweat_params.sweat_level = 0.3f;
    
    // Natural skin oiliness
    g_skin_detail.sweat_params.oil_level = 0.4f;
    
    // Overall wetness affects specular intensity
    g_skin_detail.sweat_params.wetness_factor = 0.5f;
    
    // Sweat has slight blue/clear tint
    g_skin_detail.sweat_params.sweat_tint.x = 0.95f;
    g_skin_detail.sweat_params.sweat_tint.y = 0.98f;
    g_skin_detail.sweat_params.sweat_tint.z = 1.0f;
    
    // Evaporation rate for dynamic sweat simulation
    g_skin_detail.sweat_params.evaporation_rate = 0.1f;
}

// Initialize skin detail rendering system
bool skin_detail_init(void) {
    if (g_skin_detail.initialized) return true;
    
    // Load skin G-Buffer shader
    g_skin_detail.shader_skin_gbuffer = load_shader("shaders/skin/skin_gbuffer.glsl");
    if (!g_skin_detail.shader_skin_gbuffer) {
        return false;
    }
    
    // Initialize microfiber fuzz parameters
    memset(&g_skin_detail.fuzz_params, 0, sizeof(microfiber_fuzz_t));
    
    // Setup dual-lobe specular
    setup_dual_lobe_specular();
    
    // Setup sweat/oil parameters
    setup_sweat_oil_params();
    
    g_skin_detail.initialized = true;
    return true;
}

// Shutdown skin detail rendering system
void skin_detail_shutdown(void) {
    if (!g_skin_detail.initialized) return;
    
    // Release shader
    unload_shader(g_skin_detail.shader_skin_gbuffer);
    
    // Clear parameters
    memset(&g_skin_detail.fuzz_params, 0, sizeof(microfiber_fuzz_t));
    memset(&g_skin_detail.specular_params, 0, sizeof(dual_lobe_specular_t));
    memset(&g_skin_detail.sweat_params, 0, sizeof(sweat_oil_params_t));
    
    g_skin_detail.initialized = false;
}
