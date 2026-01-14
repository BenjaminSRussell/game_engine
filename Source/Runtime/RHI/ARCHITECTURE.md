# RHI (Render Hardware Interface) Architecture

> **Priority 3** | Est. 5,000+ files | 600K+ LOC | ⭐⭐⭐⭐ Complexity

The RHI provides a unified abstraction layer over graphics APIs (Vulkan, Metal, D3D12).

---

## Directory Structure

```
RHI/
├── Private/
├── Public/
│
├── Core/                       # RHI core abstractions
│   ├── Device/                 # Logical device
│   │   ├── Creation/
│   │   ├── Features/
│   │   └── Limits/
│   ├── Queue/                  # Command queues
│   │   ├── Graphics/
│   │   ├── Compute/
│   │   ├── Transfer/
│   │   └── Present/
│   ├── Swapchain/
│   │   ├── Creation/
│   │   ├── Present/
│   │   └── Resize/
│   └── Surface/
│
├── Commands/                   # Command recording
│   ├── Buffer/                 # Command buffers
│   │   ├── Primary/
│   │   ├── Secondary/
│   │   └── Pool/
│   ├── List/                   # Command lists
│   ├── Encoder/
│   │   ├── Render/
│   │   ├── Compute/
│   │   ├── Transfer/
│   │   └── RayTracing/
│   └── Barriers/
│       ├── Memory/
│       ├── Image/
│       └── Buffer/
│
├── Resources/                  # GPU resources
│   ├── Buffer/
│   │   ├── Vertex/
│   │   ├── Index/
│   │   ├── Uniform/
│   │   ├── Storage/
│   │   └── Staging/
│   ├── Texture/
│   │   ├── Image/
│   │   ├── ImageView/
│   │   ├── Sampler/
│   │   └── Formats/
│   ├── Framebuffer/
│   └── RenderPass/
│       ├── Attachments/
│       ├── Subpasses/
│       └── Dependencies/
│
├── Pipeline/                   # Pipeline objects
│   ├── Graphics/
│   │   ├── VertexInput/
│   │   ├── InputAssembly/
│   │   ├── Tessellation/
│   │   ├── Viewport/
│   │   ├── Rasterization/
│   │   ├── Multisample/
│   │   ├── DepthStencil/
│   │   ├── ColorBlend/
│   │   └── DynamicState/
│   ├── Compute/
│   ├── RayTracing/
│   │   ├── RayGen/
│   │   ├── Miss/
│   │   ├── ClosestHit/
│   │   ├── AnyHit/
│   │   └── Intersection/
│   ├── Cache/
│   └── Layout/
│       ├── Descriptor/
│       ├── Push/
│       └── Binding/
│
├── Shaders/                    # Shader compilation
│   ├── Compiler/
│   │   ├── SPIRV/
│   │   ├── DXIL/
│   │   └── MSL/
│   ├── Module/
│   ├── Reflection/
│   ├── Variants/
│   └── HotReload/
│
├── Memory/                     # Memory management
│   ├── Allocator/
│   │   ├── VMA/                # Vulkan Memory Allocator
│   │   ├── D3D12MA/
│   │   └── Metal/
│   ├── Heap/
│   │   ├── Device/
│   │   ├── Host/
│   │   └── Shared/
│   ├── Pool/
│   │   ├── Staging/
│   │   ├── Transient/
│   │   └── Static/
│   └── Budget/
│
├── Synchronization/            # GPU sync
│   ├── Fence/
│   ├── Semaphore/
│   │   ├── Binary/
│   │   └── Timeline/
│   ├── Barrier/
│   └── Query/
│       ├── Occlusion/
│       ├── Timestamp/
│       └── Pipeline/
│
├── Descriptors/                # Resource binding
│   ├── Set/
│   ├── Pool/
│   ├── Layout/
│   └── Bindless/
│       ├── Heap/
│       ├── Table/
│       └── Update/
│
├── Vulkan/                     # Vulkan backend
│   ├── VkDevice/
│   ├── VkQueue/
│   ├── VkSwapchain/
│   ├── VkCommands/
│   ├── VkResources/
│   ├── VkPipeline/
│   ├── VkMemory/
│   ├── VkSync/
│   └── VkRayTracing/
│
├── Metal/                      # Metal backend
│   ├── MTLDevice/
│   ├── MTLQueue/
│   ├── MTLSwapchain/
│   ├── MTLCommands/
│   ├── MTLResources/
│   ├── MTLPipeline/
│   ├── MTLMemory/
│   └── MTLRayTracing/
│
├── D3D12/                      # D3D12 backend
│   ├── D3DDevice/
│   ├── D3DQueue/
│   ├── D3DSwapchain/
│   ├── D3DCommands/
│   ├── D3DResources/
│   ├── D3DPipeline/
│   ├── D3DMemory/
│   └── D3DRayTracing/
│
├── Validation/                 # Debug layers
│   ├── VulkanValidation/
│   ├── D3D12Debug/
│   └── MetalValidation/
│
└── Tools/                      # Dev tools
    ├── Capture/
    ├── Profiler/
    └── Statistics/
```

---

## Current Code Mapping

| New Location | Current Location | Files |
|--------------|------------------|-------|
| RHI/Vulkan/ | src/RHI/Vulkan/ | ~60 |
| RHI/Metal/ | src/RHI/Metal/ | ~40 |
| RHI/D3D12/ | src/RHI/D3D12/ | ~30 |
| RHI/Core/ | src/RHI/Core/ | ~20 |

---

## Design Principles

1. **Zero-overhead abstraction** - No runtime cost vs native API
2. **Unified resource model** - Same interface across backends
3. **Explicit synchronization** - Application controls sync
4. **Bindless-first** - Default to bindless resource binding
