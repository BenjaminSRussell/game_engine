#ifndef VOXEL_RENDERER_H
#define VOXEL_RENDERER_H

#include "backend/metal/mtl_buffer.h"
#include "backend/metal/mtl_device.h"
#include "backend/metal/mtl_encoder.h"
#include <stdbool.h>
#include <stdint.h>

// ============================================================================
// Voxel Renderer Types
// ============================================================================

typedef struct VoxelRenderer VoxelRenderer;

typedef struct {
  float x, y, z;     // Position
  float nx, ny, nz;  // Normal
  float u, v;        // Texture coordinates
  uint8_t ao;        // Ambient occlusion
  uint8_t light;     // Light level
  uint16_t block_id; // Block type
} VoxelVertex;

typedef struct {
  metal_buffer_t *vertex_buffer;
  metal_buffer_t *index_buffer;
  uint32_t vertex_count;
  uint32_t index_count;
  bool uploaded;
} VoxelMesh;

typedef struct {
  uint32_t draw_calls;
  uint32_t triangles_drawn;
  uint32_t chunks_rendered;
  float frame_time_ms;
} VoxelRenderStats;

// ============================================================================
// Voxel Renderer API
// ============================================================================

VoxelRenderer *voxel_renderer_create(metal_device_t *device);
void voxel_renderer_destroy(VoxelRenderer *renderer);

VoxelMesh *voxel_mesh_generate(VoxelRenderer *renderer, const uint8_t *blocks,
                               uint32_t width, uint32_t height, uint32_t depth);
void voxel_mesh_free(VoxelMesh *mesh);

void voxel_renderer_draw_mesh(VoxelRenderer *renderer,
                              mtl_render_command_encoder_t encoder,
                              VoxelMesh *mesh, const float mvp[16]);

VoxelRenderStats voxel_renderer_get_stats(const VoxelRenderer *renderer);

#endif // VOXEL_RENDERER_H
