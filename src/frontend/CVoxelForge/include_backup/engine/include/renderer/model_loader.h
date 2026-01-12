#ifndef MODEL_LOADER_H
#define MODEL_LOADER_H

#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>

// Forward declarations
typedef struct Mesh Mesh;
typedef struct Material Material;
typedef struct Skeleton Skeleton;
typedef struct Animation Animation;

// Supported model formats
typedef enum {
  MODEL_FORMAT_OBJ,
  MODEL_FORMAT_FBX,
  MODEL_FORMAT_GLTF,
  MODEL_FORMAT_GLB,
  MODEL_FORMAT_STL,
  MODEL_FORMAT_PLY,
  MODEL_FORMAT_DAE,  // Collada
  MODEL_FORMAT_MMDL, // Custom optimized format
  MODEL_FORMAT_AUTO  // Auto-detect from extension
} ModelFormat;

// Loading flags
typedef enum {
  MODEL_LOAD_DEFAULT = 0,
  MODEL_LOAD_FLIP_UVS = 1 << 0,
  MODEL_LOAD_FLIP_NORMALS = 1 << 1,
  MODEL_LOAD_GENERATE_NORMALS = 1 << 2,
  MODEL_LOAD_GENERATE_TANGENTS = 1 << 3,
  MODEL_LOAD_TRIANGULATE = 1 << 4,
  MODEL_LOAD_OPTIMIZE = 1 << 5,
  MODEL_LOAD_GENERATE_LODS = 1 << 6,
  MODEL_LOAD_ASYNC = 1 << 7,
  MODEL_LOAD_CACHE = 1 << 8
} ModelLoadFlags;

// Loading status
typedef enum {
  MODEL_STATUS_LOADING,
  MODEL_STATUS_READY,
  MODEL_STATUS_ERROR
} ModelStatus;

// Model data structure
typedef struct Model {
  char *name;
  char *path;

  // Geometry
  Mesh **meshes;
  uint32_t mesh_count;

  // Materials
  Material **materials;
  uint32_t material_count;

  // Animation data (optional)
  Skeleton *skeleton;
  Animation **animations;
  uint32_t animation_count;

  // Bounding volume
  float bounds_min[3];
  float bounds_max[3];
  float bounds_center[3];
  float bounds_radius;

  // Metadata
  uint32_t vertex_count;
  uint32_t triangle_count;
  ModelStatus status;

  // Internal
  void *internal_data;
} Model;

// Progress callback for async loading
typedef void (*ModelLoadProgressCallback)(float progress, void *user_data);

// Loading options
typedef struct {
  ModelFormat format;
  uint32_t flags;
  float scale;
  ModelLoadProgressCallback progress_callback;
  void *user_data;
} ModelLoadOptions;

// Public API
Model *model_load(const char *path, const ModelLoadOptions *options);
Model *model_load_from_memory(const void *data, size_t size,
                              const ModelLoadOptions *options);
void model_free(Model *model);

// Async loading
typedef struct ModelLoadHandle ModelLoadHandle;
ModelLoadHandle *model_load_async(const char *path,
                                  const ModelLoadOptions *options);
ModelStatus model_load_get_status(ModelLoadHandle *handle);
Model *model_load_get_result(ModelLoadHandle *handle);
void model_load_cancel(ModelLoadHandle *handle);

// Format detection
ModelFormat model_detect_format(const char *path);
const char *model_format_get_extension(ModelFormat format);
const char *model_format_get_name(ModelFormat format);

// Model utilities
void model_calculate_bounds(Model *model);
void model_center_at_origin(Model *model);
void model_scale_to_unit(Model *model);
void model_merge_meshes(Model *model);

// Default options
ModelLoadOptions model_get_default_options(void);

#endif // MODEL_LOADER_H
