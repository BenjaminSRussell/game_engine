// Shader Permutation System - Implementation
#include "rendering/materials/shader_permutations.h"
#include "core/logger.h"
#include "rendering/shader.h"
#include <stdlib.h>
#include <string.h>
#include <stdio.h>

#ifdef __APPLE__
#define GL_SILENCE_DEPRECATION
#include <OpenGL/gl3.h>
#else
#include <GL/gl.h>
#endif

// FNV-1a hash function
static u32 fnv1a_hash(const void *data, size_t size) {
    const u8 *bytes = (const u8 *)data;
    u32 hash = 2166136261u;
    for (size_t i = 0; i < size; i++) {
        hash ^= bytes[i];
        hash *= 16777619u;
    }
    return hash;
}

ShaderPermutation shader_permutation_create(u32 features, ShaderQualityLevel quality) {
    ShaderPermutation perm = {0};
    perm.features = features;
    perm.quality = quality;
    perm.hash = shader_permutation_hash(&perm);
    return perm;
}

u32 shader_permutation_hash(const ShaderPermutation *perm) {
    if (!perm) return 0;
    
    // Hash features + quality
    struct {
        u32 features;
        u32 quality;
    } key = {perm->features, perm->quality};
    
    return fnv1a_hash(&key, sizeof(key));
}

ShaderVariantCache *shader_variant_cache_create(const char *base_shader_path) {
    if (!base_shader_path) return NULL;
    
    ShaderVariantCache *cache = (ShaderVariantCache *)calloc(1, sizeof(ShaderVariantCache));
    if (!cache) {
        LOG_ERROR("Failed to allocate shader variant cache");
        return NULL;
    }
    
    strncpy(cache->base_shader_path, base_shader_path, sizeof(cache->base_shader_path) - 1);
    cache->variant_capacity = 16;  // Start small
    cache->variants = (ShaderVariant *)calloc(cache->variant_capacity, sizeof(ShaderVariant));
    cache->variant_count = 0;
    
    LOG_INFO("Created shader variant cache for %s", base_shader_path);
    return cache;
}

void shader_variant_cache_destroy(ShaderVariantCache *cache) {
    if (!cache) return;
    
    // Delete all compiled variants
    for (u32 i = 0; i < cache->variant_count; i++) {
        if (cache->variants[i].is_compiled) {
            glDeleteProgram(cache->variants[i].program_id);
        }
    }
    
    free(cache->variants);
    free(cache);
}

static ShaderVariant *find_variant_in_cache(ShaderVariantCache *cache, u32 hash) {
    for (u32 i = 0; i < cache->variant_count; i++) {
        if (cache->variants[i].permutation.hash == hash) {
            return &cache->variants[i];
        }
    }
    return NULL;
}

