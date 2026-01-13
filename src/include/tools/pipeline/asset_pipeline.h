// Pipeline/Asset Import Pipeline
#ifndef ASSET_PIPELINE_H
#define ASSET_PIPELINE_H

#include "include/common.h"

// Types of assets we handle
typedef enum {
  ASSET_TYPE_UNKNOWN,
  ASSET_TYPE_TEXTURE,
  ASSET_TYPE_MESH,
  ASSET_TYPE_SKELETAL_MESH,
  ASSET_TYPE_ANIMATION,
  ASSET_TYPE_AUDIO,
  ASSET_TYPE_FONT,
  ASSET_TYPE_SHADER
} AssetType;

typedef struct {
  bool generate_mips;
  bool srgb;
  bool compress; // BC/ASTC compression
  u32 max_resolution;
} TextureImportSettings;

typedef struct {
  bool generate_tangents;
  bool combine_meshes;
  bool import_materials;
  bool import_textures;
  f32 scale;
  bool flip_uvs;
} MeshImportSettings;

typedef struct {
  char source_path[256];
  char dest_path[256]; // Interior engine path

  AssetType type;

  union {
    TextureImportSettings texture;
    MeshImportSettings mesh;
  } settings;

  bool force_reimport;

} AssetImportTask;

#ifdef __cplusplus
extern "C" {
#endif

// The "Asset Processor" - usually runs on a background thread
void asset_pipeline_init(void);
void asset_pipeline_shutdown(void);

// Queue a file for import
void asset_import_file(const char *filepath, const char *target_dir);

// Synchronous import (blocking)
bool asset_import_texture_sync(const char *filepath,
                               TextureImportSettings settings);
bool asset_import_mesh_sync(const char *filepath, MeshImportSettings settings);

// Hot reload detection
void asset_pipeline_scan_for_changes(void);

#ifdef __cplusplus
}
#endif

#endif // ASSET_PIPELINE_H
