#ifndef ASSET_IMPORTERS_H
#define ASSET_IMPORTERS_H

#include "../common.h"
#include <renderer/renderer.h>
#include <renderer/renderer_api.h>

// Forward declarations
typedef struct AssetManager AssetManager;

// Model Import Options
typedef struct {
  bool flip_uvs;
  bool calc_tangents;
  bool combine_meshes;
  f32 scale;
} ModelImportOptions;

// Material Asset Structure
typedef struct {
  char name[128];
  MaterialProperties properties;
  char albedo_path[256];
  char normal_path[256];
  char metallic_roughness_path[256];
  char occlusion_path[256];
  char emissive_path[256];
} ImportedMaterial;

// Model Asset Structure (Intermediate CPU representation)
typedef struct {
  MeshDesc *meshes;
  u32 mesh_count;

  ImportedMaterial *materials;
  u32 material_count;

  // LOD Support (Phase 9)
  // Each mesh can have multiple LOD levels
  // lod_meshes[mesh_index][lod_level]
  MeshDesc **lod_meshes; // Array of arrays for LOD levels
  u32 *lod_counts;       // Number of LOD levels per mesh
  bool has_lods;         // Whether LOD data is present

  // Hierarchy could go here
  // Animations could go here

  void *raw_data; // To free all data at once
} ImportedModel;

// GLTF Importer
// Loads a GLTF/GLB file from disk and returns an ImportedModel
// Returns NULL on failure
ImportedModel *asset_importer_load_gltf(const char *path,
                                        const ModelImportOptions *options);

// Load GLTF/GLB from memory
ImportedModel *
asset_importer_load_gltf_from_memory(const void *data, size_t size,
                                     const char *base_path,
                                     const ModelImportOptions *options);

// OBJ Importer
// Loads a Wavefront OBJ file from disk and returns an ImportedModel
// Returns NULL on failure
ImportedModel *asset_importer_load_obj(const char *path,
                                       const ModelImportOptions *options);

// Load OBJ from memory
ImportedModel *
asset_importer_load_obj_from_memory(const void *data, size_t size,
                                    const ModelImportOptions *options);

// Texture Asset Structure
typedef struct {
  void *pixels;
  u32 width;
  u32 height;
  u32 channels; // 4 for RGBA, 3 for RGB
  u32 size_bytes;
  bool is_hdr;

  // Mipmap Support (Phase 9)
  void **mipmap_data; // Array of mipmap level data
  u32 *mipmap_sizes;  // Size of each mipmap level in bytes
  u32 mipmap_count;   // Number of mipmap levels (including base level)
} ImportedTexture;

// Load texture from disk (PNG, JPG, TGA, BMP)
// Returns NULL on failure
ImportedTexture *asset_importer_load_texture(const char *path, bool flip_y);

// Load texture from memory
ImportedTexture *asset_importer_load_texture_from_memory(const void *data,
                                                         size_t size,
                                                         bool flip_y);

// Free imported texture data
void asset_importer_free_texture(ImportedTexture *texture);

// Audio Asset Structure
typedef struct {
  void *data;      // PCM data (interleaved)
  u64 frame_count; // Total frames
  u32 channels;    // e.g. 2 for stereo
  u32 sample_rate; // e.g. 44100
  u32 size_bytes;  // Total allocation size
} ImportedAudio;

// Load audio from disk (WAV, MP3, FLAC)
// Returns NULL on failure
ImportedAudio *asset_importer_load_audio(const char *path);

// Load audio from memory
ImportedAudio *asset_importer_load_audio_from_memory(const void *data,
                                                     size_t size);

// Free imported audio data
void asset_importer_free_audio(ImportedAudio *audio);

// Free the imported model data
void asset_importer_free_model(ImportedModel *model);

#endif // ASSET_IMPORTERS_H
