/*
 * shader_variants.h
 * Shader Variant System for Forward Renderer
 *
 * Handles dynamic generation and selection of shader permutations based on material features.
 */

#ifndef RENDERING_FORWARD_SHADER_VARIANTS_H
#define RENDERING_FORWARD_SHADER_VARIANTS_H

#include <core/types.h>

/* ============================================================================
 * FEATURE FLAGS
 * ============================================================================ */

typedef enum {
    SHADER_FEATURE_NONE             = 0,
    
    // Texture Maps
    SHADER_FEATURE_ALBEDO_MAP       = 1 << 0,
    SHADER_FEATURE_NORMAL_MAP       = 1 << 1,
    SHADER_FEATURE_METALLIC_MAP     = 1 << 2,
    SHADER_FEATURE_ROUGHNESS_MAP    = 1 << 3,
    SHADER_FEATURE_AO_MAP           = 1 << 4,
    SHADER_FEATURE_EMISSIVE_MAP     = 1 << 5,
    
    // Geometry/Vertex Features
    SHADER_FEATURE_VERTEX_COLORS    = 1 << 6,
    SHADER_FEATURE_SKINNING         = 1 << 7,
    SHADER_FEATURE_INSTANCING       = 1 << 8,
    
    // Lighting/Material Models
    SHADER_FEATURE_TRANSPARENT      = 1 << 9,
    SHADER_FEATURE_GLASS            = 1 << 10,
    SHADER_FEATURE_WATER            = 1 << 11,
    SHADER_FEATURE_CLOTH            = 1 << 12,
    
    // Optimization/Debug
    SHADER_FEATURE_SHADOW_RECEIVER  = 1 << 13,
    SHADER_FEATURE_DEBUG_VIEW       = 1 << 14,

} ShaderFeatureFlags;

typedef u32 ShaderVariantKey;

/* ============================================================================
 * API
 * ============================================================================ */

/**
 * Initializes the shader variant system.
 */
void shader_variant_system_init(void);

/**
 * Shuts down the shader variant system and releases cached shaders.
 */
void shader_variant_system_shutdown(void);

/**
 * Generates a unique key based on requested features.
 */
ShaderVariantKey shader_variant_generate_key(ShaderFeatureFlags flags);

/**
 * Gets (or compiles/loads) a shader variant for the given key.
 * Returns a handle/pointer to the compiled shader pipeline state.
 */
void* shader_variant_get(ShaderVariantKey key);

/**
 * Pre-warms common shader variants to avoid runtime compilations.
 */
void shader_variant_prewarm_common(void);

#endif /* RENDERING_FORWARD_SHADER_VARIANTS_H */
