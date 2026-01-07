# Frame Graph - Test Example
# Simple demonstration of render graph API

This directory contains the render graph system implementation.

## Week 1 Status: ✅ Core API Complete

### Implemented Files:
- `frame_graph.h` - Public API
- `frame_graph_internal.h` - Internal data structures
- `frame_graph.c` - Core lifecycle and resource/pass management
- `compile_execute.c` - Compilation and execution (stubs for advanced features)

### Current Capabilities:
- ✅ Create/destroy render graphs
- ✅ Declare transient and imported resources (textures, buffers)
- ✅ Add passes with read/write dependencies
- ✅ Compile graph (basic, uses declaration order)
- ✅ Execute passes in order
- ✅ Export to DOT format for visualization
- ✅ Statistics tracking

### Not Yet Implemented (Week 2+):
- ⏳ Dependency graph analysis (topological sort)
- ⏳ Dead pass culling
- ⏳ Automatic barrier insertion
- ⏳ Transient resource pooling with aliasing
- ⏳ GPU profiling with timestamps

## Example Usage:

```c
#include "renderer/frame_graph/frame_graph.h"

void example_render_frame(Renderer *r, Scene *scene) {
    // Create graph
    RenderGraph *rg = rg_create();
    
    // Import swapchain
    RGResourceHandle swapchain = rg_import_texture(rg, r->swapchain_image, "Swapchain");
    
    // Create shadow map resource
    RGTextureDesc shadow_desc = {
        .width = 2048, .height = 2048,
        .format = FORMAT_DEPTH32F,
        .usage = TEXTURE_USAGE_DEPTH_STENCIL,
        .name = "ShadowMap"
    };
    RGResourceHandle shadow_map = rg_create_texture(rg, &shadow_desc);
    
    // Add shadow pass
    RGPassDesc shadow_pass_desc = {
        .name = "ShadowPass",
        .execute = shadow_pass_execute,
        .user_data = scene
    };
    RGPassHandle shadow_pass = rg_add_pass(rg, &shadow_pass_desc);
    rg_pass_write(rg, shadow_pass, shadow_map);
    
    // Add lighting pass
    RGPassDesc lighting_pass_desc = {
        .name = "LightingPass",
        .execute = lighting_pass_execute,
        .user_data = scene
    };
    RGPassHandle lighting_pass = rg_add_pass(rg, &lighting_pass_desc);
    rg_pass_read(rg, lighting_pass, shadow_map);
    rg_pass_write(rg, lighting_pass, swapchain);
    
    // Compile & execute
    if (rg_compile(rg)) {
        rg_execute(rg, r->cmd);
    }
    
    // Cleanup
    rg_destroy(rg);
}
```

## Building:

Add to CMakeLists.txt:
```cmake
add_library(FrameGraph
    src/engine/renderer/frame_graph/frame_graph.c
    src/engine/renderer/frame_graph/compile_execute.c
)
```

## Testing:

Generate DOT visualization:
```c
rg_export_dot(rg, "graph.dot");
```

Then: `dot -Tpng graph.dot -o graph.png`

## Next Steps (Week 2):
1. Implement proper dependency analysis
2. Topological sort with cycle detection
3. Resource state tracking and barrier generation
4. Transient resource pool with memory aliasing
