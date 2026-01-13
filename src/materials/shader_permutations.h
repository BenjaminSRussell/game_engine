// Shader Permutation System
// Automatic shader variant generation and caching

#ifndef SHADER_PERMUTATIONS_H
#define SHADER_PERMUTATIONS_H

#include "core/types.h"

#ifdef __cplusplus
extern "C" {
#endif

// Shader feature flags (up to 32 features)
typedef enum ShaderFeatureFlags {
    SHADER_FEATURE_NONE = 0,
    SHADER_FEATURE_NORMAL_MAP = BIT(0),
    SHADER_FEATURE_ROUGHNESS_MAP = BIT(1),
    SHADER_FEATURE_METALLIC_MAP = BIT(2),
    SHADER_FEATURE_EMISSIVE = BIT(3),
    SHADER_FEATURE_AO_MAP = BIT(4),
    SHADER_FEATURE_VERTEX_COLORS = BIT(5),
    SHADER_FEATURE_SKINNING = BIT(6),
    SHADER_FEATURE_INSTANCING = BIT(7),
    SHADER_FEATURE_ALPHA_TEST = BIT(8),
    SHADER_FEATURE_ALPHA_BLEND = BIT(9),
    SHADER_FEATURE_TWO_SIDED = BIT(10),
    SHADER_FEATURE_SHADOW_RECEIVE = BIT(11),
    SHADER_FEATURE_SHADOW_CAST = BIT(12),
    SHADER_FEATURE_WIND_ANIMATION = BIT(13),
} ShaderFeatureFlags;

// Quality level affects shader complexity
typedef enum ShaderQualityLevel {
    SHADER_QUALITY_LOW,     // Mobile, minimal features
    SHADER_QUALITY_MEDIUM,  // Desktop, balanced
    SHADER_QUALITY_HIGH,    // High-end, all features
    SHADER_QUALITY_ULTRA    // Cinematic, maximum quality
} ShaderQualityLevel;

// Permutation key for variant lookup
typedef struct ShaderPermutation {
    u32 features;              // Feature flags bitmask
    ShaderQualityLevel quality;
    u32 hash;                   // Pre-computed hash for fast lookup
} ShaderPermutation;

// Shader variant (compiled for specific permutation)
typedef struct ShaderVariant {
    ShaderPermutation permutation;
    u32 program_id;             // OpenGL program ID
    bool is_compiled;
    
    // Cached uniform locations
    struct {
        i32 model;
        i32 view;
        i32 projection;
        i32 normal_matrix;
        i32 base_color;
        i32 metallic;
        i32 roughness;
        i32 ao;
        // Texture samplers
        i32 base_color_map;
        i32 normal_map;
        i32 roughness_map;
        i32 metallic_map;
        i32 ao_map;
        i32 emissive_map;
    } uniforms;
} ShaderVariant;

// Shader variant cache
typedef struct ShaderVariantCache {
    ShaderVariant *variants;
    u32 variant_count;
    u32 variant_capacity;
    char base_shader_path[256];
} ShaderVariantCache;

// === API ===

// Create permutation key
ShaderPermutation shader_permutation_create(u32 features, ShaderQualityLevel quality);

// Compute hash for permutation
u32 shader_permutation_hash(const ShaderPermutation *perm);

// Create variant cache for a base shader
ShaderVariantCache *shader_variant_cache_create(const char *base_shader_path);

// Destroy variant cache
void shader_variant_cache_destroy(ShaderVariantCache *cache);

// Get or compile variant for permutation
ShaderVariant *shader_variant_get_or_compile(ShaderVariantCache *cache,
                                              ShaderPermutation permutation);

// Generate shader source with defines for permutation
char *shader_generate_source(const char *base_source,
                              ShaderPermutation permutation,
                              bool is_vertex);

// Helper: Convert features to #define strings
void shader_permutation_to_defines(ShaderPermutation perm, char **out_defines, u32 *count);

#ifdef __cplusplus
}
#endif

#endif // SHADER_PERMUTATIONS_H
