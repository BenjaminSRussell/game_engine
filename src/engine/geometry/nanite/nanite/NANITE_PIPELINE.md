# Nanite Rendering Pipeline Documentation

## Overview
Nanite is a high-detail virtualized geometry system that allows for massive polygon counts by using a hierarchical cluster-based representation and a multi-stage rendering pipeline.

## Pipeline Stages

### 1. GPU Scene Setup
- All mesh data (clusters, triangles) is uploaded to the GPU as large buffers.
- Scene hierarchy and instance data are managed in `gpu_scene.c`.

### 2. Two-Pass Occlusion Culling
- **Pass 1: Main Culling**
    - Clusters visible in the previous frame are rendered.
    - Occlusion queries (HzB) are built from the result.
- **Pass 2: Post-Culling**
    - Clusters that were previously occluded are tested against the new HzB.
    - Newly visible clusters are rendered.

### 3. Visibility Buffer Rendering
- Instead of traditional G-Buffers, Nanite outputs triangle and cluster IDs to a 64-bit Visibility Buffer.
- **Hardware Path**: Large clusters are rendered using fixed-function hardware.
- **Software Path**: Micro-triangles (smaller than a pixel) are rasterized using compute shaders and 64-bit atomics.

### 4. Deferred Texturing & Material Evaluation
- The visibility buffer is resolved to determine material parameters.
- Vertex attributes are interpolated using barycentrics derived from the triangle ID.
- Materials are evaluated in a compute shader pass, minimizing overdraw.

### 5. Streaming & Feedback
- The GPU provides feedback on which clusters were requested for rendering.
- `feedback_analysis.c` processes this data to trigger asynchronous loading of cluster data from disk.

## Key Files
- `cluster_cull_gpu.c`: Core culling logic.
- `software_raster.c`: Implementation of the micro-triangle compute rasterizer.
- `visibility_buffer_nanite.c`: Encoding/decoding of visibility data.
- `gpu_scene.c`: Manages the global scene state on the GPU.

## Future Enhancements
- Support for Variable Rate Shading (VRS).
- Integration with Virtual Shadow Maps (VSMs).
- Advanced LOD streaming with compression.
