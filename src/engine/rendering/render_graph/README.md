# Metal Render Graph System

A lightweight, C-compatible render graph system designed for Metal (and other APIs) to automate resource management and pass scheduling.

## Features

- **Automatic Resource Management**: Handles creation, lifecycle, and destruction of transient resources (textures/buffers).
- **Dependency Tracking**: Automatically tracks dependencies between passes based on resource usage (inputs/outputs).
- **Topological Sorting**: Ensures passes execute in the correct order to satisfy dependencies.
- **Resource Aliasing**: Optimizes memory usage by aliasing transient resources that don't overlap in time (using greedy interval scheduling).
- **Metal Integration**: Designed to hold `id<MTLTexture>` and `id<MTLBuffer>` handles (void* storage for C compatibility).
- **Async Compute Support**: Structure supports async compute queues (future expansion).

## Architecture

- **Render Pass Node (`render_pass_node`)**: Represents a stage in the rendering pipeline (Graphics, Compute, etc.). Holds inputs, outputs, and an execution callback.
- **Resource Node (`resource_node`)**: Represents a GPU resource (Texture, Buffer). Can be transient (managed by graph) or imported (external).
- **Graph Compiler (`graph_compiler`)**: Compiles the graph definition into a linear execution list. Performs topological sort, cycle detection, and resource aliasing analysis.
- **Graph Executor (`graph_executor`)**: Executes the compiled graph, invoking pass callbacks and handling barriers.

## Usage

### 1. Initialization
```c
rendering_graph_compiler_init();
render_graph_t* graph = render_graph_create("MyFrame");
```

### 2. Define Resources
Define resources using `rendering_resource_node_desc_t`. Mark `is_transient = true` for graph-managed memory.
```c
rendering_resource_node_desc_t tex_desc = {
    .name = "SceneColor",
    .type = RENDERING_RESOURCE_TYPE_TEXTURE,
    .width = 1920, .height = 1080,
    .format = MTLPixelFormatRGBA16Float,
    .is_transient = true
};
rg_resource_handle_t scene_color = rg_create_texture(graph, "SceneColor", &tex_desc);
```

### 3. Define Passes
Add passes that use these resources.
```c
rendering_render_pass_node_desc_t pass_desc = {
    .name = "Lighting",
    .type = RENDERING_PASS_TYPE_GRAPHICS,
    .execute = my_lighting_callback,
    .color_outputs = {scene_color},
    .color_output_count = 1,
    .texture_inputs = {gbuffer_albedo, gbuffer_normal}, // Defined previously
    .texture_input_count = 2
};
rg_add_pass(graph, "Lighting", RENDERING_PASS_TYPE_GRAPHICS, &pass_desc);
```

### 4. Compile
```c
if (rg_compile(graph) != 0) {
    // Handle error (cycles, invalid logic)
}
```

### 5. Execute
Call `rg_execute` every frame with your Metal Command Buffer.
```c
rg_execute(graph, mtlCommandBuffer);
```

## Performance
- **Aliasing**: The system calculates "High Water Mark" memory usage and reuses memory between non-overlapping transients.
- **Overhead**: Graph compilation should be done once (or when topology changes). Execution is very low overhead (linear array iteration).
