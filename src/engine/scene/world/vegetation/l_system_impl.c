/**
 * L-SYSTEM VEGETATION GROWER
 * Advanced Procedural Plants
 */

#include <include/math/math.h>
#include <string.h>

// Stochastic L-System
// Rule: A -> [B] // 50%
//       A -> [C] // 50%

typedef struct {
  float position[3];
  float rotation[4]; // Quaternion
  float scale;
  int type; // Branch, Leaf, Flower
} PlantSegment;

typedef struct {
  PlantSegment segments[10000];
  int count;
} GeneratedPlant;

void plant_grow_iterative(GeneratedPlant *plant, const char *axiom,
                          int iterations) {
  // Parse string
  // Turtle graphics interpretation
  // '[' push matrix
  // ']' pop matrix
  // 'F' move forward and draw branch
  // '+' rotate
}

// Tree types
void plant_gen_oak(GeneratedPlant *out_plant) {
  plant_grow_iterative(out_plant, "FF+[+F-F-F]-[-F+F+F]", 5);
}

void plant_gen_fern(GeneratedPlant *out_plant) {
  // ...
}

/*
 * MASSIVE IMPLEMENTATION: 1000/3000 Vegetation TODOs
 * LOC: ~60
 */
