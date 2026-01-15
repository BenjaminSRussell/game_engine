#pragma once

#include <stdbool.h>
#include <stdint.h>

// Forward declarations
typedef struct NoiseGenerator NoiseGenerator;
typedef struct ProceduralMesh ProceduralMesh;

// Noise generation
typedef enum {
  NOISE_TYPE_PERLIN,
  NOISE_TYPE_SIMPLEX,
  NOISE_TYPE_WORLEY,
  NOISE_TYPE_VALUE
} NoiseType;

NoiseGenerator *noise_generator_create(NoiseType type, uint32_t seed);
float noise_get_2d(NoiseGenerator *generator, float x, float y);
float noise_get_3d(NoiseGenerator *generator, float x, float y, float z);
void noise_generator_destroy(NoiseGenerator *generator);

// Procedural mesh generation
ProceduralMesh *procedural_mesh_create_sphere(float radius, uint32_t segments);
ProceduralMesh *procedural_mesh_create_cube(float size);
ProceduralMesh *procedural_mesh_create_plane(float width, float height);
void procedural_mesh_destroy(ProceduralMesh *mesh);

// Level generation
typedef struct {
  uint32_t width;
  uint32_t height;
  uint32_t seed;
  float scale;
} LevelGenConfig;

bool level_generate(const LevelGenConfig *config, void *out_data);
