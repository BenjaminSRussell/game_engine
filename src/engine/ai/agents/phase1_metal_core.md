# Phase 1: Metal Core Infrastructure (100+ Tasks Per Agent)

This phase establishes the complete Metal-native rendering backend. Each agent handles ~100 TODOs ensuring production-quality code.

---

## Agent 1.1: Metal Device & Capabilities (~100 TODOs)

### Objective
Complete Metal device initialization, capability queries, and memory management foundation.

### File Locations
```
src/engine/rendering/3d_rendering/backend/metal/metal/
├── mtl_device.c/h
├── mtl_device_caps.c/h (NEW)
└── mtl_memory_heap.c/h (NEW)
```

### Core Implementation Tasks

**Device Creation & Lifecycle (20 TODOs)**
- [ ] MTLCreateSystemDefaultDevice() wrapper
- [ ] Device retain/release with reference counting
- [ ] Device validity checking
- [ ] Error handling with NSError conversion to C error codes
- [ ] Fallback to software renderer if unavailable
- [ ] Device name and model querying
- [ ] Registry/database of created devices
- [ ] Device destruction and cleanup
- [ ] Singleton pattern for default device
- [ ] Thread-safe device access

**Command Queue Management (15 TODOs)**
- [ ] Create primary command queue
- [ ] Create compute-exclusive command queue
- [ ] Create transfer-exclusive command queue
- [ ] Queue priority configuration (default, high, low)
- [ ] Queue label assignment
- [ ] Queue metrics tracking
- [ ] Multiple queue support (up to 8 queues)
- [ ] Queue reuse vs per-frame creation
- [ ] Queue validation
- [ ] Queue event wait support
- [ ] Command queue pooling
- [ ] Per-queue statistics

**Capability Detection (25 TODOs)**
- [ ] macOS version detection (10.13+, 11.0+, 12.0+, 13.0+)
- [ ] GPU family detection (Apple3-8, Mac1-2)
- [ ] Ray tracing support (macOS 10.15+)
- [ ] Mesh shaders support (Apple7+)
- [ ] Indirect rendering support
- [ ] Variable rate shading support
- [ ] Sparse texture support
- [ ] Argument buffers support
- [ ] Raster order groups support
- [ ] Texture compression formats (ASTC, BC, PVRTC)
- [ ] Max texture dimensions
- [ ] Max buffer size
- [ ] Feature level determination
- [ ] Display capabilities (HDR, ProMotion)
- [ ] Memory architecture (unified vs discrete)
- [ ] Compute capabilities (max threads)
- [ ] Sampler support (16bit vs 32bit)
- [ ] Fetch texture support
- [ ] Read/write texture support

**Memory Heap Management (20 TODOs)**
- [ ] Shared memory heap info
- [ ] Private memory heap info
- [ ] Memoryless heap info
- [ ] Heap size queries
- [ ] Recommended working set size
- [ ] Memory pressure queries
- [ ] Allocate heaps for resource pooling
- [ ] Release heaps with proper cleanup
- [ ] Track heap usage per frame
- [ ] Memory budget enforcement
- [ ] Out-of-memory error handling
- [ ] Memory defragmentation hints
- [ ] Heap validation
- [ ] Memory statistics reporting

**Device State & Context (20 TODOs)**
- [ ] Current device state storage
- [ ] Thread-local device context
- [ ] Device context switching
- [ ] Feature availability querying
- [ ] Device property caching
- [ ] Property update on device change
- [ ] Device removal detection
- [ ] Device validation before use
- [ ] Device statistics collection
- [ ] Performance counter support
- [ ] Debug layer support
- [ ] Debugging tool attachment
- [ ] Device reset handling

### Success Criteria
- [x] MTLDevice created and managed properly
- [ ] All capability queries working
- [ ] Memory heaps enumerated
- [ ] Error handling comprehensive
- [ ] No resource leaks on cleanup
- [ ] Thread-safe operations

---

## Agent 1.2: Command Buffers & Encoding (~100 TODOs)

### Objective
Implement complete command buffer recording system with all encoder types.

### File Locations
```
src/engine/rendering/3d_rendering/backend/metal/metal/
├── mtl_command.c/h
├── mtl_command_buffer.c/h
├── mtl_encoder.c/h (NEW)
└── mtl_parallel_encoder.c/h (NEW)
```

### Core Implementation Tasks

**Command Buffer Lifecycle (20 TODOs)**
- [ ] Create command buffer from queue
- [ ] Set command buffer label
- [ ] Push/pop debug groups
- [ ] Add completion handlers
- [ ] Retry failed command buffers
- [ ] Command buffer pooling for reuse
- [ ] Command buffer validation
- [ ] Error status checking
- [ ] Scheduled callback support
- [ ] Frame pacing with command buffers
- [ ] Command buffer dependencies
- [ ] Triple buffering support

