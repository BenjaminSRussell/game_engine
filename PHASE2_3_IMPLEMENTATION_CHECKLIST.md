# Phase 2 & 3 Implementation Checklist

Now that Phase 1 foundation is complete, here's what needs to be done for Phases 2-3.

## Phase 2: Metal Backend Integration & GPU Upload (3-4 weeks)

### 2.1 Metal Buffer Allocation
- [ ] Implement `rendering_gpu_scene_create_buffers()` in gpu_scene.c
  - [ ] Use `metal_buffer_create()` from mtl_buffer.{c,h}
  - [ ] Allocate instance_buffer (PRIVATE storage, GPU-only)
  - [ ] Allocate material_buffer
  - [ ] Allocate indirect_args_buffer
  - [ ] Allocate culling_results_buffer
  - [ ] Allocate counters_buffer (atomic operations)
  - [ ] Track all as id<MTLBuffer> pointers

### 2.2 GPU Data Upload Implementation
- [ ] Implement `rendering_gpu_transfer_upload_instances()` in gpu_data_transfer.c
  - [ ] Allocate staging buffer if needed (SHARED memory)
  - [ ] Copy CPU data to staging buffer
  - [ ] Use `metal_blit_encoder_copy_from_buffer_to_buffer()`
  - [ ] Update GPU scene's instance_buffer
  - [ ] Support async transfers with callbacks

- [ ] Implement `rendering_gpu_transfer_upload_materials()`
  - [ ] Same pattern as instances but for material data

- [ ] Implement `rendering_gpu_transfer_readback_culling_results()`
  - [ ] Create shared buffer for results
  - [ ] Copy from GPU to CPU buffer
  - [ ] Implement proper synchronization

### 2.3 Metal Shader Compilation
- [ ] Create shader library compilation pipeline
  - [ ] Load gpu_culling.metal as library
  - [ ] Use `metal_compile_kernel()` (or create it)
  - [ ] Get function references for each kernel:
    - [ ] cull_frustum
    - [ ] select_lod
    - [ ] compact_visible
    - [ ] populate_indirect_args
    - [ ] build_batches
    - [ ] reset_counters

### 2.4 Compute Shader Dispatch
- [ ] Implement compute pipeline setup
  - [ ] Create compute pipeline state for each kernel
  - [ ] Set buffer bindings for kernel arguments
  - [ ] Configure threadgroup size

- [ ] Implement dispatch in render loop
  - [ ] Reset counters
  - [ ] Dispatch frustum culling
  - [ ] Add memory barrier
  - [ ] Dispatch LOD selection
  - [ ] Add memory barrier
  - [ ] Dispatch compaction
  - [ ] Readback visible count

### 2.5 Indirect Command Buffer Integration
- [ ] Use Metal ICB API (`mtl_indirect_command.{c,h}`)
  - [ ] Create MTLIndirectCommandBuffer descriptor
  - [ ] Set supported command types (draw_indexed)
  - [ ] Create ICB with max command count
  - [ ] Bind to compute shader for GPU encoding

### 2.6 Testing Phase 2
- [ ] Unit tests for buffer allocation
- [ ] Unit tests for data upload
- [ ] Unit tests for GPU readback
- [ ] Integration test: load mesh, batch, upload, read back
- [ ] Performance: measure upload time for 100k instances
- [ ] Memory check: verify no GPU memory leaks

---

## Phase 3: Render Loop Integration & Optimization (2-3 weeks)

### 3.1 Render Pipeline Integration
- [ ] Create `rendering_gpu_scene_render()` function
  - [ ] Inputs: scene handle, camera frustum, LOD data
  - [ ] Outputs: draw count for renderer

  **Pipeline steps**:
  1. [ ] Encode compute pass: cull_frustum
     - [ ] Bind instance buffer
     - [ ] Bind results buffer
     - [ ] Bind frustum constant
     - [ ] Dispatch with (instance_count + 63) / 64 threadgroups

  2. [ ] Add barrier for GPU sync

  3. [ ] Encode compute pass: select_lod
     - [ ] Bind LOD data
     - [ ] Dispatch
     - [ ] Barrier

  4. [ ] Encode compute pass: compact_visible
     - [ ] Reset visible_count to 0
     - [ ] Dispatch
     - [ ] Barrier

  5. [ ] Readback visible count (GPU→CPU)
     - [ ] Use `metal_blit_encoder_copy_from_buffer_to_buffer()`
     - [ ] Wait for completion

  6. [ ] Encode compute pass: populate_indirect_args
     - [ ] Dispatch based on visible_count
     - [ ] Barrier

