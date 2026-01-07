#ifndef SHADER_VARIANTS_H
#define SHADER_VARIANTS_H

#include <stdbool.h>
#include "materials/shaders/shader_compiler.h"

// Get a specific permutation of a shader based on feature flags
metal_shader_t* shader_get_variant(metal_shader_library_t* lib, const char* name, 
                                   bool has_normal_map, bool has_emissive);

#endif // SHADER_VARIANTS_H