**Render Command Encoding (30 TODOs)**
- [ ] Create render command encoder
- [ ] Set render pass descriptor
- [ ] Set render pipeline state
- [ ] Set vertex/fragment buffers
- [ ] Set vertex/fragment textures
- [ ] Set vertex/fragment samplers
- [ ] Set depth/stencil state
- [ ] Set scissor rect
- [ ] Set viewport
- [ ] Draw primitives (triangles, lines, points)
- [ ] Draw indexed primitives
- [ ] Draw instanced
- [ ] Draw indirect
- [ ] Multi-threaded parallel render encoding
- [ ] Conditional rendering support
- [ ] Occlusion queries
- [ ] Render statistics
- [ ] Render pass optimization hints
- [ ] Clear colors/depth
- [ ] End render encoding
- [ ] Render encoder validation

**Compute Command Encoding (20 TODOs)**
- [ ] Create compute command encoder
- [ ] Set compute pipeline state
- [ ] Set compute buffers
- [ ] Set compute textures
- [ ] Set compute samplers
- [ ] Set compute threadgroup memory
- [ ] Dispatch threadgroups
- [ ] Dispatch threadgroups indirect
- [ ] Compute shader arguments
- [ ] Memory barriers
- [ ] Image barriers
- [ ] Synchronization events
- [ ] End compute encoding
- [ ] Compute statistics

**Blit & Transfer Operations (15 TODOs)**
- [ ] Create blit command encoder
- [ ] Copy buffer to buffer
- [ ] Copy buffer to texture
- [ ] Copy texture to buffer
- [ ] Copy texture to texture
- [ ] Fill buffer
- [ ] Generate mipmaps
- [ ] Synchronize resource
- [ ] End blit encoding

**Command Submission (15 TODOs)**
- [ ] Encode completion handlers
- [ ] Commit command buffer
- [ ] Wait for command buffer completion
- [ ] Present drawable
- [ ] Frame synchronization
- [ ] Command buffer profiling
- [ ] GPU timestamp queries
- [ ] Frame pacing
- [ ] Adaptive frame rate
- [ ] Vsync support

### Success Criteria
- [ ] All encoder types implemented
- [ ] Command buffers properly submitted
- [ ] Synchronization working
- [ ] No GPU stalls
- [ ] Efficient encoding patterns

---

## Agent 1.3: Buffers, Textures & Resources (~100 TODOs)

### Objective
Implement comprehensive GPU resource management with pooling and lifecycle.

### File Locations
```
src/engine/rendering/3d_rendering/backend/metal/metal/
├── mtl_buffer.c/h
├── mtl_texture.c/h
├── mtl_resource_pool.c/h (NEW)
└── mtl_sampler.c/h
```

### Core Implementation Tasks

**Buffer Creation & Management (30 TODOs)**
- [ ] Create buffer with default storage mode
- [ ] Create shared storage buffer (CPU accessible)
- [ ] Create private storage buffer (GPU only)
- [ ] Create memoryless buffer
- [ ] Buffer sub-allocation for pooling
- [ ] Ring buffer for per-frame data
- [ ] Circular buffer implementation
- [ ] Buffer mapping for CPU read/write
- [ ] Persistent mapped buffers
- [ ] Buffer size queries
- [ ] Buffer storage mode checking
- [ ] GPU address queries
- [ ] Buffer validation
- [ ] Buffer usage tracking
- [ ] Buffer statistics

**Texture Creation & Configuration (35 TODOs)**
- [ ] Create 2D textures (RGBA8, RGBA16F, RGBA32F)
- [ ] Create 3D textures for volumes
- [ ] Create cube map textures
- [ ] Create 2D texture arrays
- [ ] Create cube texture arrays
- [ ] Set pixel format
- [ ] Set texture usage flags
- [ ] Set storage mode
- [ ] Enable mipmapping
- [ ] Automatic mipmap generation
- [ ] Manual mipmap updates
- [ ] Texture views and aliases
- [ ] Compressed texture formats (ASTC, BC)
- [ ] Depth textures (Depth32Float, Depth16Unorm)
- [ ] Stencil textures
- [ ] Depth-stencil textures
- [ ] HDR texture formats (RGBA16Float)
- [ ] SRGB vs linear color space
- [ ] Render target textures
- [ ] Sample count for MSAA
- [ ] Texture label and debugging
- [ ] Texture size queries
- [ ] Texture type checking
- [ ] Format compatibility checking