- [ ] Handle multiple views (stereo rendering)
  - [ ] Run culling twice with different frustums
  - [ ] Merge results (OR of visible flags)

### 3.2 Indirect Rendering Execution
- [ ] Implement multi-draw indirect rendering
  - [ ] Get indirect args buffer
  - [ ] Get draw count
  - [ ] Bind vertex/index buffers
  - [ ] Bind instance buffer
  - [ ] Bind material buffer
  - [ ] Call `drawIndexedPrimitives:indirectBuffer:` with count

### 3.3 Camera Integration
- [ ] Extract view frustum from camera
  - [ ] Get 6 plane equations from view-projection matrix
  - [ ] Fill FrustumData structure
  - [ ] Pass to culling compute shader

- [ ] Extract camera position
  - [ ] Pass to LOD selection shader

### 3.4 LOD Configuration
- [ ] Set up LOD distance thresholds
  - [ ] Configure for your mesh sizes
  - [ ] Set hysteresis value (typically 0.1-0.2)
  - [ ] Enable/disable per-scene

### 3.5 Advanced Culling (Optional, extends Phase 3)
- [ ] Occlusion culling
  - [ ] Create HZB (hierarchical Z-buffer)
  - [ ] Implement GPU occlusion test
  - [ ] Add occlusion kernel to pipeline

- [ ] Distance culling
  - [ ] Add distance field check
  - [ ] Cull very far instances

- [ ] Hierarchical culling
  - [ ] Build BVH on CPU
  - [ ] Traverse in compute shader
  - [ ] Faster frustum culling for large datasets

### 3.6 Performance Optimization
- [ ] Optimize kernel threadgroup size
  - [ ] Current: 64 threads per group
  - [ ] Experiment: 128, 256 threads
  - [ ] Profile and choose best

- [ ] Optimize memory access
  - [ ] Verify SoA layout is optimal
  - [ ] Check for bank conflicts in shared memory
  - [ ] Profile with Metal's profiler

- [ ] Reduce synchronization
  - [ ] Combine kernels if possible
  - [ ] Minimize barriers

- [ ] CPU-GPU synchronization
  - [ ] Minimize GPU→CPU readbacks
  - [ ] Use triple buffering to hide latency
  - [ ] Profile: target < 0.5ms CPU stall

### 3.7 Batching Refinement
- [ ] Enhance batch grouping
  - [ ] Group by mesh+material+LOD
  - [ ] Sort for state reduction
  - [ ] Track texture binding changes

- [ ] Implement batch-level LOD
  - [ ] Per-batch LOD selection
  - [ ] Different LOD for different batches

### 3.8 Statistics & Profiling
- [ ] Add GPU timing
  - [ ] Measure each compute kernel time
  - [ ] Track culling efficiency (visible/total ratio)
  - [ ] Monitor draw call count

- [ ] Add stats tracking
  - [ ] Visible instance percentage
  - [ ] Batch count per frame
  - [ ] Memory bandwidth usage

- [ ] Create visualization
  - [ ] Visualize culled instances (red/green)
  - [ ] Show LOD levels (colored by LOD)
  - [ ] Display draw count

### 3.9 Testing Phase 3
- [ ] Functional tests
  - [ ] Run with 1000, 10k, 100k, 1M instances
  - [ ] Verify correctness (no missing/extra draws)
  - [ ] Test with moving camera
  - [ ] Test with dynamic scene changes

- [ ] Performance tests
  - [ ] Measure GPU time per frame
  - [ ] Measure CPU overhead
  - [ ] Memory usage
  - [ ] Bandwidth usage

- [ ] Stress tests
  - [ ] Maximize instances until performance degrades
  - [ ] Find performance bottleneck
  - [ ] Document results

