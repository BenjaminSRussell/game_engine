# Metal Rendering Engine - Execution Guide

## Quick Reference

### Phase Overview (Metal-Focused)
| Phase | Focus | Prompt File | Agents |
|-------|-------|-------------|--------|
| 1 | Metal Core Infrastructure | `phase1_metal_core.md` | 6 |
| 2 | Directory Reorganization & Geometry | `phase2_reorganize_and_geometry.md` | 6 |
| 3 | Metal Rendering Pipeline | `phase3_metal_rendering.md` | 6 |
| 4 | Materials & Lighting | `phase4_materials_lighting.md` | 6 |
| 5 | Effects & Environment | `phase5_effects_environment.md` | 6 |
| 6 | Advanced Systems & Integration | `phase6_advanced_integration.md` | 6 |
| **Total** | | | **36** |

---

## Critical Notes

### Metal Only
- **NO Vulkan code** - Remove any vk* references
- **NO D3D12 code** - Remove any D3D12/DXGI references
- **NO OpenGL code** - Remove any gl* references
- Use `id<MTLDevice>`, `id<MTLCommandBuffer>`, etc.
- Use Apple SIMD types: `simd_float3`, `simd_float4x4`

### File Organization
Phase 2 Agent 2.1 handles reorganizing 154 flat directories into ~20 organized categories. Run this early in Phase 2.

---

## Phase 1: Metal Core Infrastructure

**Must complete first - foundation for everything**

| Agent | Focus | Key APIs |
|-------|-------|----------|
| 1.1 | Metal Device & Context | MTLCreateSystemDefaultDevice, MTLDevice |
| 1.2 | Command Buffers & Encoding | MTLCommandQueue, MTLCommandBuffer, encoders |
| 1.3 | Metal Buffers & Textures | MTLBuffer, MTLTexture, MTLTextureDescriptor |
| 1.4 | Metal Pipeline State | MTLRenderPipelineState, MTLComputePipelineState |
| 1.5 | Metal Synchronization | MTLEvent, MTLFence, completion handlers |
| 1.6 | Swapchain & Presentation | CAMetalLayer, nextDrawable, present |

---

## Phase 2: Reorganization & Geometry

**Depends on: Phase 1**

| Agent | Focus | Output |
|-------|-------|--------|
| 2.1 | Directory Reorganization | Move files to proper subdirectories |
| 2.2 | Mesh System | mesh_t, vertex buffers, index buffers |
| 2.3 | Vertex Formats | Flexible vertex attributes |
| 2.4 | LOD System | LOD generation, runtime selection |
| 2.5 | BVH & Culling | Spatial acceleration, frustum culling |
| 2.6 | Instancing | Instance buffers, indirect draw |

---

## Phase 3: Metal Rendering Pipeline

**Depends on: Phase 1, 2**

| Agent | Focus | Output |
|-------|-------|--------|
| 3.1 | Render Graph | Automatic resource management, pass scheduling |
| 3.2 | G-buffer System | Deferred rendering textures |
| 3.3 | Deferred Lighting | PBR lighting pass |
| 3.4 | Forward Rendering | Transparency, special materials |
| 3.5 | Shadow Mapping | Cascaded shadows, atlas |
| 3.6 | Shader Compilation | .metallib loading, reflection |

---

## Phase 4: Materials & Lighting

**Depends on: Phase 1, 2, 3**

| Agent | Focus | Output |
|-------|-------|--------|
| 4.1 | PBR Materials | Material system, instances |
| 4.2 | Light Management | Light list, clustered culling |
| 4.3 | Irradiance Probes | Diffuse GI approximation |
| 4.4 | Volumetric Fog | Froxel-based fog |
| 4.5 | Screen-Space Reflections | Hierarchical trace SSR |
| 4.6 | Bloom & Tonemapping | HDR post-processing |

---

## Phase 5: Effects & Environment

**Depends on: Phase 1, 2, 3, 4**

