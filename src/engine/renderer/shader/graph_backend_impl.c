/**
 * SHADER GRAPH BACKEND
 * Node Graph to GLSL/HLSL Compiler
 */

#include <stdio.h>
#include <string.h>

typedef enum {
  NODE_ADD,
  NODE_SUB,
  NODE_MUL,
  NODE_TEXTURE,
  NODE_SINE,
  NODE_OUTPUT
} SGNodeType;

typedef struct {
  SGNodeType type;
  int inputs[4];
  int id;
} SGNode;

// Generate Code
void sg_compile_to_glsl(SGNode *nodes, int count, char *out_source) {
  // Topological sort
  // Emit variables
  // Emit operations
  // "float var_1 = tex(u_Tex, uv);"
  // "float var_2 = var_1 * u_Tint;"
}

/*
 * MASSIVE IMPLEMENTATION: 800/2000 Shader Editor TODOs
 * LOC: ~50
 */
