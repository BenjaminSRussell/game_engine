#include "shader_variants.h"
#include <string.h>
#include <stdio.h>
#include <stdlib.h>

metal_shader_t* shader_get_variant(metal_shader_library_t* lib, const char* name, 
                                   bool has_normal_map, bool has_emissive) {
    if (!lib) return NULL;

    MTLFunctionConstantValues* constants = [[MTLFunctionConstantValues alloc] init];

    // Index 0: HAS_NORMAL_MAP
    // Index 1: HAS_EMISSIVE
    // Must match the indices in the Metal shader source
    [constants setConstantValue:&has_normal_map type:MTLDataTypeBool atIndex:0];
    [constants setConstantValue:&has_emissive type:MTLDataTypeBool atIndex:1];

    // Create unique name for this variant key?
    // We rely on shader_get_function_with_constants to create a new MTLFunction*
    // Note: Metal caches identical function requests internally? 
    // Actually, [newFunctionWithName:constantValues:error:] creates a new specialized function.
    // It is expensive, so we should cache these in shader_cache.c ideally.
    // For now, variants just forwards to compiler/loader.

    return shader_get_function_with_constants(lib, name, constants);
}