**Data Transfer to GPU (20 TODOs)**
- [ ] Copy data to buffer
- [ ] Copy data to texture
- [ ] Blit operations
- [ ] Partial updates
- [ ] Staged uploads via transfer buffer
- [ ] Async uploads
- [ ] DMA-style transfers
- [ ] Barrier synchronization
- [ ] Pipelined uploads
- [ ] GPU-to-GPU copies

**Resource Pooling (15 TODOs)**
- [ ] Buffer pool management
- [ ] Texture pool management
- [ ] Pool size limits
- [ ] Reuse vs allocation strategy
- [ ] Pool statistics
- [ ] Memory pressure handling
- [ ] Pool defragmentation
- [ ] Unused resource cleanup

### Success Criteria
- [ ] All buffer types created
- [ ] All texture types created
- [ ] Data transfers working
- [ ] Pooling functional
- [ ] No memory leaks

---

## Agent 1.4: Pipeline State Objects (~100 TODOs)

### Objective
Implement render/compute pipelines with caching and shader compilation.

### File Locations
```
src/engine/rendering/3d_rendering/backend/metal/metal/
├── mtl_pipeline.c/h
├── mtl_shader_library.c/h (NEW)
└── mtl_shader_compiler.c/h (NEW)
```

### Core Implementation Tasks

**Render Pipeline Creation (25 TODOs)**
- [ ] Create render pipeline descriptor
- [ ] Attach vertex shader function
- [ ] Attach fragment shader function
- [ ] Set color attachment pixel format
- [ ] Set depth attachment pixel format
- [ ] Set stencil attachment pixel format
- [ ] Configure vertex descriptor
- [ ] Configure blend modes (opaque, additive, alpha)
- [ ] Set rasterization options
- [ ] Set primitive topology
- [ ] Triangle fill mode (fill vs wireframe)
- [ ] Cull mode (front, back, none)
- [ ] Winding order (CW, CCW)
- [ ] Depth clip enable/disable
- [ ] Sample count for MSAA
- [ ] Conservative rasterization
- [ ] Create pipeline state from descriptor
- [ ] Handle compilation errors
- [ ] Pipeline validation
- [ ] Pipeline statistics

**Compute Pipeline Creation (15 TODOs)**
- [ ] Create compute pipeline descriptor
- [ ] Attach compute shader function
- [ ] Set threadgroup memory length
- [ ] Create compute pipeline state
- [ ] Compute function validation
- [ ] Max threads per group
- [ ] Compilation error handling

**Depth/Stencil State (15 TODOs)**
- [ ] Create depth/stencil descriptor
- [ ] Depth compare function (less, equal, greater, etc.)
- [ ] Depth write enable/disable
- [ ] Stencil front face operations
- [ ] Stencil back face operations
- [ ] Stencil reference values
- [ ] Stencil mask configuration
- [ ] Create depth/stencil state

**Shader Library Management (25 TODOs)**
- [ ] Load .metallib files
- [ ] Extract shader functions by name
- [ ] Function signature validation
- [ ] Vertex shader functions
- [ ] Fragment shader functions
- [ ] Compute shader functions
- [ ] Specialization constants support
- [ ] Dynamic library loading
- [ ] Library caching
- [ ] Hot reload support
- [ ] Error reporting
- [ ] Library statistics
- [ ] Function reflection/introspection
- [ ] Argument encoding

**Pipeline Caching (20 TODOs)**
- [ ] Cache pipelines by hash
- [ ] Cache key generation
- [ ] Lookup and reuse
- [ ] Cache invalidation
- [ ] Persistence to disk
- [ ] Cache memory limits
- [ ] Cache statistics
- [ ] LRU eviction

### Success Criteria
- [ ] Pipelines compile without error
- [ ] Caching working effectively
- [ ] Shader functions loaded
- [ ] All states configurable
- [ ] No pipeline leaks

---

## Agent 1.5: Synchronization & Frame Management (~100 TODOs)

### Objective
Implement GPU-CPU synchronization, frame pacing, and resource hazard tracking.

### File Locations
```
src/engine/rendering/3d_rendering/backend/metal/metal/
├── mtl_sync.c/h
├── mtl_frame_sync.c/h (NEW)
└── mtl_hazard_tracking.c/h (NEW)
```

### Core Implementation Tasks

**Frame Synchronization (30 TODOs)**
- [ ] Frame semaphore creation (dispatch_semaphore)
- [ ] Multiple frames in flight (2-3)
- [ ] Frame index tracking
- [ ] Frame completion detection
- [ ] Frame wait until complete
- [ ] Timeout handling
- [ ] Deadlock prevention
- [ ] Frame pacing
- [ ] Variable frame rate
- [ ] Adaptive frame rate
- [ ] Frame time measurement
- [ ] Frame budget enforcement
- [ ] Frame statistics

