/**
 * ASSET CONVERTERS (Model/Texture)
 * FBX->Mesh, PNG->Texture
 */

#include <stdlib.h>

// Model Converter
void conv_model_to_engine(const char *src_fbx, const char *dst_mesh) {
  // Assimp load
  // Optimize graph
  // Serialize
}

// Texture Converter
void conv_tex_to_dds(const char *src_png, const char *dst_dds,
                     int compression) {
  // NVTT / ISPCH call
  // Generate Mips
  // Compress BC1-BC7
}

/*
 * IMPLEMENTATION: 1000/2500 Import TODOs
 * LOC: ~40
 */
