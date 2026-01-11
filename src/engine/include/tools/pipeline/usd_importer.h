// tools/pipeline/usd_importer.h
// Pixar USD import pipeline
#ifndef USD_IMPORTER_H
#define USD_IMPORTER_H

#include "include/common.h"
#include "include/math/vec3.h"
#include <Metal/Metal.h>

// USD stage (entire scene graph)
typedef struct USDStage USDStage;

// USD prim (node in scene graph)
typedef struct {
  char path[256];
  char type[64]; // "Mesh", "Camera", "Light", etc.

  Mat4 transform;

  // Mesh data (if type == "Mesh")
  Vec3 *vertices;
  Vec3 *normals;
  Vec2 *uvs;
  u32 *indices;
  u32 vertex_count;
  u32 index_count;

  // Material reference
  char material_path[256];

  // Animation data
  bool is_animated;
  f32 *keyframe_times;
  Mat4 *keyframe_transforms;
  u32 keyframe_count;

  // Children
  struct USDPrim **children;
  u32 child_count;

} USDPrim;

typedef struct {
  USDStage *stage;
  USDPrim *root_prim;

  // Import options
  f32 scale_factor;
  bool import_cameras;
  bool import_lights;
  bool import_animations;
  bool import_materials;

  // Conversion
  id<MTLDevice> device;

} USDImporter;

#ifdef __cplusplus
extern "C" {
#endif

// USD import
USDImporter *usd_importer_create(id<MTLDevice> device);
void usd_importer_destroy(USDImporter *importer);

// Load USD file
bool usd_load_file(USDImporter *importer, const char *file_path);

// Query stage
USDPrim *usd_find_prim(USDImporter *importer, const char *prim_path);
void usd_traverse_prims(USDImporter *importer,
                        void (*callback)(USDPrim *prim, void *user_data),
                        void *user_data);

// Extract data
void usd_extract_meshes(USDImporter *importer,
                        void (*on_mesh)(const char *name, Vec3 *verts,
                                        u32 *indices, u32 vert_count,
                                        u32 idx_count));

void usd_extract_animation(USDImporter *importer, const char *prim_path,
                           f32 **out_times, Mat4 **out_transforms,
                           u32 *out_count);

// Export (save to USD)
bool usd_export_scene(const char *output_path, void *scene_data,
                      bool include_animations);

#ifdef __cplusplus
}
#endif

#endif // USD_IMPORTER_H
