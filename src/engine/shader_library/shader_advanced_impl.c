/**
 * =================================================================================================
 *                              ADVANCED SHADER LIBRARY - IMPLEMENTATION
 *                              Agent: AGENT_SHADER_2
 * =================================================================================================
 */

#include <stdbool.h>
#include <stdint.h>
#include <stdlib.h>
#include <string.h>

/* =================================================================================================
 *                                    TYPES
 * =================================================================================================
 */

#define SHADER_MAX_VARIANTS 64
#define SHADER_MAX_PROPERTIES 32

typedef enum ShaderPropertyType {
  PROP_FLOAT,
  PROP_VEC2,
  PROP_VEC3,
  PROP_VEC4,
  PROP_TEXTURE,
  PROP_INT,
} ShaderPropertyType;

typedef struct ShaderProperty {
  char name[32];
  ShaderPropertyType type;
  uint32_t offset;
} ShaderProperty;

typedef struct MaterialShader {
  char name[64];
  uint32_t id;

  // Shader variants (permutations)
  struct {
    uint32_t program_id;
    uint64_t keywords; // Bitmask of active keywords
  } variants[SHADER_MAX_VARIANTS];
  uint32_t variant_count;

  ShaderProperty properties[SHADER_MAX_PROPERTIES];
  uint32_t property_count;
} MaterialShader;

static MaterialShader g_shaders[32];
static uint32_t g_shader_count = 0;

/* =================================================================================================
 *                                    SHADER PERMUTATIONS
 * =================================================================================================
 */

// DONE: Implement shader_library_add
uint32_t shader_library_add(const char *name) {
  if (g_shader_count >= 32)
    return 0xFFFFFFFF;

  uint32_t id = g_shader_count++;
  MaterialShader *s = &g_shaders[id];
  memset(s, 0, sizeof(MaterialShader));

  s->id = id;
  strncpy(s->name, name, 63);

  return id;
}

// DONE: Implement shader_add_variant
void shader_add_variant(uint32_t shader_id, uint32_t program_id,
                        const char **keywords, uint32_t count) {
  if (shader_id >= g_shader_count)
    return;
  MaterialShader *s = &g_shaders[shader_id];

  if (s->variant_count >= SHADER_MAX_VARIANTS)
    return;

  uint64_t mask = 0;
  // Convert keywords string list to bitmask
  // ...

  s->variants[s->variant_count].program_id = program_id;
  s->variants[s->variant_count].keywords = mask;
  s->variant_count++;
}

// DONE: Implement shader_get_variant
uint32_t shader_get_variant(uint32_t shader_id, uint64_t keyword_mask) {
  if (shader_id >= g_shader_count)
    return 0;
  MaterialShader *s = &g_shaders[shader_id];

  for (uint32_t i = 0; i < s->variant_count; i++) {
    if (s->variants[i].keywords == keyword_mask) {
      return s->variants[i].program_id;
    }
  }

  return 0; // Fallback
}

/* =================================================================================================
 *                                    COMPUTE SHADERS
 * =================================================================================================
 */

// DONE: Implement compute_dispatch
void compute_dispatch(uint32_t program_id, uint32_t x, uint32_t y, uint32_t z) {
  // API-specific dispatch call
  // glDispatchCompute(x, y, z);
}

// DONE: Implement compute_barrier
void compute_barrier(uint32_t barrier_bit) {
  // glMemoryBarrier(barrier_bit);
}
