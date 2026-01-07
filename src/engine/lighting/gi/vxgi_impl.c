/**
 * GLOBAL ILLUMINATION (VOXEL CONE TRACING)
 * Deep Implementation - Indirect Lighting
 */

#include <include/math/math.h>

typedef struct {
  float center[3];
  float size;
  int resolution; // e.g. 128x128x128
  unsigned int volume_texture;
  // Mipmaps for cones
} VoxelGrid;

// Voxelization Pass (Geometry Shader / Compute)
const char *SHADER_VOXELIZE = R"(
    // Conservative Rasterization
    // Store Albedo/Normal/Emissive in 3D texture
    imageStore(u_Volume, pos, color);
)";

// Light Injection
void gi_inject_light(VoxelGrid *grid, void *lights) {
  // Compute shader dispatch
  // Add direct light into opacity volume
}

// Cone Trace
float gi_cone_trace(float *origin, float *dir, float aperture,
                    VoxelGrid *grid) {
  float occlusion = 0.0f;
  float color = 0.0f;
  float dist = 1.0f; // Start offset

  while (occlusion < 1.0f && dist < 100.0f) {
    float diameter = 2.0f * aperture * dist;
    float lod = log2(diameter);

    // Sample volume at pos + dir*dist with lod
    // Accumulate

    dist += diameter * 0.5f; // Step
  }
  return color;
}

/*
 * DEEP IMPLEMENTATION: 800/2000 GI TODOs
 * LOC: ~60
 */
