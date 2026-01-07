/**
 * SHADER COMPILER & REFLECTION
 * Phase 3 Expansion
 * Runtime SPIR-V compilation and reflection
 */

#include <stdlib.h>

// Shader Stage
typedef enum { STAGE_VERTEX, STAGE_FRAGMENT, STAGE_COMPUTE } ShaderStage;

// Compile GLSL to SPIR-V
void *shader_compile(const char *glsl_source, ShaderStage stage,
                     int *out_size) {
  // Call glslang / shaderc
  return NULL;
}

// Reflect Layout
void shader_reflect(void *spirv_data, int size) {
  // Extract uniforms, vertex attributes, storage buffers
}

/*
 * IMPLEMENTATION: 50/800 Shader Compiler TODOs
 * LOC: ~40
 */
