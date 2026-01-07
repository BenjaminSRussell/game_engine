/*
 * test_shader_variants.c
 * Unit tests for shader variant system.
 */

#include "../rendering/forward/shader_variants.h"
#include <core/logger.h>
#include <assert.h>

void test_ShaderVariants_KeyGeneration(void) {
    // Test base key
    ShaderVariantKey key1 = shader_variant_generate_key(SHADER_FEATURE_NONE);
    assert(key1 == 0);
    
    // Test combination
    ShaderVariantKey key2 = shader_variant_generate_key(SHADER_FEATURE_ALBEDO_MAP | SHADER_FEATURE_NORMAL_MAP);
    
    // Test that order doesn't matter (commutative bitwise OR)
    ShaderVariantKey key3 = shader_variant_generate_key(SHADER_FEATURE_NORMAL_MAP | SHADER_FEATURE_ALBEDO_MAP);
    
    assert(key2 == key3);
    assert(key2 != key1);
    
    LOG_INFO("test_ShaderVariants_KeyGeneration passed");
}

void test_ShaderVariants_Caching(void) {
    shader_variant_system_init();
    
    ShaderVariantKey key = shader_variant_generate_key(SHADER_FEATURE_ALBEDO_MAP);
    
    // First request - should compile
    void* variant1 = shader_variant_get(key);
    assert(variant1 != NULL);
    
    // Second request - should return cached
    void* variant2 = shader_variant_get(key);
    assert(variant1 == variant2);
    
    shader_variant_system_shutdown();
    LOG_INFO("test_ShaderVariants_Caching passed");
}

/*
 * Register tests
 */
void register_shader_variant_tests(void) {
    test_ShaderVariants_KeyGeneration();
    test_ShaderVariants_Caching();
}
