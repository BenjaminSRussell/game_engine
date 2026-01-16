#ifndef ULTIMATE_ENGINE_MESH_RENDERER_H
#define ULTIMATE_ENGINE_MESH_RENDERER_H

#include "../../Core/Public/core_types.h"
#include "../Public/Render.h"

#ifdef __cplusplus
extern "C" {
#endif

typedef struct {
  float *vertices;
  u32 vertex_count;
  u32 *indices;
  u32 index_count;
} MeshData;

typedef void *MeshHandle;

// Mesh Management
VF_API MeshHandle Mesh_Create(const MeshData *data);
VF_API void Mesh_Destroy(MeshHandle mesh);

// Rendering
VF_API void Mesh_Draw(MeshHandle mesh, vec3 position, vec3 rotation,
                      vec3 scale);

#ifdef __cplusplus
}
#endif

#endif // ULTIMATE_ENGINE_MESH_RENDERER_H
