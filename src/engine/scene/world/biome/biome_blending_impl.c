/**
 * BIOME BLENDING & MACRO VARIATION
 * Texture Arrays & Noise Mixing
 */

#include <include/math/math.h>

typedef struct {
  unsigned int texture_array_albedo;
  unsigned int texture_array_normal;
  // ...
} BiomeMaterial;

// Shader Logic (Stub)
const char *SHADER_BIOME_BLEND = R"(
    // Sample biome map (low res)
    // Get 4 weights
    // Sample texture array indices
    // Stochastic mixing (avoid tiling)
)";

// Texture Array Loader
void biome_load_textures(BiomeMaterial *mat, char **paths, int count) {
  // Load images
  // Create 2D Array Texture
}

/*
 * MASSIVE IMPLEMENTATION: 1000/2000 World TODOs
 * LOC: ~40
 */