### 3.10 Integration Tests
- [ ] Full pipeline test
  - [ ] Load scene with thousands of instances
  - [ ] Run culling and rendering
  - [ ] Verify visual correctness

- [ ] Comparison test
  - [ ] Compare GPU-driven vs traditional rendering
  - [ ] Measure FPS improvement
  - [ ] Document results

---

## Key Files to Implement

### Core Implementation Files
```
Phase 2:
  gpu_scene.c          - Implement create_buffers, upload_dirty_data
  gpu_data_transfer.c  - Implement all GPU upload functions
  (NEW) gpu_culling_pipeline.c  - Compute shader dispatch
  (NEW) gpu_shader_compiler.c   - Shader compilation

Phase 3:
  gpu_scene_render.c   - Main render pipeline
  (NEW) gpu_indirect_render.c   - Indirect drawing
  (NEW) gpu_camera_culling.c    - Frustum extraction
```

### Integration Points
```
Render Loop Entry:
  1. rendering_gpu_scene_update_instances() - CPU → GPU
  2. rendering_gpu_scene_render()          - GPU compute + indirect draw
  3. rendering_gpu_scene_get_stats()       - Profiling

Batching Flow:
  1. geometry_instance_batching_sort()     - Pre-process
  2. rendering_gpu_scene_set_instances()   - Upload batch data
  3. rendering_gpu_scene_upload_dirty_data() - GPU transfer
```

---

## Performance Targets

| Metric | Target | Success Criterion |
|--------|--------|-------------------|
| Instances | 1M | Full frame with 1M instances |
| Culling Time | < 2ms | GPU time per frame |
| Draw Calls | < 100 | For 1M instances |
| Memory | < 200MB | For 1M instances + materials |
| Visible Ratio | > 90% | Culling efficiency |
| FPS | > 60 | With culling + rendering |

---

## Dependencies

Phase 2 requires:
- [x] Metal backend (mtl_buffer, mtl_encoder, mtl_indirect_command)
- [x] GPU type definitions (gpu_types.h)
- [x] Compute shader kernels (gpu_culling.metal)

Phase 3 requires:
- [x] Phase 2 completion
- [x] Camera system integration
- [x] Main render loop access

---

## Debugging Tools

### Metal GPU Capture
```
1. Xcode → Product → Scheme → Edit Scheme
2. Diagnostics → Enable Metal validation
3. Build and run
4. Inspect compute shader dispatch and GPU memory
```

### Performance Profiling
```
Instruments → GPU Profiler
- Measure kernel execution time
- Track memory bandwidth
- Identify bottlenecks
```

### Verification Tests
```c
// Validate culling results
for (uint32_t i = 0; i < visible_count; i++) {
    CullingResult result = results[visible_indices[i]];
    assert(result.visible_flag == 1);
}

// Verify indirect args
assert(indirect_args[i].indexCount > 0);
assert(indirect_args[i].instanceCount > 0);
assert(indirect_args[i].indexStart < index_buffer_size);
```

---

## Estimated Timeline

- Phase 2 (Metal integration): 3-4 weeks
  - Buffer allocation: 1 week
  - Shader compilation: 1 week
  - Upload implementation: 1 week
  - Testing & debugging: 1 week

- Phase 3 (Render integration): 2-3 weeks
  - Render pipeline: 1 week
  - Integration: 1 week
  - Optimization: 1 week

Total: 5-7 weeks for complete GPU-driven rendering system

---

## Success Criteria

### Phase 2 Complete When:
- ✅ GPU buffers allocate without errors
- ✅ CPU data uploads to GPU successfully
- ✅ GPU readback matches uploaded data
- ✅ Compute shaders compile and dispatch
- ✅ All unit tests pass
- ✅ Memory profiler shows no leaks

### Phase 3 Complete When:
- ✅ Full pipeline runs end-to-end
- ✅ Render output is visually correct
- ✅ Performance meets targets
- ✅ 1M instances render > 60 FPS
- ✅ All integration tests pass
- ✅ Documentation complete

---

## Notes

- Keep Metal validation enabled during Phase 2 development
- Profile early and often in Phase 3
- Document any GPU-specific constraints (e.g., max threadgroup size)
- Maintain compatibility with existing rendering pipeline
- Plan for multi-GPU support (future consideration)
