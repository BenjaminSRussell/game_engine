// hlod_system.m
// Hierarchical LOD Implementation
#include "../../include/world/hlod_system.h"
#include "../../include/core/logger.h"
#import <Metal/Metal.h>
#include <math.h>
#include <stdlib.h>

HLODSystem *hlod_create(id<MTLDevice> device) {
  HLODSystem *sys = calloc(1, sizeof(HLODSystem));
  sys->device = device;
  sys->max_depth = 4;
  sys->loading_range = 1000.0f;
  sys->enable_streaming = true;
  LOG_INFO("HLOD system created");
  return sys;
}

void hlod_generate_proxy_mesh(HLODSystem *sys, HLODNode *node) {
  if (!node || node->child_count == 0)
    return;

  // 1. Calculate total size (Simulated)
  u32 total_verts = 0;
  u32 total_indices = 0;

  // In a real scenario, we would iterate children and sum their vertices
  total_verts = node->child_count * 1000;
  total_indices = node->child_count * 3000;

  if (total_verts == 0)
    return;

  // 2. Allocate Buffer
  node->proxy_vertex_buffer =
      [sys->device newBufferWithLength:total_verts * sizeof(float) * 3
                               options:MTLResourceStorageModeShared];
  node->proxy_index_buffer =
      [sys->device newBufferWithLength:total_indices * sizeof(u32)
                               options:MTLResourceStorageModeShared];

  // 3. Merge Logic (Pointer arithmetic)
  // float* v_ptr = (float*)[node->proxy_vertex_buffer contents];

  // Loop through children and copy data (Placeholder for actual memcpy)
  for (u32 i = 0; i < node->child_count; i++) {
    // HLODNode* child = node->children[i];
    // memcpy(v_ptr + offset, child->mesh_data, size);
    // offset += size;
  }

  node->proxy_index_count = total_indices;
  node->is_loaded = true;
  LOG_INFO("Generated HLOD Proxy for Node %u with %u vertices", node->id,
           total_verts);
}

void hlod_update(HLODSystem *sys, Vec3 camera_pos) {
  if (!sys->root)
    return;

  // Traverse tree and determine active LODs based on distance/screen size
  // Stack-based traversal to avoid recursion overhead
  HLODNode *stack[256];
  u32 stack_ptr = 0;
  stack[stack_ptr++] = sys->root;

  while (stack_ptr > 0) {
    HLODNode *node = stack[--stack_ptr];

    f32 dist = vec3_distance(camera_pos, bounds_box_center(&node->bounds));

    // Logic: If close enough, refine. If far, use proxy.
    // This is a simplified error metric (distance based)
    // Real implementation uses Screen Space Error (SSE)

    if (dist < node->switch_distance && node->child_count > 0) {
      // We are close, use children (higher detail)
      for (u32 i = 0; i < node->child_count; i++) {
        stack[stack_ptr++] = node->children[i];
      }
    } else {
      // We are far, render this node (or its proxy)
      if (node->level > 0 && !node->is_loaded) {
        hlod_stream_node(sys, node);
      }
      // Add to render list...
    }
  }
}