static bool compile_shader_variant(ShaderVariant *variant, 
                                   const char *vertex_src,
                                   const char *fragment_src) {
    // Compile vertex shader
    u32 vertex_id = glCreateShader(GL_VERTEX_SHADER);
    glShaderSource(vertex_id, 1, &vertex_src, NULL);
    glCompileShader(vertex_id);
    
    GLint success;
    glGetShaderiv(vertex_id, GL_COMPILE_STATUS, &success);
    if (!success) {
        char info_log[512];
        glGetShaderInfoLog(vertex_id, 512, NULL, info_log);
        LOG_ERROR("Vertex shader compilation failed: %s", info_log);
        glDeleteShader(vertex_id);
        return false;
    }
    
    // Compile fragment shader
    u32 fragment_id = glCreateShader(GL_FRAGMENT_SHADER);
    glShaderSource(fragment_id, 1, &fragment_src, NULL);
    glCompileShader(fragment_id);
    
    glGetShaderiv(fragment_id, GL_COMPILE_STATUS, &success);
    if (!success) {
        char info_log[512];
        glGetShaderInfoLog(fragment_id, 512, NULL, info_log);
        LOG_ERROR("Fragment shader compilation failed: %s", info_log);
        glDeleteShader(vertex_id);
        glDeleteShader(fragment_id);
        return false;
    }
    
    // Link program
    u32 program_id = glCreateProgram();
    glAttachShader(program_id, vertex_id);
    glAttachShader(program_id, fragment_id);
    glLinkProgram(program_id);
    
    glGetProgramiv(program_id, GL_LINK_STATUS, &success);
    if (!success) {
        char info_log[512];
        glGetProgramInfoLog(program_id, 512, NULL, info_log);
        LOG_ERROR("Shader linking failed: %s", info_log);
        glDeleteShader(vertex_id);
        glDeleteShader(fragment_id);
        glDeleteProgram(program_id);
        return false;
    }
    
    // Clean up intermediate shaders
    glDeleteShader(vertex_id);
    glDeleteShader(fragment_id);
    
    // Cache uniform locations
    variant->program_id = program_id;
    variant->is_compiled = true;
    variant->uniforms.model = glGetUniformLocation(program_id, "u_model");
    variant->uniforms.view = glGetUniformLocation(program_id, "u_view");
    variant->uniforms.projection = glGetUniformLocation(program_id, "u_projection");
    variant->uniforms.normal_matrix = glGetUniformLocation(program_id, "u_normal_matrix");
    variant->uniforms.base_color = glGetUniformLocation(program_id, "u_base_color");
    variant->uniforms.metallic = glGetUniformLocation(program_id, "u_metallic");
    variant->uniforms.roughness = glGetUniformLocation(program_id, "u_roughness");
    variant->uniforms.ao = glGetUniformLocation(program_id, "u_ao");
    
    // Texture samplers
    variant->uniforms.base_color_map = glGetUniformLocation(program_id, "u_base_color_map");
    variant->uniforms.normal_map = glGetUniformLocation(program_id, "u_normal_map");
    variant->uniforms.roughness_map = glGetUniformLocation(program_id, "u_roughness_map");
    variant->uniforms.metallic_map = glGetUniformLocation(program_id, "u_metallic_map");
    variant->uniforms.ao_map = glGetUniformLocation(program_id, "u_ao_map");
    variant->uniforms.emissive_map = glGetUniformLocation(program_id, "u_emissive_map");
    
    return true;
}

ShaderVariant *shader_variant_get_or_compile(ShaderVariantCache *cache,
                                              ShaderPermutation permutation) {
    if (!cache) return NULL;
    
    // Check if variant already exists
    u32 hash = shader_permutation_hash(&permutation);
    ShaderVariant *existing = find_variant_in_cache(cache, hash);
    if (existing) {
        return existing;
    }
    
    // Load base shader source
    // Read vertex shader
    char vertex_path[512];
    snprintf(vertex_path, sizeof(vertex_path), "%s.vert", cache->base_shader_path);
    
    FILE *vf = fopen(vertex_path, "r");
    if (!vf) {
        LOG_ERROR("Failed to open vertex shader: %s", vertex_path);
        return NULL;
    }
    
    fseek(vf, 0, SEEK_END);
    long vsize = ftell(vf);
    fseek(vf, 0, SEEK_SET);
    
    char *base_vertex_src = (char *)malloc(vsize + 1);
    fread(base_vertex_src, 1, vsize, vf);
    base_vertex_src[vsize] = '\0';
    fclose(vf);
    
    // Read fragment shader
    char fragment_path[512];
    snprintf(fragment_path, sizeof(fragment_path), "%s.frag", cache->base_shader_path);
    
    FILE *ff = fopen(fragment_path, "r");
    if (!ff) {
        LOG_ERROR("Failed to open fragment shader: %s", fragment_path);
        free(base_vertex_src);
        return NULL;
    }
    
    fseek(ff, 0, SEEK_END);
    long fsize = ftell(ff);
    fseek(ff, 0, SEEK_SET);
    
    char *base_fragment_src = (char *)malloc(fsize + 1);
    fread(base_fragment_src, 1, fsize, ff);
    base_fragment_src[fsize] = '\0';
    fclose(ff);
    
    // Generate source with permutation defines
    char *vertex_src = shader_generate_source(base_vertex_src, permutation, true);
    char *fragment_src = shader_generate_source(base_fragment_src, permutation, false);
    
    free(base_vertex_src);
    free(base_fragment_src);
    
    // Grow cache if needed
    if (cache->variant_count >= cache->variant_capacity) {
        cache->variant_capacity *= 2;
        cache->variants = (ShaderVariant *)realloc(cache->variants,
                                                    cache->variant_capacity * sizeof(ShaderVariant));
    }
    
    // Create new variant
    ShaderVariant *variant = &cache->variants[cache->variant_count];
    memset(variant, 0, sizeof(ShaderVariant));
    variant->permutation = permutation;
    
    // Compile
    if (!compile_shader_variant(variant, vertex_src, fragment_src)) {
        free(vertex_src);
        free(fragment_src);
        return NULL;
    }
    
    free(vertex_src);
    free(fragment_src);
    
    cache->variant_count++;
    
    LOG_INFO("Compiled shader variant (hash: %u, features: %u, quality: %d)",
             hash, permutation.features, permutation.quality);
    
    return variant;
}

