#ifndef SHADER_CACHE_H
#define SHADER_CACHE_H

#include "shader_compiler.h"

// Initialize cache
void shader_cache_init(void);

// Shutdown cache
void shader_cache_shutdown(void);

// Find a shader in the cache by name and constant key (simplified for now)
metal_shader_t* shader_cache_get(const char* name, uint64_t inputs_hash);

// Add a shader to the cache
void shader_cache_put(const char* name, uint64_t inputs_hash, metal_shader_t* shader);

// Helper hash function for variant switches
uint64_t shader_variant_hash(bool has_normal, bool has_emissive);

#endif // SHADER_CACHE_H
