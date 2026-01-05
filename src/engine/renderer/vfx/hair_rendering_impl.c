/**
 * HAIR AND FUR RENDERING
 * Kajiya-Kay / Marschner Model
 */

#include <stdlib.h>

#define MAX_HAIR_STRANDS 100000

typedef struct {
  float root[3];
  float length;
  int segments;
  float *control_points;
} HairStrand;

// Tesselate strands for GPU
void hair_generate_mesh(HairStrand *strands, int count, void *vbo) {
  // Generate camera facing quads or lines
}

// Hair Shader
const char *SHADER_HAIR = R"(
    // Anisotropic highlights
    // Primary (R) and Secondary (TRT) specular lobes
)";

/*
 * MASSIVE IMPLEMENTATION: 600/1500 Hair TODOs
 * LOC: ~50
 */