char *shader_generate_source(const char *base_source,
                              ShaderPermutation permutation,
                              bool is_vertex) {
    if (!base_source) return NULL;
    
    // Build defines based on permutation
    char defines[2048] = {0};
    size_t offset = 0;
    
    // Version and common defines
    offset += snprintf(defines + offset, sizeof(defines) - offset, "#version 330 core\n");
    
    // Feature flags
    if (permutation.features & SHADER_FEATURE_NORMAL_MAP) {
        offset += snprintf(defines + offset, sizeof(defines) - offset, "#define FEATURE_NORMAL_MAP 1\n");
    }
    if (permutation.features & SHADER_FEATURE_ROUGHNESS_MAP) {
        offset += snprintf(defines + offset, sizeof(defines) - offset, "#define FEATURE_ROUGHNESS_MAP 1\n");
    }
    if (permutation.features & SHADER_FEATURE_METALLIC_MAP) {
        offset += snprintf(defines + offset, sizeof(defines) - offset, "#define FEATURE_METALLIC_MAP 1\n");
    }
    if (permutation.features & SHADER_FEATURE_EMISSIVE) {
        offset += snprintf(defines + offset, sizeof(defines) - offset, "#define FEATURE_EMISSIVE 1\n");
    }
    if (permutation.features & SHADER_FEATURE_AO_MAP) {
        offset += snprintf(defines + offset, sizeof(defines) - offset, "#define FEATURE_AO_MAP 1\n");
    }
    if (permutation.features & SHADER_FEATURE_VERTEX_COLORS) {
        offset += snprintf(defines + offset, sizeof(defines) - offset, "#define FEATURE_VERTEX_COLORS 1\n");
    }
    if (permutation.features & SHADER_FEATURE_SKINNING) {
        offset += snprintf(defines + offset, sizeof(defines) - offset, "#define FEATURE_SKINNING 1\n");
    }
    if (permutation.features & SHADER_FEATURE_INSTANCING) {
        offset += snprintf(defines + offset, sizeof(defines) - offset, "#define FEATURE_INSTANCING 1\n");
    }
    if (permutation.features & SHADER_FEATURE_ALPHA_TEST) {
        offset += snprintf(defines + offset, sizeof(defines) - offset, "#define FEATURE_ALPHA_TEST 1\n");
    }
    if (permutation.features & SHADER_FEATURE_ALPHA_BLEND) {
        offset += snprintf(defines + offset, sizeof(defines) - offset, "#define FEATURE_ALPHA_BLEND 1\n");
    }
    
    // Quality level
    offset += snprintf(defines + offset, sizeof(defines) - offset, "#define QUALITY_LEVEL %d\n", permutation.quality);
    
    // Combine defines + source
    size_t total_len = strlen(defines) + strlen(base_source) + 1;
    char *final_source = (char *)malloc(total_len);
    snprintf(final_source, total_len, "%s\n%s", defines, base_source);
    
    return final_source;
}

void shader_permutation_to_defines(ShaderPermutation perm, char **out_defines, u32 *count) {
    // Helper function to get list of active defines
    // For debugging/logging
    static char *defines_list[32];
    u32 idx = 0;
    
    if (perm.features & SHADER_FEATURE_NORMAL_MAP) defines_list[idx++] = "FEATURE_NORMAL_MAP";
    if (perm.features & SHADER_FEATURE_ROUGHNESS_MAP) defines_list[idx++] = "FEATURE_ROUGHNESS_MAP";
    if (perm.features & SHADER_FEATURE_METALLIC_MAP) defines_list[idx++] = "FEATURE_METALLIC_MAP";
    // ... etc
    
    *out_defines = defines_list[0];
    *count = idx;
}