**GPU-CPU Synchronization (25 TODOs)**
- [ ] Shared events (MTLSharedEvent)
- [ ] Timeline synchronization
- [ ] Wait for event value
- [ ] Signal event from GPU
- [ ] Signal event from CPU
- [ ] Event listeners
- [ ] Synchronization points
- [ ] Resource hazards
- [ ] Memory ordering guarantees
- [ ] Consistency semantics

**Command Buffer Synchronization (20 TODOs)**
- [ ] Completion handlers
- [ ] Wait for completion
- [ ] Scheduled handlers
- [ ] Frame boundary detection
- [ ] Command buffer reuse safety
- [ ] Resource lifetime tracking
- [ ] Hazard analysis
- [ ] Barrier insertion

**Memory Barriers & Hazards (15 TODOs)**
- [ ] Texture memory barriers
- [ ] Buffer memory barriers
- [ ] Read-write hazards
- [ ] Write-write hazards
- [ ] Cache coherence
- [ ] Memory ordering
- [ ] GPU-GPU sync

**GPU Timestamps & Profiling (10 TODOs)**
- [ ] Timestamp queries
- [ ] GPU timer resolution
- [ ] Frame timing
- [ ] Pass timing
- [ ] Draw call timing
- [ ] Profiling overhead
- [ ] Time reporting

### Success Criteria
- [ ] Frame synchronization working
- [ ] No GPU stalls
- [ ] No race conditions
- [ ] Proper resource hazard handling
- [ ] Accurate timing data

---

## Agent 1.6: Swapchain & Display Output (~100 TODOs)

### Objective
Implement display output, HDR support, and presentation pipeline.

### File Locations
```
src/engine/rendering/3d_rendering/backend/metal/metal/
├── mtl_swapchain.c/h
├── mtl_display.c/h (NEW)
└── mtl_hdr.c/h (NEW)
```

### Core Implementation Tasks

**CAMetalLayer Integration (25 TODOs)**
- [ ] Create CAMetalLayer
- [ ] Configure pixel format
- [ ] Set device
- [ ] Attach to NSView/window
- [ ] Layer lifecycle management
- [ ] Layer resize handling
- [ ] Drawable acquisition (nextDrawable)
- [ ] Frame pacing configuration
- [ ] Vsync enable/disable
- [ ] Triple buffering
- [ ] Frame rate limiting
- [ ] Display sync mode
- [ ] Drawable timeout handling

**Drawable Management (20 TODOs)**
- [ ] Get current drawable
- [ ] Present drawable
- [ ] Drawable texture access
- [ ] Drawable scheduling
- [ ] Drawable completion
- [ ] Drawable disposal
- [ ] Drawable validation
- [ ] Multiple drawable support
- [ ] Drawable statistics

**HDR Output (20 TODOs)**
- [ ] EDR (Extended Dynamic Range) mode
- [ ] HDR color space selection (Display P3)
- [ ] Wide color gamut support
- [ ] Linear color space
- [ ] Float16 texture format
- [ ] Tone mapping for HDR
- [ ] Metadata for HDR streams
- [ ] SRGB vs linear conversion
- [ ] HDR capability detection
- [ ] Fallback for SDR displays

**Variable Refresh Rate (15 TODOs)**
- [ ] ProMotion support detection (120Hz, 240Hz)
- [ ] Dynamic frame rate adjustment
- [ ] Frame rate querying
- [ ] Adaptive refresh setup
- [ ] Power efficiency optimization
- [ ] Display timing queries

**Window Management (15 TODOs)**
- [ ] NSView binding
- [ ] Window resize handling
- [ ] DPI scaling
- [ ] Content scale factor
- [ ] Orientation handling
- [ ] Full-screen support
- [ ] Window validation
- [ ] Layer configuration

**Presentation Pipeline (5 TODOs)**
- [ ] Frame composition
- [ ] Presentation timing
- [ ] Drawable scheduling

### Success Criteria
- [ ] Content displays on screen
- [ ] HDR working on capable displays
- [ ] No tearing
- [ ] Proper frame pacing
- [ ] Resize handling works

---

## Phase 1 Integration Checklist

All agents must verify:
- [ ] No Vulkan/D3D12/OpenGL references
- [ ] All Metal API calls wrapped properly
- [ ] Objective-C memory management correct
- [ ] Thread-safe where required
- [ ] Error handling comprehensive
- [ ] Resource cleanup guaranteed
- [ ] Compilation succeeds without warnings
- [ ] Compatibility with macOS 10.13+
- [ ] Code compiles as both C and Objective-C

---

## Deliverables

After Phase 1:
- Complete Metal device management
- Working command recording system
- GPU resource allocation
- Pipeline creation and caching
- Frame synchronization
- Display output with HDR support
- ~600 TODOs implemented
- Ready for Phase 2 (geometry)