| Agent | Focus | Output |
|-------|-------|--------|
| 5.1 | GPU Particles | Compute-based particles |
| 5.2 | FFT Ocean | Tessendorf ocean simulation |
| 5.3 | Atmospheric Scattering | Sky, aerial perspective |
| 5.4 | TAA | Temporal anti-aliasing |
| 5.5 | Decal System | Deferred decals |
| 5.6 | Debug Visualization | Debug rendering tools |

---

## Phase 6: Advanced Systems & Integration

**Depends on: All previous phases**

| Agent | Focus | Output |
|-------|-------|--------|
| 6.1 | GPU-Driven Rendering | Cluster culling, visibility buffer |
| 6.2 | Skeletal Animation | GPU skinning, animation clips |
| 6.3 | Cloth & Hair | Physics simulation, rendering |
| 6.4 | Asset Import | glTF, OBJ, texture import |
| 6.5 | Editor Tools | Gizmos, picking, debug viz |
| 6.6 | Integration & Testing | Full pipeline, tests, benchmarks |

---

## How to Launch Agents

### Single Agent
```
Use Task tool:
- subagent_type: "general-purpose"
- prompt: [Copy full agent prompt from phase file]
- model: "sonnet"
```

### Parallel Execution (All 6 in a Phase)
```
Send single message with 6 Task tool calls:
- Each task gets one agent's prompt
- All run in parallel
- Use TaskOutput to monitor results
```

### Background Execution
```
Use Task tool with:
- run_in_background: true
- Later use TaskOutput with task_id to get results
```

---

## Agent Prompt Template

Each agent prompt should include:

1. **Objective** - What the agent should accomplish
2. **Files to Implement** - Specific file paths
3. **Implementation Requirements** - Code patterns, API usage
4. **Success Criteria** - Checklist of completion requirements

---

## Progress Tracking

Each agent should:
1. Read existing files in assigned directories
2. Replace TODO/placeholder code with real implementations
3. Use Metal API exclusively
4. Report: "Completed X TODOs in Y files"

### Verification
After each phase:
- [ ] Code compiles (clang with -framework Metal)
- [ ] No Vulkan/D3D12/OpenGL references
- [ ] All public functions implemented
- [ ] Memory management correct

---

## Common Metal Patterns

### Device Creation
```c
id<MTLDevice> device = MTLCreateSystemDefaultDevice();
```

### Buffer Creation
```c
id<MTLBuffer> buffer = [device newBufferWithLength:size
                                           options:MTLResourceStorageModeShared];
```

### Texture Creation
```c
MTLTextureDescriptor* desc = [MTLTextureDescriptor texture2DDescriptorWithPixelFormat:MTLPixelFormatRGBA8Unorm
                                                                                width:width
                                                                               height:height
                                                                            mipmapped:NO];
id<MTLTexture> texture = [device newTextureWithDescriptor:desc];
```

### Command Encoding
```c
id<MTLCommandBuffer> cmdBuffer = [commandQueue commandBuffer];
id<MTLRenderCommandEncoder> encoder = [cmdBuffer renderCommandEncoderWithDescriptor:passDesc];
[encoder setRenderPipelineState:pipelineState];
[encoder setVertexBuffer:vertexBuffer offset:0 atIndex:0];
[encoder drawPrimitives:MTLPrimitiveTypeTriangle vertexStart:0 vertexCount:count];
[encoder endEncoding];
[cmdBuffer presentDrawable:drawable];
[cmdBuffer commit];
```

---

## Troubleshooting

### "Undefined symbol" errors
- Ensure Metal framework is linked: `-framework Metal -framework MetalKit`
- Check Objective-C runtime linked: `-lobjc`

### "Invalid MTLTexture" errors
- Verify texture format matches pipeline state
- Check texture is created before use

### Memory leaks
- Every `newBuffer`, `newTexture` needs corresponding release
- Use autorelease pools: `@autoreleasepool { ... }`

---

## Files Reference

```
agents/
├── IMPLEMENTATION_PHASES.md      # Overview document
├── EXECUTION_GUIDE.md            # This file
├── phase1_metal_core.md          # Phase 1 agent prompts
├── phase2_reorganize_and_geometry.md
├── phase3_metal_rendering.md
├── phase4_materials_lighting.md
├── phase5_effects_environment.md
└── phase6_advanced_integration.md
```
