#include "materials/shader_permutations.h"
#include "include/core/memory.h"
#include "include/core/logger.h"
#include <stdio.h>
#include <string.h>

// Helper to compute hash (FNV-1a or similar simple hash)
static u32 compute_hash(const void* data, size_t size) {
    u32 hash = 2166136261u;
    const u8* ptr = (const u8*)data;
    for (size_t i = 0; i < size; ++i) {
        hash ^= ptr[i];
        hash *= 16777619u;
    }
    return hash;
}

ShaderPermutation shader_permutation_create(u32 features, ShaderQualityLevel quality) {
    ShaderPermutation perm;
    perm.features = features;
    perm.quality = quality;
    // Hash includes features and quality
    struct { u32 f; u32 q; } data = { features, (u32)quality };
    perm.hash = compute_hash(&data, sizeof(data));
    return perm;
}

u32 shader_permutation_hash(const ShaderPermutation *perm) {
    if (!perm) return 0;
    return perm->hash;
}

ShaderVariantCache *shader_variant_cache_create(const char *base_shader_path) {
    ShaderVariantCache *cache = (ShaderVariantCache*)MALLOC(sizeof(ShaderVariantCache));
    if (!cache) return NULL;

    strncpy(cache->base_shader_path, base_shader_path, sizeof(cache->base_shader_path) - 1);
    cache->base_shader_path[sizeof(cache->base_shader_path) - 1] = '\0';

    cache->variants = NULL;
    cache->variant_count = 0;
    cache->variant_capacity = 0;

    return cache;
}

void shader_variant_cache_destroy(ShaderVariantCache *cache) {
    if (!cache) return;

    if (cache->variants) {
        FREE(cache->variants);
    }
    FREE(cache);
}

// Generate shader source with defines for permutation
char *shader_generate_source(const char *base_source,
                              ShaderPermutation permutation,
                              bool is_vertex) {
    if (!base_source) return NULL;

    // Simple implementation: prepend defines to the source
    // In a real implementation, we would also process #includes etc.

    char *defines = NULL;
    u32 defines_count = 0;
    shader_permutation_to_defines(permutation, &defines, &defines_count);

    size_t base_len = strlen(base_source);
    size_t defines_len = defines ? strlen(defines) : 0;
    // defines usually needs newlines

    char *final_source = (char*)MALLOC(base_len + defines_len + 64); // + safety
    final_source[0] = '\0';

    if (defines) {
        strcat(final_source, defines);
        strcat(final_source, "\n");
        FREE(defines);
    }

    strcat(final_source, base_source);

    return final_source;
}

void shader_permutation_to_defines(ShaderPermutation perm, char **out_defines, u32 *count) {
    // Estimate size
    size_t buffer_size = 1024;
    char *buffer = (char*)MALLOC(buffer_size);
    buffer[0] = '\0';

    u32 c = 0;
    if (perm.features & SHADER_FEATURE_NORMAL_MAP) { strcat(buffer, "#define USE_NORMAL_MAP 1\n"); c++; }
    if (perm.features & SHADER_FEATURE_ROUGHNESS_MAP) { strcat(buffer, "#define USE_ROUGHNESS_MAP 1\n"); c++; }
    if (perm.features & SHADER_FEATURE_METALLIC_MAP) { strcat(buffer, "#define USE_METALLIC_MAP 1\n"); c++; }
    if (perm.features & SHADER_FEATURE_EMISSIVE) { strcat(buffer, "#define USE_EMISSIVE 1\n"); c++; }
    if (perm.features & SHADER_FEATURE_AO_MAP) { strcat(buffer, "#define USE_AO_MAP 1\n"); c++; }
    if (perm.features & SHADER_FEATURE_VERTEX_COLORS) { strcat(buffer, "#define USE_VERTEX_COLORS 1\n"); c++; }
    if (perm.features & SHADER_FEATURE_SKINNING) { strcat(buffer, "#define USE_SKINNING 1\n"); c++; }
    if (perm.features & SHADER_FEATURE_INSTANCING) { strcat(buffer, "#define USE_INSTANCING 1\n"); c++; }
    if (perm.features & SHADER_FEATURE_ALPHA_TEST) { strcat(buffer, "#define USE_ALPHA_TEST 1\n"); c++; }
    if (perm.features & SHADER_FEATURE_ALPHA_BLEND) { strcat(buffer, "#define USE_ALPHA_BLEND 1\n"); c++; }
    if (perm.features & SHADER_FEATURE_TWO_SIDED) { strcat(buffer, "#define USE_TWO_SIDED 1\n"); c++; }
    if (perm.features & SHADER_FEATURE_SHADOW_RECEIVE) { strcat(buffer, "#define USE_SHADOW_RECEIVE 1\n"); c++; }
    if (perm.features & SHADER_FEATURE_SHADOW_CAST) { strcat(buffer, "#define USE_SHADOW_CAST 1\n"); c++; }
    if (perm.features & SHADER_FEATURE_WIND_ANIMATION) { strcat(buffer, "#define USE_WIND_ANIMATION 1\n"); c++; }

    switch (perm.quality) {
        case SHADER_QUALITY_LOW: strcat(buffer, "#define QUALITY_LOW 1\n"); break;
        case SHADER_QUALITY_MEDIUM: strcat(buffer, "#define QUALITY_MEDIUM 1\n"); break;
        case SHADER_QUALITY_HIGH: strcat(buffer, "#define QUALITY_HIGH 1\n"); break;
        case SHADER_QUALITY_ULTRA: strcat(buffer, "#define QUALITY_ULTRA 1\n"); break;
    }

    *out_defines = buffer;
    *count = c;
}

ShaderVariant *shader_variant_get_or_compile(ShaderVariantCache *cache, ShaderPermutation permutation) {
    if (!cache) return NULL;

    // Linear search for now (could be optimized with hash map if needed)
    for (u32 i = 0; i < cache->variant_count; ++i) {
        if (cache->variants[i].permutation.hash == permutation.hash &&
            cache->variants[i].permutation.features == permutation.features &&
            cache->variants[i].permutation.quality == permutation.quality) {
            return &cache->variants[i];
        }
    }

    // Not found, create new
    if (cache->variant_count >= cache->variant_capacity) {
        u32 new_cap = cache->variant_capacity == 0 ? 4 : cache->variant_capacity * 2;
        cache->variants = (ShaderVariant*)REALLOC(cache->variants, new_cap * sizeof(ShaderVariant));
        cache->variant_capacity = new_cap;
    }

    ShaderVariant *variant = &cache->variants[cache->variant_count++];
    variant->permutation = permutation;
    variant->is_compiled = false;
    variant->program_id = 0; // Invalid ID

    // Initialize uniform locations to -1
    variant->uniforms.model = -1;
    variant->uniforms.view = -1;
    variant->uniforms.projection = -1;
    variant->uniforms.normal_matrix = -1;
    variant->uniforms.base_color = -1;
    variant->uniforms.metallic = -1;
    variant->uniforms.roughness = -1;
    variant->uniforms.ao = -1;
    variant->uniforms.base_color_map = -1;
    variant->uniforms.normal_map = -1;
    variant->uniforms.roughness_map = -1;
    variant->uniforms.metallic_map = -1;
    variant->uniforms.ao_map = -1;
    variant->uniforms.emissive_map = -1;

    // Stub compilation for now (simulating success)
    // In a real implementation, we would call the shader compiler here
    // passing shader_generate_source() output.

    LOG_INFO("Compiling shader variant for %s (Features: %x)", cache->base_shader_path, permutation.features);

    variant->program_id = 999; // Dummy valid ID
    variant->is_compiled = true;

    return variant;
}
