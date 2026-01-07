/*
 * shader_variants.c
 * Shader Variant System Implementation
 */

#include "shader_variants.h"
#include <core/logger.h>
#include <stdlib.h>
// #include <containers/hash_map.h> // Assuming a hash map implementation exists

/* ============================================================================
 * INTERNAL STATE
 * ============================================================================ */

// Simple placeholder for a shader cache since we don't have the full hash map API handy in this context
// In production, this would use a proper hash map (Key -> ShaderHandle)
#define MAX_CACHED_VARIANTS 256

typedef struct {
    ShaderVariantKey key;
    void* shader_pipeline;
    bool in_use;
} CachedVariant;

static struct {
    CachedVariant cache[MAX_CACHED_VARIANTS];
    u32 count;
    bool initialized;
} g_variant_system;

/* ============================================================================
 * IMPLEMENTATION
 * ============================================================================ */

void shader_variant_system_init(void) {
    if (g_variant_system.initialized) return;
    
    // Zero out cache
    for (int i = 0; i < MAX_CACHED_VARIANTS; i++) {
        g_variant_system.cache[i].in_use = false;
        g_variant_system.cache[i].shader_pipeline = NULL;
    }
    g_variant_system.count = 0;
    g_variant_system.initialized = true;
    
    LOG_INFO("ShaderVariantSystem: Initialized.");
}

void shader_variant_system_shutdown(void) {
    if (!g_variant_system.initialized) return;
    
    // Destroy all cached shaders
    for (int i = 0; i < MAX_CACHED_VARIANTS; i++) {
        if (g_variant_system.cache[i].in_use) {
            // TODO: Destroy shader pipeline resource
            // shader_destroy(g_variant_system.cache[i].shader_pipeline);
        }
    }
    
    g_variant_system.initialized = false;
    LOG_INFO("ShaderVariantSystem: Shutdown.");
}

ShaderVariantKey shader_variant_generate_key(ShaderFeatureFlags flags) {
    // Direct mapping for now, but could include deeper logic (e.g., mutually exclusive flags)
    return (ShaderVariantKey)flags;
}

static void* compile_variant(ShaderVariantKey key) {
    // This function would interface with the shader compiler/loader
    // It constructs a define string (e.g., "#define FEATURE_NORMAL_MAP 1") based on the key
    
    // Mock implementation
    // LOG_DEBUG("Compiling shader variant Key: %u", key);
    
    // In a real implementation:
    // 1. Generate "#define ..." string
    // 2. Load GLSL source
    // 3. Prepend defines
    // 4. Compile vertex/fragment shaders
    // 5. Link program
    
    // Return a dummy non-null pointer to simulate success
    return (void*)((uintptr_t)key + 1); 
}

void* shader_variant_get(ShaderVariantKey key) {
    if (!g_variant_system.initialized) return NULL;
    
    // 1. Check Cache (Linear search for this simple placeholder)
    for (int i = 0; i < MAX_CACHED_VARIANTS; i++) {
        if (g_variant_system.cache[i].in_use && g_variant_system.cache[i].key == key) {
            return g_variant_system.cache[i].shader_pipeline;
        }
    }
    
    // 2. Compile if not found
    if (g_variant_system.count >= MAX_CACHED_VARIANTS) {
        LOG_ERROR("ShaderVariantSystem: Cache full! Cannot compile new variant.");
        return NULL;
    }
    
    void* pipeline = compile_variant(key);
    if (pipeline) {
        // Find empty slot
        for (int i = 0; i < MAX_CACHED_VARIANTS; i++) {
            if (!g_variant_system.cache[i].in_use) {
                g_variant_system.cache[i].in_use = true;
                g_variant_system.cache[i].key = key;
                g_variant_system.cache[i].shader_pipeline = pipeline;
                g_variant_system.count++;
                return pipeline;
            }
        }
    }
    
    return NULL;
}

void shader_variant_prewarm_common(void) {
    // Pre-compile common combinations to avoid stutter
    ShaderFeatureFlags common_flags[] = {
        SHADER_FEATURE_NONE,
        SHADER_FEATURE_ALBEDO_MAP | SHADER_FEATURE_NORMAL_MAP,
        SHADER_FEATURE_ALBEDO_MAP | SHADER_FEATURE_NORMAL_MAP | SHADER_FEATURE_METALLIC_MAP | SHADER_FEATURE_ROUGHNESS_MAP,
        SHADER_FEATURE_TRANSPARENT,
        SHADER_FEATURE_GLASS
    };
    
    for (int i = 0; i < sizeof(common_flags) / sizeof(ShaderFeatureFlags); i++) {
        shader_variant_get(shader_variant_generate_key(common_flags[i]));
    }
    
    LOG_INFO("ShaderVariantSystem: Pre-warmed %d common variants.", sizeof(common_flags) / sizeof(ShaderFeatureFlags));
}
